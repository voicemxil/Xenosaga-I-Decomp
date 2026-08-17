#!/usr/bin/env python3
"""Disassemble VU0 microcode out of the retail ELF.

    python3 tools/vudis.py                    # every labelled routine
    python3 tools/vudis.py MatrixStackTrans   # just one

The VU0 microprogram blob is uploaded by xglGeometryInit (see
src/xgl/xglMath.c) and its entry points are the `Vu0Call*` symbols in
config/symbol_addrs.txt -- those are VU MICROMEM byte addresses, while
the `_$Vu0*` symbols are the same code sitting in EE main memory.
micromem = main - _$Vu0MicroCodeStart (0x004ADE90), confirmed by
Vu0CallSin: 0x004ADEB0 - 0x20 == that base.

Nothing else in the tree can read this code: Ghidra's
EmotionEngineReloaded plugin only defines COP2 MACRO-mode pcodeops
(data/languages/vuupper.sinc), not a VU micromode language, and
objdump has no VU support. But the microcode is exactly where the
xglMatrixStack* semantics live, so a port cannot reimplement them
without it.

ENCODING. Each instruction is 64 bits stored as two little-endian
words: word[0] = LOWER instruction, word[1] = UPPER instruction.
Upper: dest mask in bits 24-21 as x,y,z,w; ft 20-16; fs 15-11;
fd 10-6; opcode 5-0. Flags I/E/M/D/T in bits 31..27 -- E marks the
last instruction of a microprogram (two more issue after it).
Lower: primary opcode in bits 31-25; the 0x40 group is "lower2" with
a sub-opcode in bits 10-6 alongside bits 5-0.

VALIDATED against ground truth before use: MatrixStackUnit decodes to
a clean identity build (MULx zeroing exactly the complementary
off-diagonal lanes of vf28/vf29, ADDw setting each diagonal to
vf0.w == 1.0, MOVE vf31,vf0 and MR32 vf30,vf0 for rows 3 and 2), and
Vu0Sin/Vu0Cos decode as a polynomial series. If either of those ever
stops looking right, distrust the tables here before trusting the
output.
"""
import argparse, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from elflib import Elf32

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ELF  = os.path.join(ROOT, "elf", "SLUS_204.69")
SYMS = os.path.join(ROOT, "config", "symbol_addrs.txt")
MICRO_BASE = 0x004ADE90
BC = "xyzw"

UPPER = {}
for i, b in enumerate(BC):
    UPPER[0x00+i]="ADD"+b; UPPER[0x04+i]="SUB"+b; UPPER[0x08+i]="MADD"+b
    UPPER[0x0C+i]="MSUB"+b; UPPER[0x10+i]="MAX"+b; UPPER[0x14+i]="MINI"+b
    UPPER[0x18+i]="MUL"+b
UPPER.update({0x1C:"MULq",0x1D:"MAXi",0x1E:"MULi",0x1F:"MINIi",0x20:"ADDq",
    0x21:"MADDq",0x22:"ADDi",0x23:"MADDi",0x24:"SUBq",0x25:"MSUBq",0x26:"SUBi",
    0x27:"MSUBi",0x28:"ADD",0x29:"MADD",0x2A:"MUL",0x2B:"MAX",0x2C:"SUB",
    0x2D:"MSUB",0x2E:"OPMSUB",0x2F:"MINI"})
UPPER_EXT = {}
for i, b in enumerate(BC):
    UPPER_EXT[(0x3C+i,0x00)]="ADDA"+b; UPPER_EXT[(0x3C+i,0x01)]="SUBA"+b
    UPPER_EXT[(0x3C+i,0x02)]="MADDA"+b; UPPER_EXT[(0x3C+i,0x03)]="MSUBA"+b
    UPPER_EXT[(0x3C+i,0x06)]="MULA"+b
for i, s in enumerate(("0","4","12","15")):
    UPPER_EXT[(0x3C+i,0x04)]="ITOF"+s; UPPER_EXT[(0x3C+i,0x05)]="FTOI"+s
UPPER_EXT.update({(0x3C,0x07):"MULAq",(0x3D,0x07):"ABS",(0x3E,0x07):"MULAi",
    (0x3F,0x07):"CLIP",(0x3C,0x08):"ADDAq",(0x3D,0x08):"MADDAq",
    (0x3E,0x08):"ADDAi",(0x3F,0x08):"MADDAi",(0x3C,0x09):"SUBAq",
    (0x3D,0x09):"MSUBAq",(0x3E,0x09):"SUBAi",(0x3F,0x09):"MSUBAi",
    (0x3C,0x0A):"ADDA",(0x3D,0x0A):"MADDA",(0x3E,0x0A):"MULA",
    (0x3C,0x0B):"SUBA",(0x3D,0x0B):"MSUBA",(0x3E,0x0B):"OPMULA",
    (0x3F,0x0B):"NOP"})

LOWER = {0x00:"LQ",0x01:"SQ",0x04:"ILW",0x05:"ISW",0x08:"IADDIU",0x09:"ISUBIU",
    0x10:"FCEQ",0x11:"FCSET",0x12:"FCAND",0x13:"FCOR",0x14:"FSEQ",0x15:"FSSET",
    0x16:"FSAND",0x17:"FSOR",0x18:"FMEQ",0x1A:"FMAND",0x1B:"FMOR",0x1C:"FCGET",
    0x20:"B",0x21:"BAL",0x24:"JR",0x25:"JALR",0x28:"IBEQ",0x29:"IBNE",
    0x2C:"IBLTZ",0x2D:"IBGTZ",0x2E:"IBLEZ",0x2F:"IBGEZ"}
LOWER_EXT = {(0x30,None):"IADD",(0x31,None):"ISUB",(0x32,None):"IADDI",
    (0x34,None):"IAND",(0x35,None):"IOR"}
L2 = {(0x3C,0x0C):"MOVE",(0x3D,0x0C):"MR32",(0x3C,0x0D):"LQI",(0x3D,0x0D):"SQI",
    (0x3E,0x0D):"LQD",(0x3F,0x0D):"SQD",(0x3C,0x0E):"DIV",(0x3D,0x0E):"SQRT",
    (0x3E,0x0E):"RSQRT",(0x3C,0x0F):"MTIR",(0x3D,0x0F):"MFIR",(0x3E,0x0F):"ILWR",
    (0x3F,0x0F):"ISWR",(0x3C,0x10):"RNEXT",(0x3D,0x10):"RGET",(0x3E,0x10):"RINIT",
    (0x3F,0x10):"RXOR",(0x3C,0x11):"WAITQ",(0x3D,0x11):"MFP",(0x3E,0x11):"XTOP",
    (0x3F,0x11):"XGKICK",(0x3C,0x12):"ESADD",(0x3D,0x12):"ERSADD",
    (0x3E,0x12):"ELENG",(0x3F,0x12):"ERLENG",(0x3C,0x13):"EATANxy",
    (0x3D,0x13):"EATANxz",(0x3E,0x13):"ESUM",(0x3C,0x14):"ESQRT",
    (0x3D,0x14):"ERSQRT",(0x3E,0x14):"ERCPR",(0x3F,0x14):"WAITP",
    (0x3C,0x15):"ESIN",(0x3D,0x15):"EATAN",(0x3E,0x15):"EEXP"}

def dest(m):
    s = "".join(c for i, c in enumerate(BC) if m & (8 >> i))
    return ("." + s) if s else ".(none)"

def dis_upper(w):
    op, sub = w & 0x3F, (w >> 6) & 0x1F
    fd, fs, ft, d = (w>>6)&0x1F, (w>>11)&0x1F, (w>>16)&0x1F, (w>>21)&0xF
    flags = "".join(c for c, b in (("I",31),("E",30),("M",29),("D",28),("T",27)) if w & (1<<b))
    if op >= 0x3C:
        name = UPPER_EXT.get((op, sub), f"?up{op:02x}.{sub:02x}")
        if name == "NOP": return "nop", flags
        if name.startswith(("ITOF","FTOI","ABS","CLIP")):
            return f"{name}{dest(d)} vf{ft:02d}, vf{fs:02d}", flags
        if name.endswith(("A","Aq","Ai")) or name in ("ADDA","SUBA","MULA","MADDA","MSUBA","OPMULA"):
            return f"{name}{dest(d)} ACC, vf{fs:02d}, vf{ft:02d}", flags
        return f"{name}{dest(d)} ACC, vf{fs:02d}, vf{ft:02d}{BC[op&3] if name[-1] in BC else ''}", flags
    name = UPPER.get(op, f"?up{op:02x}")
    if name[-1] in BC and name[:-1] in ("ADD","SUB","MADD","MSUB","MAX","MINI","MUL"):
        return f"{name}{dest(d)} vf{fd:02d}, vf{fs:02d}, vf{ft:02d}{name[-1]}", flags
    if name.endswith(("q","i")):
        return f"{name}{dest(d)} vf{fd:02d}, vf{fs:02d}, {name[-1].upper()}", flags
    return f"{name}{dest(d)} vf{fd:02d}, vf{fs:02d}, vf{ft:02d}", flags

def dis_lower(w):
    if w == 0: return "nop"
    top = (w >> 25) & 0x7F
    ft, fs, d = (w>>16)&0x1F, (w>>11)&0x1F, (w>>21)&0xF
    if top == 0x40:
        op, sub = w & 0x3F, (w >> 6) & 0x1F
        name = L2.get((op, sub))
        if name is None:
            n2 = LOWER_EXT.get((w & 0x3F, None))
            if n2: return f"{n2} vi{(w>>6)&0x1F:02d}, vi{fs:02d}, vi{ft:02d}"
            return f"?lo{op:02x}.{sub:02x}"
        if name in ("MOVE","MR32","LQI","SQI","LQD","SQD"):
            return f"{name}{dest(d)} vf{ft:02d}, vf{fs:02d}"
        if name in ("MTIR","MFIR","ILWR","ISWR"):
            return f"{name}{dest(d)} vi{ft:02d}, vf{fs:02d}"
        if name in ("DIV","SQRT","RSQRT"):
            return f"{name} Q, vf{fs:02d}{BC[(w>>21)&3]}, vf{ft:02d}{BC[(w>>23)&3]}"
        return f"{name}{dest(d)} vf{ft:02d}, vf{fs:02d}"
    name = LOWER.get(top, f"?lo{top:02x}")
    if name in ("LQ","SQ"):
        # LQ vft, off(vis) but SQ vfs, off(vit) -- the vf and vi fields
        # swap roles between them. Getting this wrong printed "vi28",
        # which is impossible (VU has only vi00-vi15) and is what caught
        # the bug.
        off = w & 0x7FF
        if off & 0x400: off -= 0x800
        if name == "LQ": return f"LQ{dest(d)} vf{ft:02d}, {off}(vi{fs:02d})"
        return f"SQ{dest(d)} vf{fs:02d}, {off}(vi{ft:02d})"
    if name in ("IADDIU","ISUBIU"):
        imm = ((w >> 10) & 0x7800) | (w & 0x7FF)
        return f"{name} vi{ft:02d}, vi{fs:02d}, 0x{imm:x}"
    if name in ("B","BAL"):
        off = w & 0x7FF
        if off & 0x400: off -= 0x800
        return f"{name} {off:+d}"
    return f"{name} vi{ft:02d}, vi{fs:02d}"

def load_syms():
    import re
    micro, main = {}, {}
    for l in open(SYMS):
        m = re.match(r'\s*(_\$)?(Vu0\w+)\s*=\s*(0x[0-9A-Fa-f]+)', l)
        if m:
            (main if m.group(1) else micro)[m.group(2)] = int(m.group(3), 16)
    return micro, main

def main_():
    ap = argparse.ArgumentParser()
    ap.add_argument("routine", nargs="?")
    a = ap.parse_args()
    micro, mainm = load_syms()
    elf = Elf32(ELF)
    ordered = sorted(((v, k) for k, v in mainm.items()))
    for idx, (addr, name) in enumerate(ordered):
        short = name[3:] if name.startswith("Vu0") else name
        if a.routine and a.routine.lower() not in short.lower(): continue
        end = ordered[idx+1][0] if idx+1 < len(ordered) else addr+0x40
        n = max(0, (end-addr)//8)
        if n == 0 or n > 64: n = min(max(n,1), 64)
        words = elf.words_at_vaddr(addr, n*8)
        print(f"\n=== {short}  micromem 0x{addr-MICRO_BASE:04X}  ({n} instr) ===")
        for i in range(0, len(words)-1, 2):
            lo, up = words[i], words[i+1]
            u, fl = dis_upper(up)
            print(f"  {(addr-MICRO_BASE)+i*4:04X}: {u:<42} | {dis_lower(lo):<34}{(' ['+fl+']') if fl else ''}")
    return 0

if __name__ == "__main__":
    sys.exit(main_())
