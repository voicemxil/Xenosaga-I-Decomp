/* Message text conversion and print queue */

int ctrlCodeFlags;

typedef struct MSGQUEUE {
    unsigned short field_0;
    unsigned short field_2;
    unsigned short field_4;
    unsigned short field_6;
    unsigned short field_8;
    unsigned short field_A;
    unsigned short field_C;
    unsigned short field_E;
    unsigned short field_10;
} MSGQUEUE;

typedef struct MSGWND {
    unsigned char unk0[0x16];
    unsigned short field_16;
    unsigned char unk18[0xAC - 0x18];
    MSGQUEUE queue;
} MSGWND;

typedef struct MSGARG {
    int type;
    int value;
} MSGARG;

int MSG_convert(unsigned char *dest, int size, unsigned char *src, int arg);
int MSG_queuePush(MSGQUEUE *q, unsigned char *buf, int size, int arg);
int PARSE_int(unsigned char *str, int len, int base);

/* Reset the control code state */
void MSG_init(void)
{
    ctrlCodeFlags = 0;
}

/* Convert a message and push it onto the window's print queue */
void MSG_print2(MSGWND *wnd, unsigned char *src, int arg)
{
    unsigned char buf[0x400];

    if (wnd->field_16 == 4) {
        buf[0] = 0x19;
        buf[1] = 1;
        MSG_convert(buf + 2, 0x3FE, src, arg);
    } else {
        MSG_convert(buf, 0x400, src, arg);
    }
    MSG_queuePush(&wnd->queue, buf, 0x400, -1);
}

/* Stubbed out message send */
void MSG_send(void)
{
}

/* Format message bytes into 16 byte rows (debug output stubbed out) */
void MSG_dump(unsigned char *data, int size)
{
    int i;
    int j;
    unsigned char buf[16];

    for (i = 0; i < size / 16; i++) {
        for (j = 0; j < 16; j++) {
            int c = data[j];
            if (c < 0x20) {
                buf[j] = '.';
            } else {
                buf[j] = c;
            }
        }
        data += 16;
    }
}

/* Reset queue state according to the requested mode */
void MSG_queueReset(MSGQUEUE *q, int mode)
{
    if (mode & 0x100) {
        ctrlCodeFlags |= 1;
    } else {
        ctrlCodeFlags = 0;
    }
    mode = mode & 0xFF;
    if (mode == 0) {
        q->field_8 = 0;
        q->field_A = 0;
        q->field_E = 0;
        q->field_10 = 0;
        return;
    }
    if (mode == 0xA) {
        q->field_E = 0;
        q->field_10 = 0;
    }
}

/* Parse message argument strings into typed argument slots */
void MSG_setArgs(MSGARG *args, int num, unsigned char **src)
{
    MSGARG *d;
    int n;
    unsigned char **p;

    if (num > 0) {
        d = args;
        p = src;
        n = num;
        do {
            unsigned char *s;
            int c0;
            int c1;

            s = *p++;
            c0 = s[0];
            c1 = s[1];
            if (c0 != '$') {
                if ((c0 >= '0' && c0 <= '9') || (c0 == '-' && (c1 >= '0' && c1 <= '9'))) {
                    d->type = 0;
                    if (c0 == '0' && c1 == 'x') {
                        d->value = PARSE_int(s + 2, -1, 16);
                    } else {
                        d->value = PARSE_int(s, -1, 10);
                    }
                } else {
                    d->value = (int)s;
                    d->type = 1;
                }
            }
            n--;
            d++;
        } while (n != 0);
    }
}
