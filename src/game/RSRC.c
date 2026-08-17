#include "matching.h"

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
extern u_int RSRC_loadFileSub(RSRC *pResource, char *pPath, char *pFile);
extern void *memset(void *pDest, int nValue, u_int nSize);
extern int strlen(const char *pStr);
extern u_int RSRC_searchFile(RSRC *pResource, char *pName);
extern RSRCITEM *RSRC_getDirtyItem(RSRC *pResource, u_int nSize);
extern int RSRC_check(RSRC *pResource, u_int nSize);
extern int strncmp(const char *pA, const char *pB, int nLen);

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

u_int RSRC_loadFile2(RSRC *pResource, char *pPath, char *pFile)
{
    return RSRC_loadFileSub(pResource, pPath, pFile);
}

u_int RSRC_loadFile(RSRC *pResource, char *pFile)
{
    return RSRC_loadFile2(pResource, (char *)rsrcDefaultPath, pFile);
}

/* Find a dirty (free) item: without a size target, the smallest dirty
   item; with a size target, the first dirty item large enough.

   Two details of the size-search loop are load-bearing.

   `i++` sits ahead of the flag test because the retail build increments
   the index in the flag branch's (plain, non-annulled) delay slot. Left
   at the bottom of the body, gcc instead COPIES it into the delay slot
   and annuls the branch (beqzl), which costs a word.

   The retail build then has a pad word between the size sltu and its
   beqz that gcc 2.96 does not emit -- and only there; the identical
   `sltu`/branch pair in the smallest-item loop above has no pad. Nothing
   at the C level produces it (nested ifs, staged locals, do-while,
   pointer-limit and goto loop forms were all tried), so it is spelled
   with SCHED_NOP. TWO are needed to leave ONE behind: gas's reorder-mode
   delay-slot filler steals the instruction immediately before a branch,
   and with a single SCHED_NOP that instruction is the pad itself, which
   lands in the branch slot instead. Splitting the compare into `lt` is
   what puts the pads AFTER the sltu rather than before it. */
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
        i++;
        if ((pItem->nState & 0x8000) != 0) {
            int lt = (pItem->nSize < nSize);
            SCHED_NOP();
            SCHED_NOP();
            if (!lt) {
                return pItem;
            }
        }
        pItem++;
    }
    return 0;
}

/* Carve a resource heap out of [pBase, pBase+nSize): the RSRC header sits
   in the last 32 bytes and the item table grows down from it. */
RSRC *RSRC_create(void *pBase, u_int nSize, int nItemCapacity)
{
    RSRC *pResource;

    memset(pBase, 0, nSize);
    pResource = (RSRC *)((u_int)pBase + nSize - 32);
    pResource->nSize = nSize - nItemCapacity * 16 - 32;
    pResource->pBase = (u_int)pBase;
    pResource->pCurrent = (u_int)pBase;
    pResource->nItemCapacity = nItemCapacity;
    pResource->pItems = (RSRCITEM *)((u_int)pResource - nItemCapacity * 16);
    pResource->nItemCount = 0;
    return pResource;
}

/* Look up a loaded resource by file name; only type-1 (file) items are
   candidates. */
u_int RSRC_searchFile(RSRC *pResource, char *pName)
{
    int nItemCount = pResource->nItemCount;
    int nLen = strlen(pName);
    RSRCITEM *pItem = pResource->pItems;
    int i = 0;

    if (nItemCount != 0) {
        do {
            if (pItem->nType == 1) {
                if (strncmp((char *)pItem->pName, pName, nLen) == 0) {
                    return pItem->pData;
                }
            }
            i++;
            pItem++;
        } while (i < nItemCount);
    }
    return 0;
}

extern int RSRC_info(RSRC *pResource, int nMode);

/* Allocate a resource block: reuse a dirty item big enough, else append a
   fresh item (or, when the table is full, recycle the smallest dirty one)
   and carve the block off the top of the heap. */
u_int RSRC_alloc(RSRC *pResource, u_int nSize, u_int pName)
{
    RSRCITEM *pItem = RSRC_getDirtyItem(pResource, nSize);

    if (pItem == 0) {
        u_int nCount = pResource->nItemCount;
        u_int nNext = nCount + 1;

        /* The EE scheduler otherwise issues the (u_short) truncation ahead
           of the increment, which forces the truncation into a fresh
           register instead of overwriting the dead loaded value in place.
           Laundering BOTH values fences the pair into source order. */
        LAUNDER2(nNext, nCount);

        /* The (u_short) casts are what produce the retail build's
           redundant `andi 0xffff`: without them gcc knows the lhu result
           already fits and drops the truncation, costing a word. */
        if ((u_short)nCount >= pResource->nItemCapacity) {
            pItem = RSRC_getDirtyItem(pResource, 0);
            if (pItem == 0) {
                RSRC_info(pResource, 0);
            }
        } else {
            pResource->nItemCount = nNext;
            pItem = pResource->pItems + (u_short)nCount;
        }
        if (RSRC_check(pResource, nSize) != 0) {
            RSRC_info(pResource, 0);
        }
        {
            u_int nAligned;

            pItem->pData = pResource->pCurrent;
            nAligned = (nSize + 15) >> 4;
            LAUNDER_V(nAligned);
            nAligned = nAligned << 4;
            pItem->nSize = nAligned;
            pResource->pCurrent = pResource->pCurrent + nAligned;
        }
    }
    pItem->pName = pName;
    pItem->nState = 0;
    pItem->nType = 2;
    memset((void *)pItem->pData, 0, pItem->nSize);
    return pItem->pData;
}

extern int strcpy(char *pDest, const char *pSrc);
extern int strcat(char *pDest, const char *pSrc);
extern int sceOpen(const char *pName, int nFlags);
extern int sceLseek(int fd, int nOffset, int nWhence);
extern int sceRead(int fd, void *pBuf, int nSize);
extern int sceClose(int fd);

/* Load pFile from pPath into the top of the heap and append a type-1 item
   describing it. The file's name is copied in immediately behind the data
   and the item's size covers both. Returns the data address, or 0 when the
   file is missing or the heap is full. */
u_int RSRC_loadFileSub(RSRC *pResource, char *pPath, char *pFile)
{
    char szPath[1024];
    u_int pTop;
    int fd;
    int nSize;
    int nLen;
    int nBlock;
    int nNameBlock;
    RSRCITEM *pItem;
    u_int pData;

    pData = RSRC_searchFile(pResource, pFile);
    if (pData != 0) {
        return pData;
    }
    strcpy(szPath, pPath);
    strcat(szPath, pFile);
    pTop = pResource->pCurrent;
    fd = sceOpen(szPath, 1);
    if (fd < 0) {
        return 0;
    }
    nSize = sceLseek(fd, 0, 2);
    if (RSRC_check(pResource, nSize) != 0) {
        return 0;
    }
    sceLseek(fd, 0, 0);
    if (nSize != 0) {
        sceRead(fd, (void *)pTop, nSize);
    }
    sceClose(fd);
    nBlock = (nSize + 15) / 16;
    pResource->pCurrent = pResource->pCurrent + nBlock * 16;
    nLen = strlen(pFile);
    strcpy((char *)pResource->pCurrent, pFile);
    pItem = pResource->pItems + pResource->nItemCount;
    pItem->nType = 1;
    pItem->pData = pTop;
    nNameBlock = (nLen + 16) / 16;
    pItem->nState = 0;
    pItem->pName = pResource->pCurrent;
    pItem->nSize = (nBlock + nNameBlock) * 16;
    pResource->pCurrent = pResource->pCurrent + nNameBlock * 16;
    pResource->nItemCount = pResource->nItemCount + 1;
    return pTop;
}

extern int isLeave(RSRCITEM *pItem, u_int pSource);
extern void *memcpy(void *pDest, const void *pSrc, u_int nSize);

/* Garbage-collect the heap: slide every item still owned by a source other
   than pSource down against pBase, dropping the rest, then close the gaps
   the drops left in the item table. */
void RSRC_dispose(RSRC *pResource, u_int pSource)
{
    RSRCITEM *pItem;
    RSRCITEM *pFree;
    int nItemCount;
    int i;
    int j;
    int nLive;
    u_int pDst;

    pItem = pResource->pItems;
    nItemCount = pResource->nItemCount;
    pDst = pResource->pBase;
    for (i = 0; i < nItemCount; i++) {
        if (isLeave(pItem, pSource) != 0) {
            u_int pSrc = pItem->pData;

            if (pDst != pSrc) {
                memcpy((void *)pDst, (void *)pSrc, pItem->nSize);
                if (pItem->nType == 1) {
                    u_int pName = pItem->pName;
                    u_int pOld = pItem->pData;

                    pItem->pData = pDst;
                    pItem->pName = pDst + (pName - pOld);
                }
            }
            pDst += pItem->nSize;
        } else {
            pItem->pData = 0;
        }
        pItem++;
    }
    pResource->pCurrent = pDst;
    pItem = pResource->pItems;
    nLive = 0;
    for (i = 0; i < nItemCount; i++) {
        if (pItem->pData != 0) {
            pFree = pResource->pItems;
            for (j = 0; j < nItemCount; j++) {
                if (pFree->pData == 0) {
                    break;
                }
                pFree++;
            }
            if (j < i) {
                memcpy(pFree, pItem, 16);
                memset(pItem, 0, 16);
                pFree->nState = 0;
            }
            pItem->nState = 0;
            nLive++;
        } else {
            pItem->nState = 0;
        }
        pItem++;
    }
    pResource->nItemCount = nLive;
}

extern char *markedSTR[2][2];
extern char *selectedSTR[2][2];
extern char *unselectedSTR[2][2];
extern char *normalSTR[2][2];
extern void DB_pathGetShortPath(char *pDest, u_int pName, int nLen);

/* Debug listing of the item table: build the mark/select decoration for
   each item in the window set by RSRC_setPrintParam, then clear the
   window. */
int RSRC_info(RSRC *pResource, int nMode)
{
    char szMark[128];
    char szName[128];
    char szPath[128];
    int nShort;
    int nIndex;
    int i;
    int nLen;
    int nCount;
    int nEnd;

    nShort = nMode & 0xf;
    nIndex = infoIndex;
    nLen = infoLength;
    if (nIndex < 0) {
        nIndex = 0;
    }
    if (nLen < 0) {
        nEnd = pResource->nItemCount;
        nCount = nEnd;
    } else {
        nCount = pResource->nItemCount;
        nEnd = nIndex + nLen;
        if (nCount < nEnd) {
            nEnd = nCount;
        }
    }
    if (nIndex < 0 || nIndex >= nCount) {
        nIndex = 0;
    }
    for (i = nIndex; i < nEnd; i++) {
        RSRCITEM *pItem = &pResource->pItems[i];

        szName[0] = 0;
        szMark[0] = 0;
        if (pItem->nState & 2) {
            strcat(szMark, selectedSTR[nMode][0]);
            strcat(szName, selectedSTR[nMode][1]);
        } else {
            strcat(szMark, unselectedSTR[nMode][0]);
            strcat(szName, unselectedSTR[nMode][1]);
        }
        if (pItem->nState & 1) {
            strcat(szMark, markedSTR[nMode][0]);
            strcat(szName, markedSTR[nMode][1]);
        } else {
            strcat(szMark, normalSTR[nMode][0]);
            strcat(szName, normalSTR[nMode][1]);
        }
        if (pItem->nType == 1) {
            if (nShort != 0) {
                DB_pathGetShortPath(szPath, pItem->pName, 22);
            }
        }
    }
    infoLength = -1;
    infoIndex = -1;
    return 0;
}
