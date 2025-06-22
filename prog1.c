#include <stdio.h>
extern int i;
static void sub () {
i--;
printf ("i = %d\n", i);
}
void calcula () {
sub ();
}
