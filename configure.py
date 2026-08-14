#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

# Project paths
ROOT = Path(__file__).parent
CONFIG_DIR = ROOT / "config"
SPLAT_YAML = CONFIG_DIR / "SLUS_204.69.yaml"
ISO_DIR = ROOT / "elf"
BUILD_DIR = ROOT / "build"
ELF_PATH = BUILD_DIR / "SLUS_204.69.elf"

# Original ELF
ORIGINAL_ELF = ISO_DIR / "SLUS_204.69"
ORIGINAL_SHA1 = "fd206d5715a322830f7fa9285fb4a09276ac2a63"

# The ov02 ELF section is a separately loaded overlay.  Its ELF load address
# cannot be used as an offset in Splat's logical ROM image: Splat expects the
# section packed immediately after the main image at these offsets.
OV02_ROM_START = 0x2DD000
OV02_ROM_END = 0x2F08EC

# Tools
PS2DEV = "/usr/local/ps2dev"
CROSS = f"{PS2DEV}/ee/bin/mips64r5900el-ps2-elf-"
AS = f"{CROSS}as"
LD = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-ld"
OBJCOPY = f"{CROSS}objcopy"
CC = f"{PS2DEV}/ee-gcc/bin/ee-gcc"

# Flags
ASFLAGS = "-march=r5900 -mabi=eabi -Iinclude --no-warn"
CC_ASFLAGS = "-march=r5900 -mabi=eabi -Iinclude --no-warn"
CFLAGS = "-O2 -G0 -fno-schedule-insns"
LDFLAGS = "--allow-multiple-definition -m elf32lr5900 --noinhibit-exec"

# Game code was compiled with a newer compiler than the SDK libraries:
# ee-gcc 2.96-ee-001003-1 (bundled in SquareMan/xenosaga under tools/cc/,
# installed at /opt/ee-gcc2.96 in the dev container). Evidence: slti+movn
# cmov shape, xori-before-movz equality artifacts, and jump-target
# alignment nops that 2.9-ee-991111 cannot produce at any flag setting.
CC96 = "/opt/ee-gcc2.96/bin/ee-gcc"

# Compiler selection. Game code is the overwhelming majority, so it is the
# DEFAULT: ee-gcc 2.96 at -O2 -G8. Only Sony SDK translation units use the
# older 2.9 compiler. Previously this was inverted -- every game file had to
# be listed explicitly and a new file silently built with the SDK compiler
# at -G0, producing garbage that looked like a failed decompilation.
SDK_FILES = {
    "sceVif1Pk.c",
    "sceDeci2.c",
    # IPU-assisted MPEG2 sequence-layer parsing (_sequenceHeader family):
    # confirmed via the 16-byte-per-saved-register frame padding pattern,
    # matched exactly by the 2.9-ee SDK compiler at -O2 -G0
    # -fno-schedule-insns; the 2.96 game compiler cannot reproduce it.
    "mpeg.c",
}
SDK_CC = CC
SDK_CFLAGS = CFLAGS


def is_sdk(name):
    # Any sce*-prefixed translation unit is Sony SDK code. Pattern-matched
    # rather than listed so a NEW sce file gets the right compiler
    # automatically -- the inverse mistake (game code silently built with
    # the SDK compiler) already cost an agent a run.
    return name in SDK_FILES or name.startswith("sce")


def cc_for(name):
    return SDK_CC if is_sdk(name) else CC96


# Per-file -G override for game code that is otherwise correctly built
# with the 2.96 compiler but was NOT built at -G8. Confirmed on libm.c:
# floorf's huge_f/tiny_f float constants are loaded via an inline
# lui/ori/mtc1 (li.s) sequence in the original, not the %gp_rel .lit4
# load -G8 produces -- and the -G0 build also reproduces the original's
# v1 (not v0) register choice for the initial mfc1. Register-save frame
# stride stays 8 bytes/reg (2.96), ruling out the SDK 2.9-ee compiler --
# this is a lone -G threshold difference, not a compiler-family swap.
FILE_CFLAGS_OVERRIDE = {
    "libm.c": "-O2 -G0",
    # libgcc2 64-bit division TUs: Sony built libgcc with -mlong32 (long is
    # 32-bit), which keeps __ll_B's `1L << 16` in SImode -- plain `sll`
    # instead of the canonicalizing dsll32/dsra32 pair. All four functions
    # match byte-exactly with this single extra flag; without it each one
    # is ~300 diff words. Confirmed by flag sweep 2026-08-14.
    "_muldi3.c": "-O2 -G8 -mlong32",
    "_fixunsdfdi.c": "-O2 -G8 -mlong32",
    "_fixunssfdi.c": "-O2 -G8 -mlong32",
    "_floatdidf.c": "-O2 -G8 -mlong32",
    "_fixdfdi.c": "-O2 -G8 -mlong32",
    "_divdi3.c": "-O2 -G8 -mlong32",
    "_moddi3.c": "-O2 -G8 -mlong32",
    "_udivdi3.c": "-O2 -G8 -mlong32",
    "_umoddi3.c": "-O2 -G8 -mlong32",
}

# Vendored newlib 1.8.2 TUs (newlib/ dir): compiled against the vendored
# header tree plus the toolchain's own stddef/stdarg. The gcc include dir
# lives in the dev container.
NEWLIB_CFLAGS = ("-O2 -G8 -Inewlib/libc/stdlib -Inewlib/libc/stdio -Inewlib/libc/string "
                 "-Inewlib/libc/include "
                 "-I/usr/local/ps2dev/ee-gcc/lib/gcc-lib/ee/2.9-ee-991111/include")
for _f in ("newlib_reallocr.c", "newlib_callocr.c", "newlib_ungetc.c",
           "newlib_strtod.c",
           "newlib_mbtowc.c", "newlib_strlwr.c", "newlib_vfscanf.c",
           "newlib_mprec.c", "newlib_quorem.c"):
    FILE_CFLAGS_OVERRIDE[_f] = NEWLIB_CFLAGS



def cflags_for(name):
    if is_sdk(name):
        return SDK_CFLAGS
    return FILE_CFLAGS_OVERRIDE.get(name, "-O2 -G8")


# The ASSEMBLER also has its own small-data threshold (default -G8, same
# as binutils gas's built-in default) that is INDEPENDENT of the -G value
# passed to the compiler. A `li.s`/`li.d` float/double immediate macro
# only expands to inline lui+ori+mtc1 when gas's own threshold says the
# constant does not qualify for .lit4/.lit8 pooling -- passing -G0 to gcc
# alone leaves gas still pooling it via %gp_rel, silently undoing the
# compiler-side flag. Found on libm.c's floorf: huge_f/tiny_f only lose
# their .lit4 gp-relative load and match the original's inline immediate
# once BOTH cc and as see -G0.
FILE_ASFLAGS_OVERRIDE = {
    "libm.c": "-G0",
}


def asflags_for(name):
    return FILE_ASFLAGS_OVERRIDE.get(name, "")


# xglVector's original object omits some load-delay nops that are present
# in other game objects compiled with the same compiler.
FILE_FIX_FLAGS = {
    # an lwc1 in a jal delay slot followed by mov.s trips a bogus hazard nop
    "MAP.c": "--omit-hazard mov.s",
    # many util natives do lwc1->cvt.s.w with no hazard nop in the original
    "Java_util.c": "--omit-hazard cvt.s.w",
    "xglVector.c": ("--omit-hazard mul.s --omit-hazard sub.s "
                    "--omit-hazard c.lt.s --omit-hazard mov.s"),
    # EXM's original object schedules independent FP work into load-delay
    # slots before comparisons and additions.
    "EXM.c": "--omit-hazard c.lt.s --omit-hazard add.s",
    "MATRIX.c": "--omit-hazard mul.s --omit-hazard add.s",
    # xglLightIntensityAmbient: gcc's own scheduler puts the trailing sq
    # in the jr $31 delay slot; the original keeps it before the branch
    # with a genuine nop in the slot. No other function in this file has
    # a legitimate delay-slot store, so the swap is safe file-wide here.
    "xglLight.c": "--hoist-return-store",
    # nmlModelSetFadeInInterrupt: gcc emits the trailing s.s BEFORE j $31,
    # but gas's own reorder pass pulls it into the delay slot; original
    # keeps it before the branch with a plain nop in the slot. Whole-file
    # --hoist-return-store regresses other nmlModel.c functions (see
    # XENOSAGA_RESUME_PROMPT.md), so scope to just this one.
    "nmlModel.c": "--barrier-return-store nmlModelSetFadeInInterrupt",
    # ungetc's CHECK_INIT tail and the mprec leaf returns keep their
    # copies out of the delay slots, same class as fabs in libm.c.
    "newlib_ungetc.c": "--barrier-return-store ungetc",
    "newlib_mprec.c": "--barrier-return-store --barrier-branch-move",
    "newlib_strtod.c": "--barrier-return-store --barrier-branch-move",
    # The libgcc float<->DI conversion TUs never let gas fill a delay
    # slot with a preceding copy/ALU op -- barrier both classes
    # (whole-file; each TU is one function).
    "_fixdfdi.c": "--barrier-return-store --barrier-branch-move",
    "_fixunsdfdi.c": "--barrier-return-store --barrier-branch-move",
    "_fixunssfdi.c": "--barrier-return-store --barrier-branch-move",
    "_floatdidf.c": "--barrier-return-store --barrier-branch-move",
    # fabs: SET_HIGH_WORD's final `return x` computes the masked result
    # into a4/a1-family regs and needs an explicit copy into $2; gas's
    # reorder pass steals that copy into the jr $31 delay slot, but the
    # original ee-as build left it as three separate instructions (copy,
    # branch, genuine nop). See RE_RETURN_MOVE in fix_cc_asm.py.
    # --as-g0: libm.c is also assembled with `as -G0` (asflags_for below),
    # which changes when a li.s float literal carries the mtc1 COP1 hazard
    # nop -- see fix_cc_asm.py's ASSUME_NO_LIT4 comment.
    "libm.c": "--barrier-return-store fabs,__ieee754_fmod --as-g0",
    # _dtoa_r: `dpcmp(d.d, 0.0)`'s `a1 = 0` argument setup is left as a
    # separate instruction (with a genuine nop in the jal delay slot) in
    # the original, but gas's reorder pass steals it into the jal's delay
    # slot here. Same RE_RETURN_MOVE fingerprint as fabs/__ieee754_fmod
    # above, just against a `jal` instead of a leaf return.
    "libc.c": "--barrier-return-store _dtoa_r",
}


def clean():
    import shutil
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
        print(f"Cleaned {BUILD_DIR}")


def split():
    rom_path = CONFIG_DIR / "SLUS_204.69.rom"
    if not rom_path.exists():
        print("Generating ROM from ELF...")
        subprocess.run(
            [OBJCOPY, "-O", "binary", "--gap-fill=0x00",
             str(ORIGINAL_ELF), str(rom_path)],
            check=True,
        )

    # `objcopy -O binary` honors the overlay's ELF load address and leaves the
    # Splat ov02 range zero-filled.  Extract that section independently (where
    # its first byte becomes offset zero), splice it at the logical ROM offset,
    # and discard the large address-derived gap at the end of objcopy's image.
    with tempfile.NamedTemporaryFile(prefix="xenosaga-ov02-", suffix=".bin",
                                     delete=False) as tmp:
        overlay_path = Path(tmp.name)
    try:
        subprocess.run(
            [OBJCOPY, "-O", "binary", "--only-section=ov02",
             str(ORIGINAL_ELF), str(overlay_path)],
            check=True,
        )
        overlay = overlay_path.read_bytes()
    finally:
        overlay_path.unlink(missing_ok=True)

    expected_size = OV02_ROM_END - OV02_ROM_START
    if len(overlay) != expected_size:
        raise RuntimeError(
            f"ov02 is {len(overlay):#x} bytes, expected {expected_size:#x}"
        )
    with rom_path.open("r+b") as rom:
        rom.seek(OV02_ROM_START)
        rom.write(overlay)
        rom.truncate(OV02_ROM_END)

    print("Running Splat...")
    subprocess.run(
        [sys.executable, "-m", "splat", "split", str(SPLAT_YAML)],
        check=True,
    )


def find_asm_files():
    asm_files = []
    for path in sorted(ROOT.rglob("asm/**/*.s")):
        asm_files.append(path.relative_to(ROOT))
    return asm_files


def find_src_files():
    src_files = []
    src_dir = ROOT / "src"
    if src_dir.exists():
        for path in sorted(src_dir.rglob("*.c")):
            src_files.append(path.relative_to(ROOT))
    return src_files


def find_asset_files():
    asset_files = []
    assets_dir = ROOT / "assets"
    if assets_dir.exists():
        for path in sorted(assets_dir.rglob("*.bin")):
            asset_files.append(path.relative_to(ROOT))
    return asset_files


def generate_ninja(asm_files, src_files, asset_files):
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    ninja_path = ROOT / "build.ninja"
    ld_script = CONFIG_DIR / "SLUS_204.69.ld"

    with open(ninja_path, "w") as f:
        f.write("# Generated by configure.py - do not edit manually\n")
        f.write(f"builddir = {BUILD_DIR}\n\n")

        f.write(f"rule as\n")
        f.write(f"  command = {AS} {ASFLAGS} -o $out $in\n")
        f.write(f"  description = AS $in\n\n")

        f.write(f"cflags = {CFLAGS}\n")
        f.write(f"cc = {CC}\n\n")
        f.write("fixflags =\n\n")
        f.write("asflags =\n\n")

        f.write(f"rule cc\n")
        # tools/fix_cc_asm.py post-processes the compiler asm so the modern
        # assembler reproduces the original ee-as encodings (move->daddu,
        # .set mips1 wraps for la/mem-macros/FP conversions, conditional
        # li.s hazard nop).
        f.write(f"  command = $cc $cflags -S -o $out.s $in && python3 tools/fix_cc_asm.py $out.s $fixflags && {AS} {CC_ASFLAGS} $asflags -o $out $out.s\n")
        f.write(f"  description = CC $in\n\n")

        f.write(f"rule ld\n")
        f.write(f"  command = {LD} {LDFLAGS} -o $out -T $ldscript $in\n")
        f.write(f"  description = LD $out\n\n")

        f.write(f"rule incbin\n")
        f.write(f"  command = echo \'.section .data\\n.balign 16\\n.incbin \"$in\"\' | {AS} {ASFLAGS} -o $out -\n")
        f.write(f"  description = INCBIN $in\n\n")

        obj_files = []

        for asm in asm_files:
            obj = BUILD_DIR / asm.with_suffix(".o")
            obj_files.append(str(obj))
            f.write(f"build {obj}: as {asm}\n")

        for src in src_files:
            obj = BUILD_DIR / src.with_suffix(".o")
            obj_files.append(str(obj))
            # tools/fix_cc_asm.py rewrites the compiler output between cc
            # and as, so an edit to it changes every C object -- make ninja
            # know that, otherwise objects stay silently stale and verify.py
            # reports results for code the current toolchain no longer emits.
            f.write(f"build {obj}: cc {src} | tools/fix_cc_asm.py\n")
            f.write(f"  cflags = {cflags_for(src.name)}\n")
            f.write(f"  cc = {cc_for(src.name)}\n")
            if src.name in FILE_FIX_FLAGS:
                f.write(f"  fixflags = {FILE_FIX_FLAGS[src.name]}\n")
            if asflags_for(src.name):
                f.write(f"  asflags = {asflags_for(src.name)}\n")

        for asset in asset_files:
            obj = BUILD_DIR / asset.with_suffix(".o")
            obj_files.append(str(obj))
            f.write(f"build {obj}: incbin {asset}\n")

        f.write("\n")
        f.write(f"build {ELF_PATH}: ld {' '.join(obj_files)}\n")
        f.write(f"  ldscript = {ld_script}\n\n")
        f.write(f"default {ELF_PATH}\n")

    print(f"Generated {ninja_path}")
    print(f"  Assembly files: {len(asm_files)}")
    print(f"  Source files:   {len(src_files)}")
    print(f"  Asset files:    {len(asset_files)}")


def generate_objdiff():
    objdiff_config = {
        "min_version": "2.0.0",
        "custom_make": "ninja",
        "build_target": str(ELF_PATH),
        "target_dir": "expected",
        "base_dir": "build",
    }
    objdiff_path = ROOT / "objdiff.json"
    with open(objdiff_path, "w") as f:
        json.dump(objdiff_config, f, indent=2)
    print(f"Generated {objdiff_path}")


def main():
    parser = argparse.ArgumentParser(description="Configure the Xenosaga Episode I decomp build")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--split", action="store_true")
    parser.add_argument("--no-split", action="store_true")
    args = parser.parse_args()

    if not ORIGINAL_ELF.exists():
        print(f"Error: {ORIGINAL_ELF} not found.")
        sys.exit(1)

    if args.clean:
        clean()

    if args.split or (not args.no_split and not (ROOT / "asm").exists()):
        split()

    asm_files = find_asm_files()
    src_files = find_src_files()
    asset_files = find_asset_files()

    if not asm_files:
        print("Error: No assembly files found. Run with --split.")
        sys.exit(1)

    generate_ninja(asm_files, src_files, asset_files)
    generate_objdiff()
    print("\nBuild configured! Run 'ninja' to build.")


if __name__ == "__main__":
    main()
