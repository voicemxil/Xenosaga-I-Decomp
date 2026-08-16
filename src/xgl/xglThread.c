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
/* TODO: near-miss (54/64 words, 10 differing; was 26).
 * Two levers landed here: the active-list store is written BEFORE the
 * sThreadParam one and the chained assignments run
 * `sThreadParam.x = asActiveThreadList[i].x = ...` (26 -> 15), and the
 * gp pointer is spelled `&_gp` at its use instead of being hoisted into
 * a `void *gp` local (15 -> 10) -- as a local it becomes a loop
 * invariant that global_alloc ranks ahead of the sThreadParam giv, so
 * $s6/$s7 come out the other way round.
 * RESIDUE: retail holds sThreadParam+12 in $s7 and reaches +8 with an
 * immediate; gcc holds +8 and reaches +12 with an immediate, which also
 * swaps $a1/$a2 between the stack and stackSize values and moves one
 * store.  Swept with no further gain: all six positions of the gpReg
 * store in the loop body; stackSize before stack (14); each of the two
 * chains flipped independently and together (10/11/11); splitting the
 * chains into separate statements (one form is four words short, the
 * other still 10); declaring gp before next_stack; PIN(gp,"$22"). */
void xglThreadInitial()
{
    static struct ThreadParam sThreadParam;
    void* next_stack = (void*)0x1f8000;
    u32 i;

    for (i = 0; i < 4; i++)
    {
        asActiveThreadList[i].initPriority = asSystemThreadList[i].initPriority;
        sThreadParam.initPriority = asSystemThreadList[i].initPriority;
        sThreadParam.entry = asActiveThreadList[i].entry = asSystemThreadList[i].entry;
        sThreadParam.stack = asActiveThreadList[i].stack =
            (char*)next_stack - asSystemThreadList[i].stackSize;
        sThreadParam.stackSize = asActiveThreadList[i].stackSize =
            asSystemThreadList[i].stackSize;
        sThreadParam.gpReg = &_gp;

        asActiveThreadList[i].id = CreateThread(&sThreadParam);
        StartThread(asActiveThreadList[i].id, 0);
        next_stack = (char*)next_stack - asSystemThreadList[i].stackSize;
    }

    iCurrentThread = 0;
}
