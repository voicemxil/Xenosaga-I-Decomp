/* Small draw-command wrappers. */

extern int D_003626B0[];
extern void sceVif1PkAddDirectDataN(void *packet, void *data, int count);
extern float D_0099C800[];
extern void xglMatrixStackUnit(void);
extern void xglMatrixStackTrans(float *translation);
extern void xglMatrixStackSave(float *matrix);
extern void drawAxis(float *matrix, float scale);

void DrawShadow(void *draw)
{
    sceVif1PkAddDirectDataN(*(void **)draw, D_003626B0, 7);
}

void drawCursor(void)
{
    float matrix[16];

    D_0099C800[3] = 1.0f;
    xglMatrixStackUnit();
    xglMatrixStackTrans(D_0099C800);
    xglMatrixStackSave(matrix);
    drawAxis(matrix, 1.0f);
}

typedef unsigned char u8;
typedef unsigned long draw_u64;

/* Draw.c's view of xgl's shared render state */
typedef struct {
    u8 pad00[0x14];             /* 0x00 */
    unsigned short nDrawFbp;    /* 0x14 */
    u8 pad16[0x20 - 0x16];      /* 0x16 */
    unsigned short nFrontFbp;   /* 0x20 */
} DRAW_RENDER;

extern DRAW_RENDER sRender;

/* A draw work block whose tail is the DIRECT GS packet it submits */
typedef struct {
    void *packet;               /* 0x00 */
    u8 pad04[0x2C];             /* 0x04 */
    draw_u64 direct[8];         /* 0x30 */
} DRAW_BACK_PACKET;

/* Point the GS back at the front buffer (end of a full-screen effect) */
void DrawBackReset(void *work)
{
    DRAW_BACK_PACKET *packet = (DRAW_BACK_PACKET *)work;
    draw_u64 *direct = packet->direct;

    direct[0] = 0x1000000000008003UL;
    direct[1] = 0xE;
    direct[2] = 0;
    direct[3] = 0x3F;
    direct[4] = 0x31000000;
    direct[5] = 0x4E;
    direct[6] = sRender.nFrontFbp | 0x80000;
    direct[7] = 0x4C;
    sceVif1PkAddDirectDataN(packet->packet, direct, 4);
}

/* Redirect the GS to the draw buffer (start of a full-screen effect) */
void DrawBackSet(void *work)
{
    DRAW_BACK_PACKET *packet = (DRAW_BACK_PACKET *)work;
    draw_u64 *direct = packet->direct;

    direct[0] = 0x1000000000008003UL;
    direct[1] = 0xE;
    direct[2] = 0;
    direct[3] = 0x3F;
    direct[4] = 0x100000000UL;
    direct[5] = 0x4E;
    direct[6] = sRender.nDrawFbp | 0x80000;
    direct[7] = 0x4C;
    sceVif1PkAddDirectDataN(packet->packet, direct, 4);
}
