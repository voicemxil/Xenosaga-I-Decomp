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

u_int RSRC_loadFile2(RSRC *pResource, void *pPath, void *pFile)
{
    return RSRC_loadFileSub(pResource, pPath, pFile);
}

u_int RSRC_loadFile(RSRC *pResource, void *pFile)
{
    return RSRC_loadFile2(pResource, rsrcDefaultPath, pFile);
}
