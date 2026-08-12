/* Sound front-end - wraps the SSD driver with bank slots and CD streaming */

typedef struct {
    unsigned short nSwd;            /* 0x00 */
    unsigned short nData;           /* 0x02 */
} SOUND_SLOT;

typedef struct {
    char pad000[0x1C];              /* 0x00 */
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
int xglMakeSePacket();
void *memset(void *, int, int);

/* Block until any queued SPU DMA transfer has completed */
void xglSoundWaitDma(void)
{
    SsdSpuDmaCompleted(0);
}

/* TODO: near-match - the reloaded slot value lands in $v0 where the original used $v1 */
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
    p->nSwd = nId;
}

/* TODO: near-match - the reloaded slot value lands in $v0 where the original used $v1 */
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
    p->nData = nId;
}

/* Load a sequence into slot 0 */
void xglSoundSendSmd(void *pData)
{
    xglSoundSendSmd2(pData, 0);
}

/* TODO: near-match - the reloaded slot value lands in $v0 where the original used $v1 */
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
    p->nData = nId;
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

/* TODO: near-match - the file-name copy lands in $v0, blocking the pre-branch %hi load */
/* Open a PCM stream file and start it playing on channel 0 */
void xglSoundStreamOpenPcm(int nChannel, char *pName)
{
    XGL_STREAM *pStream;
    int nResult;

    if (nChannel == 0 && pName != 0) {
        pStream = &SoundWork.stream;
        if (xglCdStreamOpen(pStream, pName) >= 0) {
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

/* TODO: near-match - the file-name copy lands in $v0, blocking the pre-branch %hi load */
/* Open a stereo VAG stream file and start it at the given volume */
void xglSoundStreamOpenVagStereoParam(int nChannel, char *pName, int nPan, int nVol)
{
    XGL_STREAM *pStream;
    int nResult;
    int nMax;

    if (nChannel == 0 && pName != 0) {
        pStream = &SoundWork.stream;
        if (xglCdStreamOpen(pStream, pName) >= 0) {
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

/* TODO: near-match - the file-name copy lands in $v0, blocking the pre-branch %hi load */
/* Open a mono VAG stream on one channel with panpot, volume and pitch */
void xglSoundStreamOpenVagMultiParam(int nChannel, char *pName, int nPan, int nVol, int nPanpot)
{
    XGL_STREAM *pStream;
    int nResult;
    int nMax;

    if (nChannel == 0 && pName != 0) {
        pStream = &SoundWork.stream;
        if (xglCdStreamOpen(pStream, pName) >= 0) {
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

/* TODO: near-match - the loop guard emits bnezl/$v0 where the original has bnez/$v1 */
/* Release every loaded bank and reset the streaming work area */
void xglSoundReset(void)
{
    XGL_STREAM *pStream;
    int i;

    for (i = 0; i < 8; i++) {
        xglSoundSendSwd(0, -1 - i);
        xglSoundSendSmd2(0, i);
    }
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
