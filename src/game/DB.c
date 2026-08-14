/* On-screen debug text output - cursor state, printf helpers, path utilities */

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_stdarg_start(ap, last)
#define va_end(ap) ((void)0)

int dbCX;
int dbCY;
int dbCZ;
int dbCH;
int dbMODE;

extern int xglFontGetFlags(void);
extern void xglFontPrint(int x, int y, int z, const char *str);
extern int vsprintf(char *buf, const char *fmt, va_list ap);
extern char *strrchr(const char *s, int c);

/* Select the debug text output mode */
void DB_cmode(int mode)
{
    dbMODE = mode;
}

/* Move the text cursor home and restore the default line height */
void DB_reset(int x, int y)
{
    dbCX = x * 2;
    dbCY = y * 2;
    dbCH = 24;
}

/* Move the text cursor home with an explicit line height */
void DB_reset2(int x, int y, int h)
{
    dbCX = x * 2;
    dbCY = y * 2;
    dbCH = h * 2;
}

/* Advance the text cursor by a relative offset */
void DB_incPos(int x, int y)
{
    dbCX += x;
    dbCY += y;
}

/* Print a literal string at the cursor without advancing the line */
void DB_params(const char *str)
{
    if ((xglFontGetFlags() & 3) == 3) {
        xglFontPrint(dbCX, dbCY, dbCZ, str);
    }
}

/* Formatted print honouring the debug mode, then advance one line */
void DB_println(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    if ((xglFontGetFlags() & 3) == 3) {
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        va_end(ap);
        if (dbMODE == 0) {
            xglFontPrint(dbCX, dbCY, dbCZ, buf);
        }
        dbCY += dbCH;
    }
}

/* Formatted print at the cursor, then advance one line */
int DB_printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    if ((xglFontGetFlags() & 3) != 3) {
        return 0;
    }
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    xglFontPrint(dbCX, dbCY, dbCZ, buf);
    dbCY += dbCH;
    return 0;
}

/* Return the file name portion of a path */
char *DB_pathFindName(char *path)
{
    char *p = strrchr(path, '/');

    if (p != 0) {
        path = p + 1;
    }
    return path;
}
