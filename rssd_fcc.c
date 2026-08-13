int RssdGetCallCompletedCode(void);

void RssdFuncCallCompleted_A(int bWait)
{
    int nCode;
    for (;;) {
        nCode = RssdGetCallCompletedCode();
        if (bWait == 0) break;
        if (nCode == 0) break;
    }
}

void RssdFuncCallCompleted_B(int bWait)
{
    int nCode;
    do {
        nCode = RssdGetCallCompletedCode();
    } while (bWait && nCode);
}

void RssdFuncCallCompleted_C(int bWait)
{
    int nCode;
    do {
        nCode = RssdGetCallCompletedCode();
        if (bWait == 0) return;
    } while (nCode != 0);
}

void RssdFuncCallCompleted_D(int bWait)
{
    int nCode;
    int nDone;
    do {
        nCode = RssdGetCallCompletedCode();
        nDone = nCode;
    } while (bWait != 0 && nDone != 0);
}

void RssdFuncCallCompleted_E(int bWait)
{
    int nCode;

    do {
        nCode = RssdGetCallCompletedCode();
        if (bWait == 0) {
            break;
        }
    } while (nCode != 0);
}
