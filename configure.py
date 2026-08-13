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


def cflags_for(name):
    return SDK_CFLAGS if is_sdk(name) else "-O2 -G8"


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

        f.write(f"rule cc\n")
        # tools/fix_cc_asm.py post-processes the compiler asm so the modern
        # assembler reproduces the original ee-as encodings (move->daddu,
        # .set mips1 wraps for la/mem-macros/FP conversions, conditional
        # li.s hazard nop).
        f.write(f"  command = $cc $cflags -S -o $out.s $in && python3 tools/fix_cc_asm.py $out.s $fixflags && {AS} {CC_ASFLAGS} -o $out $out.s\n")
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
