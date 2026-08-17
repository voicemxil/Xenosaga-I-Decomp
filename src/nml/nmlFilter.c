/* Post-effect filter packet builders for the normal-map model renderer */

#include "matching.h"

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef struct {
    char pad[0x234];
    int nFilterFlag;         /* 0x234 */
} LAYOUT_FILTER;

void nmlModelSetFilterGunosys(LAYOUT_FILTER *pLayout, int nFlag, void *pTag);
void nmlModelSetFilterStealth(LAYOUT_FILTER *pLayout, int nFlag);

/* Dispatch the active filter flag bit to its Gunosys/Stealth handler.
 * The loop has no early exit: every bit 0..30 is tested, and nFlags stays
 * live across the calls, which is why the original parks pLayout/pTag and
 * the four loop constants in s0-s7. A switch (not an if/else-if chain) is
 * what emits the two forward beq-to-case-body branches plus the trailing
 * `b` to the loop test. */
void nmlFilterSetPacket(LAYOUT_FILTER *pLayout, void *pTag)
{
    int i;
    int nFlags;
    int v;

    nFlags = pLayout->nFilterFlag;
    for (i = 0; i < 31; i++) {
        v = nFlags & (1 << i);
        switch (v) {
        case 1:
            nFlags &= ~1;
            nmlModelSetFilterGunosys(pLayout, nFlags, pTag);
            break;
        case 2:
            nFlags &= ~2;
            nmlModelSetFilterStealth(pLayout, nFlags);
            break;
        }
    }
}

/* --- Scratchpad GS-entry filter primitives --- */

typedef struct {
    u_long lData;
    union {
        u_long l;
        u_int w[2];
    } uReg;
} GSENTRY;

typedef struct {
    char pad0[2];
    short nPsm;             /* 0x02 */
    char pad4[0x10];
    u_short nDrawFbp;       /* 0x14 */
} FRENDER;

extern FRENDER sRender;
extern u_short D_004B90F4[];
extern u_short D_004B9100[];

void nmlPacketGsInit(void);
void nmlPacketAddGsZbuf(u_int nZmsk);
void nmlPacketAddGsFrame(u_int nFbp, u_int nFbmsk);
void nmlPacketAddGsFBA(u_int nFba);
void nmlPacketAddGsFlush(void);
void packet_gs_entry64(u_int nReg, u_long *pData);

/* Queue a back-buffer clear: TEST/FRAME-mask/PRIM plus a full-screen
 * sprite, then flush */
void nmlFilterBackClear(void)
{
    GSENTRY *p;
    int i;

    ((GSENTRY *)0x70000000)->lData = 0x30000;
    ((GSENTRY *)0x70000000)->uReg.l = 71;
    ((GSENTRY *)0x70000010)->lData = (u_long)0xFE00 << 46;
    ((GSENTRY *)0x70000010)->uReg.l = 1;
    ((GSENTRY *)0x70000020)->lData = 6;
    ((GSENTRY *)0x70000020)->uReg.l = 0;
    ((GSENTRY *)0x70000030)->lData = 0x72007000;
    ((GSENTRY *)0x70000030)->uReg.l = 4;
    ((GSENTRY *)0x70000040)->lData = 0x8E009000;
    ((GSENTRY *)0x70000040)->uReg.l = 4;
    p = (GSENTRY *)0x70000000;
    nmlPacketGsInit();
    nmlPacketAddGsZbuf(1);
    nmlPacketAddGsFrame(D_004B90F4[0], 0);
    i = 4;
    do {
        packet_gs_entry64(p->uReg.w[0], (u_long *)p);
        p++;
        i--;
    } while (i >= 0);
    nmlPacketAddGsFlush();
}

/* Queue a texture-buffer clear sprite, then flush */
void nmlFilterSetTexClear(void)
{
    GSENTRY *p;
    int i;

    ((GSENTRY *)0x70000000)->lData = 138;
    ((GSENTRY *)0x70000000)->uReg.l = 66;
    ((GSENTRY *)0x70000010)->lData = 0x31001;
    ((GSENTRY *)0x70000010)->uReg.l = 71;
    ((GSENTRY *)0x70000020)->lData = ((u_long)0x3F800000 << 32) | 0x80000000;
    ((GSENTRY *)0x70000020)->uReg.l = 1;
    ((GSENTRY *)0x70000030)->lData = 70;
    ((GSENTRY *)0x70000030)->uReg.l = 0;
    ((GSENTRY *)0x70000040)->lData = 0x72007000;
    ((GSENTRY *)0x70000040)->uReg.l = 4;
    ((GSENTRY *)0x70000050)->lData = 0x7FF09000;
    ((GSENTRY *)0x70000050)->uReg.l = 4;
    p = (GSENTRY *)0x70000000;
    nmlPacketGsInit();
    i = 5;
    do {
        packet_gs_entry64(p->uReg.w[0], (u_long *)p);
        p++;
        i--;
    } while (i >= 0);
    nmlPacketAddGsFlush();
}

/* Queue a frame-alpha clear sprite over the draw buffer, then flush */
void nmlFilterSetFrameAlphaClear(void)
{
    GSENTRY *p;
    int i;

    ((GSENTRY *)0x70000000)->lData = 104;
    ((GSENTRY *)0x70000000)->uReg.l = 66;
    ((GSENTRY *)0x70000010)->lData = 0x31001;
    ((GSENTRY *)0x70000010)->uReg.l = 71;
    ((GSENTRY *)0x70000020)->lData = ((u_long)0x3F800000 << 32) | 0x1000000;
    ((GSENTRY *)0x70000020)->uReg.l = 1;
    ((GSENTRY *)0x70000030)->lData = 70;
    ((GSENTRY *)0x70000030)->uReg.l = 0;
    ((GSENTRY *)0x70000040)->lData = 0x72007000;
    ((GSENTRY *)0x70000040)->uReg.l = 4;
    ((GSENTRY *)0x70000050)->lData = 0x8E009000;
    ((GSENTRY *)0x70000050)->uReg.l = 4;
    p = (GSENTRY *)0x70000000;
    nmlPacketGsInit();
    nmlPacketAddGsFrame(D_004B9100[0], 0);
    nmlPacketAddGsFBA(0);
    i = 5;
    do {
        packet_gs_entry64(p->uReg.w[0], (u_long *)p);
        p++;
        i--;
    } while (i >= 0);
    nmlPacketAddGsFlush();
}

/* Queue a local-to-local transfer of the frame into a texture buffer */
void nmlFilterSetFrameToBuffer(int nFbp)
{
    GSENTRY *p;
    int i;

    ((GSENTRY *)0x70000000)->lData = 0;
    ((GSENTRY *)0x70000000)->uReg.l = 63;
    ((GSENTRY *)0x70000010)->lData = 0x80000 | ((nFbp & 0xFFFF) << 5)
        | ((u_long)sRender.nPsm << 24) | ((u_long)sRender.nDrawFbp << 37)
        | ((u_long)0x8000 << 36) | ((u_long)sRender.nPsm << 56);
    ((GSENTRY *)0x70000010)->uReg.l = 80;
    ((GSENTRY *)0x70000020)->lData = 0;
    ((GSENTRY *)0x70000020)->uReg.l = 81;
    ((GSENTRY *)0x70000030)->lData = ((u_long)448 << 32) | 0x200;
    ((GSENTRY *)0x70000030)->uReg.l = 82;
    ((GSENTRY *)0x70000040)->lData = 2;
    ((GSENTRY *)0x70000040)->uReg.l = 83;
    p = (GSENTRY *)0x70000000;
    nmlPacketGsInit();
    i = 4;
    do {
        packet_gs_entry64(p->uReg.w[0], (u_long *)p);
        p++;
        i--;
    } while (i >= 0);
    nmlPacketAddGsFlush();
}

/* Queue a local-to-local transfer of a texture buffer back into the
 * frame */
void nmlFilterSetBufferToFrame(int nFbp)
{
    GSENTRY *p;
    int i;

    ((GSENTRY *)0x70000000)->lData = 0;
    ((GSENTRY *)0x70000000)->uReg.l = 63;
    ((GSENTRY *)0x70000010)->lData = 0x80000 | (sRender.nDrawFbp << 5)
        | ((u_long)sRender.nPsm << 24) | ((u_long)(nFbp & 0xFFFF) << 37)
        | ((u_long)0x8000 << 36) | ((u_long)sRender.nPsm << 56);
    ((GSENTRY *)0x70000010)->uReg.l = 80;
    ((GSENTRY *)0x70000020)->lData = 0;
    ((GSENTRY *)0x70000020)->uReg.l = 81;
    ((GSENTRY *)0x70000030)->lData = ((u_long)448 << 32) | 0x200;
    ((GSENTRY *)0x70000030)->uReg.l = 82;
    ((GSENTRY *)0x70000040)->lData = 2;
    ((GSENTRY *)0x70000040)->uReg.l = 83;
    p = (GSENTRY *)0x70000000;
    nmlPacketGsInit();
    i = 4;
    do {
        packet_gs_entry64(p->uReg.w[0], (u_long *)p);
        p++;
        i--;
    } while (i >= 0);
    nmlPacketAddGsFlush();
}

/* Load the object-to-global place matrix into VU0 vf10-vf13. */
void _WeightToGlobalPlaceInit(void *pMtx)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf10, 0x0(%0)\n"
            "lqc2 $vf11, 0x10(%0)\n"
            "lqc2 $vf12, 0x20(%0)\n"
            "lqc2 $vf13, 0x30(%0)\n"
            ".set reorder" : : "r"(pMtx));
}

/* Transpose the 3x3 weight matrix at pSrc, transform its z-basis by the
 * current matrix and then by the vf10-vf13 place matrix loaded by
 * _WeightToGlobalPlaceInit; the result lands in pDst.  The block uses
 * $v0-$a3/$t0-$t1 by name for the pextlw/pcpyld transpose, which is why
 * the three pointers arrive in $t2-$t4. */
void _WeightToGlobalPlaceVec(void *pDst, void *pMtx, void *pSrc)
{
    PS2_ASM(".set noreorder\n"
            "lq $2, 0x0(%2)\n"
            "lq $3, 0x10(%2)\n"
            "lq $4, 0x20(%2)\n"
            "lqc2 $vf3, 0x30(%2)\n"
            "qmfc2 $5, $vf0\n"
            "pextlw $6, $3, $2\n"
            "pextuw $7, $3, $2\n"
            "pextlw $8, $5, $4\n"
            "pextuw $9, $5, $4\n"
            "pcpyld $2, $8, $6\n"
            "pcpyud $3, $6, $8\n"
            "pcpyld $4, $9, $7\n"
            "qmtc2 $2, $vf5\n"
            "qmtc2 $3, $vf6\n"
            "qmtc2 $4, $vf7\n"
            "vmulax.xyz $ACC, $vf5, $vf3x\n"
            "vmadday.xyz $ACC, $vf6, $vf3y\n"
            "vmaddz.xyz $vf2, $vf7, $vf3z\n"
            "vmove.w $vf2, $vf0\n"
            "lqc2 $vf27, 0x0(%1)\n"
            "lqc2 $vf28, 0x10(%1)\n"
            "lqc2 $vf29, 0x20(%1)\n"
            "lqc2 $vf30, 0x30(%1)\n"
            "vsub.xyz $vf2, $vf0, $vf2\n"
            "vmulax.xyzw $ACC, $vf27, $vf2x\n"
            "vmadday.xyzw $ACC, $vf28, $vf2y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf2z\n"
            "vmaddw.xyzw $vf31, $vf30, $vf2w\n"
            "vmulax.xyzw $ACC, $vf10, $vf31x\n"
            "vmadday.xyzw $ACC, $vf11, $vf31y\n"
            "vmaddaz.xyzw $ACC, $vf12, $vf31z\n"
            "vmaddw.xyzw $vf31, $vf13, $vf0w\n"
            "sqc2 $vf31, 0x0(%0)\n"
            ".set reorder"
            : : "r"(pDst), "r"(pMtx), "r"(pSrc)
            : "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9", "memory");
}

/* Queue the buffer-render pass: a twelve-entry A+D block (TEX0 pointing
 * at the draw buffer, an ALPHA/TEST setup and two sprites) whose PRIM
 * depends on whether the blend factor is below 1.0, then flush. */
/* Byte-exact.  The fixed-register C local closes the last allocator tie:
 * the original puts the %hi of sRender in $v1 and the loaded nDrawFbp in
 * $v0; unpinned natural expressions choose the opposite pair.
 * TODO: recover the original source shape that selected $v0 naturally.
 * Swept and REJECTED: both operand orders of the `| 0x24020000`, a u_int
 * local for the field, `* 32` instead of `<< 5`, a `FRENDER *` local, a
 * raw `*(u_short *)((char *)&sRender + 0x14)`, an alias array symbol at
 * the same address (all exactly 4); writing the 0x70000010 entry before
 * the 0x70000000 one (24); PIN($v0) + LAUNDER_V on the field (126 words).
 * Load-bearing and NOT to be re-derived: the 0x24020000 must be ORed
 * with the shifted field FIRST and the 64-bit constant added after, or
 * gcc folds the two constants together and the function is a word short.
 */
/* Queue the buffer-render pass: a twelve-entry A+D block (TEX0 pointing
 * at the draw buffer, an ALPHA/TEST setup and two sprites) whose PRIM
 * depends on whether the blend factor is below 1.0, then flush. */
void nmlFilterSetBufferRender(float fLevel)
{
    GSENTRY *p;
    register u_int nFbp __asm__("$2");
    u_int nTex0;
    int i;

    nFbp = sRender.nDrawFbp;
    nFbp <<= 5;
    nTex0 = nFbp | 0x24020000;
    ((GSENTRY *)0x70000000)->lData = 0;
    ((GSENTRY *)0x70000000)->uReg.l = 63;
    ((GSENTRY *)0x70000010)->lData =
        nTex0
        | (((u_long)0x20000006 << 32) | 0x40000000);
    ((GSENTRY *)0x70000010)->uReg.l = 6;
    ((GSENTRY *)0x70000020)->lData = 0;
    ((GSENTRY *)0x70000020)->uReg.l = 20;
    ((GSENTRY *)0x70000030)->lData = 0;
    ((GSENTRY *)0x70000030)->uReg.l = 8;
    ((GSENTRY *)0x70000040)->lData = 0x31001;
    ((GSENTRY *)0x70000040)->uReg.l = 71;
    ((GSENTRY *)0x70000050)->lData =
        ((u_long)(int)(fLevel * 128.0f) << 32) | 100;
    ((GSENTRY *)0x70000050)->uReg.l = 66;
    ((GSENTRY *)0x70000060)->lData = ((u_long)0x3F800000 << 32) | 0x80808080;
    ((GSENTRY *)0x70000060)->uReg.l = 1;
    ((GSENTRY *)0x70000070)->lData = 278;
    if (fLevel < 1.0f) {
        ((GSENTRY *)0x70000070)->lData = 342;
    }
    ((GSENTRY *)0x70000070)->uReg.l = 0;
    ((GSENTRY *)0x70000080)->lData = 0;
    ((GSENTRY *)0x70000080)->uReg.l = 3;
    ((GSENTRY *)0x70000090)->lData = 0x72007000;
    ((GSENTRY *)0x70000090)->uReg.l = 4;
    ((GSENTRY *)0x700000A0)->lData = 0x1C102000;
    ((GSENTRY *)0x700000A0)->uReg.l = 3;
    ((GSENTRY *)0x700000B0)->lData = 0x8E009000;
    ((GSENTRY *)0x700000B0)->uReg.l = 4;
    p = (GSENTRY *)0x70000000;
    nmlPacketGsInit();
    i = 11;
    do {
        packet_gs_entry64(p->uReg.w[0], (u_long *)p);
        p++;
        i--;
    } while (i >= 0);
    nmlPacketAddGsFlush();
}
