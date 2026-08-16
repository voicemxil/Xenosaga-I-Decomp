/* Post-effect filter packet builders for the normal-map model renderer */

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
