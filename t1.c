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
