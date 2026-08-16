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

/* Advance the current key index one tick. pState is void * so the float
 * and integer state structs (identical apart from the result field) can
 * share it. */
void sefProgressKey3(KEYREC *pTable, void *pStateArg)
{
    LERPSTATE_I *pState;
    KEYREC *p;
    int nTime;
    int nNext;

    pState = (LERPSTATE_I *)pStateArg;
    nTime = pState->nTime + 1;
    pState->nTime = nTime;
    if (nTime < 1024) {
        p = &pTable[pState->nKey];
        if (p[0].nTime != 1024) {
            if (nTime >= p[1].nTime) {
                nNext = p[1].nFlag;
                if (nNext >= 0) {
                    pState->nKey = nNext;
                    p = &pTable[nNext];
                    pState->nTime = p->nTime;
                } else {
                    pState->nKey = pState->nKey + 1;
                }
            }
        }
    }
}

/* The 10-byte-record variant. Returns the record the state now points
 * at -- which, on the "no next key" path, is one past the record that
 * was current on entry. */
typedef struct
{
    short nTime;   /* 0x00 */
    short nValue;  /* 0x02 */
    short f04;     /* 0x04 */
    short f06;     /* 0x06 */
    short nNext;   /* 0x08 */
} KEYREC5;

KEYREC5 *sefProgressKey5(KEYREC5 *pTable, LERPSTATE_I *pState)
{
    KEYREC5 *p;
    int nTime;
    int nNext;

    nTime = pState->nTime + 1;
    p = &pTable[pState->nKey];
    pState->nTime = nTime;
    if (nTime < 1024) {
        if (p->nTime != 1024) {
            if (nTime >= p[1].nTime) {
                p++;
                nNext = p->nNext;
                if (nNext >= 0) {
                    pState->nKey = nNext;
                    p = &pTable[nNext];
                    pState->nTime = p->nTime;
                } else {
                    pState->nKey = pState->nKey + 1;
                }
            }
        }
    }
    return p;
}

/* Advance the key index, then snap the integer result to the new key's
 * raw value (no interpolation) */
void sefProgressInt(KEYREC *pTable, LERPSTATE_I *pState)
{
    sefProgressKey3(pTable, pState);
    pState->nResult = pTable[pState->nKey].nValue;
}
