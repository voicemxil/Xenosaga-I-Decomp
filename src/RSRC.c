typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

typedef struct {
    u_int nSize;
    u_char nType;
    u_char nUnknown;
    u_short nState;
    u_int pData;
    u_int pName;
} RSRCITEM;

typedef struct {
    u_int pBase;
    u_int pCurrent;
    u_int nSize;
    RSRCITEM *pItems;
    u_short nItemCount;
    u_short nItemCapacity;
} RSRC;

extern void *rsrcDefaultPath;
extern int infoIndex;
extern int infoLength;
extern u_int RSRC_loadFileSub(RSRC *pResource, void *pPath, void *pFile);

void RSRC_inactiveSource(RSRC *pResource, u_int pSource)
{
    int nItemCount = pResource->nItemCount;
    int i = 0;
    RSRCITEM *pItem = pResource->pItems;

    while (i < nItemCount) {
        if (pItem->pName == pSource) {
            pItem->pName = 0;
            pItem->nState |= 0x8000;
            nItemCount = pResource->nItemCount;
        }
        i++;
        pItem++;
    }
}

void RSRC_setPrintParam(int nIndex, int nLength)
{
    infoIndex = nIndex;
    infoLength = nLength;
}

void *RSRC_getDefaultPath(void)
{
    return rsrcDefaultPath;
}

void *RSRC_setDefaultPath(void *pPath)
{
    rsrcDefaultPath = pPath;
    return pPath;
}

void RSRC_init(RSRC *pResource)
{
    int i;

    pResource->pCurrent = pResource->pBase;
    pResource->nItemCount = 0;
    for (i = 0; i < pResource->nItemCapacity; i++) {
        pResource->pItems[i].nState = 0;
    }
}

int RSRC_check(RSRC *pResource, u_int nSize)
{
    if (pResource->pCurrent + nSize - pResource->pBase < pResource->nSize) {
        return 0;
    }
    return 1;
}

RSRCITEM *RSRC_getItemID(RSRC *pResource, int nID)
{
    int nItemCount = pResource->nItemCount;
    int i = 0;
    RSRCITEM *pItem = pResource->pItems;

    while (i < nItemCount) {
        if (pItem->nUnknown == nID) {
            return pItem;
        }
        i++;
        pItem++;
    }
    return 0;
}

RSRCITEM *RSRC_getItem2(RSRC *pResource, u_int pSource)
{
    int nItemCount = pResource->nItemCount;
    int i = 0;
    RSRCITEM *pItem = pResource->pItems;

    while (i < nItemCount) {
        if (pItem->pName == pSource) {
            return pItem;
        }
        i++;
        pItem++;
    }
    return 0;
}

RSRCITEM *RSRC_getItem(RSRC *pResource, u_int pData)
{
    int nItemCount = pResource->nItemCount;
    RSRCITEM *pItem = pResource->pItems;
    int i;

    if (pData == 0) {
        return 0;
    }
    i = 0;
    while (i < nItemCount) {
        if (pItem->pData == pData) {
            return pItem;
        }
        i++;
        pItem++;
    }
    return 0;
}

u_int RSRC_loadFile2(RSRC *pResource, void *pPath, void *pFile)
{
    return RSRC_loadFileSub(pResource, pPath, pFile);
}

u_int RSRC_loadFile(RSRC *pResource, void *pFile)
{
    return RSRC_loadFile2(pResource, rsrcDefaultPath, pFile);
}

/* Find a dirty (free) item: without a size target, the smallest dirty
   item; with a size target, the first dirty item large enough.
   TODO: near-miss only - the nSize!=0 search loop emits one fewer nop
   between the size sltu and its beqz than the original (every other
   instruction/register in the function matches exactly); no source
   variant tried (nested if, nested short-circuit, staged local) changed
   it, so the extra nop is likely a genuine 2.96 scheduling artifact of
   this specific comparison shape that needs more investigation. */
RSRCITEM *RSRC_getDirtyItem(RSRC *pResource, u_int nSize)
{
    int nItemCount = pResource->nItemCount;
    RSRCITEM *pItem = pResource->pItems;
    int i;

    if (nSize == 0) {
        RSRCITEM *pBest = 0;
        u_int nBestSize = -1;

        if (nItemCount != 0) {
            i = nItemCount;
            do {
                if ((pItem->nState & 0x8000) != 0) {
                    if (nBestSize >= pItem->nSize) {
                        nBestSize = pItem->nSize;
                        pBest = pItem;
                    }
                }
                pItem++;
            } while (--i);
        }
        return pBest;
    }

    i = 0;
    while (i < nItemCount) {
        if ((pItem->nState & 0x8000) != 0 && pItem->nSize >= nSize) {
            return pItem;
        }
        i++;
        pItem++;
    }
    return 0;
}
