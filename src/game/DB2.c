/* On-screen debug text output - path shortening helper */

extern int strlen(const char *s);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strncpy(char *dst, const char *src, int n);
extern int sprintf(char *buf, const char *fmt, ...);

/* Build an abbreviated "prefix/.../name" form of a path into a caller buffer */
void DB_pathGetShortPath(char *out, char *path, int maxlen)
{
    char buf[128];
    char *p;
    int n;

    if (maxlen < (int)strlen(path)) {
        buf[0] = 0;
        p = strchr(path, '/');
        if (p != 0) {
            n = (p - path) + 1;
            strncpy(buf, path, n);
            buf[n] = 0;
        }
        p = strrchr(path, '/');
        sprintf(out, "%s.../%s", buf, p != 0 ? p : path);
    } else {
        sprintf(out, "%s", path);
    }
}
