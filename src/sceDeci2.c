/* DECI2 protocol driver entry points - thin wrappers that marshal arguments for Deci2Call */

extern unsigned char userarea[16];

int Deci2Call(int nFunc, unsigned int *pParam);

/* Register a DECI2 protocol handler and return its socket */
/* TODO: near-match - instruction scheduling around the userarea address
 * differs (andi/or and the a1 setup are ordered differently); the original
 * Sony SDK object was built with a patched ee-gcc we do not have. */
int sceDeci2Open(unsigned short nProtocol, void *pOpt, void (*pHandler)())
{
    unsigned int param[4];

    param[0] = nProtocol;
    param[1] = (unsigned int)pOpt;
    param[2] = (unsigned int)pHandler;
    param[3] = (unsigned int)userarea | 0x20000000;
    return Deci2Call(1, param);
}

/* Release a DECI2 socket */
int sceDeci2Close(int nSocket)
{
    unsigned int param[4];

    param[0] = nSocket;
    return Deci2Call(2, param);
}

/* Request transmission of a queued DECI2 packet */
int sceDeci2ReqSend(int nSocket, char nDest)
{
    unsigned int param[4];

    param[0] = nSocket;
    param[1] = nDest;
    return Deci2Call(3, param);
}

/* Poll a DECI2 socket for pending activity */
int sceDeci2Poll(int nSocket)
{
    unsigned int param[4];

    param[0] = nSocket;
    return Deci2Call(4, param);
}

/* Receive into a caller-supplied buffer (extended entry) */
/* TODO: near-match - the original copies a1 into v0 before overwriting a1
 * with the param pointer (one extra daddu); pure scheduling difference from
 * the patched Sony ee-gcc. */
int sceDeci2ExRecv(int nSocket, void *pBuf, unsigned short nLen)
{
    unsigned int param[4];

    param[0] = nSocket;
    param[1] = (unsigned int)pBuf;
    param[2] = nLen;
    return Deci2Call(-5, param);
}

/* Send from a caller-supplied buffer (extended entry) */
/* TODO: near-match - the original copies a1 into v0 before overwriting a1
 * with the param pointer (one extra daddu); pure scheduling difference from
 * the patched Sony ee-gcc. */
int sceDeci2ExSend(int nSocket, void *pBuf, unsigned short nLen)
{
    unsigned int param[4];

    param[0] = nSocket;
    param[1] = (unsigned int)pBuf;
    param[2] = nLen;
    return Deci2Call(-6, param);
}

/* Request transmission of a queued packet (extended entry) */
int sceDeci2ExReqSend(int nSocket, char nDest)
{
    unsigned int param[4];

    param[0] = nSocket;
    param[1] = nDest;
    return Deci2Call(-7, param);
}

/* Take the DECI2 transfer lock */
int sceDeci2ExLock(int nSocket)
{
    unsigned int param[4];

    param[0] = nSocket;
    return Deci2Call(-8, param);
}

/* Release the DECI2 transfer lock */
int sceDeci2ExUnLock(int nSocket)
{
    unsigned int param[4];

    param[0] = nSocket;
    return Deci2Call(-9, param);
}
