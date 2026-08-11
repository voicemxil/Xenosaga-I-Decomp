/* Occlusion-culling helpers for the xgl engine */

typedef struct {
    unsigned char aData[0x160];
} XGLCULLINGCELL;

typedef struct {
    char szName[0x20];
    XGLCULLINGCELL aCell[10];
    int nCount;
    int *pMap;
    int nModel;
} XGLCULLING;

XGLCULLING s_inCulling;
int s_nIgnoreCulling;

void setup_occlusion(XGLCULLINGCELL *pCell, void *p);
int check_occlusion(XGLCULLINGCELL *pCell, void *p1, void *p2);
void culling_cell_disp(XGLCULLINGCELL *pCell);
void nmlModelSetMapLastEntry(int nModel, int nEntry);

/* Test every culling cell until one occludes the given object */
int xglCullingCheck(void *p1, void *p2)
{
    int nRet;
    int i;

    nRet = 0;
    if (s_nIgnoreCulling == 0) {
        /* NEARLY MATCHED: one dead delay-slot copy differs — the original
         * fills the blez delay slot with `move s2,a0` (i = nRet as a
         * register copy) where 2.96 constant-propagates `move s2,zero`.
         * No source idiom found yet that suppresses the propagation. */
        i = nRet;
        while (i < s_inCulling.nCount) {
            setup_occlusion(&s_inCulling.aCell[i], p1);
            nRet = check_occlusion(&s_inCulling.aCell[i], p1, p2);
            if (nRet != 0) {
                break;
            }
            i++;
        }
    }
    return nRet;
}



/* Prepare every culling cell for separate occlusion checks */
void xglCullingCheckSeparateInit(void *p)
{
    int i;

    for (i = 0; i < s_inCulling.nCount; i++) {
        setup_occlusion(&s_inCulling.aCell[i], p);
    }
}

/* Return the number of active culling cells */
int xglCullingExist(void)
{
    return s_inCulling.nCount;
}

/* Register the mapped culling entries with the model */
void xglCullingMapLastCheck(void)
{
    int i;
    int n;

    if (s_inCulling.pMap != 0) {
        i = 0;
loop:
        if (i < 12) {
            n = s_inCulling.pMap[i];
            if (n >= 0) {
                nmlModelSetMapLastEntry(s_inCulling.nModel, n);
                i++;
                goto loop;
            }
        }
    }
}

/* Clear the culling map state */
void xglCullingMapInit(void)
{
    s_nIgnoreCulling = 0;
    s_inCulling.nCount = 0;
    s_inCulling.szName[0] = '\0';
    s_inCulling.pMap = 0;
}

/* Draw every culling cell for debugging */
void xglCullingMapDisp(void)
{
    XGLCULLINGCELL *pCell;
    int n;

    if (s_inCulling.nCount > 0) {
        pCell = s_inCulling.aCell;
        n = s_inCulling.nCount;
        do {
            culling_cell_disp(pCell);
            pCell++;
            n--;
        } while (n != 0);
    }
}

/* Disable culling checks */
void xglCullingIgnore(void)
{
    s_nIgnoreCulling = 1;
}

/* Re-enable culling checks */
void xglCullingIgnoreOff(void)
{
    s_nIgnoreCulling = 0;
}

/* Debug hook (nothing to do) */
void xglCullingMapDebug(void)
{
}
