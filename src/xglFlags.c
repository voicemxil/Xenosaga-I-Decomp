/* Bit-packed persistent flag accessors */

int xglFlagsSet(int, int, int);
int xglFlagsGet(int, int);

int xglFlagsSet1(int nFlag, int nValue) { return xglFlagsSet(nFlag, 1, nValue); }
int xglFlagsSet2(int nFlag, int nValue) { return xglFlagsSet(nFlag, 2, nValue); }
int xglFlagsSet4(int nFlag, int nValue) { return xglFlagsSet(nFlag, 4, nValue); }
int xglFlagsSet8(int nFlag, int nValue) { return xglFlagsSet(nFlag, 8, nValue); }
int xglFlagsSet16(int nFlag, int nValue) { return xglFlagsSet(nFlag, 0x10, nValue); }
int xglFlagsSet32(int nFlag, int nValue) { return xglFlagsSet(nFlag, 0x20, nValue); }

int xglFlagsGet1(int nFlag) { return xglFlagsGet(nFlag, 1); }
int xglFlagsGet2(int nFlag) { return xglFlagsGet(nFlag, 2); }
int xglFlagsGet4(int nFlag) { return xglFlagsGet(nFlag, 4); }
int xglFlagsGet8(int nFlag) { return xglFlagsGet(nFlag, 8); }
int xglFlagsGet16(int nFlag) { return xglFlagsGet(nFlag, 0x10); }
int xglFlagsGet32(int nFlag) { return xglFlagsGet(nFlag, 0x20); }
