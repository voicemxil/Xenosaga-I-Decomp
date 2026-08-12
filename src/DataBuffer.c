/* DataBuffer - byte-stream cursor with pluggable endian integer readers */

typedef struct DataBuffer {
    unsigned char *pStart;                                  /* 0x00 */
    unsigned char *pCur;                                    /* 0x04 */
    unsigned char *pEnd;                                    /* 0x08 */
    int nSize;                                              /* 0x0C */
    unsigned int (*pGetUIntegerAt)(struct DataBuffer *, int); /* 0x10 */
} DataBuffer;

unsigned int DataBuffer_LittleEndian_getUIntegerAt(DataBuffer *pBuf, int nBytes);
unsigned int DataBuffer_BigEndian_getUIntegerAt(DataBuffer *pBuf, int nBytes);

/* Bind a buffer to a memory range and select the endian reader */
void DataBuffer_init(DataBuffer *pBuf, unsigned char *pData, int nSize, int bBigEndian)
{
    if (bBigEndian == 0) {
        pBuf->pGetUIntegerAt = DataBuffer_LittleEndian_getUIntegerAt;
    } else {
        pBuf->pGetUIntegerAt = DataBuffer_BigEndian_getUIntegerAt;
    }
    pBuf->pStart = pData;
    pBuf->pEnd = pData + nSize;
    pBuf->pCur = pData;
    pBuf->nSize = nSize;
}

/* Byte offset of the cursor from the start of the buffer */
int DataBuffer_getPos(DataBuffer *pBuf)
{
    return pBuf->pCur - pBuf->pStart;
}

/* Move the cursor to an absolute byte offset */
void DataBuffer_setPos(DataBuffer *pBuf, int nPos)
{
    pBuf->pCur = pBuf->pStart + nPos;
}

/* Advance the cursor by a relative byte count */
void DataBuffer_seek(DataBuffer *pBuf, int nOffset)
{
    pBuf->pCur = pBuf->pCur + nOffset;
}

/* Copy raw bytes out of the buffer, failing if they run past the end */
int DataBuffer_getBytes(DataBuffer *pBuf, unsigned char *pDst, unsigned int nLen)
{
    unsigned char *pSrc;
    unsigned char *pTail;

    pSrc = pBuf->pCur;
    pTail = pSrc + nLen;
    if (pBuf->pEnd < pTail) {
        return -1;
    }
    while (pSrc < pTail) {
        *pDst = *pSrc;
        pSrc++;
        pDst++;
    }
    pBuf->pCur = pTail;
    return nLen;
}

/* Read one unsigned byte and advance */
unsigned int DataBuffer_getUByteAt(DataBuffer *pBuf, int nBytes)
{
    unsigned char *p;
    unsigned int nVal;

    p = pBuf->pCur;
    nVal = *p;
    p++;
    pBuf->pCur = p;
    return nVal;
}

/* Read a 16-bit value through the selected endian reader */
unsigned int DataBuffer_getUShortAt(DataBuffer *pBuf)
{
    return pBuf->pGetUIntegerAt(pBuf, 2) & 0xFFFF;
}

/* Read a 32-bit value through the selected endian reader */
unsigned int DataBuffer_getUIntAt(DataBuffer *pBuf)
{
    return pBuf->pGetUIntegerAt(pBuf, 4);
}

/* Assemble an n-byte little-endian integer and advance the cursor */
unsigned int DataBuffer_LittleEndian_getUIntegerAt(DataBuffer *pBuf, int nBytes)
{
    unsigned int nVal;
    int i;

    nVal = pBuf->pCur[nBytes - 1];
    for (i = nBytes - 2; i >= 0; i--) {
        nVal = (nVal << 8) + pBuf->pCur[i];
    }
    pBuf->pCur = pBuf->pCur + nBytes;
    return nVal;
}

/* Assemble an n-byte big-endian integer and advance the cursor */
unsigned int DataBuffer_BigEndian_getUIntegerAt(DataBuffer *pBuf, int nBytes)
{
    unsigned int nVal;
    int i;

    nVal = pBuf->pCur[0];
    for (i = 1; i < nBytes; i++) {
        nVal = (nVal << 8) + pBuf->pCur[i];
    }
    pBuf->pCur = pBuf->pCur + nBytes;
    return nVal;
}
