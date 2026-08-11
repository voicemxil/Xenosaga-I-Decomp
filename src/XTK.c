/* Toolkit state accessors */

extern int resourceID;
extern int windowOwner;
extern int currentScriptDB;

typedef struct {
    void *pDatabase;
    char pad04[24];
} XTK_SCRIPT_DATABASE;

typedef struct {
    char pad00[8];
    void *pData;
} XTK_FILE;

extern XTK_SCRIPT_DATABASE D_004DEDE8[];
extern XTK_FILE *PDB_findFile(void *pDatabase, const char *pName);

void XTK_setResourceID(int nResourceID)
{
    resourceID = nResourceID;
}

int XTK_getResourceID(void)
{
    return resourceID;
}

void XTK_setWindowOwner(int nWindowOwner)
{
    windowOwner = nWindowOwner;
}

int XTK_getWindowOwner(void)
{
    return windowOwner;
}

void *XTK_findFile(const char *pName)
{
    XTK_FILE *pFile;

    pFile = PDB_findFile(D_004DEDE8[currentScriptDB].pDatabase, pName);
    if (pFile != 0) {
        return pFile->pData;
    }
    return 0;
}
