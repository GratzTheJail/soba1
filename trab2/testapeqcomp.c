/*Guilherme Melo Gratz 2211068 3WA*/

#include "peqcomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

void dump(void* p, int n){
	unsigned char* p1 = (unsigned char*)p;
	while(n--)
		printf("%02x ", *(p1++));
}

int main (void) {
  FILE *f;

  unsigned char codigo[1000];

  if ((f = fopen ("sbas/retvar.sbas", "r")) == NULL) {
    	perror ("nao conseguiu abrir arquivo!");
    	exit(1);
  }
  funcp func = peqcomp(f, codigo);
  fclose(f);

  printf("Retorno: %d\n", (*func)(2, -53432, 4));
  // dump(func, 13);
  printf("\n");

  struct dirent *de;  // Pointer for directory entry

  // opendir() returns a pointer of DIR type. 
  DIR *dr = opendir("sbas/");
  if (dr == NULL){ printf("Could not open current directory" ); return 0;}

  while ((de = readdir(dr)) != NULL){
    if(de->d_name[0] != '.'){
      char nomeArq[50] = "sbas/";
      if ((f = fopen (strcat(nomeArq, de->d_name), "r")) == NULL) {
    	  perror ("nao conseguiu abrir arquivo!");
    	  exit(1);
      }
      printf("%s:\n", de->d_name);
      funcp func = peqcomp(f, codigo);
      fclose(f);

      printf("Retorno: %d\n", (*func)(2, -3, 4));
      // dump(func, 50);
      printf("\n");
    }
  }

  closedir(dr);
  
  return 0;
}

/*
COMPILAÇÃO
Fedora Linux 42 (Workstation Edition) x86_64
Kernel: 6.14.9-300.fc42.x86_64
CPU: 11th Gen Intel i5-1135G7 (8) @ 4.200GHz
gcc (GCC) 15.1.1 20250521 (Red Hat 15.1.1-2)

gcc -Wall -Wa,--execstack -o testapeqcomp testapeqcomp.c peqcomp.c
*/