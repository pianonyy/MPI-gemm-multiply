#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#define NRA 10                  /* number of rows in matrix A */
#define NCA 20                 /* number of columns in matrix A */
#define NCB 30                  /* number of columns in matrix B */

#define MASTER 0               /* taskid of first task */
#define FROM_HOST 1          /* setting a message type */
#define FROM_WORKER 2          /* setting a message type */


#define ALPHA 1.2
#define BETA 1.5

int killed_proc, killed_line;
int taskid;

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


static
void die_try(int curr_line) {
    if (taskid== killed_proc) {
        if (curr_line == killed_line) {
            printf("Proc: %d \tRUNNING line in A matrix: %d \t BUT I AM SUICIDING 'BYE'\n", taskid, curr_line);
            fflush(stdout);
            raise(SIGKILL);
        }
    }
    printf("Rank: %d \tRUNNING line: %d\n", taskid, curr_line);
    fflush(stdout);
}



int main (int argc, char *argv[])
{
int	numtasks,              /* number of tasks in partition */
		numworkers,            /* number of worker tasks */
	source,                /* task id of message source */
	dest,                  /* task id of message destination */
	mtype,                 /* message type */
	rows,                  /* rows of matrix A sent to each worker */
	averow, extra, offset, /* used to determine rows sent to each worker */
	i, j, k, rc;           /* misc */
double	a[NRA][NCA],           /* matrix A to be multiplied */
	b[NCA][NCB],           /* matrix B to be multiplied */
	c[NRA][NCB];           /* result matrix C */
MPI_Status status;

MPI_Init(&argc,&argv);
MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

if (numtasks < 2 ) {
  printf("Need at least two MPI tasks. Quitting...\n");
  MPI_Abort(MPI_COMM_WORLD, rc);
  exit(1);
  }
numworkers = numtasks-1;


/**************************** master task ************************************/
   if (taskid == MASTER)
   {
      printf("start with %d tasks.\n",numtasks);
      printf("Initializing arrays...\n");
      for (i=0; i<NRA; i++)
         for (j=0; j<NCA; j++)
            a[i][j]= (i + j)/20;

      for (i=0; i<NCA; i++)
         for (j=0; j<NCB; j++)
            b[i][j]= (2* i + 1)/20;

      for (i=0; i<NRA; i++)
         for (j=0; j<NCB; j++)
            c[i][j]= 2 * i + 5;

      srand(time(NULL));
      killed_proc = rand() % (numtasks);
      printf("proc to be killed %d\n", killed_proc);

      killed_line = rand() % (NRA);
      printf("proc to be killed %d\n", killed_line);

      printf("A Matrix:\n");
      mat_print(NRA, NCA,a);

      printf("B Matrix:\n");
      mat_print(NCA, NCB,b);

      printf("C Matrix:\n");
      mat_print(NRA, NCB,c);
      /* Send matrix data to the worker tasks */
      averow = NRA/numworkers;
      extra = NRA%numworkers;
      offset = 0;
      mtype = FROM_HOST;
      for (dest=1; dest<=numworkers; dest++)
      {
         rows = (dest <= extra) ? averow+1 : averow;
         printf("Sending %d rows to task %d offset=%d\n",rows,dest,offset);
         MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&a[offset][0], rows*NCA, MPI_DOUBLE, dest, mtype,
                   MPI_COMM_WORLD);
         MPI_Send(&b, NCA*NCB, MPI_DOUBLE, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&c[offset][0], rows*NCB, MPI_DOUBLE, dest, mtype, MPI_COMM_WORLD);
         offset = offset + rows;
      }

      /* Receive results from worker tasks */
      mtype = FROM_WORKER;
      for (i=1; i<=numworkers; i++)
      {
         source = i;
         MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
         MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
         MPI_Recv(&c[offset][0], rows*NCB, MPI_DOUBLE, source, mtype,
                  MPI_COMM_WORLD, &status);
         printf("Received results from task %d\n",source);
      }

      /* Print results */
      printf("******************************************************\n");
      printf("Result Matrix:\n");
      for (i=0; i<NRA; i++)
      {
         printf("\n");
         for (j=0; j<NCB; j++)
            printf("%6.2f   ", c[i][j]);
      }
      printf("\n******************************************************\n");
      printf ("Done.\n");
   }


/**************************** worker task ************************************/

   if (taskid > MASTER)
   {
      mtype = FROM_HOST;
      MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&a, rows*NCA, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&b, NCA*NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&c, rows * NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);

      //debug print-------------------------------------------------------------

      // printf("proc %d rec A matrix\n", taskid );
      // mat_print(rows,NCA,a);
      // fflush(stdout);
      //
      // printf("proc %d rec B matrix\n", taskid );
      // mat_print(NCA,NCB,b);
      // fflush(stdout);
      //
      // printf("proc %d rec C matrix\n", taskid );
      // mat_print(NRA,NCB,c);
      // fflush(stdout);

      for (int i = 0; i < rows; i++){
        for (int k = 0; k < NCB; k++)
        {
            c[i][k] *= BETA;

            //die_try(i);
            for (int j = 0; j < NCA; j++){


               c[i][k] = c[i][k] + ALPHA * a[i][j] * b[j][k];
            }

         }

      }

      //debug print-------------------------------------------------------------
      // printf("proc %d compute C matrix\n",taskid );
      // mat_print(rows,NCB,c);
      // fflush(stdout);

      mtype = FROM_WORKER;
      MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&c, rows*NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD);
   }



   MPI_Finalize();
}
