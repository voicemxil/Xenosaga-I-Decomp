/* EXM - the extra-motion layer: the secondary physics that drives skirts and
 * hair after the skeleton has been posed, plus the global wind that feeds it.
 *
 * All of the wind state lives in one small block.  `wind` normally points at
 * the built in `_wind`, but a caller can swap in its own block (a cut scene
 * wants its own wind) and later restore the default.  The block holds a mode
 * byte, one vector that is read either as a direction or as a point source
 * depending on that mode, and the shake oscillator (amplitude, per frame step
 * and the accumulated phase).  `waverad` is the second, always running,
 * oscillator used for the ambient sway; both phases are kept inside one turn.
 *
 * The moved-hair entry points poke the per model hair state that lives at
 * offset 0xA40 of a character work block.
 */

typedef struct {
    float x;                    /* 0x00 */
    float y;                    /* 0x04 */
    float z;                    /* 0x08 */
    float w;                    /* 0x0C */
} EXM_VECTOR;

typedef struct {
    unsigned char nType;        /* 0x00 */
    char pad001[0xF];           /* 0x01 */
    EXM_VECTOR vDir;            /* 0x10 */
    float fShakePower;          /* 0x20 */
    float fShakeStep;           /* 0x24 */
    float fShakeRad;            /* 0x28 */
    float pad02C;               /* 0x2C */
} __attribute__((aligned(8))) EXM_WIND;

/* Character work block; only the moved-hair tail is described here. */
typedef struct {
    char pad000[0xA40];         /* 0x000 */
    unsigned char nHairMode;    /* 0xA40 */
    char padA41[0x3];           /* 0xA41 */
    float fHairDelay;           /* 0xA44 */
    char padA48[0x8];           /* 0xA48 */
    int nHairA50;               /* 0xA50 */
    int nHairA54;               /* 0xA54 */
    int nHairA58;               /* 0xA58 */
    int nHairA5C;               /* 0xA5C */
} EXM_MODEL;

#define EXM_2PI 6.283185482f

EXM_WIND _wind;

EXM_WIND *wind = &_wind;
float wave;
float waverad;

/* Arm the moved hair state of a model */
void EXM_InitMovedHair(EXM_MODEL *pModel)
{
    if (pModel) {
        pModel->nHairMode = 2;
        pModel->fHairDelay = 0.5f;
        pModel->nHairA50 = 0;
        pModel->nHairA54 = 0;
        pModel->nHairA58 = 0;
        pModel->nHairA5C = 0;
    }
}

/* Re-enable moved hair without touching the rest of its state */
void EXM_OnMovedHair(EXM_MODEL *pModel)
{
    if (pModel) {
        pModel->nHairMode = 2;
    }
}

/* Freeze moved hair */
void EXM_OffMovedHair(EXM_MODEL *pModel)
{
    if (pModel) {
        pModel->nHairMode = 0;
    }
}

/* Set how far the hair lags the skeleton */
void EXM_SetDelayMovedHair(EXM_MODEL *pModel, float fDelay)
{
    if (pModel) {
        pModel->fHairDelay = fDelay;
    }
}

/* Clear every field of the current wind block */
void EXM_ResetWind(void)
{
    wind->nType = 0;
    wind->vDir.x = wind->vDir.y = wind->vDir.z = wind->vDir.w = 0.0f;
    wind->fShakePower = 0.0f;
    wind->fShakeStep = 0.0f;
    wind->fShakeRad = 0.0f;
}

/* Copy a caller supplied wind block into the current one */
void EXM_SetWindPara(EXM_WIND *pPara)
{
    if (pPara) {
        *wind = *pPara;
    }
}

/* Copy the current wind block out to the caller */
void EXM_GetWindPara(EXM_WIND *pPara)
{
    if (pPara) {
        *pPara = *wind;
    }
}

/* Install the built in wind block and clear it */
void EXM_InitWind(void)
{
    wind = &_wind;
    EXM_ResetWind();
}

/* Turn the wind off, dropping the shake phase with it */
void EXM_StopWind(void)
{
    wind->nType = 0;
    wind->fShakeRad = 0.0f;
}

/* Set the shake amplitude */
void EXM_SetShakePower(float fPower)
{
    wind->fShakePower = fPower;
}

/* Set the per frame shake step, clamped to one turn either way */
void EXM_SetShakeTime(float fStep)
{
    if (fStep > EXM_2PI) {
        fStep = EXM_2PI;
    } else if (fStep < -EXM_2PI) {
        fStep = -EXM_2PI;
    }
    wind->fShakeStep = fStep;
}

/* Point the wind globals at a caller supplied block */
void EXM_SetWindStruct(EXM_WIND *pWind)
{
    wind = pWind;
}

/* Restore the wind globals to the built in block */
void EXM_ClearWindStruct(void)
{
    wind = &_wind;
}

/* Advance the shake and the ambient wave, wrapping both to one turn */
void EXM_StepShakeWind(void)
{
    if (wind) {
        wind->fShakeRad += wind->fShakeStep;
        if (wind->fShakeRad > EXM_2PI) {
            wind->fShakeRad -= EXM_2PI;
        } else if (wind->fShakeRad < 0.0f) {
            wind->fShakeRad += EXM_2PI;
        }
    }
    waverad += 0.5f;
    if (waverad > EXM_2PI) {
        waverad -= EXM_2PI;
    } else if (waverad < 0.0f) {
        waverad += EXM_2PI;
    }
}

/* Force the shake phase */
void EXM_SetShakeWind(float fRad)
{
    wind->fShakeRad = fRad;
}

/* Clear the wave counter */
void EXM_ResetWave(void)
{
    wave = 0.0f;
}

/* Current ambient wave phase */
float EXM_GetWaveRad(void)
{
    return waverad;
}

/* Current shake phase */
float EXM_GetShakeRad(void)
{
    return wind->fShakeRad;
}
