/* Zero-init helpers for various subsystem state blocks (model system, back buffer, fade control, etc) */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u_int;

int s_nAlphaGroup;
int s_nNonAlphaGroup;

typedef struct {
    int a00;
    int a04;
    int a08;
    int a0C;
    int a10;
    int a14;
    int a18;
    int a1C;
    int a20;
    int a24;
} BACKBUF;

BACKBUF s_inBackBuffer;

int s_nParentBuf;

typedef struct {
    int nUnk00;
    int nUnk04;
} MAP_HANDLE;

typedef struct {
    u8 pad00[0x10];
    int a10;
    int a14;
    int a18;
    u8 pad1C[0x8];
    int a24;
    int a28;
    int a2C;
    int a30;
} FADECTRL;

/* Clears both alpha-group draw-block counters */
void CONSTRUCT_ALPHA_GROUP(void)
{
    s_nAlphaGroup = 0;
    s_nNonAlphaGroup = 0;
}

/* Resets the sub-window back-buffer descriptor to its default state */
void CONSTRUCT_BACK_BUFFER(void)
{
    s_inBackBuffer.a10 = -1;
    s_inBackBuffer.a14 = 0;
    s_inBackBuffer.a00 = 0;
    s_inBackBuffer.a04 = 0;
    s_inBackBuffer.a08 = 0;
    s_inBackBuffer.a0C = 0;
    s_inBackBuffer.a18 = 0;
    s_inBackBuffer.a1C = 0;
    s_inBackBuffer.a20 = 0;
    s_inBackBuffer.a24 = 0;
}

/* Clears the parent-buffer handle */
void CONSTRUCT_PARENT_BUF(void)
{
    s_nParentBuf = 0;
}

/* Clears a map handle pair */
void CONSTRUCT_MAP_HANDLE(MAP_HANDLE *p)
{
    p->nUnk04 = 0;
    p->nUnk00 = 0;
}

/* Resets a fade-control block to its default (inactive) state */
void CONSTRUCT_FADE_CONTROL(FADECTRL *p)
{
    p->a10 = -1;
    p->a14 = 1;
    p->a18 = 0;
    p->a24 = 0;
    p->a28 = 0;
    p->a2C = 0;
    p->a30 = 0;
}
