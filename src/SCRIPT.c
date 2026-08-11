/* Script engine global state accessors */

int s_nScriptTalkLock;
unsigned char s_nScriptChangeTime;
int s_nScriptEventActive;
int s_nScriptEventFin;
int s_nScriptSequenceReset;
int s_nScriptFadeRequest;
int s_nScriptFadeOutTime;
int s_nScriptCfTime;

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
