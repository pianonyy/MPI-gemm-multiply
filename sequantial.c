#include <stdio.h>
#include <stdlib.h>


#define NRA 10                 /* number of rows in matrix A */
#define NCA 20                 /* number of columns in matrix A */
#define NCB 30                  /* number of columns in matrix B */


#define ALPHA 1.2
#define BETA 1.5

void mat_print(int n_row, int n_col, double matrix[n_row][n_col]){

  for(int i = 0; i < n_row; i++)
  {
    printf("\n");
    for(int j = 0; j < n_col; j++)
    {
      printf("%6.2f", matrix[i][j]);
    }

  }
  printf("\n_________________________________________________________\n");
}


int main(int argc, char *argv[]) {
  double	a[NRA][NCA],
  	      b[NCA][NCB],
  	      c[NRA][NCB];

  for (int i = 0; i < NRA; i++)
     for (int j =  0; j < NCA; j++)
        a[i][j]= (i + j)/20;

  for (int i = 0; i < NCA; i++)
     for (int j = 0; j < NCB; j++)
        b[i][j]= (2* i + 1)/20;

  for (int i = 0; i < NRA; i++)
     for (int j = 0; j < NCB; j++)
        c[i][j] = 2 * i + 5;



  printf("A Matrix:\n");
  mat_print(NRA, NCA,a);

  printf("B Matrix:\n");
  mat_print(NCA, NCB,b);

  printf("C Matrix:\n");
  mat_print(NRA, NCB,c);

  for (int i=0; i<NRA; i++){
    for (int k=0; k<NCB; k++)
    {
        c[i][k] *= BETA;

        for (int j=0; j < NCA; j++){

           c[i][k] = c[i][k] + ALPHA * a[i][j] * b[j][k];
        }
     }
  }
  printf("C=ALPHA*A*B+BETA*C ResultMatrix:\n");
  mat_print(NRA, NCB,c);
  return 0;
}
