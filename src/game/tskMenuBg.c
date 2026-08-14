/* tskMenuBg - menu background fade task (MenuBg*) */

/* Three fade-alpha bytes; the per-frame driver pokes [1] and [2] through
 * a pointer to the first. The pointer is laundered through an empty asm
 * so the stores keep the retail pointer-plus-offset form instead of being
 * folded back into gp-relative addressing. */
unsigned char MenuBgAlpha[3];
unsigned char MenuBgAlphaNow;

extern int MenuModelTask;

extern void endBackTexDraw(unsigned char *p);
extern void xglTaskExecute(int task);
extern void MenuModelDirectSendZClear(void);

/* Fade the background alpha trio down by 8 each frame, then kick the
 * model task and clear the direct-send Z buffer */
void MenuBgTaskMain(void)
{
    unsigned char *p = MenuBgAlpha;
    unsigned char *q;
    int v;

    __asm__("" : "+r"(p));
    q = p;
    v = *p + 248;
    if (*p != 0) {
        p[2] = v;
        MenuBgAlphaNow = v;
        p[1] = v;
    }
    endBackTexDraw(q);
    xglTaskExecute(MenuModelTask);
    MenuModelDirectSendZClear();
}

/* --- init --- */

typedef struct MENUBGCAM {
    long long pad[0x5F0 / 8];
} MENUBGCAM;

typedef struct MENUBGWORK {
    char pad000[0x820];
    void *pStudio;             /* 0x820 */
    char pad824[0x82C - 0x824];
    unsigned char nR;          /* 0x82C */
    unsigned char nG;          /* 0x82D */
    unsigned char nB;          /* 0x82E */
    unsigned char nA;          /* 0x82F */
    float fX;                  /* 0x830 */
    int n834;                  /* 0x834 */
    float fY;                  /* 0x838 */
    float fZ;                  /* 0x83C */
} MENUBGWORK;

MENUBGWORK *MenuBgWork;
static float MenuBgSpeedZ = 0.2f;
MENUBGCAM *MenuBgCam;
void *MenuBgStudio;

extern int xglTaskInitial(void *base, int n, int m);
extern MENUBGCAM *xglStudioGetCamera2(void *studio);
extern void xglCameraInit(MENUBGCAM *cam);
extern void *xglStudioGetActiveCamera(void);
extern void tyaMenuBgEntry(int task, MENUBGWORK *w);

void *MenuBgTaskInit(void *mem, void *studio)
{
    MENUBGWORK *w;
    MENUBGCAM *cam2;
    MENUBGCAM *dst;
    MENUBGCAM *cam;
    char *end;
    int aligned;
    int i;
    unsigned char *p;
    char c;
    float fz;

    aligned = ((int)mem + 15) & ~15;
    MenuModelTask = aligned;
    w = (MENUBGWORK *)((xglTaskInitial((void *)aligned, 16, 0) + 127) & ~127);
    MenuBgWork = w;
    cam2 = xglStudioGetCamera2(studio);
    dst = (MENUBGCAM *)(((int)w + 0x84F) & ~15);
    MenuBgCam = dst;
    end = (char *)dst + 0x5F0;
    *dst = *cam2;
    MenuBgStudio = studio;
    cam = xglStudioGetCamera2(studio);
    xglCameraInit(cam);
    if (xglStudioGetActiveCamera() == 0) {
        *(int *)cam = 1;
    }
    MenuBgWork->pStudio = studio;
    MenuBgWork->nR = 27;
    MenuBgWork->nG = 27;
    fz = MenuBgSpeedZ;
    MenuBgWork->nB = 25;
    MenuBgWork->nA = 0x80;
    MenuBgWork->fX = 0.1f;
    MenuBgWork->fY = 0.2f;
    MenuBgWork->fZ = fz;
    MenuBgWork->n834 = 0;
    tyaMenuBgEntry(MenuModelTask, MenuBgWork);
    c = 0x80;
    i = 2;
    p = &MenuBgAlpha[2];
    do {
        i--;
        *p = c;
        p--;
    } while (i >= 0);
    return end;
}
