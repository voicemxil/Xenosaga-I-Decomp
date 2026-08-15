/*
 * PS2 SDK kernel syscall stubs (the SDK's hand-written kernel.s table).
 * Each entry loads the syscall number into $v1 and traps; the C form
 * below reproduces the exact four-instruction shape (li/syscall/jr/nop)
 * because a leaf function with no frame emits only the asm plus its
 * return. Generated from the original ELF's syscall table.
 */

void RFU000_FullReset(void)
{
    __asm__ __volatile__("li $3,0\n\tsyscall");
}

void ResetEE(void)
{
    __asm__ __volatile__("li $3,1\n\tsyscall");
}

void SetGsCrt(void)
{
    __asm__ __volatile__("li $3,2\n\tsyscall");
}

void RFU003(void)
{
    __asm__ __volatile__("li $3,3\n\tsyscall");
}

void _Exit(void)
{
    __asm__ __volatile__("li $3,4\n\tsyscall");
}

void RFU005(void)
{
    __asm__ __volatile__("li $3,5\n\tsyscall");
}

void _LoadExecPS2(void)
{
    __asm__ __volatile__("li $3,6\n\tsyscall");
}

void _ExecPS2(void)
{
    __asm__ __volatile__("li $3,7\n\tsyscall");
}

void RFU008(void)
{
    __asm__ __volatile__("li $3,8\n\tsyscall");
}

void RFU009(void)
{
    __asm__ __volatile__("li $3,9\n\tsyscall");
}

void AddSbusIntcHandler(void)
{
    __asm__ __volatile__("li $3,10\n\tsyscall");
}

void RemoveSbusIntcHandler(void)
{
    __asm__ __volatile__("li $3,11\n\tsyscall");
}

void Interrupt2Iop(void)
{
    __asm__ __volatile__("li $3,12\n\tsyscall");
}

void SetVTLBRefillHandler(void)
{
    __asm__ __volatile__("li $3,13\n\tsyscall");
}

void SetVCommonHandler(void)
{
    __asm__ __volatile__("li $3,14\n\tsyscall");
}

void SetVInterruptHandler(void)
{
    __asm__ __volatile__("li $3,15\n\tsyscall");
}

void AddIntcHandler(void)
{
    __asm__ __volatile__("li $3,16\n\tsyscall");
}

void AddIntcHandler2(void)
{
    __asm__ __volatile__("li $3,16\n\tsyscall");
}

void RemoveIntcHandler(void)
{
    __asm__ __volatile__("li $3,17\n\tsyscall");
}

void AddDmacHandler(void)
{
    __asm__ __volatile__("li $3,18\n\tsyscall");
}

void AddDmacHandler2(void)
{
    __asm__ __volatile__("li $3,18\n\tsyscall");
}

void RemoveDmacHandler(void)
{
    __asm__ __volatile__("li $3,19\n\tsyscall");
}

void _EnableIntc(void)
{
    __asm__ __volatile__("li $3,20\n\tsyscall");
}

void _DisableIntc(void)
{
    __asm__ __volatile__("li $3,21\n\tsyscall");
}

void _EnableDmac(void)
{
    __asm__ __volatile__("li $3,22\n\tsyscall");
}

void _DisableDmac(void)
{
    __asm__ __volatile__("li $3,23\n\tsyscall");
}

void SetAlarm(void)
{
    __asm__ __volatile__("li $3,252\n\tsyscall");
}

void ReleaseAlarm(void)
{
    __asm__ __volatile__("li $3,253\n\tsyscall");
}

void _iEnableIntc(void)
{
    __asm__ __volatile__("li $3,-26\n\tsyscall");
}

void _iDisableIntc(void)
{
    __asm__ __volatile__("li $3,-27\n\tsyscall");
}

void _iEnableDmac(void)
{
    __asm__ __volatile__("li $3,-28\n\tsyscall");
}

void _iDisableDmac(void)
{
    __asm__ __volatile__("li $3,-29\n\tsyscall");
}

void iSetAlarm(void)
{
    __asm__ __volatile__("li $3,-254\n\tsyscall");
}

void iReleaseAlarm(void)
{
    __asm__ __volatile__("li $3,-255\n\tsyscall");
}

void CreateThread(void)
{
    __asm__ __volatile__("li $3,32\n\tsyscall");
}

void DeleteThread(void)
{
    __asm__ __volatile__("li $3,33\n\tsyscall");
}

void StartThread(void)
{
    __asm__ __volatile__("li $3,34\n\tsyscall");
}

void ExitThread(void)
{
    __asm__ __volatile__("li $3,35\n\tsyscall");
}

void ExitDeleteThread(void)
{
    __asm__ __volatile__("li $3,36\n\tsyscall");
}

void TerminateThread(void)
{
    __asm__ __volatile__("li $3,37\n\tsyscall");
}

void iTerminateThread(void)
{
    __asm__ __volatile__("li $3,-38\n\tsyscall");
}

void DisableDispatchThread(void)
{
    __asm__ __volatile__("li $3,39\n\tsyscall");
}

void EnableDispatchThread(void)
{
    __asm__ __volatile__("li $3,40\n\tsyscall");
}

void ChangeThreadPriority(void)
{
    __asm__ __volatile__("li $3,41\n\tsyscall");
}

void iChangeThreadPriority(void)
{
    __asm__ __volatile__("li $3,-42\n\tsyscall");
}

void RotateThreadReadyQueue(void)
{
    __asm__ __volatile__("li $3,43\n\tsyscall");
}

void _iRotateThreadReadyQueue(void)
{
    __asm__ __volatile__("li $3,-44\n\tsyscall");
}

void ReleaseWaitThread(void)
{
    __asm__ __volatile__("li $3,45\n\tsyscall");
}

void iReleaseWaitThread(void)
{
    __asm__ __volatile__("li $3,-46\n\tsyscall");
}

void GetThreadId(void)
{
    __asm__ __volatile__("li $3,47\n\tsyscall");
}

void ReferThreadStatus(void)
{
    __asm__ __volatile__("li $3,48\n\tsyscall");
}

void iReferThreadStatus(void)
{
    __asm__ __volatile__("li $3,-49\n\tsyscall");
}

void SleepThread(void)
{
    __asm__ __volatile__("li $3,50\n\tsyscall");
}

void WakeupThread(void)
{
    __asm__ __volatile__("li $3,51\n\tsyscall");
}

void _iWakeupThread(void)
{
    __asm__ __volatile__("li $3,-52\n\tsyscall");
}

void CancelWakeupThread(void)
{
    __asm__ __volatile__("li $3,53\n\tsyscall");
}

void iCancelWakeupThread(void)
{
    __asm__ __volatile__("li $3,-54\n\tsyscall");
}

void SuspendThread(void)
{
    __asm__ __volatile__("li $3,55\n\tsyscall");
}

void _iSuspendThread(void)
{
    __asm__ __volatile__("li $3,-56\n\tsyscall");
}

void ResumeThread(void)
{
    __asm__ __volatile__("li $3,57\n\tsyscall");
}

void iResumeThread(void)
{
    __asm__ __volatile__("li $3,-58\n\tsyscall");
}

void JoinThread(void)
{
    __asm__ __volatile__("li $3,59\n\tsyscall");
}

void RFU060(void)
{
    __asm__ __volatile__("li $3,60\n\tsyscall");
}

void RFU061(void)
{
    __asm__ __volatile__("li $3,61\n\tsyscall");
}

void EndOfHeap(void)
{
    __asm__ __volatile__("li $3,62\n\tsyscall");
}

void RFU063(void)
{
    __asm__ __volatile__("li $3,63\n\tsyscall");
}

void CreateSema(void)
{
    __asm__ __volatile__("li $3,64\n\tsyscall");
}

void DeleteSema(void)
{
    __asm__ __volatile__("li $3,65\n\tsyscall");
}

void SignalSema(void)
{
    __asm__ __volatile__("li $3,66\n\tsyscall");
}

void iSignalSema(void)
{
    __asm__ __volatile__("li $3,-67\n\tsyscall");
}

void WaitSema(void)
{
    __asm__ __volatile__("li $3,68\n\tsyscall");
}

void PollSema(void)
{
    __asm__ __volatile__("li $3,69\n\tsyscall");
}

void iPollSema(void)
{
    __asm__ __volatile__("li $3,-70\n\tsyscall");
}

void ReferSemaStatus(void)
{
    __asm__ __volatile__("li $3,71\n\tsyscall");
}

void iReferSemaStatus(void)
{
    __asm__ __volatile__("li $3,-72\n\tsyscall");
}

void RFU073(void)
{
    __asm__ __volatile__("li $3,73\n\tsyscall");
}

void SetOsdConfigParam(void)
{
    __asm__ __volatile__("li $3,74\n\tsyscall");
}

void GetOsdConfigParam(void)
{
    __asm__ __volatile__("li $3,75\n\tsyscall");
}

void GetGsHParam(void)
{
    __asm__ __volatile__("li $3,76\n\tsyscall");
}

void GetGsVParam(void)
{
    __asm__ __volatile__("li $3,77\n\tsyscall");
}

void SetGsHParam(void)
{
    __asm__ __volatile__("li $3,78\n\tsyscall");
}

void SetGsVParam(void)
{
    __asm__ __volatile__("li $3,79\n\tsyscall");
}

void RFU080_CreateEventFlag(void)
{
    __asm__ __volatile__("li $3,80\n\tsyscall");
}

void RFU081_DeleteEventFlag(void)
{
    __asm__ __volatile__("li $3,81\n\tsyscall");
}

void RFU082_SetEventFlag(void)
{
    __asm__ __volatile__("li $3,82\n\tsyscall");
}

void RFU083_iSetEventFlag(void)
{
    __asm__ __volatile__("li $3,-83\n\tsyscall");
}

void RFU084_ClearEventFlag(void)
{
    __asm__ __volatile__("li $3,84\n\tsyscall");
}

void RFU085_iClearEventFlag(void)
{
    __asm__ __volatile__("li $3,-85\n\tsyscall");
}

void RFU086_WaitEvnetFlag(void)
{
    __asm__ __volatile__("li $3,86\n\tsyscall");
}

void RFU087_PollEvnetFlag(void)
{
    __asm__ __volatile__("li $3,87\n\tsyscall");
}

void RFU088_iPollEvnetFlag(void)
{
    __asm__ __volatile__("li $3,-88\n\tsyscall");
}

void RFU089_ReferEventFlagStatus(void)
{
    __asm__ __volatile__("li $3,89\n\tsyscall");
}

void RFU090_iReferEventFlagStatus(void)
{
    __asm__ __volatile__("li $3,-90\n\tsyscall");
}

void RFU091(void)
{
    __asm__ __volatile__("li $3,91\n\tsyscall");
}

void EnableIntcHandler(void)
{
    __asm__ __volatile__("li $3,92\n\tsyscall");
}

void iEnableIntcHandler(void)
{
    __asm__ __volatile__("li $3,-92\n\tsyscall");
}

void DisableIntcHandler(void)
{
    __asm__ __volatile__("li $3,93\n\tsyscall");
}

void iDisableIntcHandler(void)
{
    __asm__ __volatile__("li $3,-93\n\tsyscall");
}

void EnableDmacHandler(void)
{
    __asm__ __volatile__("li $3,94\n\tsyscall");
}

void iEnableDmacHandler(void)
{
    __asm__ __volatile__("li $3,-94\n\tsyscall");
}

void DisableDmacHandler(void)
{
    __asm__ __volatile__("li $3,95\n\tsyscall");
}

void iDisableDmacHandler(void)
{
    __asm__ __volatile__("li $3,-95\n\tsyscall");
}

void KSeg0(void)
{
    __asm__ __volatile__("li $3,96\n\tsyscall");
}

void EnableCache(void)
{
    __asm__ __volatile__("li $3,97\n\tsyscall");
}

void DisableCache(void)
{
    __asm__ __volatile__("li $3,98\n\tsyscall");
}

void GetCop0(void)
{
    __asm__ __volatile__("li $3,99\n\tsyscall");
}

void FlushCache(void)
{
    __asm__ __volatile__("li $3,100\n\tsyscall");
}

void CpuConfig(void)
{
    __asm__ __volatile__("li $3,102\n\tsyscall");
}

void iGetCop0(void)
{
    __asm__ __volatile__("li $3,-103\n\tsyscall");
}

void iFlushCache(void)
{
    __asm__ __volatile__("li $3,-104\n\tsyscall");
}

void iCpuConfig(void)
{
    __asm__ __volatile__("li $3,-106\n\tsyscall");
}

void sceSifStopDma(void)
{
    __asm__ __volatile__("li $3,107\n\tsyscall");
}

void SetCPUTimerHandler(void)
{
    __asm__ __volatile__("li $3,108\n\tsyscall");
}

void SetCPUTimer(void)
{
    __asm__ __volatile__("li $3,109\n\tsyscall");
}

void SetOsdConfigParam2(void)
{
    __asm__ __volatile__("li $3,110\n\tsyscall");
}

void GetOsdConfigParam2(void)
{
    __asm__ __volatile__("li $3,111\n\tsyscall");
}

void GsGetIMR(void)
{
    __asm__ __volatile__("li $3,112\n\tsyscall");
}

void iGsGetIMR(void)
{
    __asm__ __volatile__("li $3,-112\n\tsyscall");
}

void GsPutIMR(void)
{
    __asm__ __volatile__("li $3,113\n\tsyscall");
}

void iGsPutIMR(void)
{
    __asm__ __volatile__("li $3,-113\n\tsyscall");
}

void SetPgifHandler(void)
{
    __asm__ __volatile__("li $3,114\n\tsyscall");
}

void SetVSyncFlag(void)
{
    __asm__ __volatile__("li $3,115\n\tsyscall");
}

void RFU116(void)
{
    __asm__ __volatile__("li $3,116\n\tsyscall");
}

void _print(void)
{
    __asm__ __volatile__("li $3,117\n\tsyscall");
}

void sceSifDmaStat(void)
{
    __asm__ __volatile__("li $3,118\n\tsyscall");
}

void isceSifDmaStat(void)
{
    __asm__ __volatile__("li $3,-118\n\tsyscall");
}

void sceSifSetDma(void)
{
    __asm__ __volatile__("li $3,119\n\tsyscall");
}

void isceSifSetDma(void)
{
    __asm__ __volatile__("li $3,-119\n\tsyscall");
}

void sceSifSetDChain(void)
{
    __asm__ __volatile__("li $3,120\n\tsyscall");
}

void isceSifSetDChain(void)
{
    __asm__ __volatile__("li $3,-120\n\tsyscall");
}

void sceSifSetReg(void)
{
    __asm__ __volatile__("li $3,121\n\tsyscall");
}

void sceSifGetReg(void)
{
    __asm__ __volatile__("li $3,122\n\tsyscall");
}

void _ExecOSD(void)
{
    __asm__ __volatile__("li $3,123\n\tsyscall");
}

void Deci2Call(void)
{
    __asm__ __volatile__("li $3,124\n\tsyscall");
}

void PSMode(void)
{
    __asm__ __volatile__("li $3,125\n\tsyscall");
}

void MachineType(void)
{
    __asm__ __volatile__("li $3,126\n\tsyscall");
}

void GetMemorySize(void)
{
    __asm__ __volatile__("li $3,127\n\tsyscall");
}

void _InitTLB(void)
{
    __asm__ __volatile__("li $3,130\n\tsyscall");
}

void setup(void)
{
    __asm__ __volatile__("li $3,116\n\tsyscall");
}

void Copy(void)
{
    __asm__ __volatile__("li $3,90\n\tsyscall");
}

void GetEntryAddress(void)
{
    __asm__ __volatile__("li $3,91\n\tsyscall");
}

void PutTLBEntry(void)
{
    __asm__ __volatile__("li $3,85\n\tsyscall");
}

void iPutTLBEntry(void)
{
    __asm__ __volatile__("li $3,-85\n\tsyscall");
}

void _SetTLBEntry(void)
{
    __asm__ __volatile__("li $3,86\n\tsyscall");
}

void iSetTLBEntry(void)
{
    __asm__ __volatile__("li $3,-86\n\tsyscall");
}

void GetTLBEntry(void)
{
    __asm__ __volatile__("li $3,87\n\tsyscall");
}

void iGetTLBEntry(void)
{
    __asm__ __volatile__("li $3,-87\n\tsyscall");
}

void ProbeTLBEntry(void)
{
    __asm__ __volatile__("li $3,88\n\tsyscall");
}

void iProbeTLBEntry(void)
{
    __asm__ __volatile__("li $3,-88\n\tsyscall");
}

void ExpandScratchPad(void)
{
    __asm__ __volatile__("li $3,89\n\tsyscall");
}

void FindAddress(void)
{
    __asm__ __volatile__("li $3,131\n\tsyscall");
}

