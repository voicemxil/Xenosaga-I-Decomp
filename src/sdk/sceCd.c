/* PS2 SDK sceCd (CD/DVD filesystem) thin wrappers. */

extern int sceCdLayerSearchFile(int a0, int a1, int a2);

int sceCdSearchFile(int a0, int a1)
{
    return sceCdLayerSearchFile(a0, a1, 0);
}
