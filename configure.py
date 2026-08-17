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
# NOTE: do NOT add --noinhibit-exec here. It makes ld write an output file
# even when the link failed, so ninja sees a success and the build looks
# green while the ELF is full of unresolved references. That is exactly how
# the missing INCLUDE of config/undefined_syms_auto.txt went unnoticed.
# Link errors must fail the build.
LDFLAGS = "--allow-multiple-definition -m elf32lr5900"

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
    # kernel.c: the syscall stubs are frameless so they match under either
    # compiler, but the launch forwarders (Exit/ExecOSD/LoadExecPS2/ExecPS2)
    # need the SDK compilers 16-byte-per-saved-register frame stride --
    # 2.96 packs the saves 8 bytes apart. All 156 functions match at
    # -O2 -G0 -fno-schedule-insns.
    "kernel.c",
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
    # InitExecPS2 is its own SDK TU and was built with instruction scheduling
    # enabled; kernel.c's other SDK units use -fno-schedule-insns.
    "sceKernelExec.c": "-O2 -G0",
    "sceKernelTLB.c": "-O2 -G0",
    "sceKernelAlarm.c": "-O2 -G0",
    "sceKernelThread.c": "-O2 -G0",
    "sceVif1PkRefLoadImage.c": "-O2 -G0",
    "sceMpegDec.c": "-O2 -G0",
    "sceTty.c": "-O2 -G0",
    "sceIpu.c": "-O2 -G0",
    "sceFs.c": "-O2 -G0",
    # These SDK translation units were built WITHOUT -fno-schedule-insns:
    # the original pairs independent multiplies onto the R5900's two
    # multiplier units (mult1 + mult), which only the first scheduling
    # pass emits. Verified per-file -- adding it to SDK_CFLAGS globally
    # breaks sceMpeg.c (1 -> 7) and sceVif1Pk.c (6 -> 15).
    "scePad.c": "-O2 -G0",
    "sceCd.c": "-O2 -G0",
    "sceSif.c": "-O2 -G0",
    "sceMc.c": "-O2 -G0",
    "sceDeci2.c": "-O2 -G0",
    "libm.c": "-O2 -G0",
    # Three camera functions need -fno-strict-aliasing; verified per-file
    # only (the flag regresses xglTask.c's matches if applied globally).
    "xglCamera.c": "-O2 -G8 -fno-strict-aliasing",
    "malloc_lock.c": "-O2 -G0",
    # libgcc2 64-bit division TUs: Sony built libgcc with -mlong32 (long is
    # 32-bit), which keeps __ll_B's `1L << 16` in SImode -- plain `sll`
    # instead of the canonicalizing dsll32/dsra32 pair. All four functions
    # match byte-exactly with this single extra flag; without it each one
    # is ~300 diff words. Confirmed by flag sweep 2026-08-14.
    "_muldi3.c": "-O2 -G8 -mlong32",
    "_fixunsdfdi.c": "-O2 -G8 -mlong32",
    "_fixunssfdi.c": "-O2 -G8 -mlong32",
    "_floatdidf.c": "-O2 -G8 -mlong32",
    "gcc_frame.c": ("-O2 -G0 -mlong32 -Inewlib/gccsrc -Inewlib/libc/include "
                "-I/usr/local/ps2dev/ee-gcc/lib/gcc-lib/ee/2.9-ee-991111/include"),
    "_main.c": ("-O2 -G0 -mlong32 -Inewlib/gccsrc -Inewlib/libc/include "
                "-I/usr/local/ps2dev/ee-gcc/lib/gcc-lib/ee/2.9-ee-991111/include"),
    "fpbit_df.c": ("-O2 -G0 -mlong32 -Inewlib/gccsrc -Inewlib/libc/include "
                "-I/usr/local/ps2dev/ee-gcc/lib/gcc-lib/ee/2.9-ee-991111/include"),
    "fpbit_sf.c": ("-O2 -G0 -mlong32 -Inewlib/gccsrc -Inewlib/libc/include "
                "-I/usr/local/ps2dev/ee-gcc/lib/gcc-lib/ee/2.9-ee-991111/include"),
    "_fixdfdi.c": "-O2 -G8 -mlong32",
    "_divdi3.c": "-O2 -G8 -mlong32",
    "_moddi3.c": "-O2 -G8 -mlong32",
    "_udivdi3.c": "-O2 -G8 -mlong32",
    "_umoddi3.c": "-O2 -G8 -mlong32",
}

# Vendored newlib 1.9.0 TUs (newlib/ dir): compiled against the vendored
# header tree plus the toolchain's own stddef/stdarg (gcc include dir in
# the dev container). -G0 because Sony's libc keeps _impure_ptr/_ctype_
# etc. out of small data. newlib/gccinc overrides stdarg/va-mips.h with
# the fetch-then-advance va_arg the original uses.
NEWLIB_CFLAGS = ("-O2 -G0 -Inewlib/gccinc -Inewlib/libc/stdlib "
                 "-Inewlib/libc/stdio -Inewlib/libc/string "
                 "-Inewlib/libc/signal -Inewlib/libc/reent "
                 "-Inewlib/libc/include "
                 "-I/usr/local/ps2dev/ee-gcc/lib/gcc-lib/ee/2.9-ee-991111/include")
for _f in ("newlib_reallocr.c", "newlib_callocr.c", "newlib_ungetc.c",
           "newlib_strtod.c", "newlib_strtoul.c", "newlib_vfprintf.c",
           "newlib_mbtowc.c", "newlib_strlwr.c", "newlib_vfscanf.c",
           "newlib_mprec.c", "newlib_dtoa2.c", "newlib_freer.c",
           "newlib_signal.c", "newlib_signalr.c", "newlib_printf.c",
           "newlib_sprintf.c", "newlib_sscanf.c", "newlib_atexit.c",
           "newlib_exit.c", "newlib_strcasecmp.c", "newlib_mallocr.c"):
    FILE_CFLAGS_OVERRIDE[_f] = NEWLIB_CFLAGS



def cflags_for(name):
    # A per-file override wins over BOTH defaults. It used to be consulted
    # only for game code, so an override on an sce* file was silently dead
    # -- which hid that several SDK units were built without
    # -fno-schedule-insns (their originals pair independent multiplies
    # across the R5900's two multiplier units, which only the first
    # scheduling pass emits).
    if name in FILE_CFLAGS_OVERRIDE:
        return FILE_CFLAGS_OVERRIDE[name]
    if is_sdk(name):
        return SDK_CFLAGS
    return "-O2 -G8"


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
    "tskUmnBgCube.c": " -G0",
    "libm.c": "-G0",
}


def asflags_for(name):
    return FILE_ASFLAGS_OVERRIDE.get(name, "")


# xglVector's original object omits some load-delay nops that are present
# in other game objects compiled with the same compiler.
FILE_FIX_FLAGS = {
    "ssd.c": ("--swap-regs SsdSpuDirectRead:2-3:11,13 "
              "--swap-regs SsdSpuDirectRead:3-8:12,21"),
    "Enemy.c": ("--hoist-div-arg Enemy_Escape,Enemy_Route_Chase "
                "--rotate-seq Enemy_Escape:63:2,Enemy_Escape:65:2,Enemy_Escape:67:2,"
                "Enemy_Route_Chase:61:2,Enemy_Route_Chase:63:2,Enemy_Route_Chase:65:2"),
    "HddTest.c": "--swap-regs HddTest1024Save:2-3:16-17",
    "xglHdd.c": "--swap-regs xglHddMcCheckCore:10-11:50 --swap-regs xglHddMcCheckCore:3-10:52-55 --swap-regs xglHddMcCheckCore:11-3:52 --rotate xglHddMcCheckYourSaves:1:3 --split-hi-lo xglHddMcCheckYourSaves:1:2",
    "Hit.c": ("--rotate HitCheckCorner:17:5 "
              "--swap-regs HitCheckMapUnitWithNyuru:19-20:73-91 "
              "--rotate-seq HitCheckMapUnitWithNyuru:72:3,HitCheckMapUnitWithNyuru:73:-3"),
    "RES.c": ("--swap-regs RES_loadFile:2-3:110-116 "
              "--swap-regs RES_loadFileSubMapSub:4-5:49-68 "
              "--swap-mem-into-slot RES_loadFile:4 "
              "--post-rotate-seq RES_loadFile:46:3,RES_loadFile:47:2 "
              "--retime-byte-copy-guard RES_loadFileSubMapSub:29 "
              "--split-loop-byte-load RES_loadFileSubMapSub:56:59 "
              "--branch-likely RES_loadFileSubMapSub:4"),
    "GameResource.c": (
        "--swap-reg-sources GameResourceAlloc:17:4-18,GameResourceAlloc:19:4-18 "
        "--rotate-seq GameResourceAlloc:13:3,GameResourceAlloc:14:2 "
        "--swap-regs GameResourceAlloc:4-5:28,32 "
        "--swap-regs GameResourceRealloc:6-7:23,27 "
        "--branch-likely GameResourceRealloc:1"),
    "Disp.c": "--swap-fp-operands DispPillar:27,DispPillar:51",
    "task.c": ("--swap-regs taskItemGet:2-6:18-26,32 "
               "--swap-regs taskItemGet:2-7:18-26,32 "
               "--swap-regs taskItemGet:6-8:27,29 "
               "--swap-regs taskItemGet:2-3:35,38 "
               "--swap-adjacent taskItemGet:17 "
               "--rotate taskItemGet:19:4"),
    "nmlPacket.c": ("--swap-regs nmlPacketAddGifTag:16-17:25,36 "
                    "--swap-int-operands nmlPacketAddGifTag:25 "
                    "--rotate-seq nmlPacketAddGifTag:24:4,"
                    "nmlPacketAddGifTag:26:2,nmlPacketAddGifTag:27:3"),
    "update.c": ("--branch-unlikely updateCursor:22 "
                 "--rotate-seq updateCursor:169:2,updateCursor:170:2,"
                 "updateCursor:189:2,updateCursor:190:2,"
                 "updateCursorMode1:83:-13,updateCursorMode1:84:-12,"
                 "updateCursorMode1:85:-11 "
                 "--swap-into-slot updateCursor:10,updateCursor:11,"
                 "updateCursor:15,updateCursor:16 "
                 "--swap-regs updateCursorMode1:3-7:74,78,80-87 "
                 "--swap-regs updateCursorMode1:2-7:82-83,86-87 "
                 "--rebase-stack-mem updateCursorMode1:89,91,93,95:5:32"),
    "sceSif.c": ("--swap-regs sceSifInitIopHeap:2-3:25,27 "
                 "--swap-adjacent sceSifInitIopHeap:26! "
                 "--retime-rpc-call-setup sceSifCallRpc:26:88"),
    "sceFs.c": "--swap-regs sceFsInit:2-3:31,32,37,48,74,76",
    "sceMpegDec.c": "--swap-regs _isOutSizeOK:2-3:17-22",
    "xglMc.c": "--byte-move-andi xglMcSetMapName:0,xglMcSetMapName:1",
    "sceMc.c": "--swap-adjacent sceMcRename:53",
    "sceVif1PkRefLoadImage.c": "--swap-regs sceVif1PkRefLoadImage:5-6:89-98 --swap-regs sceVif1PkRefLoadImage:16-20:91,98",
    "xglMenu.c": "--rotate-seq xglMenuDrawType1Sub:11:3,xglMenuDrawType1Sub:12:2",
    "sub.c": "--swap-adjacent subJoutoPosSet:58",
    "sef.c": "--branch-likely sefAllocLocalData:2",
    "sc.c": ("--branch-likely scDispatchScript:4 "
             "--exchange-slot-prior scInitScript:2:2,scParseScript:0:1 "
             "--swap-mem-into-slot scMISSILE3Script:2 "
             "--swap-adjacent scDestroyScript2:10,scDestroyScript2:13 "
             "--retime-branch-args scDestroyScript2:0"),
    "Game.c": (
        "--rotate-seq GameModeDebugMenu:10:-2,GameModeDebugMenu:12:3,"
        "GameModeDebugMenu:12:2,GameResourceDump:26:5,"
        "GameResourceDump:27:2,GameResourceDump:29:2,"
        "GameMovieInit:22:3,GameMovieInit:23:7,GameMovieInit:26:3,"
        "GameMovieInit:27:2,GameMovieInit:28:3,GameMovieInit:31:2 "
        "--swap-regs GameResourceDump:3-16:15-23 "
        "--split-indexed-scan-base GameResourceDump:8:2032:16 "
        "--branch-likely GameResourceDump:3"),
    "tskUmnBgCube.c": " --as-g0 --fp-pair-hazard tskUmnBgCubeMain",
    "sdv.c": "--branch-unlikely sdvScheduleSound:1 --rotate-seq sdvCreateAlter:3:3,sdvCreateAlter:4:2",
    "File.c": "--branch-unlikely FileObjectJpegDecChange:1 --swap-adjacent FileObjectJpegDecChange:3",
    "xglRender.c": "--fp-pair-hazard xglRenderGlobalFadeSet --barrier-branch-move xglRenderInit",
    "Calc.c": "--mtc1-nop CalcNearCrossPointCircle:0 --rotate-seq CalcNearCrossPointBox:24:7,CalcNearCrossPointBox:26:-3",
    "kernel.c": "--swap-adjacent PatchIsNeeded:8,PatchIsNeeded:10",
    "sceMpeg.c": "--swap-adjacent sceMpegReset:3",
    "TCAMERA.c": "--unfill-gcc-slots TCAMERA_mpackGetInterest --swap-regs TCAMERA_mpackGetInterest:2-3",
    "mpeg.c": "--swap-adjacent _sequenceHeader:26 --swap-into-slot _sequenceHeader:7,_sequenceHeader:12",
    "sv.c": "",
    "xglPacket.c": "--rotate-seq xglPacketInterpolate:26:2,xglPacketInterpolate:26:5",
    "tskMenuPausePage.c": "--swap-adjacent PauseMenuPage0:12 --rotate PauseMenuPage0:21:3",
    "xglCd.c": "--swap-adjacent xglCdReadCancel:41! --short-loop-pad FileSelectListReload:3:5,FileSelectListReload:4:0,FileSelectListReload:5:0",
    "xglDma.c": "--barrier-return-store xglDmaMFIFOKick --swap-adjacent xglDmaMFIFOKick:14",
    "Font.c": ("--branch-likely FontTestP1:2 "
               "--swap-regs FontTest:3-4:12,14-17 "
               "--coalesce-symbol-address FontTest:11:13:3"),
    "Drill.c": "--mtc1-nop DrillZMoveFunc:1,DrillPowerOffFunc:1 --rotate DrillReturnFunc:139:3",
    "eMessage.c": "--rotate eMessageNextGyouMaxGet:2:-5",
    "INIT.c": "--rotate InitEvsSymbol:33:-3,InitRetSymbol:33:-3 --rotate-seq InitShopSymbol:39:-4,InitShopSymbol:39:-3,InitSaveSymbol:39:-4,InitSaveSymbol:39:-3 --barrier-lo-load InitItemSymbol,InitItemBox",
    "Get.c": "--swap-adjacent Get_Rnd:5",
    "Check.c": "--mtc1-nop CheckDoorDist:0",
    "Undu.c": "--swap-adjacent UnduParamInit:5",
    "SEQ.c": "--mtc1-nop SEQ_motion:3",
    "xglFont.c": "--swap-adjacent xglFontAscii2Euc:47",
    # JS_classLight_setDirection2: the li/lw pair is a scheduling
    # tie-break, and the $f0/$f1 assignment across the three
    # load-store pairs is an allocator naming tie-break -- the loads and
    # stores are already in the original's order.
    # JS_loadClass needs two OVERLAPPING left-rotations, which a single
    # --rotate scan cannot fire (it steps past the first window), hence
    # --rotate-seq. The order is not commutative: 12:-7 first.
    "JS.c": ("--swap-adjacent JS_classLight_setDirection2:6 "
             "--swap-regs JS_classLight_setDirection2:f0-f1 "
             "--rotate-seq JS_loadClass:12:-7,JS_loadClass:10:-4"),

    "Java_Chr.c": ("--mtc1-nop Java_xeno_Chr_look_eye_set__FF:0 "
                   "--rotate Java_xeno_Chr_setFilter__I:58:2 "
                   "--swap-adjacent Java_xeno_Chr_setPointLightReset__:18 "
                   "--zero-quad-store Java_xeno_Chr_setPointLightReset__ "
                   "--rotate-seq Java_xeno_Chr_setPeer__Ljava_lang_Object_:21:-5,"
                   "Java_xeno_Chr_setPeer__Ljava_lang_Object_:21:2,"
                   "Java_xeno_Chr_setPeer__Ljava_lang_Object_:23:2"),
    "JNI.c": "--rotate JNI_searchClasses:16:2",
    "JTHREAD.c": "--rotate JTHREAD_cntl:2:2",
    "sceGs.c": "--swap-adjacent sceGsSyncVCallback:26",
    # sceVif1PkCnt/End: the original scheduler ordered an or/sw pair the
    # other way -- parked for months as "needs a mid-block instruction
    # swap flag", which --swap-adjacent now is. NOTE all sites for one
    # pass must be a single comma-separated value (argparse stores, not
    # appends; fix_cc_asm.py rejects duplicates).
    "sceVif1Pk.c": ("--swap-adjacent sceVif1PkCnt:9,sceVif1PkEnd:9,sceVif1PkAddUpkData128:10,sceVif1PkAddUpkData128:15!,sceVif1PkAlign:6,sceVif1PkAlign:22,sceVif1PkOpenUpkCode:16 --rotate sceVif1PkOpenUpkCode:15:-3"),
    # an lwc1 in a jal delay slot followed by mov.s trips a bogus hazard nop
    "MAP.c": ("--omit-hazard mov.s "
              "--exchange-derived-results MAP_updateUnitEnemy:14 "
              "--swap-regs MAP_updateUnitEnemy:2-5:16,18"),
    "Party.c": "--retime-branch-call PartyAttackerGet:2",
    # The file-item tail has the same operations as retail, but GCC assigns
    # its short-lived item/name roles to the opposite argument registers and
    # schedules the independent stores differently.
    "RSRC.c": "--retime-resource-file-item RSRC_loadFileSub:64",
    # ACT_setArms: gcc annuls the arm-slot compare's branch because it
    # filled the delay slot with a copy of the merge point's instruction;
    # the original executes that copy unconditionally and re-does it at the
    # merge. Ten source-level shapes were tried against this and none moved
    # it -- it is not reachable from C.
    "ACT.c": "--branch-unlikely ACT_setArms:2 --rotate ACT_createChr:24:2 --rotate-seq ACT_createChr:18:5,ACT_createChr:19:-4",
    # EtherTreeRightDraw / EtherTreeLine2SelectChange: gcc annuls a branch
    # whose delay slot it filled by copying the merge point's instruction
    # (the loop test, and the epilogue's `ld ra`). Both are safe to execute
    # on the fall-through path -- the original executes them unconditionally
    # and re-does them at the merge -- so 2.96 emits the plain form and we
    # emit the branch-likely one. Same annulment class as ACT_setArms.
    "Ether.c": ("--branch-unlikely EtherTreeRightDraw:2,"
                "EtherTreeLine2SelectChange:1"),
    # many util natives do lwc1->cvt.s.w with no hazard nop in the original
    "Java_util.c": ("--omit-hazard cvt.s.w --mtc1-nop Java_xeno_util_Spline_getValue__I:0 --swap-adjacent Java_xeno_util_Runtime_jumpCF__II:7,Java_xeno_util_Format_toString__Z:27! --rotate Java_xeno_util_Runtime_setLocation__III:15:2,Java_xeno_util_Runtime_setLocation__III:17:3,Java_xeno_util_Format_toString__F:32:2,Java_xeno_util_Format_toString__F:34:-4 --swap-into-slot Java_xeno_util_Format_toString__F:3"),
    "xglVector.c": ("--omit-hazard mul.s --omit-hazard sub.s "
                    "--omit-hazard c.lt.s --omit-hazard mov.s"),
    # EXM's original object schedules independent FP work into load-delay
    # slots before comparisons and additions.
    "EXM.c": "--omit-hazard c.lt.s --omit-hazard add.s",
    "MATRIX.c": "--omit-hazard mul.s --omit-hazard add.s --fp-pair-hazard",
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
    # nmlModelSetFadeIn / nmlModelFogPara: ee-as padded COP1-move stalls
    # after mtc1s whose next FP consumer reads a DIFFERENT register --
    # gas only pads the same-register mtc1->swc1 case, and no
    # register/distance heuristic reproduces the rest (see the
    # chain-tracking NOTE in fix_cc_asm.py's main). Site-indexed nops:
    # FadeIn's mtc1 $1,$f1 (site 1), FogPara's mtc1 $0,$f1 (2) and
    # mtc1 $1,$f3 (3).
    "nmlModel.c": ("--barrier-return-store nmlModelSetFadeInInterrupt --mtc1-nop nmlModelSetFadeIn:1,nmlModelFogPara:2,nmlModelFogPara:3 --pin-slot-nop nmlModelSetGlobalPointLightPos:0,nmlModelSetActiveFadeOut:0,nmlModelSetActiveFadeOut:1,nmlModelSetActiveFadeOut:2,nmlModelSetActiveFadeIn:0,nmlModelSetActiveFadeIn:1,nmlModelSetActiveFadeIn:2 --rotate-seq CONSTRUCT_MODELSYSTEM:6:4,CONSTRUCT_MODELSYSTEM:9:-18,CONSTRUCT_MODELSYSTEM:8:-3"),
    # ungetc's CHECK_INIT tail and the mprec leaf returns keep their
    # copies out of the delay slots, same class as fabs in libm.c.
    "newlib_ungetc.c": "--barrier-return-store ungetc",
    # MenuRWeaponCheck2: the original keeps its trailing store above the
    # jr with a genuine nop in the slot (found by the Menu subagent).
    # MenuFaceEpidGet: one branch-likely annul bit at site 3 (flagged by
    # the wave-3 Menu subagent, site index found by sweep).
    # MenuSkillEquip: same single annul bit at site 2 (the free-slot
    # search loop back-edge bnez -> bnezl; wave-5 Menu subagent).
    # MenuAgwsListMake_Gun/_Acc/_Pilot: the original's sched2 emits the
    # loop-preheader `addiu s0,s0,8/9` increment BEFORE the MenuWork
    # %lo() addiu; ours orders them the other way. Instruction 30 is the
    # %lo addiu in all three (same preheader shape).
    # MenuShopModelDisp: filled-branch site 6 (beqz to $L907) -- gcc
    # hoists the target block's l.s gp-load into the slot, the original
    # hoisted the move $4,$16 that follows it.
    # tskUmnSimulationInfo wants an argument-setup addu hoisted above
    # the lui/mtc1 pair for a float constant -- a 3-element rotation,
    # which --swap-adjacent structurally cannot express.
    "tskUmn.c": "--rotate tskUmnSimulationInfo:100 --swap-adjacent tskUmnSimulationList:5,tskUmnDataBaseName:353,tskUmnDataBaseName:358 --fp-pair-hazard UmnDataBaseModel --short-loop-pad tskUmnDataBaseName:0:3,tskUmnDataBaseName:1:0 --swap-regs tskUmnDataBaseName:18-21:230-299 --remat-call-constant tskUmnSimulationList:18-2-4:20",
    # JNT root accessors: TI-mode quadword copies whose allocator
    # tie-break lands the pair backwards (JVM subagent, verified).
    "JNT.c": (
        "--hoist-return-store JNT_getRootTrans,JNT_getRootRotate,JNT_getRootScale "
        "--swap-regs JNT_getRootTrans:2-3 "
        "--swap-regs JNT_getRootRotate:2-3 "
        "--swap-regs JNT_getRootScale:2-3 "
        "--retime-root-matrix-setup JNT_resetMatrix:1952:40 "
        "--swap-regs JNT_resetMatrix:17-20:60-71 "
        "--swap-regs JNT_resetMatrix:2-3:72,74-75,77,80-81,86-87 "
        "--swap-regs JNT_resetMatrix:3-4:76-77,79,81,83,86 "
        "--swap-regs JNT_resetMatrix:2-4:76,84-85,88 "
        "--swap-adjacent JNT_resetMatrix:59 "
        "--rebase-stack-mem JNT_resetMatrix:73,77,79,81:17:64 "
        "--rotate-seq JNT_resetMatrix:73:-3 "
        "--branch-unlikely JNT_setModel:1"),
    # Each flag may appear ONCE per entry: argparse stores rather than
    # appends, so a repeated flag silently drops the earlier sites --
    # that regressed three matched functions before fix_cc_asm.py
    # grew a duplicate-flag guard. Comma-separate every site.
    "Menu.c": "--barrier-return-store MenuRWeaponCheck2 --branch-likely MenuFaceEpidGet:3,MenuSkillEquip:2,MenuItemInfoMain:13 --swap-adjacent MenuAgwsListMake_Gun:30,MenuAgwsListMake_Acc:30,MenuAgwsListMake_Pilot:30,MenuModelUnitDispose:15,MenuCfTaikiPush:6 --rotate MenuTecL1R1Main:43:-4,MenuEtherL1R1Main:43:-4,MenuSkillL1R1Main:43:-4,MenuSystemInfoMain:60:5,MenuModelUnitDispose:22:5,MenuCfTaikiPop:10:2 --swap-slot-target MenuShopModelDisp:6 --branch-unlikely MenuCharNameGet:0",
    "TMENU.c": "--swap-adjacent TMENU_addItem:4 --mtc1-nop TMENU_updateDefault:2,TMENU_updateDefault:5",
    # xgl delay-slot shapes found by the xgl subagent (all verified):
    "xglTimer.c": "--barrier-return-store xglTimer0Reset",
    # xglCameraTravelProc: one branch-likely annul bit at site 3 (xgl
    # wave-4 subagent, full-match verified).
    "xglCamera.c": "--branch-likely xglCameraTravelProc:3",
    # PauseMenu: jal site 3 (GameSnapShotSaveFile) -- the original fills
    # the delay slot with move $5,$16 and keeps move $6,$2 above the
    # jal; gcc fills it the other way round.
    "tskMenuPause.c": ("--pin-slot-nop PauseMenu:0 "
                       "--swap-into-slot PauseMenu:3"),
    "xglMovie.c": "--barrier-branch-move xglMovieClose --rotate-seq xglMovieMakeXtxHeader:28:3,xglMovieMakeXtxHeader:29:2,xglMpeg2InfoInit2:33:3,xglMpeg2InfoInit2:33:3 --branch-likely fillBuff:3",
    "xglMath.c": "--omit-hazard qmtc2 --lis-hazard-nop xglAtan2 --swap-adjacent xglGeometryInit:0",
    "newlib_mprec.c": "--barrier-return-store --barrier-branch-move",
    "newlib_strtod.c": "--barrier-return-store --barrier-branch-move",
    "newlib_strtoul.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_vfscanf.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_vfprintf.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads --pin-slot-nop _vfprintf_r:4",
    "newlib_reallocr.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_callocr.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_freer.c": ("--barrier-return-store --barrier-branch-move --expand-sym-loads "
                       "--swap-regs _free_r:3-4:33,35,37 "
                       "--rotate-seq _free_r:30:-6,_free_r:30:-6,_free_r:30:-6,"
                       "_free_r:34:3,_free_r:36:2 --retime-branch-slot _free_r"),
    "newlib_mallocr.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_dtoa2.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_signal.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_signalr.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_printf.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_sprintf.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_sscanf.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_atexit.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_exit.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "newlib_strcasecmp.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    # The libgcc float<->DI conversion TUs never let gas fill a delay
    # slot with a preceding copy/ALU op -- barrier both classes
    # (whole-file; each TU is one function).
    "_fixdfdi.c": "--barrier-return-store --barrier-branch-move",
    "_fixunsdfdi.c": "--barrier-return-store --barrier-branch-move",
    "_fixunssfdi.c": "--barrier-return-store --barrier-branch-move",
    "_floatdidf.c": "--barrier-return-store --barrier-branch-move",
    # __deregister_frame_info: the found-path epilogue's ld $16/$17 pair
    # is swapped in the original because `move $2,$16` still reads $16
    # (WAR delay in the original's sched2); see war_restore_swap.
    "gcc_frame.c": ("--war-restore-swap __deregister_frame_info"),
    "_main.c": "--barrier-return-store --barrier-branch-move --expand-sym-loads",
    "fpbit_df.c": "--branch-likely dpmul:5",
    # fabs: SET_HIGH_WORD's final `return x` computes the masked result
    # into a4/a1-family regs and needs an explicit copy into $2; gas's
    # reorder pass steals that copy into the jr $31 delay slot, but the
    # original ee-as build left it as three separate instructions (copy,
    # branch, genuine nop). See RE_RETURN_MOVE in fix_cc_asm.py.
    # --as-g0: libm.c is also assembled with `as -G0` (asflags_for below),
    # which changes when a li.s float literal carries the mtc1 COP1 hazard
    # nop -- see fix_cc_asm.py's ASSUME_NO_LIT4 comment.
    # ... plus: gas steals .lit4 %lo loads / la macros into call slots
    # the original keeps as nops (diagnosed function-by-function; see
    # libm.c's header comment).
    # sinf/cosf/__ieee754_atan2/__ieee754_rem_pio2/scalbn: gcc's
    # delayed-branch pass disagrees with the original over which filled
    # branch gets the annulled (branch-likely) form -- same slot, same
    # target, only the l-bit.  No structural heuristic discriminates, so
    # the sites are named explicitly (FUNC:N = Nth conditional branch of
    # FUNC in asm order; see flip_branch_likely in fix_cc_asm.py).
    # scalbn needs one flip in EACH direction (its likely bit sits on the
    # wrong one of two adjacent branches).
    "libm.c": ("--barrier-return-store fabs,__ieee754_fmod --as-g0 --fp-pair-hazard --barrier-branch-move --barrier-lo-load --rotate cos:27:2,sin:28:2 --pin-slot-nop cos:8,cos:10,sin:7,sin:9,__kernel_rem_pio2:32,__kernel_rem_pio2:42,__kernel_rem_pio2:43,__ieee754_pow:87,__ieee754_pow:95 --short-loop-pad __ieee754_pow:0:3 --branch-likely sinf:2,cosf:2,__ieee754_atan2:2,__ieee754_rem_pio2:5,scalbn:6,__kernel_tanf:2,__ieee754_rem_pio2f:5,cos:3,sin:3 --branch-unlikely scalbn:7"),
    # _dtoa_r: `dpcmp(d.d, 0.0)`'s `a1 = 0` argument setup is left as a
    # separate instruction (with a genuine nop in the jal delay slot) in
    # the original, but gas's reorder pass steals it into the jal's delay
    # slot here. Same RE_RETURN_MOVE fingerprint as fabs/__ieee754_fmod
    # above, just against a `jal` instead of a leaf return.
    "libc.c": "--expand-sym-loads ",
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


def generate_symbol_ld():
    """Write build/symbol_addrs.ld: symbol_addrs.txt minus // comments.

    config/symbol_addrs.txt is splat's format and carries `// type:func`
    annotations. GNU ld's script parser has no `//` comment -- it reads
    the first one as a syntax error -- so the pinned linker script cannot
    INCLUDE that file directly. Splat needs the annotations; ld cannot
    have them. So generate a stripped copy for ld and leave the source of
    truth alone.

    This surfaced when six new symbols were prepended and the link began
    failing at symbol_addrs.txt:7. Worth knowing: a broken link is easy
    to miss, because verify.py checks per-function objects and only
    verify_elf.py looks at the linked image.
    """
    import re as _re
    src = CONFIG_DIR / "symbol_addrs.txt"
    if not src.exists():
        return
    text = _re.sub(r"[ \t]*//.*$", "", src.read_text(), flags=_re.M)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    (BUILD_DIR / "symbol_addrs.ld").write_text(text)


def generate_ninja(asm_files, src_files, asset_files):
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    ninja_path = ROOT / "build.ninja"

    # Link against the PINNED linker script, not Splat's auto-generated
    # config/SLUS_204.69.ld. The auto-generated script is untracked, is
    # rewritten by every `splat split`, and -- critically -- has none of the
    # INCLUDE lines that supply the hardware/absolute symbol definitions
    # (undefined_syms_auto.txt, undefined_funcs_auto.txt, symbol_addrs.txt).
    # tools/post_split.sh used to copy the pinned script over the generated
    # one, but any split run without it silently reverted the build to a
    # script that cannot resolve those symbols. Referencing the pinned file
    # directly removes that failure mode entirely.
    ld_script = CONFIG_DIR / "SLUS_204.69.pinned.ld"
    # Files INCLUDEd by the linker script: ninja must relink when they change.
    ld_deps = [
        CONFIG_DIR / "undefined_funcs_auto.txt",
        CONFIG_DIR / "undefined_syms_auto.txt",
        CONFIG_DIR / "symbol_addrs.txt",
        CONFIG_DIR / "linker_script_extra.ld",
    ]

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
        f.write(f"  command = $cc $cflags -Iinclude -S -o $out.s $in && python3 tools/fix_cc_asm.py $out.s $fixflags && {AS} {CC_ASFLAGS} $asflags -o $out $out.s\n")
        f.write(f"  description = CC $in\n\n")

        f.write(f"rule ld\n")
        # On a link failure, delete any stale ELF so nothing downstream
        # (verify_elf.py, objdiff, PCSX2) can be fooled by an old artifact.
        f.write(f"  command = {LD} {LDFLAGS} -o $out -T $ldscript $in || {{ rm -f $out; false; }}\n")
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
        implicit = " ".join(str(p) for p in [ld_script] + ld_deps)
        f.write(f"build {ELF_PATH}: ld {' '.join(obj_files)} | {implicit}\n")
        f.write(f"  ldscript = {ld_script}\n\n")
        f.write(f"default {ELF_PATH}\n\n")

        # `ninja check` -- whole-image verification. Complements
        # tools/verify.py (per-function) by flattening the linked ELF and
        # diffing it against config/SLUS_204.69.rom. --strict makes any
        # difference a build failure.
        f.write("rule verify_elf\n")
        f.write("  command = python3 tools/verify_elf.py --strict -q && touch $out\n")
        f.write("  description = VERIFY-ELF\n")
        f.write("  pool = console\n\n")
        stamp = BUILD_DIR / ".verify_elf.stamp"
        f.write(f"build {stamp}: verify_elf | {ELF_PATH} tools/verify_elf.py "
                f"{CONFIG_DIR / 'SLUS_204.69.rom'}\n")
        f.write(f"build check: phony {stamp}\n")

        # Regenerate build.ninja whenever configure.py changes. Without
        # this, editing configure.py and running `ninja` rebuilds nothing
        # -- the old build.ninja still carries the old per-file flags, so
        # a file keeps its stale object and verify.py reports a PHANTOM
        # REGRESSION in a file the reader never touched. That has cost
        # several agents hours of chasing failures that recompiled to
        # MATCH from source. `generator = 1` also tells ninja not to
        # treat this edge as dirty work during -n/-t clean.
        f.write("\nrule configure\n")
        f.write("  command = python3 configure.py --no-split\n")
        f.write("  description = CONFIGURE\n")
        f.write("  generator = 1\n\n")
        f.write("build build.ninja: configure | configure.py "
                f"{CONFIG_DIR / 'symbol_addrs.txt'}\n")

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

    generate_symbol_ld()
    generate_ninja(asm_files, src_files, asset_files)
    generate_objdiff()
    print("\nBuild configured! Run 'ninja' to build.")


if __name__ == "__main__":
    main()
