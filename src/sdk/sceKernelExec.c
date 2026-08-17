/* Sony kernel ExecPS2 patch installer.
 *
 * This is a separate original translation unit from the syscall stubs in
 * kernel.c.  It uses the SDK compiler with scheduling enabled; compiling it
 * with kernel.c's -fno-schedule-insns swaps the Copy call's final argument
 * setup even when the C is otherwise identical. */

extern int PatchIsNeeded(void);
extern void setup(int a, int b);
extern void Copy(void *pDst, const void *pSrc, int nSize);
extern void FlushCache(int nMode);
extern int GetEntryAddress(int nEntry);
extern int SysExecPS2Entry[];
extern char osdsrc[];

void InitExecPS2(void)
{
    int *pTable;
    volatile int *pSetupTable;
    int *pEntry;
    int nSetup0;
    int nSetup1;
    unsigned int i;

    if (PatchIsNeeded() != 0) {
        i = 2;
        pTable = SysExecPS2Entry;
        setup(pTable[0], pTable[1]);
        pEntry = pTable + 4;
        Copy((void *)0x80074000, osdsrc, 0x7A8);
        FlushCache(0);
        FlushCache(2);

        /* These are kernel patch-table reads.  Keeping their read order
         * observable produces the SDK's original a0/a1 call setup. */
        pSetupTable = pTable;
        nSetup0 = pSetupTable[2];
        nSetup1 = pSetupTable[3];
        setup(nSetup0, nSetup1);

        do {
            i++;
            setup(*pEntry, GetEntryAddress(*pEntry));
            pEntry += 2;
        } while (i < 3);
    }
}
