/*
 * xgl thread scheduler, ported from the SquareMan decomp
 * (src/xgl/thread.c). Data objects are extern.
 */

#include "common.h"
#include "sdk/eekernel.h"

extern u32 iRotateSignal;
extern u32 iCurrentThread;

typedef struct
{
    void* entry;
    void* stack;
    s32 stackSize;
    s32 initPriority;
    s32 id;
} UnkThreadEntry;

extern UnkThreadEntry asActiveThreadList[20];
extern UnkThreadEntry asSystemThreadList[4];

// Matches, Needs data migration
s32 xglCreateSignal()
{
    static struct SemaParam sSemaParam;
    sSemaParam.maxCount = 1;
    sSemaParam.initCount = 0;
    return CreateSema(&sSemaParam);
}

void xglThreadRotate()
{
    iRotateSignal = xglCreateSignal();

    while (TRUE)
    {
        WakeupThread(asActiveThreadList[iCurrentThread].id);
        WaitSema(iRotateSignal);
        iCurrentThread++;
        if (iCurrentThread >= 4) iCurrentThread = 0;
    }
}

void xglSleep()
{
    CancelWakeupThread(GetThreadId());
    SignalSema(iRotateSignal);
    SleepThread();
}

/* Bring up the four engine threads: copy each entry out of the static
 * system table into the active table and into the ThreadParam block,
 * carving the stacks downward from 0x1f8000. */
void xglThreadInitial()
{
    static struct ThreadParam sThreadParam;
    void* next_stack = (void*)0x1f8000;
    void* gp = &_gp;
    u32 i;

    for (i = 0; i < 4; i++)
    {
        sThreadParam.initPriority = asSystemThreadList[i].initPriority;
        asActiveThreadList[i].initPriority = asSystemThreadList[i].initPriority;
        asActiveThreadList[i].entry = sThreadParam.entry = asSystemThreadList[i].entry;
        asActiveThreadList[i].stack = sThreadParam.stack =
            (char*)next_stack - asSystemThreadList[i].stackSize;
        asActiveThreadList[i].stackSize = sThreadParam.stackSize =
            asSystemThreadList[i].stackSize;
        sThreadParam.gpReg = gp;

        asActiveThreadList[i].id = CreateThread(&sThreadParam);
        StartThread(asActiveThreadList[i].id, 0);
        next_stack = (char*)next_stack - asSystemThreadList[i].stackSize;
    }

    iCurrentThread = 0;
}
