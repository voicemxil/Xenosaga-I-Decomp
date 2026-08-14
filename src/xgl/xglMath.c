/* Core geometry/math helpers */

extern long long iRandSeed;

void xglRandSeedInit(void)
{
    iRandSeed = 0x12345678;
}

float I2F(int nValue)
{
    return (float)nValue;
}

extern float sqrtf(float);

/* TODO: near-miss (16/34 words, REGISTER/OPERANDS class per triage.py) -
 * plain sqrtf(1.0f-x) already reproduces the hardware sqrt.s + NaN-check
 * + software-sqrtf-fallback expansion (this compiler's own sqrtf()
 * lowering does that automatically -- no manual NaN check needed in the
 * source). Remaining diffs are pure float-register allocation: original
 * keeps the polynomial accumulator in $f0 (freed by the sqrt call) and
 * the sqrt result in $f5; every natural statement order tried keeps the
 * same registers just shuffled ($f1/$f2/$f6). Logic and constants are
 * fully verified against asm/data/cod's .lit4 pool. */
/* asin(x) via sqrt(1-x) times a cubic minimax poly; the hardware sqrt.s
 * result is checked for NaN and falls back to the software sqrtf() */
float xglAsin(float x)
{
    float s, r;

    s = sqrtf(1.0f - x);
    r = x * -0.01872929931f + 0.07426100224f;
    r = r * x - 0.2121143937f;
    r = r * x + 1.570728779f;
    r = s * r;
    r = 1.570796371f - r;
    return r;
}

/* TODO: near-miss (77/107 words differ, +1 word length). Structure is
 * fully correct (signed-zero y==0 case via raw bit test, octant
 * classification, reciprocal-range reduction, degree-8 minimax poly all
 * verified against the asm) -- remaining diffs are $f12 vs $f2 register
 * allocation for the abs'd y/x and a bc1fl/bc1tl branch-likely polarity
 * swap in the octant test. Every if/else polarity and abs-placement
 * variant tried keeps the same two symptoms; likely allocator tie-break
 * territory (LENGTH/REGISTER-adjacent), not a logic error. */
/* atan2 via minimax polynomial approximation of atan(t) on [0,1],
 * with octant classification + argument reduction */
float xglAtan2(float y, float x)
{
    float t, t2, r;
    int octant;
    int flip;

    r = 0.002866225783f;
    flip = 0;
    octant = 0;

    if (y == 0.0f) {
        if (x >= 0.0f) {
            return 0.0f;
        }
        if (*(int *)&y < 0) {
            return -3.141592741f;
        }
        return 3.141592741f;
    }
    if (x == 0.0f) {
        if (y < 0.0f) {
            return -1.570796371f;
        }
        return 1.570796371f;
    }

    if (y >= 0.0f) {
        if (x >= 0.0f) {
            octant = 0;
        } else {
            octant = 1;
        }
    } else {
        if (x >= 0.0f) {
            octant = 2;
        } else {
            octant = 3;
        }
    }
    y = __builtin_fabsf(y);
    x = __builtin_fabsf(x);

    if (x < y) {
        t = x / y;
        flip = 1;
    } else {
        t = y / x;
    }

    t2 = t * t;
    r = r * t2 - 0.01616573706f;
    r = r * t2 + 0.04290961474f;
    r = r * t2 - 0.07528963685f;
    r = r * t2 + 0.1065626368f;
    r = r * t2 - 0.1420889944f;
    r = r * t2 + 0.1999355108f;
    r = r * t2 - 0.3333314657f;
    r = r * t2 + 1.0f;
    r = r * t;
    if (flip != 0) {
        r = 1.570796371f - r;
    }

    if (octant == 1) {
        r = 3.141592741f - r;
    } else if (octant == 2) {
        r = -r;
    } else if (octant == 3) {
        r = r - 3.141592741f;
    }
    return r;
}
