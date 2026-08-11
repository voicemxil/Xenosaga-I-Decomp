/* Sound driver (SSD) client API - packet builders around the IOP RPC bridge */

typedef struct {
    int nFlags;                     /* 0x000 */
    char pad004[0xC];               /* 0x004 */
    unsigned short nResolution;     /* 0x010 */
    char pad012[0xE];               /* 0x012 */
    void *pServerFunc;              /* 0x020 */
    void *pServerArg;               /* 0x024 */
    void *pFuncFunc;                /* 0x028 */
    void *pFuncArg;                 /* 0x02C */
    char pad030[0x50];              /* 0x030 */
    short nResultCode;              /* 0x080 */
    char pad082[0x6];               /* 0x082 */
    int nResultValue;               /* 0x088 */
    char pad08C[0x134];             /* 0x08C */
    char *pMemStart;                /* 0x1C0 */
    char *pMemEnd;                  /* 0x1C4 */
    int nMemSize;                   /* 0x1C8 */
    int nDmaSize;                   /* 0x1CC */
    void *pDmaAddr;                 /* 0x1D0 */
    int nDmaId;                     /* 0x1D4 */
    char pad1D8[0x4];               /* 0x1D8 */
    short nDmaFlag;                 /* 0x1DC */
    char pad1DE[0xA];               /* 0x1DE */
    void *pSampleDmaFunc;           /* 0x1E8 */
    void *pSampleDmaArg;            /* 0x1EC */
    void *pSampleKeyoffFunc;        /* 0x1F0 */
    void *pSampleKeyoffArg;         /* 0x1F4 */
    void *pStreamEndFunc;           /* 0x1F8 */
    void *pStreamEndArg;            /* 0x1FC */
} RSSD_WORK;

typedef struct {
    int nHead[4];                   /* 0x00 */
    int nArg[4];                    /* 0x10 */
} RSSD_PACKET;

typedef struct {
    int nMagic;                     /* 0x00 */
    int nUnk04;                     /* 0x04 */
    int nSize;                      /* 0x08 */
} SSD_DATA;

typedef struct {
    unsigned short nUnk00;          /* 0x00 */
    unsigned short nCount;          /* 0x02 */
} SSD_PATCHSET;

typedef struct SSD_MEMBLOCK {
    struct SSD_MEMBLOCK *pNext;     /* 0x00 */
    char *pEnd;                     /* 0x04 */
    signed char nUsed;              /* 0x08 */
    signed char nUnk09;             /* 0x09 */
    short pad0A;                    /* 0x0A */
    int nMagic;                     /* 0x0C */
} SSD_MEMBLOCK;

extern RSSD_WORK RssdWork;
extern int D_004AA20C[4];
extern SSD_MEMBLOCK *D_004AA240 __attribute__((section(".data")));
extern void *D_004AA284[4];
extern void *D_004AA294[4];

int RssdCallFunc(int, RSSD_PACKET *, void *, int);
void SsdSpuDmaCompleted(int);
void *iSsdNewMemoryPtr(int, int);
void *iSsdNewMemoryPtr2(int, int);
void iSsdDisposeMemoryPtr(void *);
void DIntr(void);
void EIntr(void);
void *sceSifGetNextRequest(void *);
void sceSifExecRequest(void *);
int printf(const char *, ...);

/* Fetch the scalar result of the last completed sound-driver request */
int SsdGetResultValue(int *pValue)
{
    int nFlags;
    int nResult;

    nFlags = RssdWork.nFlags;
    if (nFlags & 8) {
        nResult = (nFlags & 0x20) ? -1 : -2;
    } else if (RssdWork.nResultCode >= 0) {
        nResult = ((nFlags >> 5) ^ 1) & 1;
        if (pValue != 0) {
            *pValue = RssdWork.nResultValue;
        }
    } else {
        nResult = -3;
        printf("Ssd: request error\n");
    }
    return nResult;
}

/* Install the callback invoked when the sound server posts an event */
void SsdSetServerCallback(void *pFunc, void *pArg)
{
    RssdWork.pServerFunc = pFunc;
    RssdWork.pServerArg = pArg;
}

/* Install the callback invoked when a driver function call completes */
void SsdSetFuncCallback(void *pFunc, void *pArg)
{
    RssdWork.pFuncFunc = pFunc;
    RssdWork.pFuncArg = pArg;
}

/* Install the callback invoked when a stream reaches its end */
void SsdSetStreamEndCallback(void *pFunc, void *pArg)
{
    RssdWork.pStreamEndFunc = pFunc;
    RssdWork.pStreamEndArg = pArg;
}

/* Resume the sound driver after a suspend */
int SsdResume(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(5, &pkt, 0, 0);
}

/* Suspend all sound driver processing */
int SsdSuspend(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(6, &pkt, 0, 0);
}

/* Select the analog output mode (stereo / monaural) */
int SsdSetOutputMode(int nMode)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nMode;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x10, &pkt, 0, 0);
}

/* Select the digital (S/PDIF) output mode */
int SsdSetSPDIFMode(int nMode)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nMode;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x11, &pkt, 0, 0);
}

/* Set the SPU master output volume */
int SsdSetSpuVolume(int nCore, int nVolume)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVolume;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x12, &pkt, 0, 0);
}

/* Set the CD/external input volume */
int SsdSetCDVolume(int nCore, int nVolume)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVolume;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x13, &pkt, 0, 0);
}

/* Configure the SPU reverb unit */
int SsdSetReverbParameter(int nMode, int nDepthL, int nDepthR, int nDelay)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nMode;
    pkt.nArg[1] = nDepthL;
    pkt.nArg[2] = nDepthR;
    pkt.nArg[3] = nDelay;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x14, &pkt, 0, 0);
}

/* Forward a block of raw driver command packets to the IOP */
void SsdSendFuncPacket(void *pPacket, int nCount)
{
    RSSD_PACKET pkt;

    if (nCount != 0) {
        pkt.nArg[0] = (int)pPacket;
        pkt.nArg[1] = nCount;
        RssdWork.nFlags &= ~0x20;
        RssdCallFunc(4, &pkt, pPacket, nCount * 32);
    }
}

/* Enable segment based allocation for the following wave transfers */
int SsdSetSegmentAllocMode(int nMode, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nMode;
    pkt.nArg[1] = nSize;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x2C, &pkt, 0, 0);
    return 0;
}

/* Restore the default (linear) wave allocation mode */
int SsdResetSegmentAllocMode(int nMode)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nMode;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x2D, &pkt, 0, 0);
    return 0;
}

/* Release every wave loaded into the given wave bank */
int SsdDisposeWaveBank(int nBankId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nBankId;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x22, &pkt, 0, 0);
}

/* Ask the driver whether a wave transfer has finished */
int SsdCheckWaveData(int nBankId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nBankId;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x24, &pkt, 0, 0);
    return 0;
}

/* Drain the SIF request queue while an SPU DMA is in flight */
void SsdSpuDmaCompleted(int bWait)
{
    void *pRequest;
    int bBusy;

    bBusy = (RssdWork.nFlags >> 2) & 1;
    if (bWait != 0 && bBusy) {
        do {
            while ((pRequest = sceSifGetNextRequest(D_004AA20C)) != 0) {
                sceSifExecRequest(pRequest);
            }
        } while ((RssdWork.nFlags >> 2) & 1);
    }
}

/* Query the amount of SPU local memory still available */
int SsdCheckSpuMemory(int nUnused)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nUnused;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x100, &pkt, 0, 0);
}

/* Register a sound effect bank ('sesd') with the driver */
int SsdAddEffectData(SSD_DATA *pData)
{
    RSSD_PACKET pkt;
    int nSize;

    if (pData->nMagic != 0x73646573) {
        printf("Ssd: not effect data\n");
        return -1;
    }
    nSize = pData->nSize;
    pkt.nArg[0] = (int)pData;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x60, &pkt, pData, nSize);
    return RssdWork.nResultValue;
}

/* Release a previously registered sound effect bank */
int SsdDisposeEffectData(int nId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x61, &pkt, 0, 0);
}

/* Ask the driver whether an effect bank has finished loading */
int SsdCheckEffectData(int nId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x62, &pkt, 0, 0);
    return 0;
}

/* Play a sound effect using the parameters stored in its bank */
int SsdPlayEffectNormal(int nId, int nNumber)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNumber;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x70, &pkt, 0, 0);
}

/* Play a sound effect with an explicit volume and panpot */
int SsdPlayEffectParam(int nId, int nNumber, int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNumber;
    pkt.nArg[2] = nVolume;
    pkt.nArg[3] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x71, &pkt, 0, 0);
}

/* Update the stored playback parameters of an effect entry */
int SsdSetPlayEffectParam(int nId, int nNumber, int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNumber;
    pkt.nArg[2] = nVolume;
    pkt.nArg[3] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x7C, &pkt, 0, 0);
}

/* Change the parameters of a currently sounding effect */
int SsdSetEffectParam(int nId, int nNumber, int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNumber;
    pkt.nArg[2] = nVolume;
    pkt.nArg[3] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x7B, &pkt, 0, 0);
}

/* Silence every sound effect voice */
int SsdStopAllEffect(void)
{
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x78, 0, 0, 0);
}

/* Stop one sound effect */
int SsdStopEffect(int nId, int nNumber)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNumber;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x79, &pkt, 0, 0);
}

/* Let a single looped effect run out instead of looping again */
int SsdContinueOneEffect(int nId, int nNumber)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNumber;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x7D, &pkt, 0, 0);
}

/* Fade one sound effect out over the given time */
int SsdFadeoutEffect(int nId, int nNumber, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNumber;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x7E, &pkt, 0, 0);
}

/* Fade out every effect sourced from one file id */
int SsdFadeoutEffectFileID(int nFileId, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nFileId;
    pkt.nArg[1] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x7F, &pkt, 0, 0);
}

/* Play a raw wave with its default parameters */
int SsdPlayWaveNormal(int nBankId, int nWaveId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nBankId;
    pkt.nArg[1] = nWaveId;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x90, &pkt, 0, 0);
}

/* Play a raw wave with an explicit volume and panpot */
int SsdPlayWaveParam(int nBankId, int nWaveId, int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nBankId;
    pkt.nArg[1] = nWaveId;
    pkt.nArg[2] = nVolume;
    pkt.nArg[3] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x91, &pkt, 0, 0);
}

/* Stop a raw wave voice */
int SsdStopWave(int nBankId, int nWaveId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nBankId;
    pkt.nArg[1] = nWaveId;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x92, &pkt, 0, 0);
}

/* Stop every effect sourced from one file id */
int SsdStopEffectFileID(int nFileId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nFileId;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x7A, &pkt, 0, 0);
}

/* Ask whether any sound effect is currently playing */
int SsdCheckPlayEffectAll(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x80, &pkt, 0, 0);
    return 0;
}

/* Ask whether one sound effect is currently playing */
int SsdCheckPlayEffect(int nId, int nNumber)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNumber;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x81, &pkt, 0, 0);
    return 0;
}

/* Ask whether any effect from one file id is currently playing */
int SsdCheckPlayEffectFileID(int nFileId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nFileId;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x82, &pkt, 0, 0);
    return 0;
}

/* Reserve SPU memory and voices for sample playback */
int SsdInitSampling(int nVoice, int nAddr, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVoice;
    pkt.nArg[1] = nAddr;
    pkt.nArg[2] = nSize;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xD0, &pkt, 0, 0);
}

/* Release the sample playback resources */
int SsdDisposeSampling(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xD1, &pkt, 0, 0);
}

/* Install the callback fired when a sample DMA completes */
void SsdSetSampleDmaCallback(void *pFunc, void *pArg)
{
    RssdWork.pSampleDmaFunc = pFunc;
    RssdWork.pSampleDmaArg = pArg;
}

/* Install the callback fired when a sample voice is keyed off */
void SsdSetSampleKeyoffCallback(void *pFunc, void *pArg)
{
    RssdWork.pSampleKeyoffFunc = pFunc;
    RssdWork.pSampleKeyoffArg = pArg;
}

/* Start transferring a sample buffer into SPU memory */
int SsdTransferSampling(int nVoice, void *pAddr, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVoice;
    pkt.nArg[1] = (int)pAddr;
    pkt.nArg[2] = nSize;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xD2, &pkt, pAddr, nSize);
}

/* Queue the next sample buffer of an ongoing transfer */
int SsdTransferSamplingNext(void *pAddr, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = (int)pAddr;
    pkt.nArg[1] = nSize;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xD3, &pkt, pAddr, nSize);
}

/* Key on a sample voice */
int SsdPlaySampling(int nVoice, int nPitch, int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVoice;
    pkt.nArg[1] = nPitch;
    pkt.nArg[2] = nVolume;
    pkt.nArg[3] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xD4, &pkt, 0, 0);
}

/* Key off a sample voice */
int SsdStopSampling(int nVoice)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVoice;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xD5, &pkt, 0, 0);
}

/* Change pitch, volume and panpot of a sounding sample voice */
int SsdSetSamplingParam(int nVoice, int nPitch, int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVoice;
    pkt.nArg[1] = nPitch;
    pkt.nArg[2] = nVolume;
    pkt.nArg[3] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xD6, &pkt, 0, 0);
}

/* Route a sample voice through the effect (reverb) send */
int SsdSetSamplingEffect(int nVoice, int nEffect, int nDepth)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVoice;
    pkt.nArg[1] = nEffect;
    pkt.nArg[2] = nDepth;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xD7, &pkt, 0, 0);
}

/* Issue a bare driver request that carries no packet payload */
int RssdRequestCall(int nCmd)
{
    return RssdCallFunc(nCmd, 0, 0, 0);
}

/* Register a sequence and start it playing */
int SsdPlaySequence(SSD_DATA *pData, int nTrack, int nLoop)
{
    RSSD_PACKET pkt;
    int nSize;

    nSize = pData->nSize;
    pkt.nArg[0] = (int)pData;
    pkt.nArg[1] = nTrack;
    pkt.nArg[2] = nLoop;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x30, &pkt, pData, nSize);
    return RssdWork.nResultValue;
}

/* Register a sequence without starting it */
int SsdAddSequenceData(SSD_DATA *pData)
{
    RSSD_PACKET pkt;
    int nSize;

    nSize = pData->nSize;
    pkt.nArg[0] = (int)pData;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x31, &pkt, pData, nSize);
    return RssdWork.nResultValue;
}

/* Register the effect bank that belongs to a sequence */
int SsdAddSeqEffectData(SSD_DATA *pData)
{
    RSSD_PACKET pkt;
    int nSize;

    nSize = pData->nSize;
    pkt.nArg[0] = (int)pData;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x38, &pkt, pData, nSize);
    return RssdWork.nResultValue;
}

/* Release a registered sequence */
int SsdDisposeSequence(int nId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x32, &pkt, 0, 0);
    return 0;
}

/* Start playback of an already registered sequence */
int SsdStartSequence(int nId, int nTrack, int nLoop)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nTrack;
    pkt.nArg[2] = nLoop;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x34, &pkt, 0, 0);
}

/* Restore a paused sequence by driving its master volume back up */
int SsdResumeSequence(int nId, int nVolume, int nTime)
{
    return SsdSetSeqMasterVolume(nId, nVolume, nTime);
}

/* Stop a sequence */
int SsdStopSequence(int nId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x36, &pkt, 0, 0);
}

/* Pause or unpause a sequence */
int SsdPauseSequence(int nId, int nPause)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nPause;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x37, &pkt, 0, 0);
}

/* Ask whether a sequence is still playing */
int SsdGetSeqPlayStatus(int nId)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0x40, &pkt, 0, 0);
    return RssdWork.nResultValue;
}

/* Set the master tempo of a sequence */
int SsdSetSeqMasterTempo(int nId, int nTempo, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nTempo;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x41, &pkt, 0, 0);
}

/* Set the master volume of a sequence */
int SsdSetSeqMasterVolume(int nId, int nVolume, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nVolume;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x42, &pkt, 0, 0);
}

/* Transpose a whole sequence */
int SsdSetSeqMasterNote(int nId, int nNote, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNote;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x43, &pkt, 0, 0);
}

/* Set the master panpot of a sequence */
int SsdSetSeqMasterPanpot(int nId, int nPanpot, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nPanpot;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x44, &pkt, 0, 0);
}

/* Slide the transposition of a sequence over time */
int SsdSetSeqNoteFader(int nId, int nNote, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nNote;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x48, &pkt, 0, 0);
}

/* Slide the volume of a sequence over time */
int SsdSetSeqVolumeFader(int nId, int nVolume, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nVolume;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x49, &pkt, 0, 0);
}

/* Slide the panpot of a sequence over time */
int SsdSetSeqPanpotFader(int nId, int nPanpot, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nPanpot;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x4A, &pkt, 0, 0);
}

/* Set the output bus volume used by sequences */
int SsdSetSequenceOutputVolume(int nCore, int nVolume)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nCore;
    pkt.nArg[1] = nVolume;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x4B, &pkt, 0, 0);
}

/* Set the output bus volume used by sound effects */
int SsdSetEffectOutVolume(int nCore, int nVolume)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nCore;
    pkt.nArg[1] = nVolume;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x4C, &pkt, 0, 0);
}

/* Select the patch bank a sequence plays with */
int SsdSetSeqPatchNumber(int nId, int nPatch)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nPatch;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x50, &pkt, 0, 0);
}

/* Upload a per-track patch assignment table */
int SsdSetTrackPatchSet(int nId, SSD_PATCHSET *pSet)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = 0;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x51, &pkt, pSet, pSet->nCount * 8 + 4);
}

/* Mute or unmute a sequence track */
int SsdSetSeqMute(int nId, int nMute)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nMute;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x45, &pkt, 0, 0);
}

/* Arm a user signal that fires when the sequence reaches a marker */
int SsdSetSeqSignal(int nId, int nSignal)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nId;
    pkt.nArg[1] = nSignal;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0x58, &pkt, 0, 0);
}

/* Set up the PCM streaming ring buffer */
void SsdInitPcmStream(void *pBuffer, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = (int)pBuffer;
    pkt.nArg[1] = nSize;
    RssdWork.nFlags &= ~0x20;
    RssdCallFunc(0xA0, &pkt, 0, 0);
    D_004AA284[0] = pBuffer;
}

/* Tear down the PCM stream */
int SsdDisposePcmStream(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xA1, &pkt, 0, 0);
}

/* Start PCM stream playback */
int SsdPlayPcmStream(int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVolume;
    pkt.nArg[1] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xA2, &pkt, 0, 0);
}

/* Stop PCM stream playback */
int SsdStopPcmStream(int nFade)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nFade;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xA3, &pkt, 0, 0);
}

/* Hand another PCM block to the streaming ring buffer */
int SsdSetPcmStreamData(void *pAddr, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = (int)pAddr;
    pkt.nArg[1] = nSize;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xA4, &pkt, pAddr, nSize);
}

/* Ask how much of the PCM stream has been consumed */
int SsdGetPcmStreamStatus(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0xA5, &pkt, 0, 0);
    return 0;
}

/* Change the PCM stream volume */
int SsdSetPcmStreamVolume(int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nVolume;
    pkt.nArg[1] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xA6, &pkt, 0, 0);
}

/* Set up a stereo VAG stream ring buffer */
void SsdInitVagStreamStereo(void *pBuffer, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = (int)pBuffer;
    pkt.nArg[1] = nSize;
    RssdWork.nFlags &= ~0x20;
    RssdCallFunc(0xB0, &pkt, 0, 0);
    D_004AA294[0] = pBuffer;
}

/* Set up a monaural VAG stream ring buffer */
void SsdInitVagStreamMono(void *pBuffer, int nSize, int nChannels)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = (int)pBuffer;
    pkt.nArg[1] = nSize;
    pkt.nArg[2] = nChannels;
    RssdWork.nFlags &= ~0x20;
    RssdCallFunc(0xB1, &pkt, 0, 0);
    D_004AA294[0] = pBuffer;
}

/* Tear down the VAG stream */
int SsdDisposeVagStream(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xB2, &pkt, 0, 0);
}

/* Start VAG stream playback */
int SsdPlayVagStream(int nChannel, int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    pkt.nArg[1] = nVolume;
    pkt.nArg[2] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xB3, &pkt, 0, 0);
}

/* Stop VAG stream playback */
int SsdStopVagStream(int nChannel)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xB4, &pkt, 0, 0);
}

/* Hand another stereo VAG block to the stream */
int SsdSetVagStreamDataStereo(int nChannel, void *pAddr, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    pkt.nArg[1] = (int)pAddr;
    pkt.nArg[2] = nSize;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xB5, &pkt, pAddr, nSize);
}

/* Hand another monaural VAG block to the stream */
int SsdSetVagStreamDataMono(int nChannel, void *pAddr, int nSize)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    pkt.nArg[1] = (int)pAddr;
    pkt.nArg[2] = nSize;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xB6, &pkt, pAddr, nSize);
}

/* Ask how much of the stereo VAG stream has been consumed */
int SsdGetVagStreamStatusStereo(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0xB7, &pkt, 0, 0);
    return 0;
}

/* Ask how much of one monaural VAG stream channel has been consumed */
int SsdGetVagStreamStatusMono(int nChannel)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0xB8, &pkt, 0, 0);
    return 0;
}

/* Read back the running parameters of a VAG stream channel */
int SsdGetVagStreamParam(int nChannel)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0xB8, &pkt, 0, 0);
    return 0;
}

/* Ask for the consumed size of every VAG stream channel at once */
int SsdGetVagStreamStatusAll(void)
{
    RSSD_PACKET pkt;

    RssdWork.nFlags |= 0x20;
    RssdCallFunc(0xBE, &pkt, 0, 0);
    return 0;
}

/* Change the playback pitch of a VAG stream channel */
int SsdSetVagStreamPitch(int nChannel, int nPitch, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    pkt.nArg[1] = nPitch;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xBA, &pkt, 0, 0);
}

/* Change the volume of a VAG stream channel */
int SsdSetVagStreamVolume(int nChannel, int nVolume, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    pkt.nArg[1] = nVolume;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xBB, &pkt, 0, 0);
}

/* Change the panpot of a VAG stream channel */
int SsdSetVagStreamPanpot(int nChannel, int nPanpot, int nTime)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    pkt.nArg[1] = nPanpot;
    pkt.nArg[2] = nTime;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xBC, &pkt, 0, 0);
}

/* Change pitch, volume and panpot of a VAG stream channel at once */
int SsdSetVagStreamParam(int nChannel, int nPitch, int nVolume, int nPanpot)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    pkt.nArg[1] = nPitch;
    pkt.nArg[2] = nVolume;
    pkt.nArg[3] = nPanpot;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xBD, &pkt, 0, 0);
}

/* Discard everything queued in the stream ring buffer */
int SsdClearStreamRingBuffer(int nChannel)
{
    RSSD_PACKET pkt;

    pkt.nArg[0] = nChannel;
    RssdWork.nFlags &= ~0x20;
    return RssdCallFunc(0xA8, &pkt, 0, 0);
}

/* Initialize the 64-byte-aligned sound heap and its first free block */
void SsdInitMemoryManager(void *pAddr, int nSize)
{
    volatile RSSD_WORK *pWork;
    volatile SSD_MEMBLOCK *pBlock;
    register signed char nUsed;
    register char *pBlockEnd;
    char *pStart;
    int nMagic;

    pStart = pAddr;
    nSize &= ~0x3F;
    if ((unsigned int)pStart & 0x3F) {
        pStart = (char *)(((unsigned int)pStart + 0x3F) & ~0x3F);
        nSize -= 0x40;
    }

    pWork = &RssdWork;
    pWork->nMemSize = nSize;
    pWork->pMemEnd = pStart + nSize;
    pWork->pMemStart = pStart;

    pBlock = (SSD_MEMBLOCK *)pStart;
    nUsed = -0x7F;
    nMagic = 0x504F544D;
    pBlockEnd = pStart + 0x40;
    pBlock->nMagic = nMagic;
    pBlock->pNext = 0;
    pBlock->nUsed = nUsed;
    pBlock->pEnd = pBlockEnd;
    pBlock->nUnk09 = 0;
}

/* Allocate from the sound heap with interrupts disabled */
void *SsdNewMemoryPtr(int nSize, int nAlign)
{
    void *pPtr;

    DIntr();
    pPtr = iSsdNewMemoryPtr(nSize, nAlign);
    EIntr();
    return pPtr;
}

/* Allocate from the alternate sound heap path with interrupts disabled */
void *SsdNewMemoryPtr2(int nSize, int nAlign)
{
    void *pPtr;

    DIntr();
    pPtr = iSsdNewMemoryPtr2(nSize, nAlign);
    EIntr();
    return pPtr;
}

/* Return a sound heap block with interrupts disabled */
void SsdDisposeMemoryPtr(void *pPtr)
{
    DIntr();
    iSsdDisposeMemoryPtr(pPtr);
    EIntr();
}

/* Report the usable size of one sound heap block */
int SsdGetBlockMemorySize(void *pPtr)
{
    SSD_MEMBLOCK *pBlock;

    pBlock = (SSD_MEMBLOCK *)((char *)pPtr - 0x40);
    return pBlock->pEnd - (char *)pBlock;
}

/* Count allocated blocks in the sound heap */
int SsdGetMemoryBlocks(void)
{
    /* TODO: Find the natural source shape for these matched pointer registers. */
    register SSD_MEMBLOCK *pRoot __asm__("$2");
    register SSD_MEMBLOCK *pBlock __asm__("$3");
    SSD_MEMBLOCK *pNext;
    int nBlocks;

    DIntr();
    nBlocks = 0;
    pRoot = D_004AA240;
    pBlock = pRoot->pNext;
    if (pBlock != 0) {
        do {
            pNext = pBlock->pNext;
            nBlocks++;
            pBlock = pNext;
        } while (pNext != 0);
    }
    EIntr();
    return nBlocks;
}
