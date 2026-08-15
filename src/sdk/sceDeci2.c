/* DECI2 protocol driver entry points - thin wrappers that marshal arguments for Deci2Call */

#include "matching.h"

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

/* Receive/Send into a caller-supplied buffer (extended entry). Plain C
 * used to near-match (12 words vs the original's 13: the original
 * saves a1 into v0 before overwriting a1 with the stack param pointer,
 * one extra daddu no phrasing reproduced, plus a scheduling tie-break
 * around where `sd ra` lands relative to the first param store).
 * PIN-ing both the saved-data copy ("$2"/v0) and the &param pointer
 * ("$5"/a1) to the exact registers the original used -- rather than
 * leaving them to the allocator's choice -- reproduces both the extra
 * register hop and the original's instruction order, so this now
 * compiles as real C. */

int sceDeci2ExRecv(int nSocket, void *data, unsigned short nLen)
{
    unsigned int param[4];
    unsigned int len = nLen & 0xffff;
    PIN(void *saved, "$2") = data;
    PIN(unsigned int *p, "$5");

    param[0] = nSocket;
    p = param;
    param[1] = (unsigned int)saved;
    param[2] = len;
    return Deci2Call(-5, p);
}

int sceDeci2ExSend(int nSocket, void *data, unsigned short nLen)
{
    unsigned int param[4];
    unsigned int len = nLen & 0xffff;
    PIN(void *saved, "$2") = data;
    PIN(unsigned int *p, "$5");

    param[0] = nSocket;
    p = param;
    param[1] = (unsigned int)saved;
    param[2] = len;
    return Deci2Call(-6, p);
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
