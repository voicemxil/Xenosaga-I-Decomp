#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
import sys
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

# Per-file overrides. Game code accesses small globals through $gp
# (%gp_rel), so it needs -G8 like the original build; -G0 would emit
# lui/lw pairs that can't match.
FILE_CC = {
    "SCRIPT.c": CC96,
    "nmlModel.c": CC96,
    "xglDma.c": CC96,
    "xglCd.c": CC96,
    "xglPacket.c": CC96,
    "xglRender.c": CC96,
    "xglCulling.c": CC96,
    "MSG.c": CC96,
    "MBUF.c": CC96,
    "JNI.c": CC96,
    "Menu.c": CC96,
    "LOOK.c": CC96,
    "CONSTRUCT.c": CC96,
    "Enemy.c": CC96,
    "MAP.c": CC96,
    "Check.c": CC96,
    "Get.c": CC96,
    "PLAY.c": CC96,
    "TMENU.c": CC96,
    "TWIN.c": CC96,
    "DB2.c": CC96,
    "Java_vm.c": CC96,
    "Java_Stage.c": CC96,
    "Java_Sound.c": CC96,
    "Java_PlayControl.c": CC96,
    "Java_Movie.c": CC96,
    "Java_Light.c": CC96,
    "Java_Effect.c": CC96,
    "ACT.c": CC96,
    "JNT.c": CC96,
    "Java_Unit.c": CC96,
    "Java_Camera.c": CC96,
    "Java_util.c": CC96,
    "DataBuffer.c": CC96,
    "CUR.c": CC96,
    "MIF.c": CC96,
    "Java_Chr.c": CC96,
    "JTHREAD.c": CC96,
    "JS.c": CC96,
    "Umn.c": CC96,
    "DB.c": CC96,
    "Game.c": CC96,
    "tya.c": CC96,
    "ssd.c": CC96,
    "Party.c": CC96,
    "xglHdd.c": CC96,
    "xglMenu.c": CC96,
    "xglStudio.c": CC96,
    "nmlPacket.c": CC96,
    "xglFlags.c": CC96,
    "xglMovie.c": CC96,
    "xglMath.c": CC96,
    "xglVector.c": CC96,
    "RSRC.c": CC96,
    "XTK.c": CC96,
    "EXM.c": CC96,
    "FCV.c": CC96,
    "RES.c": CC96,
    "command.c": CC96,
    "EW.c": CC96,
    "MATRIX.c": CC96,
    "TCAMERA.c": CC96,
}
# Note: game code (2.96) matches with plain -O2 -G8; adding
# -fno-schedule-insns perturbs its register allocation.
FILE_CFLAGS = {
    "SCRIPT.c": "-O2 -G8",
    "nmlModel.c": "-O2 -G8",
    "xglDma.c": "-O2 -G8",
    "xglCd.c": "-O2 -G8",
    "xglPacket.c": "-O2 -G8",
    "xglRender.c": "-O2 -G8",
    "xglCulling.c": "-O2 -G8",
    "MSG.c": "-O2 -G8",
    "MBUF.c": "-O2 -G8",
    "JNI.c": "-O2 -G8",
    "Menu.c": "-O2 -G8",
    "LOOK.c": "-O2 -G8",
    "CONSTRUCT.c": "-O2 -G8",
    "Enemy.c": "-O2 -G8",
    "MAP.c": "-O2 -G8",
    "Check.c": "-O2 -G8",
    "Get.c": "-O2 -G8",
    "PLAY.c": "-O2 -G8",
    "TMENU.c": "-O2 -G8",
    "TWIN.c": "-O2 -G8",
    "DB2.c": "-O2 -G8",
    "Java_vm.c": "-O2 -G8",
    "Java_Stage.c": "-O2 -G8",
    "Java_Sound.c": "-O2 -G8",
    "Java_PlayControl.c": "-O2 -G8",
    "Java_Movie.c": "-O2 -G8",
    "Java_Light.c": "-O2 -G8",
    "Java_Effect.c": "-O2 -G8",
    "ACT.c": "-O2 -G8",
    "JNT.c": "-O2 -G8",
    "Java_Unit.c": "-O2 -G8",
    "Java_Camera.c": "-O2 -G8",
    "Java_util.c": "-O2 -G8",
    "DataBuffer.c": "-O2 -G8",
    "CUR.c": "-O2 -G8",
    "MIF.c": "-O2 -G8",
    "Java_Chr.c": "-O2 -G8",
    "JTHREAD.c": "-O2 -G8",
    "JS.c": "-O2 -G8",
    "Umn.c": "-O2 -G8",
    "DB.c": "-O2 -G8",
    "Game.c": "-O2 -G8",
    "tya.c": "-O2 -G8",
    "ssd.c": "-O2 -G8",
    "Party.c": "-O2 -G8",
    "xglHdd.c": "-O2 -G8",
    "xglMenu.c": "-O2 -G8",
    "xglStudio.c": "-O2 -G8",
    "nmlPacket.c": "-O2 -G8",
    "xglFlags.c": "-O2 -G8",
    "xglMovie.c": "-O2 -G8",
    "xglMath.c": "-O2 -G8",
    "xglVector.c": "-O2 -G8",
    "RSRC.c": "-O2 -G8",
    "XTK.c": "-O2 -G8",
    "EXM.c": "-O2 -G8",
    "FCV.c": "-O2 -G8",
    "RES.c": "-O2 -G8",
    "command.c": "-O2 -G8",
    "EW.c": "-O2 -G8",
    "MATRIX.c": "-O2 -G8",
    "TCAMERA.c": "-O2 -G8",
}

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
            f.write(f"build {obj}: cc {src}\n")
            if src.name in FILE_CFLAGS:
                f.write(f"  cflags = {FILE_CFLAGS[src.name]}\n")
            if src.name in FILE_CC:
                f.write(f"  cc = {FILE_CC[src.name]}\n")
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
