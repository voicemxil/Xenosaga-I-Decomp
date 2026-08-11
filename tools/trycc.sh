#!/bin/bash
# Compile a standalone snippet through the real game-code pipeline and
# objdump one function. Usage: trycc.sh <src.c> <FuncName>
IN="$1"; FN="$2"
BASE="/tmp/trycc_$$"
/opt/ee-gcc2.96/bin/ee-gcc -O2 -G8 -S -o "$BASE.s" "$IN" 2>&1 | head -5
python3 /work/tools/fix_cc_asm.py "$BASE.s"
/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-as -march=r5900 -mabi=eabi --no-warn -o "$BASE.o" "$BASE.s" 2>&1 | head -5
FOFF=$(/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump -t "$BASE.o" | awk -v f="$FN" '$NF==f && /F .text/{print $1}')
/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump -drz --start-address=0x$FOFF "$BASE.o" 2>/dev/null | awk "/<$FN>:/{p=1} p{print} p&&NR>1&&/jr.*ra/{c++} c&&/^\$/{exit}" | grep -E ":\s+[0-9a-f]{8}" | sed -E 's/^ +[0-9a-f]+:\t[0-9a-f ]{8,} \t?//' | head -40
rm -f "$BASE.s" "$BASE.o"
