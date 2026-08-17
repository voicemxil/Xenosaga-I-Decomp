/* Sony kernel TLB patch-function installer. */

extern void setup(int a, int b);
extern void Copy(void *pDst, const void *pSrc, int nSize);
extern void FlushCache(int nMode);
extern int GetEntryAddress(int nEntry);
extern int SysEntry_004AB660[];
extern char tlbsrc[];

void InitTLBFunctions(void)
{
    int *pTable;
    volatile int *pSetupTable;
    int *pEntry;
    int nSetup0;
    int nSetup1;
    unsigned int i;

    i = 3;
    pTable = SysEntry_004AB660;
    pEntry = pTable + 6;
    setup(pTable[0], pTable[1]);
    Copy((void *)0x80075000, tlbsrc, 0x328);
    FlushCache(0);
    FlushCache(2);
    setup(pTable[2], pTable[3]);

    /* Preserve the patch-table read order used by the SDK compiler. */
    pSetupTable = pTable;
    nSetup0 = pSetupTable[4];
    nSetup1 = pSetupTable[5];
    setup(nSetup0, nSetup1);

    do {
        i++;
        setup(*pEntry, GetEntryAddress(*pEntry));
        pEntry += 2;
    } while (i < 8);
}
