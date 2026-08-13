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
        u_int nCurSize;

        if ((pItem->nState & 0x8000) != 0) {
            nCurSize = pItem->nSize;
            if (nCurSize >= nSize) {
                return pItem;
            }
        }
        i++;
        pItem++;
    }
    return 0;
}
