#include <stdio.h>

void foo(double *vd, int n);

int main(){
	double teste[] = {12.0, 532.0, 1.32};
	foo(teste, 3);

	for(int i = 0; i < 3; i++){
		printf("%.1f ", teste[i]);
	}

	printf("\n");
	return 0;
}
