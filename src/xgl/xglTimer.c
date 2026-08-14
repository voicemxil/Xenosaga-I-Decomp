/* EE hardware timer 0 access */

/* Read the current TIMER0 count */
int xglTimer0Get(void)
{
    return *(volatile unsigned int *)0x10000000 & 0xFFFF;
}

/* Zero the count and restart TIMER0 with the given clock-select mode */
void xglTimer0Reset(int nMode)
{
    *(volatile unsigned int *)0x10000000 = 0;
    *(volatile unsigned int *)0x10000010 = nMode + 128;
}
