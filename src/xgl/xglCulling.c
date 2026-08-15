#include "matching.h"

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
        /* Repeated assignment preserves the original delay-slot register copy */
        i = nRet;
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

/* Test the prepared culling cells until one occludes the given object */
int xglCullingCheckSeparate(void *p1, void *p2)
{
    int nRet;
    int i;

    nRet = 0;
    i = nRet;
    i = nRet;
    for (; i < s_inCulling.nCount; i++) {
        nRet = check_occlusion(&s_inCulling.aCell[i], p1, p2);
        if (nRet != 0) {
            break;
        }
    }
    return nRet;
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

extern float s_aCullingBase[9];
void culling_matrix(float *pMtx);

/* Append a culling cell built from the current 3x3 base matrix (rows
 * padded to vec4, w=1 on rows 0 and 2) and derive its planes */
void xglCullingMapCreate(void)
{
    float *pDst;
    float *pSrc = s_aCullingBase;
    PIN(float *pArg, "$4");
    int n;

    n = s_inCulling.nCount;
    pDst = (float *)&s_inCulling.aCell[n];
    if (n < 10) {
        PIN(float fXY, "$f0");
        PIN(float fYY, "$f1");
        PIN(float fZY, "$f2");
        PIN(float fXZ, "$f3");
        PIN(float fYZ, "$f4");
        PIN(float fXX, "$f5");
        PIN(float fYX, "$f6");
        PIN(float fZX, "$f7");
        PIN(float fOne, "$f8");
        PIN(float fZZ, "$f9");

        __asm__ __volatile__("" : "=f"(fZZ) : "0"(pSrc[8]));
        PASSTHRU_V(pArg, pDst);
        __asm__ __volatile__("" : "=f"(fXX) : "0"(pSrc[0]));
        __asm__ __volatile__("" : "=f"(fYX) : "0"(pSrc[1]));
        __asm__ __volatile__("" : "=f"(fZX) : "0"(pSrc[2]));
        __asm__ __volatile__("" : "=f"(fXY) : "0"(pSrc[3]));
        __asm__ __volatile__("" : "=f"(fYY) : "0"(pSrc[4]));
        __asm__ __volatile__("" : "=f"(fZY) : "0"(pSrc[5]));
        __asm__ __volatile__("" : "=f"(fXZ) : "0"(pSrc[6]));
        __asm__ __volatile__("" : "=f"(fYZ) : "0"(pSrc[7]));
        __asm__ __volatile__("" : "=f"(fOne) : "0"(1.0f));

        pDst[1] = fYX;
        pDst[2] = fZX;
        pDst[4] = fXY;
        pDst[5] = fYY;
        pDst[6] = fZY;
        pDst[8] = fXZ;
        pDst[9] = fYZ;
        pDst[10] = fZZ;
        pDst[3] = fOne;
        pDst[11] = fOne;
        pDst[0] = fXX;
        culling_matrix(pArg);
        s_inCulling.nCount++;
    }
}
