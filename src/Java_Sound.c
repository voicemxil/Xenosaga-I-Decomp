/* Native bindings for the script VM's xeno.Sound class */

typedef union {
    int i;
    unsigned int u;
    float f;
    short h;
    unsigned char c;
    void *p;
} JVAL;

void xglSoundSequenceNormal(int);
void xglSoundSequenceNormal2(int, int);
void xglSoundSequenceFadeOut(int);
void xglSoundSequenceFadeOut2(int, int);
void xglSoundEffectParamID(int, int, int, int);
void xglSoundEffectStopID(int, int);
void xglSoundStreamStop(int);
void xglSoundStreamOpenVagMultiParam(int, char *, int, int, int);
void xglSoundStreamOpenVagStereoParam(int, char *, int, int);

/* Start a BGM sequence */
void Java_xeno_Sound_sequencePlay__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    xglSoundSequenceNormal(pArgs[0].i);
}

/* Start a BGM sequence with an explicit variation */
void Java_xeno_Sound_sequencePlay__II(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    xglSoundSequenceNormal2(pArgs[0].i, pArgs[1].i);
}

/* Fade a BGM sequence out */
void Java_xeno_Sound_sequenceStop__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    xglSoundSequenceFadeOut(pArgs[0].i);
}

/* Fade a BGM sequence out over a given time */
void Java_xeno_Sound_sequenceStop__II(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    xglSoundSequenceFadeOut2(pArgs[0].i, pArgs[1].i);
}

/* Play a sound effect by id */
void Java_xeno_Sound_effectPlay__III(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    xglSoundEffectParamID(pArgs[0].i, pArgs[1].i, pArgs[2].i, 0);
}

/* Stop a sound effect by id */
void Java_xeno_Sound_effectStop__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    xglSoundEffectStopID(pArgs[0].i, 0);
}

/* Build the VAG stream path from the id and open it (mono or stereo) */
void Java_xeno_Sound_streamPlay__IIII(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    static char base[] = "data\\sound\\vda\\s";
    char aName[256];
    char *pDst;
    char *pSrc;
    int nId;
    int i;

    nId = pArgs[0].i;
    pDst = aName;
    pSrc = base;
    while ((*pDst = *pSrc) != 0) {
        pSrc++;
        pDst++;
    }
    for (i = 5; i >= 0; i--) {
        pDst[i] = nId % 10 + '0';
        nId /= 10;
    }
    pDst[6] = '.';
    pDst[7] = 'v';
    pDst[8] = 'd';
    if (nId == 0) {
        pDst[9] = 'm';
        pDst[10] = 0;
        xglSoundStreamStop(0);
        xglSoundStreamOpenVagMultiParam(0, aName, (pArgs[1].i << 12) / 48000, pArgs[2].i, pArgs[3].i);
    } else {
        pDst[9] = 's';
        pDst[10] = 0;
        xglSoundStreamStop(0);
        xglSoundStreamOpenVagStereoParam(0, aName, (pArgs[1].i << 12) / 48000, pArgs[2].i);
    }
}
