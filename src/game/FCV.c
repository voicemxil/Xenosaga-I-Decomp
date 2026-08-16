/* FCV - the float-curve layer: the packed animation keys that JNT feeds the
 * skeleton with.
 *
 * Two generations live side by side.  The original FCV_* pack walks three
 * parallel arrays hanging off a curve header (key types, times, values) and
 * carries a two-bit key type per key, four packed into every byte.  The newer
 * FCV2_* form is a chain of length prefixed attributes: each attribute starts
 * with { size, type } and the low nibble of the type selects how the payload
 * is read - 0 is "no value", 1 is a single inline float, 3 is a real key list
 * and anything else is ignored.  A pack is just a cursor into that chain, so
 * reading a value both produces the float and steps the cursor past it.
 *
 * `defaultAttr` is the attribute handed back when a pack has run past the end
 * of its chain, so callers never have to test for it.
 */

/* Header of the block an attribute chain hangs off; nSize covers the chain. */
typedef struct {
    unsigned int nSize;         /* 0x00 */
} FCV2_DATA;

/* Scratch the attribute scan fills in for one curve id. */
typedef struct {
    char pad000[0xC];           /* 0x00 */
    int nUnk0C;                 /* 0x0C */
    unsigned short nId;         /* 0x10 */
    char pad012[0x2];           /* 0x12 */
    unsigned char *pKeyAttr;    /* 0x14 */
    unsigned char *pHiChannel;  /* 0x18 */
    unsigned char *pLoChannel;  /* 0x1C */
} FCV2_INFO;

/* One entry of an FCV2 attribute chain; the payload follows the header. */
typedef struct {
    unsigned short nSize;       /* 0x00 */
    unsigned short nType;       /* 0x02 */
} FCV2_ATTRIBUTE;

/* A cursor into an attribute chain. */
typedef struct {
    FCV2_ATTRIBUTE *pAttr;      /* 0x00 */
} FCV2_PACK;

/* Cursor over the three parallel key arrays of an old style curve. */
typedef struct {
    int nUnk00;                 /* 0x00 */
    unsigned char *pKeyType;    /* 0x04 */
    unsigned char *pTime;       /* 0x08 */
    unsigned char *pValue;      /* 0x0C */
    short nIndex;               /* 0x10 */
} FCV_PACK;

#define FCV_SIGNATURE 0x564346  /* 'FCV' */

float FCV_getPackVal2f(FCV_PACK *pPack, float fTime);
float FCV2_getVal(int *pKey, void *pData, int nNum);
float FCV2_getPackVal(FCV2_PACK *pPack, void *pData, int nNum);

FCV2_ATTRIBUTE defaultAttr[4];

float fcv2Step;

/* Point a pack at the key arrays of a curve and rewind it */
void FCV_resetPack(FCV_PACK *pPack, unsigned char *pCurve)
{
    pPack->pKeyType = pCurve + 0x10;
    pPack->pTime = pCurve + 0x14;
    pPack->pValue = pCurve + 0x34;
    pPack->nIndex = 0;
}

/* Step a pack over a zero length interval, discarding the value */
void FCV_skipPack(FCV_PACK *pPack)
{
    FCV_getPackVal2f(pPack, 0.0f);
}

/* Two bit key type of key nIndex; four keys share a byte, highest pair first */
int FCV_getFKeyType(unsigned int nIndex, unsigned char *pKeyType)
{
    return (pKeyType[nIndex >> 2] >> ((~nIndex & 3) << 1)) & 3;
}

/* Value of a single attribute */
float FCV2_getValue(FCV2_ATTRIBUTE *pAttr)
{
    if (pAttr->nType == 3) {
        return FCV2_getVal(0, pAttr + 1, pAttr->nSize);
    }
    return (pAttr->nType == 1) ? *(float *)(pAttr + 1) : 0.0f;
}

/* Value of a single attribute, also reporting which key it came from */
float FCV2_getValueAndKey(int *pKey, FCV2_ATTRIBUTE *pAttr)
{
    *pKey = 0;
    if (pAttr->nType == 3) {
        return FCV2_getVal(pKey, pAttr + 1, pAttr->nSize);
    }
    return (pAttr->nType == 1) ? *(float *)(pAttr + 1) : 0.0f;
}

/* Scan a whole attribute chain, recording the pieces that match pInfo->nId */
void FCV2_readAttribute(FCV2_INFO *pInfo, FCV2_DATA *pData)
{
    FCV2_ATTRIBUTE *pAttr;
    unsigned char *p;
    int *pVal;
    unsigned int nOffset;

    pInfo->pKeyAttr = 0;
    pInfo->pHiChannel = 0;
    pInfo->pLoChannel = 0;
    nOffset = 4;
    pAttr = (FCV2_ATTRIBUTE *)((char *)pData + 4);
    do {
        switch (pAttr->nType) {
        case 1:
            pVal = (int *)(pAttr + 1);
            pInfo->nUnk0C = pVal[0] * 31 + pVal[1];
            break;
        case 3:
            pInfo->pKeyAttr = (unsigned char *)(pAttr + 1);
            break;
        case 2:
            p = (unsigned char *)(pAttr + 1);
            if ((p[0] & 0x7F) == pInfo->nId) {
                if (p[0] & 0x80) {
                    pInfo->pHiChannel = p;
                } else {
                    pInfo->pLoChannel = p;
                }
            }
            break;
        default:
            break;
        }
        if (pAttr->nType == 0) {
            break;
        }
        nOffset += pAttr->nSize;
        pAttr = (FCV2_ATTRIBUTE *)((char *)pAttr + pAttr->nSize);
    } while (nOffset < pData->nSize);
}

/* 0 when the block carries a valid FCV signature, 1 when there is no block */
int FCV2_checkData(int *pData)
{
    if (pData == 0) {
        return 1;
    }
    return (*pData == FCV_SIGNATURE) ? 0 : 2;
}

/* Set the sampling step; only its magnitude matters, and it is halved */
void FCV2_setStep(float fStep)
{
    fcv2Step = __builtin_fabsf(fStep) * 0.5f;
}

/* Attribute under the cursor, stepping it past a real one */
FCV2_ATTRIBUTE *FCV2_getPackAttribute(FCV2_PACK *pPack)
{
    FCV2_ATTRIBUTE *pAttr;
    FCV2_ATTRIBUTE *pResult;

    pAttr = pPack->pAttr;
    if (pAttr->nType == 4) {
        pResult = pAttr;
        pPack->pAttr = (FCV2_ATTRIBUTE *)((char *)pAttr + pAttr->nSize + 4);
    } else {
        pResult = defaultAttr;
    }
    return pResult;
}

/* Value under the cursor, stepping it past the payload it consumed */
float FCV2_getPackValue(FCV2_PACK *pPack)
{
    FCV2_ATTRIBUTE *pAttr;
    float fValue;

    pAttr = pPack->pAttr;
    fValue = 0.0f;
    switch (pAttr->nType & 0xF) {
    case 0:
        pPack->pAttr = pAttr + 1;
        break;
    case 1:
        fValue = *(float *)(pAttr + 1);
        pPack->pAttr = (FCV2_ATTRIBUTE *)((char *)pAttr + 8);
        break;
    case 3:
        fValue = FCV2_getPackVal(pPack, pAttr + 1, pAttr->nSize);
        break;
    default:
        pPack->pAttr = pAttr + 1;
        fValue = 0.0f;
        break;
    }
    return fValue;
}

/* Walk every key of a key list attribute without reading any of them */
void FCV2_dump(FCV2_ATTRIBUTE *pAttr)
{
    unsigned char *pKey;
    int i;
    int nNum;

    if (pAttr->nType != 3) {
        return;
    }
    nNum = pAttr->nSize;
    pKey = (unsigned char *)(pAttr + 1);
    i = 0;
    if (nNum == 0) {
        return;
    }
    do {
        switch (*(unsigned short *)pKey) {
        case 1:
            pKey += 0x10;
            break;
        case 0:
            pKey += 8;
            break;
        case 2:
        case 3:
            pKey += 8;
            break;
        default:
            break;
        }
        i++;
    } while (i < nNum);
}

/* One key of an FCV2 key list: a { type, time } header with the payload
 * following. Type 1 carries a tangent pair and is 16 bytes; types 0, 2
 * and 3 are 8; anything else has no payload at all. */
typedef struct {
    unsigned short nType;       /* 0x00 */
    unsigned short nTime;       /* 0x02 */
} FCV2_KEY;

/* First key of an attribute's key list at or after fTime. Returns null
 * if the attribute is not a key list, and the last key if fTime is past
 * the end.
 *
 * NEAR MISS (4 diffs, 46 orig vs 44 built): the body is right; the
 * original ends with two return sites -- a duplicated `jr ra` whose
 * delay slot holds the fallthrough's `move v0,a0`, then an alignment nop
 * and a bare `jr ra` shared by the two jumps -- where gcc emits the move
 * ahead of a single shared return. */
FCV2_KEY *FCV2_getKey(FCV2_ATTRIBUTE *pAttr, float fTime)
{
    FCV2_KEY *pKey;
    int nNum;
    int i;

    if ((pAttr->nType & 0xF) != 3) {
        return 0;
    }
    nNum = pAttr->nSize - 1;
    pKey = (FCV2_KEY *)(pAttr + 1);
    for (i = 0; i < nNum; i++) {
        if (fTime <= pKey->nTime * fcv2Step) {
            return pKey;
        }
        switch (pKey->nType) {
        case 0:
        case 2:
        case 3:
            pKey = (FCV2_KEY *)((char *)pKey + 8);
            break;
        case 1:
            pKey = (FCV2_KEY *)((char *)pKey + 16);
            break;
        default:
            break;
        }
    }
    return pKey;
}
