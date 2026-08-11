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
