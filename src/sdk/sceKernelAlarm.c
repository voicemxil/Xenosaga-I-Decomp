/* Sony kernel alarm patch-function installer. */

extern void setup(int a, int b);
extern void Copy(void *pDst, const void *pSrc, int nSize);
extern void FlushCache(int nMode);
extern int GetEntryAddress(int nEntry);
extern int SysEntry_004AB2A0[];
extern char srcfile[];
extern char eenull[];

void InitAlarm(void)
{
    int *pTable;
    volatile int *pSetupTable;
    int *pEntry;
    int nSetup0;
    int nSetup1;
    unsigned int i;

    if ((*(volatile unsigned int *)0x10001810 & 0x100) == 0) {
        i = 2;
        pTable = SysEntry_004AB2A0;
        setup(pTable[0], pTable[1]);
        pEntry = pTable + 4;
        Copy((void *)0x80076000, srcfile, 0x740);
        Copy((void *)0x82000, eenull, 0x28);
        FlushCache(0);
        FlushCache(2);

        /* Preserve the patch-table read order used by the SDK compiler. */
        pSetupTable = pTable;
        nSetup0 = pSetupTable[2];
        nSetup1 = pSetupTable[3];
        setup(nSetup0, nSetup1);

        do {
            i++;
            setup(*pEntry, GetEntryAddress(*pEntry));
            pEntry += 2;
        } while (i < 8);
    }
}
