/*Guilherme Melo Gratz 2211068 3WA*/
#include "peqcomp.h"
#include<stdio.h>
#include<stdlib.h>

// estrutura auxiliar para organização da tabela de desvios
typedef struct desvio Desvio;
struct desvio{
    int i; // deslocamento em bytes desde o início do código da onde o jump será chamado
    int linhaDestino; // linha (do arquivo sbas) para a qual será pulado
};

// mensagem de erro e termino de programa
static void error (const char *msg, int line) {
  fprintf(stderr, "erro %s na linha %d\n", msg, line);
  exit(EXIT_FAILURE);
}

// escreve nos proximos 4 bytes de 'codigo' o valor inteiro i em little endian
int int2LE(unsigned char codigo[], int i){
    unsigned char* p = (unsigned char*)&i;
    for(int j = 0; j < 4; j++)
        codigo[j] = *(p++);
    return 4;
}

// escreve o prologo da função traduzida de sbas para assembly (criação do RA)
// retorna quantidade de bytes escritos
int prologo(unsigned char codigo[]){
    int i = 0;
    // pushq %rbp (acerto de alinhamento do RA + salva base do RA do caller)
    codigo[i++] = 0x55;
    // movq %rsp, %rbp (salva propria base do RA)
    codigo[i++] = 0x48;
    codigo[i++] = 0x89;
    codigo[i++] = 0xe5;
    
    // ALOCA ESPAÇO PARA RA
    // subq $32, %rsp (como teremos até 5 variáveis (5x4=20, precisamos de 32 bytes no RA))
    // mais nada é preciso ser feito, variáveis apenas têm de ser chamadas corretamente
    // (subtração correta em relação a rbp)
    codigo[i++] = 0x48;
    codigo[i++] = 0x83;
    codigo[i++] = 0xec;
    codigo[i++] = 0x20;

    return i;
}

// epílogo da função (liberação do RA)
int epilogo(unsigned char codigo[]){
    int i = 0;
    // leave
    codigo[i++] = 0xc9;
    return i;
}

// calcula offset em relação a rbp para instruções movl (1=-4, 2=-8, 3=-12, ...)
//                                      (-4 -> 0xfc; -8 -> 0xf8; -12 -> 0xf4; ...)
char rbp_offset(int i){
    return 0xfc - (i-1)*4;
}

// escreve no vetor codigo o codigo de maquina em assembly correspondente ao comando de retorno em sbas
// v = tipo de inteiro (variável [v] OU constante [$])
// n = depende de v (se v=v -> n=numero da variavel | se v=& -> n=constante a ser retornada)
// retorna a quantidade de bytes escritos
int ret(unsigned char codigo[], char v, int n){
    // epílogo obrigatório (leave = voltar rsp para base do RA da função e voltar rbp para rpb do caller)
    int i = 0; 
    
    switch (v) {
        /* caso: retorno constante */
        case '$': {
            codigo[i++] = 0xb8;
            i += int2LE(codigo + i, n);
            
            // leave + ret
            i += epilogo(codigo + i);
            codigo[i++] = 0xc3;

            break;
        }

        /* caso: retorno é uma variável*/
        case 'v': {
            // movl -((i-1)*4)(%rbp), %eax
            codigo[i++] = 0x8b; // movl da memoria para registrador
            codigo[i++] = 0x45; // indicando que é para RA a partir de constante
            codigo[i++] = rbp_offset(n);

            // leave + ret
            i += epilogo(codigo + i);
            codigo[i++] = 0xc3;

            break;
        }
    }

    return i;
}

// realiza a atribuição de um valor para uma variável
// retorna a quantidade de bytes usados
int atribuicao(unsigned char codigo[], char v1, int i1, char v2, int i2){
    int i = 0;

    switch (v2) {
        /* caso constante */
        case '$': {
            // movl %(i2), -(i1)(%rbp)
            codigo[i++] = 0xc7; // instrução padrão de constante para memoria no RA
            codigo[i++] = 0x45; // indicando que é para RA a partir de constante
        
            codigo[i++] = rbp_offset(i1);
            // escreve em little endian valor da constante
            i += int2LE(codigo + i, i2);

            break;
        }

        /* caso variável */
        case 'v': {
            // quando transferimos de uma parte do RA para outra, precisamos utilizar
            // um registrador como mediador
            // em sbas temos no máximo 3 parâmetros, portanto o registrador %ecx (4o param) foi escolhido
            
            // movl -((i-1)*4)(%rbp), %ecx
            codigo[i++] = 0x8b; // movl da memoria para registrador
            codigo[i++] = 0x4d; // indicando que envolve ecx e RA
            codigo[i++] = rbp_offset(i2);

            // movl %ecx, -((i-1)*4)(%rbp) 
            codigo[i++] = 0x89; // movl de registrador para RA
            codigo[i++] = 0x4d; // indicando que envolve ecx e RA
            codigo[i++] = rbp_offset(i1);

            break;
        }

        /* caso parâmetro */
        case 'p': {
            // movl %edi||%esi||%edx, -((i-1)*4)(%rbp)
            codigo[i++] = 0x89; // movl de registrador para RA
            
            switch (i2) {
                case 1:{
                    codigo[i++] = 0x7d; // edi
                    break;
                }

                case 2:{
                    codigo[i++] = 0x75; // esi
                    break;
                }

                case 3:{
                    codigo[i++] = 0x55; // edx
                    break;
                }
            }
            
            codigo[i++] = rbp_offset(i1);

            break;
        }
    }

    return i;
}

// realiza o comando v1i1 = v2i2 + v3i3
int adicao(unsigned char codigo[], int v1, int i1, int v2, int i2, int v3, int i3){
    int i = 0;

    // para realizar uma operação usaremos o registrador %ecx como auxiliar
    switch (v2) {
        case '$':{
            switch (v3) {
                /* caso c2 + c3 */
                case '$':{
                    // movl $i2, %ecx
                    codigo[i++] = 0xb9;
                    i += int2LE(codigo + i, i2);

                    // addl $i3, %ecx
                    codigo[i++] = 0x81;
                    codigo[i++] = 0xc1;
                    i += int2LE(codigo + i, i3);

                    break;   
                }
                
                /* caso c2 + v3 */
                case 'v':{
                    // movl $i2, %ecx
                    codigo[i++] = 0xb9;
                    i += int2LE(codigo + i, i2);

                    // addl -((i3-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x03;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i3);

                    break;   
                }
            }

            break;
        }

        case 'v':{
            switch (v3) {
                /* caso v2 + c3 */
                case '$':{
                    // movl $i2, %ecx
                    codigo[i++] = 0xb9;
                    i += int2LE(codigo + i, i3);

                    // addl -((i2-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x03;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i2);

                    break;   
                }
                
                /* caso v2 + v3 */
                case 'v':{
                    // movl -((i2-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x8b;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i2);

                    // addl -((i3-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x03;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i3);

                    break;   
                }
            }

            break;
        }
    }

    // movl %ecx, -((i1-1)*4)(%rbp)
    codigo[i++] = 0x89;
    codigo[i++] = 0x4d;
    codigo[i++] = rbp_offset(i1);

    return i;
}

// realiza o comando v1i1 = v2i2 - v3i3
int subtracao(unsigned char codigo[], int v1, int i1, int v2, int i2, int v3, int i3){
    int i = 0;

    // para realizar uma operação usaremos o registrador %ecx como auxiliar
    switch (v2) {
        case '$':{
            switch (v3) {
                /* caso c2 - c3 */
                case '$':{
                    // movl $i2, %ecx
                    codigo[i++] = 0xb9;
                    i += int2LE(codigo + i, i2);

                    // subl $i3, %ecx
                    codigo[i++] = 0x81;
                    codigo[i++] = 0xe9;
                    i += int2LE(codigo + i, i3);

                    break;   
                }
                
                /* caso c2 - v3 */
                case 'v':{
                    // movl $i2, %ecx
                    codigo[i++] = 0xb9;
                    i += int2LE(codigo + i, i2);

                    // subl -((i3-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x2b;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i3);

                    break;   
                }
            }

            break;
        }

        case 'v':{
            switch (v3) {
                /* caso v2 - c3 */
                case '$':{
                    // movl -((i2-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x8b;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i2);

                    // subl $i3, %ecx
                    codigo[i++] = 0x81;
                    codigo[i++] = 0xe9;
                    i += int2LE(codigo + i, i3);

                    break;   
                }
                
                /* caso v2 - v3 */
                case 'v':{
                    // movl -((i2-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x8b;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i2);

                    // subl -((i3-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x2b;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i3);

                    break;   
                }
            }

            break;
        }
    }

    // movl %ecx, -((i1-1)*4)(%rbp)
    codigo[i++] = 0x89;
    codigo[i++] = 0x4d;
    codigo[i++] = rbp_offset(i1);

    return i;
}

// realiza o comando v1i1 = v2i2 + v3i3
int multiplicacao(unsigned char codigo[], int v1, int i1, int v2, int i2, int v3, int i3){
    int i = 0;

    // para realizar uma operação usaremos o registrador %ecx como auxiliar
    switch (v2) {
        case '$':{
            switch (v3) {
                /* caso c2 * c3 */
                case '$':{
                    // movl $i2, %ecx
                    codigo[i++] = 0xb9;
                    i += int2LE(codigo + i, i2);

                    // imull $i3, %ecx
                    codigo[i++] = 0x69;
                    codigo[i++] = 0xc9;
                    i += int2LE(codigo + i, i3);

                    break;   
                }
                
                /* caso c2 * v3 */
                case 'v':{
                    // movl $i2, %ecx
                    codigo[i++] = 0xb9;
                    i += int2LE(codigo + i, i2);

                    // imull -((i3-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x0f;
                    codigo[i++] = 0xaf;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i3);

                    break;   
                }
            }

            break;
        }

        case 'v':{
            switch (v3) {
                /* caso v2 * c3 */
                case '$':{
                    // movl $i2, %ecx
                    codigo[i++] = 0xb9;
                    i += int2LE(codigo + i, i3);

                    // imull -((i2-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x0f;
                    codigo[i++] = 0xaf;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i2);

                    break;   
                }
                
                /* caso v2 * v3 */
                case 'v':{
                    // movl -((i2-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x8b;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i2);

                    // imull -((i3-1)*4)(%rbp), %ecx
                    codigo[i++] = 0x0f;
                    codigo[i++] = 0xaf;
                    codigo[i++] = 0x4d;
                    codigo[i++] = rbp_offset(i3);

                    break;   
                }
            }

            break;
        }
    }

    // movl %ecx, -((i1-1)*4)(%rbp)
    codigo[i++] = 0x89;
    codigo[i++] = 0x4d;
    codigo[i++] = rbp_offset(i1);

    return i;
}

// realiza uma operação do tipo v1i1 = v2i2 op v3i3
int operacao(unsigned char codigo[], int v1, int i1, int v2, int i2, int op, int v3, int i3){
    int i = 0;

    switch (op) {
        case '+':{
            i += adicao(codigo + i, v1, i1, v2, i2, v3, i3);
            break;
        }
        case '-':{
            i += subtracao(codigo + i, v1, i1, v2, i2, v3, i3);
            break;
        }
        case '*':{
            i += multiplicacao(codigo + i, v1, i1, v2, i2, v3, i3);
            break;
        }
    }

    return i;
}

// cria as bases para uma condicional com um desvio, que só será calculado e escrito ao final do programa 
// atual = i atual
// desvioLine é uma tabela com o lugar dos desvios no codigo e a linha (sbas) para qual será desviado 
// desvioN = nr do desvio atual (nr do proximo desvio sera retornado atraves dessa variavel)
int condicional(unsigned char codigo[], int atual, char v1, int i1, int n, Desvio desvioLine[], int* desvioN){
// CMPL: 0X83 0X7D -...(%rbp) 0x0
    int i = 0;
    
    // escreve para qual linha esse desvio pulará em caso de sucesso
    desvioLine[*desvioN].linhaDestino = n;
    
    // instruções de comparação de int
    codigo[i++] = 0x83;
    codigo[i++] = 0x7d;
    
    // compara a variável específica com o valor 0
    codigo[i++] = rbp_offset(i1);
    codigo[i++] = 0x0;
    
    // jbe extendido (near jump)
    codigo[i++] = 0x0f;
    codigo[i++] = 0x8e;
    
    // define o endereço que será substituído pelo offset do comando jmp
    desvioLine[*desvioN].i = atual + i;

    
    // neste caso salvamos 0 na instrução para depois calcularmos o offset
    i += int2LE(codigo + i, 0);
    
    (*desvioN)++;
    return i;
}

// vetor codigo completo a partir da primeira posição
void escreve_desvios(unsigned char codigo[], Desvio desvioLine[], int qtdDesvio, int lineEnd[]){
    for(int j = 0; j < qtdDesvio; j++){
        int i = desvioLine[j].i;
        int endLine = lineEnd[desvioLine[j].linhaDestino - 1];
        
        // calcula offset do jmp como endereço da linha para a qual haverá o jump - endereço do próximo comando
        int offset = endLine - (i + 0x04);
        // escreve esse offset no codigo
        int2LE(codigo + i, offset);
    }
}

// DICA: PARA IFS USAR TABELA - OLHAR FOTO DO QUADRO
funcp peqcomp(FILE *f, unsigned char codigo[]){
    int line = 1;
    int c;

    // vetor com o endereço de cada linha
    int lineEnd[50];

    // vetor com a linha para qual será desviado o código a cada desvio
    // 1o desvio = pos 0; 2o desvio = pos 1; ...
    Desvio desvioLine[15];
    int desvioN = 0;

    
    // PROLOGO + CRIAÇÃO DO RA
    int i = prologo(codigo);


    while ((c = fgetc(f)) != EOF) {
    lineEnd[line - 1] = i;
    switch (c) {
      /* retorno */
      case 'r': { 
        char var0;
        int idx0;
        if (fscanf(f, "et %c%d", &var0, &idx0) != 2)
          error("comando invalido", line);
        printf("%d ret %c%d\n", line, var0, idx0); 
        i += ret(codigo + i, var0, idx0);
        break;
      }
      /* atribuiçãoo e op. aritmetica */
      case 'v': {
        int idx0, idx1;
        char var0 = c, c0, var1;
        if (fscanf(f, "%d %c", &idx0, &c0) != 2)
          error("comando invalido", line);

        /* atribuição */
        if (c0 == ':') {
          if (fscanf(f, " %c%d", &var1, &idx1) != 2)
            error("comando invalido", line);
          printf("%d %c%d : %c%d\n", line, var0, idx0, var1, idx1);
          i += atribuicao(codigo + i, var0, idx0, var1, idx1);
        }

        /* operação aritmética */
        else {
          char var2, op;
          int idx2;
          if (c0 != '=')
            error("comando invalido", line);
          if (fscanf(f, " %c%d %c %c%d", &var1, &idx1, &op, &var2, &idx2) != 5)
            error("comando invalido", line);
          printf("%d %c%d = %c%d %c %c%d\n", 
                 line, var0, idx0, var1, idx1, op, var2, idx2);
          i += operacao(codigo + i, var0, idx0, var1, idx1, op, var2, idx2);
        }
        break;
      }
      case 'i': { /* desvio condicional */
        char var0;
        int idx0, n;
        if (fscanf(f, "flez %c%d %d", &var0, &idx0, &n) != 3)
            error("comando invalido", line);
          i += condicional(codigo + i, i, var0, idx0, n, desvioLine, &desvioN);
          printf("%d iflez %c%d %d\n", line, var0, idx0, n);
        break;
      }
      default: error("comando desconhecido", line);
    }
    line ++;
    fscanf(f, " ");
  }

//   printf("tam:%d\n", i);
// insere os offsets dos desvios caso existam
  escreve_desvios(codigo, desvioLine, desvioN, lineEnd);

  return (funcp)codigo;
}

/*
COMPILAÇÃO
Fedora Linux 42 (Workstation Edition) x86_64
Kernel: 6.14.9-300.fc42.x86_64
CPU: 11th Gen Intel i5-1135G7 (8) @ 4.200GHz
gcc (GCC) 15.1.1 20250521 (Red Hat 15.1.1-2)

gcc -Wall -Wa,--execstack -o testapeqcomp testapeqcomp.c peqcomp.c
*/