#include <stdio.h>
typedef int (*funcp) (int p1, int p2, int p3);
funcp peqcomp(FILE *f, unsigned char codigo[]);