/* Message line buffer pool */

typedef struct MBUF {
    int active;                 /* 0x0 */
    int size;                   /* 0x4 */
    unsigned char **lines;      /* 0x8 */
    unsigned char *ptr;         /* 0xC */
    unsigned char data[0x800];  /* 0x10 */
} MBUF;

MBUF msg_buffer[8];

/* Mark every buffer in the pool as free */
void MBUF_init(void)
{
    int i;

    for (i = 7; i >= 0; i--) {
        msg_buffer[i].active = 0;
    }
}

/* Claim a free buffer as one flat 0x800 byte area */
MBUF *MBUF_create2(void)
{
    MBUF *buf;
    int no;
    int i;

    no = -1;
    for (i = 0; i < 8; i++) {
        buf = &msg_buffer[i];
        if (!(buf->active & 1)) {
            no = i;
            break;
        }
    }
    if (no < 0) {
        return 0;
    }
    buf = &msg_buffer[no];
    buf->active = 1;
    buf->size = 0x800;
    buf->ptr = buf->data;
    return buf;
}

/* Claim a free buffer and carve it into num lines of len bytes */
MBUF *MBUF_create(int len, int num)
{
    MBUF *buf;
    unsigned char **lines;
    int no;
    int i;

    no = -1;
    for (i = 0; i < 8; i++) {
        buf = &msg_buffer[i];
        if (!(buf->active & 1)) {
            no = i;
            break;
        }
    }
    if (no < 0) {
        return 0;
    }
    buf = &msg_buffer[no];
    lines = (unsigned char **)buf->data;
    len = ((len + 3) / 4) * 4;
    buf->active = 1;
    buf->size = 0;
    buf->lines = lines;
    buf->ptr = (unsigned char *)(lines + num);
    for (i = 0; i < num; i++) {
        *buf->ptr = 0;
        buf->lines[i] = buf->ptr;
        buf->ptr += len;
    }
    return buf;
}

/* Return a buffer to the pool */
void MBUF_dispose(MBUF *buf)
{
    buf->active = 0;
}
