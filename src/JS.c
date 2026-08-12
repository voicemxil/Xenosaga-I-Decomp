/* Script VM class registration and constant-pool helpers */

typedef struct PRIM
{
    int nType; /* 0x00 - entry kind (1 = integer const, 8 = method) */
    int f4;    /* 0x04 */
    int f8;    /* 0x08 */
} PRIM;

typedef struct JCLASS
{
    int  f0;
    int  f4;
    int  nMethods;    /* 0x08 */
    PRIM aMethod[1];  /* 0x0C */
} JCLASS;

int numPrimitive;
PRIM *primitive;

extern int D_004DA5A8[4];

/* Append an integer constant to the primitive pool */
int JS_loadConstInteger(int nValue, int nExtra)
{
    PRIM *pPrim = &primitive[numPrimitive++];

    pPrim->nType = 1;
    pPrim->f4 = nValue;
    pPrim->f8 = nExtra;
    return 0;
}

/* Bind a class to its peer table and reset its method count */
void JS_classSetup(JCLASS *pClass, int nPeer)
{
    pClass->f4 = nPeer;
    pClass->nMethods = 0;
}

/* Append a method entry to a class */
void JS_classAddMethod(JCLASS *pClass, int nName, int pFunc)
{
    PRIM *pMethod = &pClass->aMethod[pClass->nMethods++];

    pMethod->nType = 8;
    pMethod->f4 = nName;
    pMethod->f8 = pFunc;
}

/* Return the native peer for the light class, or null for other handles */
int *JS_classLight_getPeer(int nHandle)
{
    if (nHandle == -1) {
        return D_004DA5A8;
    }
    return 0;
}
