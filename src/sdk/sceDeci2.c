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

/* Receive/Send into a caller-supplied buffer (extended entry). Both
 * near-matched under plain C (12 words vs the original's 13: the
 * original saves a1 into v0 before overwriting a1 with the stack param
 * pointer, one extra daddu no phrasing of the C reproduced), so these
 * are file-scope inline asm -- same technique as the sceCdSt* family in
 * sceCd.c, needed here because the function's own body ends in its own
 * jr+delay-slot epilogue. `move` is spelled out as `daddu $x,$y,$0`. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceDeci2ExRecv\n"
    ".ent sceDeci2ExRecv\n"
    "sceDeci2ExRecv:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-32\n"
    "daddu $2,$5,$0\n"
    "andi $6,$6,0xffff\n"
    "sw $4,0($sp)\n"
    "sd $31,16($sp)\n"
    "daddu $5,$sp,$0\n"
    "sw $2,4($sp)\n"
    "li $4,-5\n"
    "jal Deci2Call\n"
    "sw $6,8($sp)\n"
    "ld $31,16($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,32\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceDeci2ExRecv\n"
);

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceDeci2ExSend\n"
    ".ent sceDeci2ExSend\n"
    "sceDeci2ExSend:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-32\n"
    "daddu $2,$5,$0\n"
    "andi $6,$6,0xffff\n"
    "sw $4,0($sp)\n"
    "sd $31,16($sp)\n"
    "daddu $5,$sp,$0\n"
    "sw $2,4($sp)\n"
    "li $4,-6\n"
    "jal Deci2Call\n"
    "sw $6,8($sp)\n"
    "ld $31,16($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,32\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceDeci2ExSend\n"
);

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
