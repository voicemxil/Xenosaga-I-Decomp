/* Script engine global state accessors */

int s_nScriptTalkLock;
unsigned char s_nScriptChangeTime;
int s_nScriptEventActive;
int s_nScriptEventFin;
int s_nScriptSequenceReset;
int s_nScriptFadeRequest;
int s_nScriptFadeOutTime;
int s_nScriptCfTime;
int s_nScriptFrameLockEntry;
int stageVM;
int defaultVM;
int evtVM[2];
int UseVMFlag;
int currentScriptDB;
int classJava_xeno_Chr;
int classJava_xeno_Stage;

extern unsigned short D_0033868C[];

typedef struct ScriptObj {
    int unk0[4];
    int field_10;
} ScriptObj;

typedef struct {
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    ScriptObj *field_10;
    int field_14;
    int field_18;
} SCRIPTDB;

extern SCRIPTDB scriptDB[];

typedef struct {
    int unk0[0x130];
    int field_4C0;
    int unk4C4[(0x9F4 - 0x4C4) / 4];
    int field_9F4;
    int field_9F8;
} CHR_WORK;

int JNI_initThread(int);
int JNI_callMethod(int, int, int *, int);
int JNI_createThread(int, int, int);
int JNI_pushFrame(void);
int JNI_loadNativeClass(void);
int JNI_isInstanceOf(int, int);
int initVM(void);
int loadScriptCD(SCRIPTDB *, int);
int loadScriptCD2(SCRIPTDB *, int);
void createTalkTask(CHR_WORK *, int);

/* Call a script method on the stage VM with a single argument */
void SCRIPT_test(int arg0, int method)
{
    int args[4];

    JNI_initThread(stageVM);
    if (method != 0) {
        args[0] = arg0;
        JNI_callMethod(stageVM, method, args, 0);
    }
}

/* Block talk interactions while a script runs */
void SCRIPT_talkIgnoreSet(void)
{
    s_nScriptTalkLock = 1;
}

/* Re-enable talk interactions */
void SCRIPT_talkIgnoreClr(void)
{
    s_nScriptTalkLock = 0;
}

/* Return whether talk interactions are blocked */
int SCRIPT_talkIgnoreGet(void)
{
    return s_nScriptTalkLock;
}

/* Reset the scene-change timer */
void SCRIPT_sceneChangeTimeInit(void)
{
    s_nScriptChangeTime = 0;
}

/* Return the scene-change timer */
int SCRIPT_sceneChangeTimeGet(void)
{
    return s_nScriptChangeTime;
}

/* Set the scene-change timer */
void SCRIPT_sceneChangeTimeSet(int time)
{
    s_nScriptChangeTime = time;
}

/* Tick the scene-change timer down, clamping at zero */
void SCRIPT_sceneChangeTimeDec(void)
{
    if (s_nScriptChangeTime != 0) {
        s_nScriptChangeTime--;
    } else {
        s_nScriptChangeTime = 0;
    }
}

/* Finish the current event if a skippable movie is playing */
void SCRIPT_sendMovieSkipSignal(void)
{
    if (D_0033868C[0] == 3) {
        s_nScriptEventFin = 1;
    }
}

/* Clear the event-active flag */
void SCRIPT_clrEventActiveFlag(void)
{
    s_nScriptEventActive = 0;
}

/* Mark a script event as active */
void SCRIPT_setEventActiveFlag(void)
{
    s_nScriptEventActive = 1;
}

/* Return whether a script event is active */
int SCRIPT_getEventActiveFlag(void)
{
    return s_nScriptEventActive;
}

/* Signal that the current event has finished */
void SCRIPT_eventFinish(void)
{
    s_nScriptEventFin = 1;
}

/* Request a script sequence reset */
void SCRIPT_methodClearSet(void)
{
    s_nScriptSequenceReset = 1;
}

/* Return whether a sequence reset was requested */
int SCRIPT_methodClearGet(void)
{
    return s_nScriptSequenceReset;
}

/* Request a fade with the given duration, clamped to >= 0 */
void SCRIPT_fade(int time)
{
    if (time < 0) {
        time = 0;
    }
    s_nScriptFadeRequest = 1;
    s_nScriptFadeOutTime = time;
}

/* Return the fade-out duration */
int SCRIPT_getFadeTime(void)
{
    return s_nScriptFadeOutTime;
}

/* Return the fade duration if a fade is pending, else 0 */
int SCRIPT_fadeGet(void)
{
    if (s_nScriptFadeRequest == 0) {
        return 0;
    }
    return s_nScriptFadeOutTime;
}

/* Return the cf timer */
int SCRIPT_getCfTime(void)
{
    return s_nScriptCfTime;
}

/* Increment the cf timer */
void SCRIPT_incCfTime(void)
{
    s_nScriptCfTime++;
}

/* Request a frame lock on battle entry */
void SCRIPT_frameLock2Battle(void)
{
    s_nScriptFrameLockEntry = 1;
}

/* Reinitialize the script VMs and reload the native class table */
int SCRIPT_reset(void)
{
    int i;

    initVM();
    defaultVM = JNI_createThread(0, 8, 0x80);
    stageVM = JNI_createThread(0, 8, 0x46);
    for (i = 0; i < 2; i++) {
        evtVM[i] = JNI_createThread(0, 8, 0x40);
    }
    UseVMFlag = 0;
    JNI_pushFrame();
    JNI_pushFrame();
    return JNI_loadNativeClass();
}

/* Start the talk-to script task for a character if the stage allows it */
void SCRIPT_execTalkto(CHR_WORK *chr)
{
    SCRIPTDB *db = &scriptDB[currentScriptDB];

    if (chr->field_9F4 != 0) {
        if (db->field_C != 0) {
            if (s_nScriptTalkLock != 0) {
                return;
            }
            if (JNI_isInstanceOf(chr->field_4C0, classJava_xeno_Chr) == 0) {
                return;
            }
            if (JNI_isInstanceOf(db->field_10->field_10, classJava_xeno_Stage) != 0) {
                createTalkTask(chr, chr->field_9F4);
            }
        }
    }
}

/* Start the touch-to script task for a character if the stage allows it */
void SCRIPT_execTouchto(CHR_WORK *chr)
{
    SCRIPTDB *db = &scriptDB[currentScriptDB];

    if (chr->field_9F4 != 0) {
        if (db->field_C != 0) {
            if (s_nScriptTalkLock != 0) {
                return;
            }
            if (JNI_isInstanceOf(chr->field_4C0, classJava_xeno_Chr) == 0) {
                return;
            }
            if (JNI_isInstanceOf(db->field_10->field_10, classJava_xeno_Stage) != 0) {
                createTalkTask(chr, chr->field_9F8);
            }
        }
    }
}

/* Load the next script database entry from CD */
int SCRIPT_load(int arg)
{
    return loadScriptCD(&scriptDB[(currentScriptDB + 1) & 1], arg);
}

/* Load the next script database entry from CD (variant 2) */
int SCRIPT_load2(int arg)
{
    return loadScriptCD2(&scriptDB[(currentScriptDB + 1) & 1], arg);
}
