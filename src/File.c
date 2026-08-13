/* Save-file and thumbnail helpers. */

extern int D_0036D628[];
extern char *FileObjectData;
extern char *FileObjectDataSystem;
extern char *FileJpegDec;
extern char *FileWork;

typedef struct {
    void *source;
    void *target;
    int field_08;
    int field_0C;
    short field_10;
    short field_12;
    short field_14;
    short field_16;
    int field_18;
    int field_1C;
} JPEG_DECODE_PARAM;

extern void *memset(void *dst, int value, unsigned int size);
extern void xglJpegDecode(JPEG_DECODE_PARAM *param);
extern void FileCheckSumGet(void *data, long long *checksum);
extern void FileJpegDecode(int number);
extern void FileJpegCheck(void);

/* TODO: near-match (LENGTH) - checksum arithmetic is recovered, but GCC's
 * byte-buffer copy expansion differs substantially (63 original vs 92 built
 * instructions). Recover the original local-buffer/copy source shape. */
void FileCheckSumGet(void *data, long long *checksum)
{
    unsigned char bytes[0x163B0];
    unsigned int i;

    __builtin_memcpy(bytes, data, sizeof(bytes));
    *checksum = 0;
    for (i = 0; i <= 0x163AF; i++) {
        *checksum += (long long)bytes[i] * i + 0x7CA;
    }
}

void FileVersionCheck(void)
{
}

int FileObjectDataNoGet(int number, int offset)
{
    int result = number + offset;

    if (result < 0) {
        return result + 99;
    }
    if (result < 99) {
        return result;
    }
    return result - 99;
}

void FileObjectDataClear(void)
{
    int i;

    for (i = 0; i <= 100; i++) {
        FileObjectData[i * 0x1060 + 0x1004] = 0;
        FileObjectData[i * 0x1060 + 0x1005] = -1;
    }
    for (i = 0; i <= 4; i++) {
        FileObjectDataSystem[i * 0xE100 + 1] = -1;
    }
}

void FileThumbnailDecode(void *source, void *target)
{
    JPEG_DECODE_PARAM param;

    memset(&param, 0, sizeof(param));
    param.source = source;
    param.target = target;
    param.field_10 = 0;
    param.field_12 = 0;
    param.field_14 = 0;
    param.field_16 = 0;
    xglJpegDecode(&param);
}

int FileCheckSumCheck(void *data)
{
    long long checksum;

    FileCheckSumGet(data, &checksum);
    return *(long long *)((char *)data + 8) == checksum;
}

/* TODO: near-match (LENGTH) - the recovered search/update logic differs in
 * loop setup and FileJpegDec reload scheduling (21 original vs 22 built
 * instructions). Find the original pointer/loop source shape. */
void FileObjectJpegDecChange(int number)
{
    int i;

    for (i = 0; i < 5; i++) {
        if (FileJpegDec[i * 0xE100 + 1] == number) {
            FileJpegDec[i * 0xE100 + 1] = -1;
            FileJpegDecode(number);
            return;
        }
    }
}

void FileObjectJpegSet(void)
{
    int i;

    FileJpegCheck();
    i = -1;
    do {
        FileJpegDecode(FileObjectDataNoGet(FileWork[9], i));
        i++;
    } while (i < 4);
}

void FileSinkiSaveDataPush(void)
{
}

int FileSlotNameGet(int number)
{
    return D_0036D628[number];
}
