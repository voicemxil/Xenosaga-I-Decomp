/* Keyframe interpolation ("sef" = sequence effect) helpers */

/* One 6-byte keyframe record; interpolation reads the current record and
 * the next one (p[0], p[1]) */
typedef struct
{
    short nTime;   /* 0x00 */
    short nValue;  /* 0x02 */
    short nFlag;   /* 0x04 */
} KEYREC;

typedef struct
{
    int   nKey;     /* 0x00 */
    int   nTime;    /* 0x04 */
    float fResult;  /* 0x08 */
} LERPSTATE_F;

typedef struct
{
    int nKey;     /* 0x00 */
    int nTime;    /* 0x04 */
    int nResult;  /* 0x08 */
} LERPSTATE_I;

void sefProgressKey3(KEYREC *pTable, void *pState);

/* Interpolate the float value for the current keyframe, advancing the
 * key index first unless it's already at the terminal (1024) key */
void sefLerpFloat(KEYREC *pTable, LERPSTATE_F *pState)
{
    KEYREC *p;
    float dt, dv, d1, v2;

    p = &pTable[pState->nKey];
    if (p[1].nTime != 1024) {
        sefProgressKey3(pTable, pState);
        p = &pTable[pState->nKey];
        if (p[0].nTime != p[1].nTime) {
            dt = p[1].nTime - p[0].nTime;
            dv = pState->nTime - p[0].nTime;
            d1 = p[1].nValue - p[0].nValue;
            v2 = p[0].nValue;
            pState->fResult = (d1 * (dv / dt) + v2) * 0.01f;
            return;
        }
    }
    pState->fResult = (float)p[0].nValue * 0.01f;
}

/* Interpolate the integer value for the current keyframe, advancing the
 * key index first unless either bound is already the terminal key */
void sefLerpInt(KEYREC *pTable, LERPSTATE_I *pState)
{
    KEYREC *p;
    float dt, dv, d1, v2;

    p = &pTable[pState->nKey];
    if (p[0].nTime == 1024) {
        pState->nResult = p[0].nValue;
        return;
    }
    if (p[1].nTime == 1024) {
        pState->nResult = p[0].nValue;
        return;
    }
    sefProgressKey3(pTable, pState);
    p = &pTable[pState->nKey];
    if (p[0].nTime != p[1].nTime) {
        dt = p[1].nTime - p[0].nTime;
        dv = pState->nTime - p[0].nTime;
        d1 = p[1].nValue - p[0].nValue;
        v2 = p[0].nValue;
        pState->nResult = (short)(d1 * (dv / dt) + v2);
        return;
    }
    pState->nResult = p[0].nValue;
}
