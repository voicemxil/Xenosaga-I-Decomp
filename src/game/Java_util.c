/* Native (JNI-style) method bindings for the xeno.util script classes */

typedef union JValue {
    int i;
    unsigned int u;
    float f;
    short c;
    unsigned short h;
    unsigned char z;
    void *p;
} JValue;

typedef struct JArray {
    int field_0;
    unsigned int nLength;
    char *pData;
} JArray;

typedef struct JString {
    int field_0;
    JArray *pArray;
} JString;

typedef struct JClass {
    int unk0[6];
    int field_18;
} JClass;

typedef struct JField {
    int unk0[4];
    int nOffset;
} JField;

typedef struct JThread {
    char pad00[0x24];
    int nFlags;                     /* 0x24 */
    char pad28[0x14];
    short field_3C;                 /* 0x3C */
    unsigned short field_3E;        /* 0x3E */
} JThread;

typedef struct Spline {
    char pad00[6];
    short field_6;                  /* 0x06 */
    short field_8;                  /* 0x08 */
} Spline;

typedef struct Vector4fObj {
    int field_0;
    float x;
    float y;
    float z;
    float w;
} Vector4fObj;

typedef struct PadWork {
    unsigned short nButton;         /* 0x00 */
    unsigned short nEdge;           /* 0x02 */
    unsigned short nRepeat;         /* 0x04 */
    char pad06[0x62];
} PadWork;

typedef struct TWin {
    char pad000[0x10];
    int nFlags;                     /* 0x010 */
    char pad014[0xC];
    float fX;                       /* 0x020 */
    float fY;                       /* 0x024 */
    char pad028[0xA];
    short nSignal;                  /* 0x032 */
    char pad034[0x20];
    signed char nCursor;            /* 0x054 */
    signed char n055;               /* 0x055 */
    char pad056[0x12];
    int n068;                       /* 0x068 */
    char pad06C[0x11A];
    unsigned char nWidth;           /* 0x186 */
    unsigned char nHeight;          /* 0x187 */
} TWin;

typedef struct IdLight {
    char pad00[0xF0];
} IdLight;

typedef struct GameLoop {
    int field_00;                   /* 0x00 */
    long long *pRetPoint;           /* 0x04 */
    char pad08[8];
    int nFlags;                     /* 0x10 */
    char pad14[4];
    int field_18;                   /* 0x18 */
    char pad1C[4];
    int nShootFlags;                /* 0x20 */
    char pad24[0x7C];
    float fA0;                      /* 0xA0 */
    float fA4;                      /* 0xA4 */
    float fA8;                      /* 0xA8 */
    float fAC;                      /* 0xAC */
    char padB0[0x11];
    unsigned char nC1;              /* 0xC1 */
    char padC2[0x29F56];
    int field_2A018;                /* 0x2A018 */
    char pad2A01C[4];
    long long field_2A020;          /* 0x2A020 */
    long long field_2A028;          /* 0x2A028 */
} GameLoop;

JClass *classJava_xeno_util_Spline;
JClass *classJava_xeno_util_Vector4f;
JClass *classJava_xeno_util_Input;
JClass *classString;
JClass *classByte;
JString *JAVA_tmpString;
JClass *classJava_xeno_util_Layout;
JClass *classJava_xeno_util_Window;
JClass *classJava_xeno_util_Menu;
JClass *classJava_xeno_Effect;
JClass *classJava_xeno_Enepc;
JClass *classJava_xeno_Uwamono;
JClass *classJava_xeno_Stage;
int defaultLayout;
int D_004DC6B4;
int s_nMpeg2ReserveCrossFade;
extern JClass *classJava_xeno_Chr;
extern JClass *classJava_xeno_Unit;
void (*jthreadResetFunc)(void);

extern char D_004DC098[];
extern char D_004DC0A0[];
extern char D_004DC0A8[];
extern char D_004DC110[];
extern char D_004D16B0[];
extern PadWork D_00490DB8[];
extern GameLoop GameLoopState;
extern int VMRegister[32];
extern int D_00338688[];
extern int D_00338690[];
extern short D_003386D2[];
short peerGroup[4];
extern float D_003625C4[];
extern int D_00367544[][17];
extern unsigned short D_004A1854[];
extern int args_jump[2];
extern IdLight GameIdLight[];
extern int D_00362698[];
extern char D_004D1658[];
extern char D_004D1668[];
extern char D_004D1678[];
extern char D_004D1688[];
extern char D_004D1698[];
extern void scriptReset_jump(void);
extern void scriptReset_evsExit(void);

void *xmalloc(int, int);
void SPL_init2(Spline *, int, void *, int, int);
void SPL_getValueXYZ(float *, void *, float);
void *newObject(JClass *);
JArray *newArray(JClass *, int);
int sprintf(char *, const char *, ...);
int strlen(const char *);
int fptodp(float);
char *STRING_int(char *, int);
void *loadConstString(char *, int);
JField *lookupClassField(JClass *, void *, int);
int xglFlagsGet(int, int);
void xglFlagsSet(int, int, int);
void LoadMapOnly(void);
void MapChange2(int);
int MapGetNo(void);
void GameCfPlayerMoveParamSet(float, float, float);
void tyaCaptureEnd(void);
void tyaCaptureStart(char *, int);
void SCRIPT_talkIgnoreSet(void);
void GameDefocusQuickSet(int, int, int, int);
void GameDefocusSet(int, int, char *);
void setEventTimerTaskEntry(char *, int);
void MapChangeResource(int, int);
int dataBoxInc(int, int);
int CreateEvtItemGetTask(int, int);
int dataBoxDec(int, int);
int dataBoxChk(int, int);
char *GetItemName(int, int);
int dataMoneyBoxInc(int);
int dataMoneyBoxDec(int);
int dataMoneyBoxChk(void);
void sefProgressEffect(int);
void GameStateSaveCameraLight(void);
void MenuShopMain(int);
void GameStateRestoreCameraLight(void);
int cid2bic(int);
void PartyFriendOn(int);
void PartyFriendOff(int);
int PartyFriendCheck(int);
void PartyLockPartyOn(int);
void PartyLockPartyOff(int);
int PartyLockPartyCheck(int);
void PartyOutFriendOn(int);
void PartyOutFriendOff(int);
int PartyOutFriendCheck(int);
void PartyTakeAgwsOn(int);
void PartyTakeAgwsOff(int);
int PartyTakeAgwsCheck(int);
void PartyBattleChange(int);
int JNI_isInstanceOf(void *, JClass *);
void LAYOUT_mapID_setChr(void *, int, int);
void LAYOUT_mapID_setUnit(void *, int, int);
void LAYOUT_mapID_setEffect(void *, int, int);
void xglLightIntensityAmbient(IdLight *, float *);
void xglLightIntensityParallel(IdLight *, int, float *);
void xglLightAngle(IdLight *, int, float *);
void xglLightDirection(IdLight *, int, float *);
void EXM_StopWind(void);
void EXM_SetShakePower(float);
void EXM_SetShakeTime(float);
void EXM_SetDirectionalWind(float *);
void EXM_SetPointWind(float *);
void nmlModelSetMpeg2CrossFadeTime(int);
void SCRIPT_sceneChangeTimeSet(int);
void func_A193C8(int);
void CharactorAllRecovery(void);
void AgwsAllRecovery(void);
void GameStateSaveCameraLight(void);
void xglCullingIgnore(void);
void UmnMailMain(int);
void xglCullingIgnoreOff(void);
void MSG_print2(TWin *, char *, int);
TWin *TWIN_create2(int);
void TWIN_initCF(TWin *);
void TWIN_initScene(TWin *);
void TWIN_init2(TWin *);
void *TMENU_create(int);
void TMENU_addQuery(void *, char *);
void TMENU_addQuery2(void *, char **, int);
void TMENU_addItem(void *, char *);
char *getPartyDataOfs(unsigned int);
int XTK_findFile(char *);
int XTK_getResourceID(void *);
int getPeer_Chr(void *);
int getPeer_Enepc(void *);
int getPeer_Unit(void *);
int getPeer_Uwamono(void *);
int getPeer_Effect(void *);
int getPeer_Stage(void *);
void ACT_loadResource(void *, int);
void ACT_loadMotion(void *, int, int);
void MAP_loadUnitResource(void *, int);
int RES_loadFile(int, int, int, int);
void xglSleep(void);
void GameCfPlayerLoadResource(int);
extern char *D_00338684[];

/* Byte offset of a named field within a script object of the given class */
#define TK_FIELD(cls, name) \
    (lookupClassField(cls, loadConstString(name, -1), 0)->nOffset)

/* Allocate the native side of a Spline object */
void Java_xeno_util_Spline_create__(JThread *thread, JValue *args, JValue *ret)
{
    JClass *cls;
    int *obj;

    cls = classJava_xeno_util_Spline;
    obj = (int *)xmalloc(0x494, 0xE);
    obj[0] = cls->field_18;
    ret->p = obj;
}

/* Bind a float array as the spline control vertices */
void Java_xeno_util_Spline_setCtrlVertex__aFIII(JThread *thread, JValue *args, JValue *ret)
{
    Spline *spl;
    JArray *arr;
    int n;
    int idx;

    spl = (Spline *)args[0].p;
    n = args[3].i;
    arr = (JArray *)args[1].p;
    idx = args[2].i;
    spl->field_6 = n;
    spl->field_8 = args[4].h;
    SPL_init2(spl, idx, arr->pData, arr->nLength >> 2, 3);
}

/* Sample the spline and return a shared Vector4f */
/* TODO: near-miss - the original assembler emits an mtc1->swc1 hazard nop that fix_cc_asm does not reproduce */
void Java_xeno_util_Spline_getValue__I(JThread *thread, JValue *args, JValue *ret)
{
    static Vector4fObj value;
    float xyz[3];

    SPL_getValueXYZ(xyz, args[0].p, (float)args[1].i);
    value.z = xyz[0];
    value.y = xyz[1];
    value.x = xyz[2];
    value.w = 1.0f;
    value.field_0 = classJava_xeno_util_Vector4f->field_18;
    ret->p = &value;
}

/* Reinterpret a float as its raw bits */
void Java_xeno_util_Format_floatToIntBits__F(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = args[0].i;
}

/* Convert an int to a float */
void Java_xeno_util_Format_intBitsToFloat__I(JThread *thread, JValue *args, JValue *ret)
{
    ret->f = (float)args[0].i;
}

/* Format.toInt is not implemented */
void Java_xeno_util_Format_toInt__Ljava_lang_String_(JThread *thread, JValue *args, JValue *ret)
{
}

/* Format a char value into a fresh String */
void Java_xeno_util_Format_toString__C(JThread *thread, JValue *args, JValue *ret)
{
    JString *str;
    JArray *arr;
    JArray *arr2;

    str = (JString *)newObject(classString);
    arr = newArray(classByte, 0xFF);
    str->pArray = arr;
    sprintf(arr->pData, D_004DC098, args[0].c);
    arr2 = str->pArray;
    arr2->nLength = strlen(arr2->pData);
    ret->p = str;
}

/* Format a float value into the shared temp String */
/* TODO: near-miss - the format string address is not kept in a callee-saved register */
void Java_xeno_util_Format_toString__F(JThread *thread, JValue *args, JValue *ret)
{
    JArray *arr;
    char *buf;

    if (JAVA_tmpString == 0) {
        JAVA_tmpString = (JString *)newObject(classString);
        JAVA_tmpString->pArray = newArray(classByte, 0xFF);
    }
    buf = JAVA_tmpString->pArray->pData;
    sprintf(buf, D_004DC0A0, fptodp(args[0].f));
    arr = JAVA_tmpString->pArray;
    arr->nLength = strlen(arr->pData);
    ret->p = JAVA_tmpString;
}

/* Format an int value into the shared temp String */
void Java_xeno_util_Format_toString__I(JThread *thread, JValue *args, JValue *ret)
{
    JArray *arr;
    char *p;

    if (JAVA_tmpString == 0) {
        JAVA_tmpString = (JString *)newObject(classString);
        JAVA_tmpString->pArray = newArray(classByte, 0xFF);
    }
    p = STRING_int(JAVA_tmpString->pArray->pData, args[0].i);
    *p = 0;
    arr = JAVA_tmpString->pArray;
    arr->nLength = p - arr->pData;
    ret->p = JAVA_tmpString;
}

/* Format a boolean value into the shared temp String.
 * Do not cache JAVA_tmpString in a local: the original re-reads the
 * global after each call (it has to -- a call may store to it), and
 * caching costs an extra callee-saved register and a wider frame.
 * The only remaining difference was gcc scheduling the epilogue
 * `ld $ra` one slot ahead of the nLength store, fixed by
 *   --swap-adjacent Java_xeno_util_Format_toString__Z:27!
 * The `!` forces the swap past swap_ok's alias check, which cannot see
 * that a $sp-relative restore and a heap store through $s0 are
 * independent. */
void Java_xeno_util_Format_toString__Z(JThread *thread, JValue *args, JValue *ret)
{
    JArray *arr;
    JString *str;
    char *buf;
    int nLen;

    if (JAVA_tmpString == 0) {
        JAVA_tmpString = (JString *)newObject(classString);
        JAVA_tmpString->pArray = newArray(classByte, 0xFF);
    }
    sprintf(JAVA_tmpString->pArray->pData, D_004DC098, args[0].z);
    arr = JAVA_tmpString->pArray;
    arr->nLength = strlen(arr->pData);
    ret->p = JAVA_tmpString;
}

/* Create an Input object bound to a pad number */
void Java_xeno_util_Input_create__I(JThread *thread, JValue *args, JValue *ret)
{
    char *obj;
    JField *fld;

    obj = (char *)newObject(classJava_xeno_util_Input);
    fld = lookupClassField(classJava_xeno_util_Input, loadConstString(D_004DC0A8, -1), 0);
    *(int *)(obj + fld->nOffset) = args[0].i;
    ret->p = obj;
}

/* Current button state of the bound pad */
void Java_xeno_util_Input_getButton__(JThread *thread, JValue *args, JValue *ret)
{
    char *obj;
    JField *fld;

    obj = (char *)args[0].p;
    fld = lookupClassField(classJava_xeno_util_Input, loadConstString(D_004DC0A8, -1), 0);
    ret->i = D_00490DB8[*(int *)(obj + fld->nOffset)].nButton;
}

/* Button edge state of the bound pad */
void Java_xeno_util_Input_getEdge__(JThread *thread, JValue *args, JValue *ret)
{
    char *obj;
    JField *fld;

    obj = (char *)args[0].p;
    fld = lookupClassField(classJava_xeno_util_Input, loadConstString(D_004DC0A8, -1), 0);
    ret->i = D_00490DB8[*(int *)(obj + fld->nOffset)].nEdge;
}

/* Button repeat state of the bound pad */
void Java_xeno_util_Input_getRepeat__(JThread *thread, JValue *args, JValue *ret)
{
    char *obj;
    JField *fld;

    obj = (char *)args[0].p;
    fld = lookupClassField(classJava_xeno_util_Input, loadConstString(D_004DC0A8, -1), 0);
    ret->i = D_00490DB8[*(int *)(obj + fld->nOffset)].nRepeat;
}

/* Attach an object to the layout manager slot */
void Java_xeno_util_Layout_set__Ljava_lang_Object_I(JThread *thread, JValue *args, JValue *ret)
{
    void *obj;
    int *layout;
    int id;

    obj = args[1].p;
    layout = (int *)args[0].p;
    id = args[2].i;
    if (JNI_isInstanceOf(obj, classJava_xeno_Chr) == 1) {
        LAYOUT_mapID_setChr(obj, layout[1], id);
    } else if (JNI_isInstanceOf(obj, classJava_xeno_Unit) == 1) {
        LAYOUT_mapID_setUnit(obj, layout[1], id);
    } else if (JNI_isInstanceOf(obj, classJava_xeno_Effect) == 1) {
        LAYOUT_mapID_setEffect(obj, layout[1], id);
    }
}

/* Return the shared layout manager object */
void Java_xeno_util_Layout_getManager__I(JThread *thread, JValue *args, JValue *ret)
{
    int *p;

    p = &defaultLayout;
    *p = classJava_xeno_util_Layout->field_18;
    D_004DC6B4 = args[0].i;
    ret->p = p;
}

/* Read a game flag */
void Java_xeno_util_Runtime_getFlags__II(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = xglFlagsGet(args[0].i, args[1].i);
}

/* Write a game flag */
void Java_xeno_util_Runtime_setFlags__III(JThread *thread, JValue *args, JValue *ret)
{
    xglFlagsSet(args[0].i, args[1].i, args[2].i);
}

/* Change the current map location */
/* TODO: near-miss - delay-slot fill and store order tie-break */
/* Change map. See the consolidated flag request at the head of the
   jumpCF note above for this function's two --rotate sites. */
void Java_xeno_util_Runtime_setLocation__III(JThread *thread, JValue *args, JValue *ret)
{
    int nMap = args[0].i;

    if (args[2].i == 0 || D_00338688[0] == 0) {
        MapChange2(nMap);
        return;
    }
    LoadMapOnly();
    thread->nFlags |= 9;
    thread->field_3C = thread->field_3E;
}

/* Current map number */
void Java_xeno_util_Runtime_getLocation__(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = MapGetNo();
}

/* Write a script VM register */
void Java_xeno_util_Runtime_setRegister__II(JThread *thread, JValue *args, JValue *ret)
{
    VMRegister[args[0].i & 0x1F] = args[1].i;
}

/* Read a script VM register */
void Java_xeno_util_Runtime_getRegister__I(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = VMRegister[args[0].i & 0x1F];
}

/* Entrance index the map was entered through */
void Java_xeno_util_Runtime_getEntrance__(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = D_003386D2[0];
}

/* Enable or disable player control */
void Java_xeno_util_Runtime_setPlayerControl__Z(JThread *thread, JValue *args, JValue *ret)
{
    if (args[0].z) {
        GameLoopState.nFlags &= ~0x8000;
        GameLoopState.nC1 |= 0x20;
    } else {
        GameLoopState.nFlags |= 0x8000;
    }
}

/* Set the player movement parameters */
void Java_xeno_util_Runtime_setPlayerMoveParam__FFF(JThread *thread, JValue *args, JValue *ret)
{
    GameCfPlayerMoveParamSet(args[0].f, args[1].f, args[2].f);
}

/* Clear game loop state bits */
void Java_xeno_util_Runtime_disable__I(JThread *thread, JValue *args, JValue *ret)
{
    GameLoopState.nFlags &= ~args[0].i;
}

/* Set game loop state bits */
void Java_xeno_util_Runtime_enable__I(JThread *thread, JValue *args, JValue *ret)
{
    GameLoopState.nFlags |= args[0].i;
}

/* Current game state value */
void Java_xeno_util_Runtime_getGameState__(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = D_00338690[0];
}

/* Stop the debug capture */
void Java_xeno_util_Runtime_CaptureEnd__(JThread *thread, JValue *args, JValue *ret)
{
    tyaCaptureEnd();
}

/* Start the debug capture */
void Java_xeno_util_Runtime_CaptureStart__Ljava_lang_String_I(JThread *thread, JValue *args, JValue *ret)
{
    tyaCaptureStart(((JString *)args[0].p)->pArray->pData, args[1].i);
}

/* Jump to another cutscene script */
/* CONSOLIDATED FLAG REQUEST for Java_util.c (all three verified together
 * on top of the existing --omit-hazard cvt.s.w, taking the file from
 * 100 match/8 not to 103 match/5 not):
 *
 *   --mtc1-nop Java_xeno_util_Spline_getValue__I:0
 *   --swap-adjacent Java_xeno_util_Runtime_jumpCF__II:7
 *   --rotate Java_xeno_util_Runtime_setLocation__III:15:2,\
 *            Java_xeno_util_Runtime_setLocation__III:17:3
 *
 * Once wired, register:
 *   Java_xeno_util_Spline_getValue__I = 0x002F6798, 0x74; // Java_util.c
 *   Java_xeno_util_Runtime_jumpCF__II = 0x002F76F0, 0x58; // Java_util.c
 *   Java_xeno_util_Runtime_setLocation__III = 0x002F7340, 0x6C; // Java_util.c
 *
 * setLocation's two rotate sites are deliberately non-overlapping, so
 * unlike JS_loadClass's pair they both fire in a single pass and need no
 * change to rotate_insns itself. Its source is otherwise exact once the
 * MapChange2 argument is hoisted into a local ahead of the guard (that
 * alone takes it from 14 diffs to 5); four tail shapes including the
 * read-early/write-late split were swept and none beat 5. */
/* TODO: near-miss - one scheduling tie-break between the parameter copy
 * (move s0,a0, preserving `thread` across the SCRIPT_talkIgnoreSet call)
 * and the %lo addiu completing the jthreadResetFunc address. Original
 * order: store args_jump[0], move s0,a0, addiu-lo; ours: store
 * args_jump[0], addiu-lo, move s0,a0 -- a pure list-scheduler tie-break,
 * both instructions are ready at the same point with no dependency
 * between them. jumpEvent__I (identical shape plus one extra `+
 * 0x10000000` add) naturally gets the original's ordering, so the extra
 * instruction tips the scheduler, but there's no semantically-neutral
 * source change to reproduce that here. Tried: reordering the 3
 * assignment statements (every permutation regresses to 8 diffs),
 * register-pinning `thread` to $16 (no effect), hoisting it through a
 * plain local `JThread *t = thread` (no effect). Leave as-is. */
void Java_xeno_util_Runtime_jumpCF__II(JThread *thread, JValue *args, JValue *ret)
{
    args_jump[0] = args[0].i;
    args_jump[1] = args[1].i;
    jthreadResetFunc = scriptReset_jump;
    SCRIPT_talkIgnoreSet();
    thread->field_3C = thread->field_3E;
    thread->nFlags |= 9;
}

/* Jump to an event script */
void Java_xeno_util_Runtime_jumpEvent__I(JThread *thread, JValue *args, JValue *ret)
{
    args_jump[0] = args[0].i + 0x10000000;
    args_jump[1] = args[1].i;
    jthreadResetFunc = scriptReset_jump;
    SCRIPT_talkIgnoreSet();
    thread->field_3C = thread->field_3E;
    thread->nFlags |= 9;
}

/* Set the quick defocus parameters */
void Java_xeno_util_Runtime_setDefocusQuick__IIII(JThread *thread, JValue *args, JValue *ret)
{
    GameDefocusQuickSet(args[0].i, args[1].i, args[2].i, args[3].i);
}

/* Set the defocus target list */
void Java_xeno_util_Runtime_setDefocus__IIaI(JThread *thread, JValue *args, JValue *ret)
{
    JArray *arr;

    arr = (JArray *)args[2].p;
    if (arr == 0) {
        GameDefocusSet(args[0].i, args[1].i, 0);
    } else {
        GameDefocusSet(args[0].i, args[1].i, arr->pData);
    }
}

/* Store one defocus parameter */
void Java_xeno_util_Runtime_setDefocusParam__III(JThread *thread, JValue *args, JValue *ret)
{
    D_00367544[args[0].i][args[1].i] = args[2].i;
}

/* Register an event timer task */
void Java_xeno_util_Runtime_setEventTimer__Ljava_lang_String_I(JThread *thread, JValue *args, JValue *ret)
{
    setEventTimerTaskEntry(((JString *)args[0].p)->pArray->pData, args[1].i);
}

/* Load a map resource */
void Java_xeno_util_Runtime_setMap__II(JThread *thread, JValue *args, JValue *ret)
{
    MapChangeResource(args[0].i, args[1].i);
}

/* Enable or disable the shooting system */
void Java_xeno_util_Runtime_setShootFlag__Z(JThread *thread, JValue *args, JValue *ret)
{
    if (args[0].z) {
        GameLoopState.nShootFlags |= 1;
    } else {
        GameLoopState.nShootFlags &= ~1;
    }
}

/* Enable or disable the shooting height check */
void Java_xeno_util_Runtime_setShootHeightCheck__Z(JThread *thread, JValue *args, JValue *ret)
{
    if (args[0].z) {
        GameLoopState.nShootFlags |= 2;
    } else {
        GameLoopState.nShootFlags &= ~2;
    }
}

/* Enable or disable the shooting ID check */
void Java_xeno_util_Runtime_setShootIDCheck__Z(JThread *thread, JValue *args, JValue *ret)
{
    if (args[0].z) {
        GameLoopState.nShootFlags |= 4;
    } else {
        GameLoopState.nShootFlags &= ~4;
    }
}

/* Enable or disable the shooting surface check */
void Java_xeno_util_Runtime_setShootUwaCheck__Z(JThread *thread, JValue *args, JValue *ret)
{
    if (args[0].z) {
        GameLoopState.nShootFlags |= 8;
    } else {
        GameLoopState.nShootFlags &= ~8;
    }
}

/* Set the shooting range */
void Java_xeno_util_Runtime_setShootRange__F(JThread *thread, JValue *args, JValue *ret)
{
    D_003625C4[0] = args[0].f;
}

/* Add an item to the inventory */
void Java_xeno_util_Runtime_addItem__II(JThread *thread, JValue *args, JValue *ret)
{
    ret->z = dataBoxInc(args[0].i, args[1].i);
}

/* Add an item and show the acquisition window */
void Java_xeno_util_Runtime_addItemWin__II(JThread *thread, JValue *args, JValue *ret)
{
    ret->z = CreateEvtItemGetTask(args[0].i, args[1].i);
}

/* Remove an item from the inventory */
void Java_xeno_util_Runtime_removeItem__II(JThread *thread, JValue *args, JValue *ret)
{
    ret->z = dataBoxDec(args[0].i, args[1].i);
}

/* Test whether the inventory holds an item */
void Java_xeno_util_Runtime_checkItem__II(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = dataBoxChk(args[0].i, args[1].i);
}

/* Look up an item name and wrap it in a String */
void Java_xeno_util_Runtime_getItemName__II(JThread *thread, JValue *args, JValue *ret)
{
    char *name;
    JString *str;
    JArray *arr;

    name = GetItemName(args[0].i, args[1].i);
    str = (JString *)newObject(classString);
    arr = newArray(classByte, 0xFF);
    str->pArray = arr;
    arr->pData = name;
    arr->nLength = strlen(name);
    ret->p = str;
}

/* Add gold to the party wallet */
void Java_xeno_util_Runtime_addGold__I(JThread *thread, JValue *args, JValue *ret)
{
    ret->z = dataMoneyBoxInc(args[0].i);
}

/* Remove gold from the party wallet */
void Java_xeno_util_Runtime_removeGold__I(JThread *thread, JValue *args, JValue *ret)
{
    ret->z = dataMoneyBoxDec(args[0].i);
}

/* Current gold amount */
void Java_xeno_util_Runtime_checkGold__(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = dataMoneyBoxChk();
}

/* Trigger a progress effect */
void Java_xeno_util_Runtime_progressEffect__I(JThread *thread, JValue *args, JValue *ret)
{
    sefProgressEffect(args[0].i);
}

/* Run the shop menu */
void Java_xeno_util_Runtime_enterShop__I(JThread *thread, JValue *args, JValue *ret)
{
    GameStateSaveCameraLight();
    MenuShopMain(args[0].i);
    GameStateRestoreCameraLight();
}

/* Character id of the party leader */
void Java_xeno_util_Runtime_getLeader__(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = D_004A1854[0];
}

/* Read one field of a party member record */
/* TODO: near-miss - gcc balances the switch tree around case 2 instead of case 1 */
void Java_xeno_util_Runtime_getPartyData__I(JThread *thread, JValue *args, JValue *ret)
{
    unsigned int id;
    int type;
    char *p;
    int v;

    id = args[0].u;
    type = (int)(id >> 24);
    p = getPartyDataOfs(id);
    /* Not a switch, and not a plain if-chain either.  gcc's expand_case
       roots its decision tree at 2 and reorders every arm; a plain
       `if (type == 1) goto t1;` gets the t1 block inlined at the branch
       instead of laid out after the chain.  Routing the first test
       through its inverted form plus an explicit `goto t1` keeps the
       original's layout: all four bodies after the tests, one shared
       store, and the ==3 arm folded into its own branch-likely delay
       slot (reorg moves its single load and deletes the block, which is
       what lets the ==2 arm fall straight through to the store). */
    if (type != 1) {
        goto n1;
    }
    goto t1;
n1:
    if (type < 2) {
        goto def;
    }
    if (type == 2) {
        goto t2;
    }
    if (type == 3) {
        goto t3;
    }
def:
    v = *(unsigned char *)p;
    goto done;
t1:
    v = *(short *)p;
    goto done;
t2:
    v = *(int *)p;
    goto done;
t3:
    v = *(unsigned int *)p;
done:
    ret->i = v;
}

/* Write one party-data field, sized by the key's type tag */
/* TODO: near-miss - the switch tree and everything up to the reload now match; the
   tail keeps the actor pointer in $s1 and the saved slot in $s0 where the original
   has them the other way round, costing one extra instruction in the epilogue. */
/* TODO: near-miss (60 built vs 61 original words).  The four-way store
   switch is byte-exact -- note it keeps an explicit `case 0:` so gcc roots
   its decision tree at 1, which is exactly the shape getPartyData above
   has to be hand-written to reach.  The single missing word is in the
   reload guard: the original allocates chr to $s0 and the saved field to
   $s1, keeping $s1's `args` value dead but not reusing it, and its
   equal-case exit is a plain `beq` with the nSave load in the delay slot.
   gcc reuses $s1 for chr, which frees $s0 for nSave and lets it fold the
   exit into a branch-likely with an epilogue load annulled instead.
   --swap-regs cannot fix it: $s1 also carries `args` through the whole
   first half, so a whole-function 16-17 rename breaks the prologue.
   Five source shapes were swept (chr before/after nId, the id inlined
   into the compare, both guards as early returns, and an else-return
   before the tail call) -- all give the same 22 diffs. */
void Java_xeno_util_Runtime_setPartyData__II(JThread *thread, JValue *args, JValue *ret)
{
    char *chr;
    char *p;
    int nType;
    int nSave;
    int nId;

    nType = args[0].u >> 24;
    p = getPartyDataOfs(args[0].u);
    switch (nType) {
    case 0:
    default:
        *(char *)p = args[1].z;
        break;
    case 1:
        *(short *)p = args[1].h;
        break;
    case 2:
        *(int *)p = args[1].i;
        break;
    case 3:
        *(long long *)p = args[1].i;
        break;
    }
    if (args[0].i == 0x100002C) {
        nId = args[1].i;
        chr = D_00338684[0];
        if (*(short *)(chr + 0x86) != nId) {
            nSave = *(int *)(chr + 0x8D0);
            *(int *)(chr + 0x8D0) = 0;
            xglSleep();
            xglSleep();
            *(int *)(chr + 0x8D0) = nSave;
            GameCfPlayerLoadResource(1);
        }
    }
}

/* Add a character to the party */
void Java_xeno_util_Runtime_setFriend__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyFriendOn(cid2bic(args[0].i));
}

/* Remove a character from the party */
void Java_xeno_util_Runtime_resetFriend__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyFriendOff(cid2bic(args[0].i));
}

/* Test whether a character is in the party */
void Java_xeno_util_Runtime_checkFriend__I(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = PartyFriendCheck(cid2bic(args[0].i));
}

/* Lock a character into the party */
void Java_xeno_util_Runtime_setLockParty__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyLockPartyOn(cid2bic(args[0].i));
}

/* Unlock a character from the party */
void Java_xeno_util_Runtime_resetLockParty__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyLockPartyOff(cid2bic(args[0].i));
}

/* Test whether a character is locked into the party */
void Java_xeno_util_Runtime_checkLockParty__I(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = PartyLockPartyCheck(cid2bic(args[0].i));
}

/* Mark a character as an out-of-party friend */
void Java_xeno_util_Runtime_setOutFriend__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyOutFriendOn(cid2bic(args[0].i));
}

/* Clear the out-of-party friend flag */
void Java_xeno_util_Runtime_resetOutFriend__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyOutFriendOff(cid2bic(args[0].i));
}

/* Test the out-of-party friend flag */
void Java_xeno_util_Runtime_checkOutFriend__I(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = PartyOutFriendCheck(cid2bic(args[0].i));
}

/* Allow a character to take an A.G.W.S. */
void Java_xeno_util_Runtime_setTakeAgws__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyTakeAgwsOn(cid2bic(args[0].i));
}

/* Disallow a character from taking an A.G.W.S. */
void Java_xeno_util_Runtime_resetTakeAgws__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyTakeAgwsOff(cid2bic(args[0].i));
}

/* Test whether a character may take an A.G.W.S. */
void Java_xeno_util_Runtime_checkTakeAgws__I(JThread *thread, JValue *args, JValue *ret)
{
    ret->i = PartyTakeAgwsCheck(cid2bic(args[0].i));
}

/* Swap the battle party member */
void Java_xeno_util_Runtime_battleChangeParty__I(JThread *thread, JValue *args, JValue *ret)
{
    PartyBattleChange(cid2bic(args[0].i));
}

/* Set the colour of one ID light */
void Java_xeno_util_Runtime_setIdLightCol__IIFFF(JThread *thread, JValue *args, JValue *ret)
{
    float col[4];
    int no;
    int type;

    no = args[0].i;
    if (no > 0) {
        no--;
    }
    type = args[1].i;
    col[0] = args[2].f;
    col[1] = args[3].f;
    col[2] = args[4].f;
    col[3] = 1.0f;
    if (type == 0) {
        xglLightIntensityAmbient(&GameIdLight[no], col);
    } else {
        xglLightIntensityParallel(&GameIdLight[no], type - 1, col);
    }
}

/* Set the angle of one ID light */
void Java_xeno_util_Runtime_setIdLightDir__IIFFF(JThread *thread, JValue *args, JValue *ret)
{
    float dir[4];
    int no;
    int type;

    no = args[0].i;
    if (no > 0) {
        no--;
    }
    type = args[1].i;
    dir[0] = args[2].f;
    dir[1] = args[3].f;
    dir[2] = args[4].f;
    dir[3] = 1.0f;
    if (type > 0) {
        xglLightAngle(&GameIdLight[no], type - 1, dir);
    }
}

/* Set the direction vector of one ID light */
void Java_xeno_util_Runtime_setIdLightVec__IIFFF(JThread *thread, JValue *args, JValue *ret)
{
    float vec[4];
    int no;
    int type;

    no = args[0].i;
    if (no > 0) {
        no--;
    }
    type = args[1].i;
    vec[0] = args[2].f;
    vec[1] = args[3].f;
    vec[2] = args[4].f;
    vec[3] = 1.0f;
    if (type > 0) {
        xglLightDirection(&GameIdLight[no], type - 1, vec);
    }
}

/* Drive the wind and screen shake effects */
void Java_xeno_util_Runtime_setWindParam__IFFFF(JThread *thread, JValue *args, JValue *ret)
{
    float param[4];

    param[0] = args[1].f;
    param[1] = args[2].f;
    param[2] = args[3].f;
    param[3] = args[4].f;
    switch (args[0].i) {
    case 0:
        EXM_StopWind();
        break;
    case 1:
        EXM_SetShakePower(args[1].f);
        EXM_SetShakeTime(param[1]);
        break;
    case 2:
        EXM_SetDirectionalWind(param);
        break;
    case 3:
        EXM_SetPointWind(param);
        break;
    }
}

/* Reserve a cross fade after the next movie */
void Java_xeno_util_Runtime_mpeg2AfterCrossFade__I(JThread *thread, JValue *args, JValue *ret)
{
    s_nMpeg2ReserveCrossFade = 1;
    nmlModelSetMpeg2CrossFadeTime(args[0].i);
}

/* Run the mail viewer */
void Java_xeno_util_Runtime_mailExec__I(JThread *thread, JValue *args, JValue *ret)
{
    GameStateSaveCameraLight();
    xglCullingIgnore();
    UmnMailMain(args[0].i);
    xglCullingIgnoreOff();
    GameStateRestoreCameraLight();
}

/* Lock the menu for the scene change */
void Java_xeno_util_Runtime_setMenuLock__(JThread *thread, JValue *args, JValue *ret)
{
    SCRIPT_sceneChangeTimeSet(0x3E);
}

/* Remember the current camera as the event return point */
void Java_xeno_util_Runtime_evsSetRetPoint__(JThread *thread, JValue *args, JValue *ret)
{
    long long *p;

    p = GameLoopState.pRetPoint;
    GameLoopState.field_2A018 = GameLoopState.field_18;
    GameLoopState.field_2A020 = p[2];
    GameLoopState.field_2A028 = p[3];
}

/* Leave the event script */
void Java_xeno_util_Runtime_evsExit__(JThread *thread, JValue *args, JValue *ret)
{
    args_jump[0] = D_00362698[0];
    args_jump[1] = -1;
    jthreadResetFunc = scriptReset_evsExit;
    thread->field_3C = thread->field_3E;
    thread->nFlags |= 9;
}

/* Set the ether tech flag */
void Java_xeno_util_Runtime_etherTecSet__I(JThread *thread, JValue *args, JValue *ret)
{
    func_A193C8(args[0].i);
}

/* Fully heal every character */
void Java_xeno_util_Runtime_charAllRecovery__(JThread *thread, JValue *args, JValue *ret)
{
    CharactorAllRecovery();
}

/* Fully repair every A.G.W.S. */
void Java_xeno_util_Runtime_AGWSAllRecovery__(JThread *thread, JValue *args, JValue *ret)
{
    AgwsAllRecovery();
}

/* Consume the window signal */
void Java_xeno_util_Window_getSignal__(JThread *thread, JValue *args, JValue *ret)
{
    TWin *win;

    win = (TWin *)args[0].p;
    ret->i = win->nSignal;
    win->nSignal = 0;
}

/* Raise a window signal */
void Java_xeno_util_Window_signal__I(JThread *thread, JValue *args, JValue *ret)
{
    ((TWin *)args[0].p)->nSignal = args[1].h;
}

/* Emit the clear control code */
void Java_xeno_util_Window_clear__(JThread *thread, JValue *args, JValue *ret)
{
    MSG_print2((TWin *)args[0].p, D_004D1658, -1);
}

/* Emit the close control code */
void Java_xeno_util_Window_close__(JThread *thread, JValue *args, JValue *ret)
{
    MSG_print2((TWin *)args[0].p, D_004D1668, -1);
}

/* Emit the wait-for-key control code */
void Java_xeno_util_Window_waitkey__I(JThread *thread, JValue *args, JValue *ret)
{
    char buf[0x40];
    TWin *win;

    win = (TWin *)args[0].p;
    sprintf(buf, D_004D1678, args[1].i);
    MSG_print2(win, buf, -1);
}

/* Emit the timed wait control code */
void Java_xeno_util_Window_wait__I(JThread *thread, JValue *args, JValue *ret)
{
    char buf[0x40];
    TWin *win;

    win = (TWin *)args[0].p;
    sprintf(buf, D_004D1688, args[1].i);
    MSG_print2(win, buf, -1);
}

/* Create a text window of the requested kind */
void Java_xeno_util_Window_create__I(JThread *thread, JValue *args, JValue *ret)
{
    JClass *cls;
    TWin *win;
    int mode;

    cls = classJava_xeno_util_Window;
    win = TWIN_create2(-1);
    *(int *)win = cls->field_18;
    mode = args[0].i;
    switch (mode) {
    case 0:
        TWIN_initCF(win);
        break;
    case 1:
        TWIN_initScene(win);
        break;
    }
    ret->p = win;
}

/* Resize a window */
void Java_xeno_util_Window_setSize__II(JThread *thread, JValue *args, JValue *ret)
{
    TWin *win;

    win = (TWin *)args[0].p;
    win->nWidth = args[2].z;
    win->nHeight = args[1].z;
    TWIN_init2(win);
}

/* Move a window */
void Java_xeno_util_Window_setLocation__II(JThread *thread, JValue *args, JValue *ret)
{
    TWin *win;

    win = (TWin *)args[0].p;
    win->nFlags &= ~0x40;
    win->fX = (float)args[1].i;
    win->fY = (float)args[2].i;
}

/* Print a String into a window */
void Java_xeno_util_Window_print__Ljava_lang_String_(JThread *thread, JValue *args, JValue *ret)
{
    TWin *win;
    JString *str;
    JArray *arr;

    win = (TWin *)args[0].p;
    str = (JString *)args[1].p;
    if (win != 0 && str != 0) {
        arr = str->pArray;
        MSG_print2(win, arr->pData, arr->nLength);
    }
}

/* Print an array of Strings into a window */
void Java_xeno_util_Window_print__aLjava_lang_String_I(JThread *thread, JValue *args, JValue *ret)
{
    TWin *win;
    JArray *arr;
    JArray *item;
    unsigned int i;

    win = (TWin *)args[0].p;
    arr = (JArray *)args[1].p;
    if (win == 0 || arr == 0) {
        return;
    }
    for (i = 0; i < arr->nLength; i++) {
        item = ((JString **)arr->pData)[i]->pArray;
        MSG_print2(win, item->pData, item->nLength);
    }
}

/* Window.setName is not implemented */
void Java_xeno_util_Window_setName__Ljava_lang_String_(JThread *thread, JValue *args, JValue *ret)
{
}

/* Emit the close-after-key control code */
void Java_xeno_util_Window_closeWaitKey__(JThread *thread, JValue *args, JValue *ret)
{
    MSG_print2((TWin *)args[0].p, D_004D1698, -1);
}

/* Append a query line to a menu */
void Java_xeno_util_Menu_addQuery__Ljava_lang_String_(JThread *thread, JValue *args, JValue *ret)
{
    JString *str;

    str = (JString *)args[1].p;
    TMENU_addQuery(args[0].p, str->pArray->pData);
}

/* Append several query lines to a menu */
/* TODO: near-miss - irreducible $v1/$a0 register tie-break in the prologue */
void Java_xeno_util_Menu_addQuery__aLjava_lang_String_I(JThread *thread, JValue *args, JValue *ret)
{
    char *strs[32];
    JArray *arr;
    JString **items;
    int count;
    void *menu;
    int i;

    arr = (JArray *)args[1].p;
    count = arr->nLength;
    count = (count < 0x21) ? count : 0x20;
    menu = args[0].p;
    /* The index is zeroed ahead of the guard and the element pointer is
     * only fetched inside it: that leaves the guard branch with the menu
     * load as its delay-slot fill, which is what the original does. Any
     * shape where the element pointer is live before the guard fills the
     * slot with that load instead, and shifts the clamp constant off
     * $v1. */
    i = 0;
    if (count > 0) {
        items = (JString **)arr->pData;
        do {
            strs[i] = items[i]->pArray->pData;
            i++;
        } while (i < count);
    }
    TMENU_addQuery2(menu, strs, count);
}

/* Append an item line to a menu */
void Java_xeno_util_Menu_addItem__Ljava_lang_String_(JThread *thread, JValue *args, JValue *ret)
{
    JString *str;

    str = (JString *)args[1].p;
    TMENU_addItem(args[0].p, str->pArray->pData);
}

/* Create a menu object */
void Java_xeno_util_Menu_create__(JThread *thread, JValue *args, JValue *ret)
{
    JClass *cls;
    int *menu;

    cls = classJava_xeno_util_Menu;
    menu = (int *)TMENU_create(-1);
    menu[0] = cls->field_18;
    ret->p = menu;
}

/* Index of the entry the user chose */
void Java_xeno_util_Menu_getSelected__(JThread *thread, JValue *args, JValue *ret)
{
    TWin *menu;

    menu = (TWin *)args[0].p;
    if (menu->n055 < 0) {
        ret->i = menu->n055;
    } else {
        ret->i = menu->nCursor;
    }
}

/* Move a menu */
void Java_xeno_util_Menu_setLocation__II(JThread *thread, JValue *args, JValue *ret)
{
    TWin *menu;

    menu = (TWin *)args[0].p;
    menu->fX = (float)args[1].i;
    menu->fY = (float)args[2].i;
}

/* Menu.setVisible is not implemented */
void Java_xeno_util_Menu_setVisible__Z(JThread *thread, JValue *args, JValue *ret)
{
}

/* Move the menu cursor */
void Java_xeno_util_Menu_setCursor__I(JThread *thread, JValue *args, JValue *ret)
{
    TWin *menu;
    int no;

    menu = (TWin *)args[0].p;
    no = args[1].i;
    if (menu->nCursor != no) {
        menu->n068 = no;
        menu->nFlags |= 0x200;
    }
}

/* Resolve the native peer of any script object the toolkit knows about */
void Java_xeno_util_Toolkit_getPeer__Ljava_lang_Object_(JThread *thread, JValue *args, JValue *ret)
{
    void *obj = args[0].p;

    if (JNI_isInstanceOf(obj, classJava_xeno_Chr) == 1) {
        if (JNI_isInstanceOf(obj, classJava_xeno_Enepc) == 1) {
            ret->i = getPeer_Enepc(obj);
        } else {
            ret->i = getPeer_Chr(obj);
        }
    } else if (JNI_isInstanceOf(obj, classJava_xeno_Unit) == 1) {
        if (JNI_isInstanceOf(obj, classJava_xeno_Uwamono) == 1) {
            ret->i = getPeer_Uwamono(obj);
        } else {
            ret->i = getPeer_Unit(obj);
        }
    } else if (JNI_isInstanceOf(obj, classJava_xeno_Effect) == 1) {
        ret->i = getPeer_Effect(obj);
    } else if (JNI_isInstanceOf(obj, classJava_xeno_Stage) == 1) {
        ret->i = getPeer_Stage(obj);
    } else {
        ret->i = 0;
    }
}

/* Toolkit.call is not implemented */
void Java_xeno_util_Toolkit_call__Ljava_lang_Object_Ljava_lang_String_(JThread *thread, JValue *args, JValue *ret)
{
}

/* Load a character's model plus motion set, or a unit's resource */
void Java_xeno_util_Toolkit_loadResource__Ljava_lang_Object_I(JThread *thread, JValue *args, JValue *ret)
{
    int nRes;
    char *obj;
    int nNo;
    void *peer;

    nRes = XTK_getResourceID(thread);
    obj = (char *)args[0].p;
    nNo = args[1].i;
    if (JNI_isInstanceOf(obj, classJava_xeno_Chr) != 0) {
        peer = *(void **)(obj + TK_FIELD(classJava_xeno_Chr, D_004DC110));
        ACT_loadResource(peer, nNo);
        ACT_loadMotion(peer, nNo, nRes);
    } else if (JNI_isInstanceOf(obj, classJava_xeno_Unit) != 0) {
        peer = *(void **)(obj + TK_FIELD(classJava_xeno_Unit, D_004DC110));
        if ((*(int *)(obj + TK_FIELD(classJava_xeno_Unit, D_004D16B0)) & 0xF00) == 0) {
            MAP_loadUnitResource(peer, nNo);
        }
    }
}

/* Look up a resource file by name */
void Java_xeno_util_Toolkit_loadResource__Ljava_lang_String_(JThread *thread, JValue *args, JValue *ret)
{
    JString *str;

    str = (JString *)args[0].p;
    ret->i = XTK_findFile(str->pArray->pData);
}

/* Attach a loaded resource object to one of the peer's resource slots */
void Java_xeno_util_Toolkit_loadResource__Ljava_lang_Object_Ljava_lang_Object_I(JThread *thread, JValue *args, JValue *ret)
{
    char *obj;
    int nIdx;
    int nRes;
    char *peer;

    obj = (char *)args[0].p;
    nRes = args[1].i;
    nIdx = args[2].i;
    if (JNI_isInstanceOf(obj, classJava_xeno_Chr) != 0) {
        peer = *(char **)(obj + TK_FIELD(classJava_xeno_Chr, D_004DC110));
        if (nIdx < 11) {
            *(int *)(peer + nIdx * 4 + 0x8D0) = nRes;
        }
    } else if (JNI_isInstanceOf(obj, classJava_xeno_Unit) != 0) {
        obj = *(char **)(obj + TK_FIELD(classJava_xeno_Unit, D_004DC110));
        TK_FIELD(classJava_xeno_Unit, D_004D16B0);
        if (nIdx == 0) {
            *(int *)(obj + 0xD4) = nRes;
        } else {
            nIdx--;
            if (nIdx < 2) {
                *(int *)(obj + nIdx * 4 + 0xD8) = nRes;
            }
        }
    }
}

/* Load a resource and, for units, its extra streamed file */
void Java_xeno_util_Toolkit_loadResource__Ljava_lang_Object_II(JThread *thread, JValue *args, JValue *ret)
{
    char *obj;
    int nNo;
    int nMot;
    char *peer;

    XTK_getResourceID(thread);
    obj = (char *)args[0].p;
    nNo = args[1].i;
    nMot = args[2].i;
    if (JNI_isInstanceOf(obj, classJava_xeno_Chr) != 0) {
        peer = *(char **)(obj + TK_FIELD(classJava_xeno_Chr, D_004DC110));
        ACT_loadResource(peer, nNo);
        ACT_loadMotion(peer, nMot, 3);
    } else if (JNI_isInstanceOf(obj, classJava_xeno_Unit) != 0) {
        peer = *(char **)(obj + TK_FIELD(classJava_xeno_Unit, D_004DC110));
        if ((*(int *)(obj + TK_FIELD(classJava_xeno_Unit, D_004D16B0)) & 0xF00) == 0) {
            MAP_loadUnitResource(peer, nNo);
            *(int *)(peer + 0xDC) = RES_loadFile(-1, 2, 0x3000000 + nMot, 0);
        }
    }
}

/* Assign a peer to a resource group */
void Java_xeno_util_Toolkit_peerSetGroup__II(JThread *thread, JValue *args, JValue *ret)
{
    int group;

    group = args[1].i;
    peerGroup[args[0].i & 3] = group;
}
