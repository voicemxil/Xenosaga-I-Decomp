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

void *memset(void *, int, u_int);

RSRC *RSRC_create(void *pBuf, u_int nSize, u_int nItemCapacity)
{
    RSRC *pResource;
    u_int nItemsSize;
    u_int nPoolSize;

    memset(pBuf, 0, nSize);
    nItemsSize = nItemCapacity << 4;
    pResource = (RSRC *)((u_char *)pBuf + nSize - 0x20);
    nPoolSize = nSize - nItemsSize - 0x20;
    pResource->pCurrent = (u_int)pBuf;
    pResource->nSize = nPoolSize;
    pResource->pItems = (RSRCITEM *)((u_char *)pResource - nItemsSize);
    pResource->nItemCapacity = (u_short)nItemCapacity;
    pResource->pBase = (u_int)pBuf;
    pResource->nItemCount = 0;
    return pResource;
}
