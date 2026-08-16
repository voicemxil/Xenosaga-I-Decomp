#include "matching.h"

/* Sound front-end - wraps the SSD driver with bank slots and CD streaming */

typedef struct {
    unsigned short nSwd;            /* 0x00 */
    unsigned short nData;           /* 0x02 */
} SOUND_SLOT;

typedef struct {
    char pad000[0x8];               /* 0x00 */
    unsigned char nFill;            /* 0x08 */
    char pad009[0x13];              /* 0x09 */
    int nRingSize;                  /* 0x1C */
    char *pRead;                    /* 0x20 */
    int nSize;                      /* 0x24 */
    int nPos;                       /* 0x28 */
    int nSent;                      /* 0x2C */
    unsigned char nMode;            /* 0x30 */
    char pad031[0x2];               /* 0x31 */
    unsigned char nPause;           /* 0x33 */
    char *pBuffer;                  /* 0x34 */
    char pad038[0x8];               /* 0x38 */
    int nCount;                     /* 0x40 */
    char aRing[0x800];              /* 0x44 */
} XGL_STREAM;

typedef struct {
    SOUND_SLOT aSeq[8];             /* 0x00 */
    SOUND_SLOT aSe[16];             /* 0x20 */
    XGL_STREAM stream;              /* 0x60 */
} SOUND_WORK;

SOUND_WORK SoundWork;
char SoundDataPath[12] = "data\\sound\\";
static char SoundEffectPath[8] = "sed\\";
static char SoundSequencePath[8] = "smd\\";

extern char StreamBuffer[];

int SsdGetResultValue(int *);
int SsdSpuDmaCompleted(int);
int SsdInit(int);
int SsdStopSequence(int);
int SsdStopEffectFileID(int);
int SsdDisposeWaveBank(int);
int SsdAddWaveData(void *, int, int);
int SsdDisposeSequence(int);
int SsdAddSequenceData(void *);
int SsdDisposeEffectData(int);
int SsdAddEffectData(void *);
int SsdStartSequence(int, int, int);
int SsdPauseSequence(int, int);
int SsdPlayEffectNormal(int, int);
int SsdPlayEffectParam(int, int, int, int);
int SsdStopEffect(int, int);
int SsdCheckPlayEffectAll(void);
int SsdSendFuncPacket(void *, int);
int SsdCheckPlayEffect(int);
int SsdInitPcmStream(int, int);
int SsdPlayPcmStream(int, int);
int SsdInitVagStreamStereo(int, int);
int SsdInitVagStreamMono(int, int, int);
int SsdPlayVagStream(int, int, int);
int SsdSetVagStreamPanpot(int, int, int);
int xglCdReadFile(char *, void *, int, int);
int xglCdStreamOpen(XGL_STREAM *, char *);
int xglCdStreamReadRing(XGL_STREAM *, int);
void xglCdStreamParamInit(XGL_STREAM *);
int xglSRand(void);
void xglMakeSePacket(short nCode, ...);
void *memset(void *, int, int);

/* Block until any queued SPU DMA transfer has completed */
void xglSoundWaitDma(void)
{
    SsdSpuDmaCompleted(0);
}

/* Load a wave bank into a slot, releasing whatever that slot held */
void xglSoundSendSwd(void *pData, int nNo)
{
    SOUND_SLOT *p;
    int nId;

    if (nNo < 0) {
        p = SoundWork.aSeq - nNo - 1;
        if (p->nData != 0xFFFF) {
            SsdStopSequence(p->nData);
            while (SsdGetResultValue(&nId) < 0) {
                ;
            }
        }
    } else {
        p = SoundWork.aSe + nNo;
        if (p->nData != 0xFFFF) {
            SsdStopEffectFileID(p->nData);
            while (SsdGetResultValue(&nId) < 0) {
                ;
            }
        }
    }
    if (p->nSwd != 0xFFFF) {
        SsdDisposeWaveBank(p->nSwd);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
    }
    nId = 0xFFFF;
    if (pData != 0) {
        SsdAddWaveData(pData, 0, 0);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
        if (nId < 0) {
            nId = 0xFFFF;
        }
    }
    {
        PIN(unsigned short nId2, "$3") = nId;
        p->nSwd = nId2;
    }
}

/* Load a sequence into a slot, releasing whatever that slot held */
void xglSoundSendSmd2(void *pData, int nNo)
{
    SOUND_SLOT *p;
    int nId;

    p = SoundWork.aSeq + nNo;
    if (p->nData != 0xFFFF) {
        SsdStopSequence(p->nData);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
        SsdDisposeSequence(p->nData);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
    }
    nId = 0xFFFF;
    if (pData != 0) {
        SsdAddSequenceData(pData);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
        if (nId < 0) {
            nId = 0xFFFF;
        }
    }
    {
        PIN(unsigned short nId2, "$3") = nId;
        p->nData = nId2;
    }
}

/* Load a sequence into slot 0 */
void xglSoundSendSmd(void *pData)
{
    xglSoundSendSmd2(pData, 0);
}

/* Load an effect bank into a slot, releasing whatever that slot held */
void xglSoundSendSed(void *pData, int nNo)
{
    SOUND_SLOT *p;
    int nId;

    p = SoundWork.aSe + nNo;
    if (p->nData != 0xFFFF) {
        SsdStopEffectFileID(p->nData);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
        SsdDisposeEffectData(p->nData);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
    }
    nId = 0xFFFF;
    if (pData != 0) {
        SsdAddEffectData(pData);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
        if (nId < 0) {
            nId = 0xFFFF;
        }
    }
    {
        PIN(unsigned short nId2, "$3") = nId;
        p->nData = nId2;
    }
}

/* Build "data\sound\<name>.SWD" and read that file into pBuf */
int xglSoundLoadSwd(char *pName, void *pBuf)
{
    static char ext[8] = ".SWD";
    char szPath[256];
    char *p;
    char *q;

    p = szPath;
    for (q = SoundDataPath; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = pName; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = ext; (*p = *q) != '\0'; q++) {
        p++;
    }
    return xglCdReadFile(szPath, pBuf, 0, 0);
}

/* Load a wave bank and its effect bank into one slot, waiting out the SPU DMA */
void xglSoundSendEffect(void *pSwd, void *pSed, int nNo)
{
    xglSoundSendSwd(pSwd, nNo);
    while (SsdSpuDmaCompleted(0) != 0) {
        ;
    }
    xglSoundSendSed(pSed, nNo);
}

/* Start the sequence held in a slot at the given volume and tempo */
void xglSoundSequenceNormal3(int nNo, int nVol, int nTempo)
{
    int nResult;
    int nMax;

    nMax = 0x7F;
    SsdStartSequence(SoundWork.aSeq[nNo].nData, (nVol < 0x80) ? nVol : nMax, nTempo / 2);
    while (SsdGetResultValue(&nResult) < 0) {
        ;
    }
}

/* Start the sequence held in a slot at the default tempo */
void xglSoundSequenceNormal2(int nNo, int nVol)
{
    xglSoundSequenceNormal3(nNo, nVol, 0);
}

/* Start the sequence held in slot 0 */
void xglSoundSequenceNormal(int nVol)
{
    xglSoundSequenceNormal3(0, nVol, 0);
}

/* Pause the sequence held in a slot over the given number of steps */
void xglSoundSequenceFadeOut2(int nNo, int nFade)
{
    int nId;

    nId = SoundWork.aSeq[nNo].nData;
    if (nId != 0xFFFF) {
        SsdPauseSequence(nId, nFade / 2);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
    }
}

/* Pause the sequence held in slot 0 */
void xglSoundSequenceFadeOut(int nFade)
{
    xglSoundSequenceFadeOut2(0, nFade);
}

/* Stop the sequence held in a slot */
void xglSoundSequenceStop2(int nNo)
{
    int nId;

    nId = SoundWork.aSeq[nNo].nData;
    if (nId != 0xFFFF) {
        SsdStopSequence(nId);
        while (SsdGetResultValue(&nId) < 0) {
            ;
        }
    }
}

/* Stop the sequence held in slot 0 */
void xglSoundSequenceStop(void)
{
    xglSoundSequenceStop2(0);
}

/* Play a sound effect straight through the driver, seeding the variation slots */
void xglSoundEffectNormalDirect(int nCode)
{
    int nBank;
    int nRand;

    nBank = SoundWork.aSe[nCode >> 16].nSwd;
    if (nBank != 0xFFFF) {
        nRand = 0;
        if (nCode > 0 && (nCode < 4 || nCode == 5)) {
            nRand = xglSRand() & 0x7FFF;
        }
        SsdPlayEffectNormal((nBank << 16) + (nCode & 0xFFFF), nRand);
    }
}

/* Queue a sound effect packet, filling in a random seed when none was given */
void xglSoundEffectNormalID(int nCode, int nRand)
{
    int nBank;

    nBank = SoundWork.aSe[nCode >> 16].nSwd;
    if (nBank != 0xFFFF) {
        if (nRand == 0 && nCode > 0 && (nCode < 4 || nCode == 5)) {
            nRand = xglSRand() & 0x7FFF;
        }
        xglMakeSePacket(0x70, (nBank << 16) + (nCode & 0xFFFF), nRand);
    }
}

/* Play a sound effect straight through the driver with volume and panpot */
void xglSoundEffectParamDirect(int nCode, int nVol, int nPan)
{
    int nBank;
    int nMax;

    nBank = SoundWork.aSe[nCode >> 16].nSwd;
    if (nBank != 0xFFFF) {
        nMax = 0x7F;
        SsdPlayEffectParam((nBank << 16) + (nCode & 0xFFFF), (nVol < 0x80) ? nVol : nMax, nPan, 0);
    }
}

/* Queue a sound effect packet with volume, panpot and pitch */
void xglSoundEffectParamID(int nCode, int nVol, int nPan, int nPitch)
{
    int nBank;
    int nMax;

    nBank = SoundWork.aSe[nCode >> 16].nSwd;
    if (nBank != 0xFFFF) {
        nMax = 0x7F;
        xglMakeSePacket(0x71, (nBank << 16) + (nCode & 0xFFFF), (nVol < 0x80) ? nVol : nMax, nPan, nPitch);
    }
}

/* Stop one playing sound effect straight through the driver */
void xglSoundEffectStopDirect(int nCode)
{
    int nBank;

    nBank = SoundWork.aSe[nCode >> 16].nSwd;
    if (nBank != 0xFFFF) {
        SsdStopEffect((nBank << 16) + (nCode & 0xFFFF), 0);
    }
}

/* Stop every sound effect belonging to one loaded bank */
void xglSoundEffectStopBank(int nNo)
{
    int nId;

    nId = SoundWork.aSe[nNo].nData;
    if (nId != 0xFFFF) {
        SsdStopEffectFileID(nId);
    }
}

/* Queue a stop-effect packet */
void xglSoundEffectStopID(int nCode, int nArg)
{
    int nBank;

    nBank = SoundWork.aSe[nCode >> 16].nSwd;
    if (nBank != 0xFFFF) {
        xglMakeSePacket(0x79, (nBank << 16) + (nCode & 0xFFFF), nArg);
    }
}

/* Ask the driver whether one sound effect - or any at all - is still playing */
int xglSoundEffectCheckID(int nCode)
{
    int nResult;
    int nBank;

    nResult = 0;
    if (nCode < 0) {
        nResult = SsdCheckPlayEffectAll();
    } else {
        nBank = SoundWork.aSe[nCode >> 16].nSwd;
        if (nBank != 0xFFFF) {
            SsdCheckPlayEffect((nBank << 16) + (nCode & 0xFFFF));
        }
    }
    while (SsdGetResultValue(&nResult) < 0) {
        ;
    }
    return nResult;
}

/* Matched by splitting the entry condition: `if (nChannel==0) {
 * pStream = &SoundWork.stream; if (pN && open()>=0)` puts the %hi(SoundWork)
 * lui between the two condition branches, exactly as the original. */
/* Open a PCM stream file and start it playing on channel 0 */
void xglSoundStreamOpenPcm(int nChannel, char *pName)
{
    PIN(char *pN, "$3");
    XGL_STREAM *pStream;
    int nResult;

    pN = pName;
    if (nChannel == 0) {
        pStream = &SoundWork.stream;
        if (pN != 0 && xglCdStreamOpen(pStream, pN) >= 0) {
            pStream->nRingSize = 0x1000;
            xglCdStreamReadRing(pStream, 0x2000);
            pStream->nSent = 0;
            SsdInitPcmStream(0x1000, 4);
            while (SsdGetResultValue(&nResult) < 0) {
                ;
            }
            SsdPlayPcmStream(0x7F, 0);
            while (SsdGetResultValue(&nResult) < 0) {
                ;
            }
            pStream->nPause = 0;
            pStream->nMode = 1;
        }
    }
}

/* Matched by splitting the entry condition: `if (nChannel==0) {
 * pStream = &SoundWork.stream; if (pN && open()>=0)` puts the %hi(SoundWork)
 * lui between the two condition branches, exactly as the original. */
/* Open a stereo VAG stream file and start it at the given volume */
void xglSoundStreamOpenVagStereoParam(int nChannel, char *pName, int nPan, int nVol)
{
    PIN(char *pN, "$3");
    XGL_STREAM *pStream;
    int nResult;
    int nMax;

    pN = pName;
    if (nChannel == 0) {
        pStream = &SoundWork.stream;
        if (pN != 0 && xglCdStreamOpen(pStream, pN) >= 0) {
            pStream->nRingSize = 0x1000;
            xglCdStreamReadRing(pStream, 0x2000);
            pStream->nSent = 0;
            SsdInitVagStreamStereo(0x1000, 4);
            while (SsdGetResultValue(&nResult) < 0) {
                ;
            }
            nMax = 0x7F;
            SsdPlayVagStream((nVol < 0x80) ? nVol : nMax, 0, nPan);
            while (SsdGetResultValue(&nResult) < 0) {
                ;
            }
            pStream->nPause = 0;
            pStream->nMode = 2;
        }
    }
}

/* Open a stereo VAG stream with the default panpot and volume */
void xglSoundStreamOpenVagStereo(int nChannel, char *pName)
{
    xglSoundStreamOpenVagStereoParam(nChannel, pName, 0x1000, 0x7F);
}

/* Matched by splitting the entry condition: `if (nChannel==0) {
 * pStream = &SoundWork.stream; if (pN && open()>=0)` puts the %hi(SoundWork)
 * lui between the two condition branches, exactly as the original. */
/* Open a mono VAG stream on one channel with panpot, volume and pitch */
void xglSoundStreamOpenVagMultiParam(int nChannel, char *pName, int nPan, int nVol, int nPanpot)
{
    PIN(char *pN, "$3");
    XGL_STREAM *pStream;
    int nResult;
    int nMax;

    pN = pName;
    if (nChannel == 0) {
        pStream = &SoundWork.stream;
        if (pN != 0 && xglCdStreamOpen(pStream, pN) >= 0) {
            pStream->nRingSize = 0x1000;
            xglCdStreamReadRing(pStream, 0x2000);
            pStream->nSent = 0;
            SsdInitVagStreamMono(0x1000, 1, 4);
            while (SsdGetResultValue(&nResult) < 0) {
                ;
            }
            SsdSetVagStreamPanpot(nChannel, nPanpot, 0);
            while (SsdGetResultValue(&nResult) < 0) {
                ;
            }
            nMax = 0x7F;
            SsdPlayVagStream((nVol < 0x80) ? nVol : nMax, 0, nPan);
            while (SsdGetResultValue(&nResult) < 0) {
                ;
            }
            pStream->nPause = 0;
            pStream->nMode = 3;
        }
    }
}

/* Open a mono VAG stream with the default panpot, volume and pitch */
void xglSoundStreamOpenVagMulti(int nChannel, char *pName)
{
    xglSoundStreamOpenVagMultiParam(nChannel, pName, 0x1000, 0x7F, 0x40);
}

/* Release every loaded bank and reset the streaming work area */
void xglSoundReset(void)
{
    XGL_STREAM *pStream;
    int i;
    PIN(int more, "$3");

    i = 0;
    do {
        xglSoundSendSwd(0, -1 - i);
        xglSoundSendSmd2(0, i);
        i++;
        more = (i < 8);
    } while (more);
    for (i = 0; i < 16; i++) {
        xglSoundSendEffect(0, 0, i);
    }
    pStream = &SoundWork.stream;
    pStream->nMode = 0;
    xglCdStreamParamInit(pStream);
    pStream->pRead = pStream->pBuffer;
    pStream->nSize = 0x4000;
    pStream->nCount = 0;
    memset(pStream->aRing, 0, 0x800);
}

/* Boot the sound driver and mark every bank slot empty */
void xglSoundInitial(void)
{
    SOUND_SLOT *p;
    int nResult;
    int i;

    SsdInit(0x20000);
    while (SsdGetResultValue(&nResult) < 0) {
        ;
    }
    SoundWork.stream.pBuffer = StreamBuffer;
    p = SoundWork.aSeq;
    for (i = 7; i >= 0; i--) {
        p->nSwd = -1;
        p->nData = -1;
        p++;
    }
    for (i = 0; i < 16; i++) {
        SoundWork.aSe[i].nSwd = -1;
        SoundWork.aSe[i].nData = -1;
    }
    xglSoundReset();
}

/* Send the accumulated SE command packet to the SSD driver and clear it */
void xglSendSePacket(void)
{
    SsdSendFuncPacket(SoundWork.stream.aRing, SoundWork.stream.nCount);
    SoundWork.stream.nCount = 0;
    memset(SoundWork.stream.aRing, 0, 0x800);
}

/* --- Stream stop / mute --- */
int SsdGetPcmStreamStatus(void);
int SsdStopPcmStream(int);
int SsdDisposePcmStream(void);
int SsdGetVagStreamStatusStereo(void);
int SsdStopVagStream(int);
int SsdDisposeVagStream(void);
int SsdSetVagStreamDataStereo(int, void *, int);
void xglCdStreamClose(XGL_STREAM *);
extern unsigned char *WorkEnd;

/* Stop whichever stream is playing on channel 0 and dispose its decoder */
void xglSoundStreamStop(int nChannel)
{
    XGL_STREAM *pStream;
    int nResult;
    int nMode;

    if (nChannel == 0) {
        pStream = &SoundWork.stream;
        xglCdStreamClose(pStream);
        nMode = pStream->nMode;
        if (nMode == 1) {
            goto pcm;
        }
        if (nMode < 2) {
            goto out;
        }
        if (nMode == 2) {
            goto stereo;
        }
        if (nMode != 3) {
            goto out;
        }
        goto multi;
    pcm:
        SsdGetPcmStreamStatus();
        while (SsdGetResultValue(&nResult) < 0) {
            ;
        }
        if (*(unsigned char *)&nResult != 0) {
            SsdStopPcmStream(0);
            SsdDisposePcmStream();
        }
        goto out;
    stereo:
        SsdGetVagStreamStatusStereo();
        while (SsdGetResultValue(&nResult) < 0) {
            ;
        }
        if (*(unsigned char *)&nResult != 0) {
            SsdStopVagStream(0);
            SsdDisposeVagStream();
        }
        goto out;
    multi:
        SsdStopVagStream(0);
        while (SsdGetResultValue(&nResult) < 0) {
            ;
        }
        SsdDisposeVagStream();
        while (SsdGetResultValue(&nResult) < 0) {
            ;
        }
    out:
        pStream->nMode = 0;
    }
}

/* The final drain loop's decrement must land in $v0 (with the slti temp
 * in $v1); every plain source shape allocates them swapped, and a bare
 * register-asm local gets coalesced away. The zero-code empty-asm
 * passthrough (PASSTHRU(t, expr) with t pinned to $2)
 * forces the subtraction to compute directly into $v0 without emitting
 * any instruction. */
/* Flush the stereo VAG stream with silence until its queue drains */
void xglSoundStreamMute(void)
{
    unsigned char *pBuf;
    int nResult;
    int i;

    unsigned char *p;

    pBuf = WorkEnd;
    SsdGetVagStreamStatusStereo();
    p = pBuf + 4095;
    for (i = 4095; i >= 0; i--) {
        *p-- = 0;
    }
    while (SsdGetResultValue(&nResult) < 0) {
        ;
    }
    while (nResult >= 256) {
        PIN(int t, "$2");

        SsdSetVagStreamDataStereo(0, pBuf, 4096);
        PASSTHRU(t, nResult - 256);
        nResult = t;
    }
}

/* --- SE packet builder (varargs) --- */
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_stdarg_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

typedef struct {
    char pad00[0xA4];
    short nCode;                    /* 0xA4: stream.aRing entry code */
    char padA6[0xE];
    int aParam[4];                  /* 0xB4: entry params */
} SE_ENTRY;

/* Append one SE command (code + up to four params) to the pending packet.
 * Register shape: the (nCount << 5) offset must compute into $v0 (a
 * plain pinned register var suffices here -- gcc honors it without a
 * passthrough), the entry pointer lands in $v1, and the original's
 * redundant pointer copy back into $v0 is reproduced by the zero-code
 * tied empty-asm passthrough (ofs and pEnt2 share $2; disjoint lives). */
void xglMakeSePacket(short nCode, ...)
{
    va_list ap;
    PIN(SE_ENTRY *pEnt, "$3");
    PIN(SE_ENTRY *pEnt2, "$2");
    PIN(int ofs, "$2");
    int *pDst;
    int i;

    if (SoundWork.stream.nCount < 65) {
        ofs = SoundWork.stream.nCount << 5;
        pEnt = (SE_ENTRY *)((char *)&SoundWork + ofs);
        PASSTHRU(pEnt2, pEnt);
        pEnt->nCode = nCode;
        va_start(ap, nCode);
        pDst = pEnt2->aParam;
        for (i = 3; i >= 0; i--) {
            *pDst = va_arg(ap, int);
            pDst++;
        }
        SoundWork.stream.nCount++;
    }
}

/* Stream watchdog: while fewer than six pre-fill blocks have been
 * pushed, zero the next 4 KB of the ring and advance; afterwards report
 * whether the consumer has caught up */
int stream_check(int nArg, XGL_STREAM *pStr)
{
    char *p;
    int i;

    nArg >>= 8;
    if (nArg == 0) {
        return 0;
    }
    if (pStr->nFill == 0) {
        goto one;
    }
    if (pStr->nFill < 6) {
        p = pStr->pRead + pStr->nPos;
        for (i = 0; i < 4096; i++) {
            *p = 0;
            p++;
        }
        pStr->nPos = (pStr->nPos + 4096) % pStr->nSize;
        pStr->nFill++;
        goto one;
    }
    if (pStr->nSent != pStr->nPos) {
        goto one;
    }
    return (nArg == 3) ? -1 : 0;
one:
    return 1;
}

int xglSoundLoadSwd(char *pName, void *pBuf);

/* TODO: near-miss (24 words, 116/116). Everything is structurally right;
 * the residue is a register tie-break -- the original puts pName in $s0
 * and pBuf in $s1, gcc here does the reverse, which renames every use of
 * both -- plus the guard branch being `bnez` where the original has the
 * branch-likely `bnezl` with `move a1,sp` annulled, and the second copy
 * loop's entry `lbu` sitting one slot later (a `nop` in its place).
 * Swept: swapping the `char *p; char *q;` declaration order; the guard
 * as nested ifs with a goto (111); hoisting `p = szPath` above the
 * guard (118 words); no spelling changes which parameter wins $s0. */
/* Load one effect's wave bank and effect bank into a slot: first
 * "sed\<name>" through the SWD loader, then
 * "data\sound\sed\<name>.SED" straight off the disc */
void xglSoundLoadEffect(char *pName, void *pBuf, int nNo)
{
    static char ext[8] = ".SED";
    char szPath[256];
    char *p;
    char *q;

    if (pName == 0 || pBuf == 0) {
        xglSoundSendSwd(0, nNo);
        xglSoundSendSed(0, nNo);
        return;
    }
    p = szPath;
    for (q = SoundEffectPath; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = pName; (*p = *q) != '\0'; q++) {
        p++;
    }
    if (xglSoundLoadSwd(szPath, pBuf) > 0) {
        xglSoundSendSwd(pBuf, nNo);
        while (SsdSpuDmaCompleted(0) != 0) {
            ;
        }
    }
    p = szPath;
    for (q = SoundDataPath; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = SoundEffectPath; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = pName; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = ext; (*p = *q) != '\0'; q++) {
        p++;
    }
    if (xglCdReadFile(szPath, pBuf, 0, 0) > 0) {
        xglSoundSendSed(pBuf, nNo);
    }
}

/* TODO: near-miss (13 words). The $s0/$s1 parameter tie-break is fixed
 * by the PINs below (gcc gives pName $s1 and pBuf $s0 on its own, and no
 * source shape flips it -- char* vs void* parameter, an extra use of
 * pName before the guard, while-loops instead of for-loops, nested-if
 * guards were all tried). What is left is ONE thing and its cascade: the
 * guard is `bnez` where the original has the branch-likely `bnezl`, so
 * the annulled `move a1,sp` (p = szPath) is not safe on the reset path
 * -- where $a1 must hold -1 for xglSoundSendSwd -- and gcc puts p in
 * $a2 instead, renaming the whole first copy loop. Pinning p to $5
 * makes it worse (18). This is a --branch-likely site
 * (xglSoundLoadRequestSmd:1) but the flag only flips the l-bit; the
 * register choice would still need fixing, so it is left unregistered.
 * xglSoundLoadEffect above has the identical residue. */
/* Load a sequence's wave bank and sequence data, then start it: first
 * "smd\<name>" through the SWD loader, then
 * "data\sound\smd\<name>.SMD" straight off the disc */
void xglSoundLoadRequestSmd(char *pNameArg, void *pBufArg)
{
    static char ext[8] = ".SMD";
    char szPath[256];
    /* PIN: the parameters' $s0/$s1 tie-break goes the other way here. */
    PIN(char *pName, "$16");
    PIN(void *pBuf, "$17");
    char *p;
    char *q;

    pName = pNameArg;
    pBuf = pBufArg;

    if (pName == 0 || pBuf == 0) {
        xglSoundSendSwd(0, -1);
        xglSoundSendSmd2(0, 0);
        return;
    }
    p = szPath;
    for (q = SoundSequencePath; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = pName; (*p = *q) != '\0'; q++) {
        p++;
    }
    if (xglSoundLoadSwd(szPath, pBuf) > 0) {
        xglSoundSendSwd(pBuf, -1);
        while (SsdSpuDmaCompleted(0) != 0) {
            ;
        }
    }
    p = szPath;
    for (q = SoundDataPath; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = SoundSequencePath; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = pName; *q != '\0'; q++) {
        *p++ = *q;
    }
    for (q = ext; (*p = *q) != '\0'; q++) {
        p++;
    }
    if (xglCdReadFile(szPath, pBuf, 0, 0) > 0) {
        xglSoundSendSmd2(pBuf, 0);
    }
    xglSoundSequenceNormal2(0, 127);
}
