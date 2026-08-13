/* Small UTF/string-table helpers used by the script VM's string interner. */

extern void *xmalloc(int nSize, int nAlign);

/* Allocate a UTF-8 string object (header + payload) */
void *StringUtf_create(int nLen)
{
    return xmalloc(nLen + 13, 20);
}

/* Classic djb2-style rolling hash (hash*31 + c) over [p, p+nLen) */
unsigned int StringUtf_getHash(const char *p, unsigned int nLen)
{
    const char *pEnd = p + nLen;
    unsigned int hash = 0;

    if (p < pEnd) {
        do {
            unsigned char c = *p;
            p++;
            hash = hash * 31 + c;
        } while (p < pEnd);
    }
    return hash;
}
