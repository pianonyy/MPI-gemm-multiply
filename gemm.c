#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#define NRA 100                 /* number of rows in matrix A */
#define NCA 200                /* number of columns in matrix A */
#define NCB 300                  /* number of columns in matrix B */

#define MASTER 0               /* taskid of first task */
#define FROM_HOST 1          /* setting a message type */
#define FROM_WORKER 2          /* setting a message type */
#define RECOVERY_COMM 3       /* setting a message type */


#define ALPHA 1.2
#define BETA 1.5

int killed_proc, killed_line;
int taskid;
int numworkers, numtasks;

int rc, len;
char errstr[MPI_MAX_ERROR_STRING];

void mat_print(int n_row, int n_col, float matrix[n_row][n_col]){

  for(int i = 0; i < n_row; i++)
  {
    printf("\n");
    for(int j = 0; j < n_col; j++)
    {
      printf("%.6f ", matrix[i][j]);
    }

  }
  printf("\n_________________________________________________________\n");
}


static void die_try(int curr_line) {
        if (curr_line == killed_line) {
            printf("killed line %d\n", killed_line );
            printf("Proc: %d   line in A matrix: %d SIGKILLED\n", taskid, curr_line);
            fflush(stdout);
            raise(SIGKILL);
        }

    // printf("Rank: %d \tRUNNING line: %d\n", taskid, curr_line);
    fflush(stdout);
}






int main (int argc, char *argv[])
{
   int	source,                /* task id of message source */
  	dest,                  /* task id of message destination */
  	mtype,                 /* message type */
  	rows,                  /* rows of matrix A sent to each worker */
  	averow, extra, offset, /* used to determine rows sent to each worker */
  	i, j, k, rc;           
   float	a[NRA][NCA],           /* matrix A to be multiplied */
  	b[NCA][NCB],           /* matrix B to be multiplied */
  	c[NRA][NCB];           /* result matrix C */
   MPI_Status status;

   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
   MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
   MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

   MPI_Barrier(MPI_COMM_WORLD);

   if (numtasks < 2 ) {
     printf("Need at least two MPI tasks. Quitting...\n");
     MPI_Abort(MPI_COMM_WORLD, rc);
     exit(1);
   }

   numworkers = numtasks-1;

   srand(20);
   //killed_proc = 1 + rand() % (numworkers -1);
   // printf("proc to be killed %d\n", killed_proc);

   killed_line = rand() % (NRA- (NRA / (numworkers - 1)) );


/**************************** master task ************************************/
   if (taskid == MASTER)
   {
      printf("start with %d tasks.\n",numtasks);
      printf("Initializing arrays...\n");
      fflush(stdout);
      for (i=0; i<NRA; i++)
         for (j=0; j<NCA; j++)
            a[i][j]= (i + j)/20;

      for (i=0; i<NCA; i++)
         for (j=0; j<NCB; j++)
            b[i][j]= (2* i + 1)/20;

      for (i=0; i<NRA; i++)
         for (j=0; j<NCB; j++)
            c[i][j]= 2 * i + 5;

      printf("line to be killed %d\n", killed_line);
      fflush(stdout);

      printf("A Matrix:\n");
      mat_print(NRA, NCA,a);
      fflush(stdout);

      printf("B Matrix:\n");
      mat_print(NCA, NCB,b);
      fflush(stdout);

      printf("C Matrix:\n");
      mat_print(NRA, NCB,c);
      fflush(stdout);

      /* Send matrix data to the worker tasks */
      averow = NRA/(numworkers-1);
      extra = NRA%(numworkers-1);
      offset = 0;
      mtype = FROM_HOST;
      for (dest=1; dest<=(numworkers-1); dest++)
      {
         rows = (dest <= extra) ? averow + 1 : averow;
         printf("Sending %d rows to task %d offset=%d\n",rows,dest,offset);
         MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&a[offset][0], rows*NCA, MPI_FLOAT, dest, mtype,
                   MPI_COMM_WORLD);
         MPI_Send(&b, NCA*NCB, MPI_FLOAT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&c[offset][0], rows*NCB, MPI_FLOAT, dest, mtype, MPI_COMM_WORLD);
         offset = offset + rows;
      }

      /* Receive results from worker tasks */


      for (i = 1; i <= (numworkers - 1); i++)
      {
         source = i;
         rc = MPI_Recv(&offset, 1, MPI_INT, source, FROM_WORKER, MPI_COMM_WORLD, &status);
          //check the correct receive with result matrix

         if (rc != 0){
           MPI_Error_string(rc, errstr, &len);
           printf("Rank %d: Notified of error %s. Resend procedure has started\n",
                    source, errstr);
           fflush(stdout);
           averow = NRA / (numworkers-1);
           extra = NRA % (numworkers-1);
           offset = 0;
           for (dest = 1; dest <= (numworkers-1); dest++)
           {
               rows = (dest <= extra) ? averow+1 : averow;
               if (dest == source){

                 printf("Sending %d rows to task %d offset=%d\n",rows,numworkers,offset);
                 fflush(stdout);

                 MPI_Send(&offset, 1, MPI_INT, numworkers, RECOVERY_COMM, MPI_COMM_WORLD);
                 MPI_Send(&rows, 1, MPI_INT, numworkers, RECOVERY_COMM, MPI_COMM_WORLD);
                 MPI_Send(&a[offset][0], rows*NCA, MPI_FLOAT, numworkers, RECOVERY_COMM,
                           MPI_COMM_WORLD);
                 MPI_Send(&b, NCA*NCB, MPI_FLOAT, numworkers, RECOVERY_COMM, MPI_COMM_WORLD);
                 MPI_Send(&c[offset][0], rows*NCB, MPI_FLOAT, numworkers, RECOVERY_COMM, MPI_COMM_WORLD);
                 MPI_Send(&source, 1, MPI_INT, numworkers, RECOVERY_COMM, MPI_COMM_WORLD);
                 MPI_Send(&killed_line, 1, MPI_INT, numworkers, RECOVERY_COMM, MPI_COMM_WORLD);

                 printf("DONE !!!! Sent %d rows to task %d offset=%d\n",rows,numworkers,offset);
                 fflush(stdout);
               }
               offset = offset + rows;
           }

           /* receive from recovery proc(numworkers)*/
           MPI_Recv(&offset, 1, MPI_INT, numworkers, RECOVERY_COMM , MPI_COMM_WORLD, &status);
           MPI_Recv(&rows, 1, MPI_INT, numworkers, RECOVERY_COMM , MPI_COMM_WORLD, &status);
           MPI_Recv(&c[offset][0], rows*NCB, MPI_FLOAT, numworkers, RECOVERY_COMM ,
                    MPI_COMM_WORLD, &status);
           printf("Received results from task %d\n",numworkers);

         }
         else {
           MPI_Recv(&rows, 1, MPI_INT, source, FROM_WORKER, MPI_COMM_WORLD, &status);
           MPI_Recv(&c[offset][0], rows*NCB, MPI_FLOAT, source, FROM_WORKER,
                  MPI_COMM_WORLD, &status);
          printf("Received results from task %d\n",source);
         }


      }



      FILE *fptr = fopen("results.txt", "w");
      if (fptr == NULL)
      {
        printf("Could not open file");
        return 0;
      }

      for (int i = 0; i < NRA; i++)
      {
        fprintf(fptr,"\n");
        for(int j = 0;j < NCB; j++ ){

          fprintf(fptr,"%.6f ", c[i][j]);

        }
      }
      fprintf(fptr,"\n");
      fclose(fptr);

      // /* Print results */ standart output
      // printf("******************************************************\n");
      // printf("Result Matrix:\n");
      // for (i=0; i<NRA; i++)
      // {
      //    printf("\n");
      //    for (j=0; j<NCB; j++)
      //       printf("%.6f   ", c[i][j]);
      // }
      // printf("\n******************************************************\n");
      // printf ("Done.\n");
   }


/**************************** worker task ************************************/

   if ( (taskid > MASTER) && (taskid <= (numworkers - 1)) )
   {
      mtype = FROM_HOST;
      MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&a, rows*NCA, MPI_FLOAT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&b, NCA*NCB, MPI_FLOAT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&c, rows * NCB, MPI_FLOAT, MASTER, mtype, MPI_COMM_WORLD, &status);

     

      for (int i = 0; i < rows; i++){
        for (int k = 0; k < NCB; k++)
        {
            die_try(i + offset);
            c[i][k] *= BETA;


            for (int j = 0; j < NCA; j++){


               c[i][k] = c[i][k] + ALPHA * a[i][j] * b[j][k];
            }

         }
         // create control point------------------------------------------------

         char name[20];
         printf("num_proc %d computed_line %d\n", taskid, (i + offset) );
         snprintf(name, sizeof name, "proc_%d_%d.txt", taskid, (i + offset) );
         FILE *f = fopen(name, "w");

         for(int j = 0; j < NCB; j++) {
           fprintf(f, "%.6f\n", c[i][j]);
         }

         fclose(f);

      }

      //debug print-------------------------------------------------------------
      // printf("proc %d compute C matrix\n",taskid );
      // mat_print(rows,NCB,c);
      // fflush(stdout);

      mtype = FROM_WORKER;
      MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&c, rows*NCB, MPI_FLOAT, MASTER, mtype, MPI_COMM_WORLD);
   }

/**************************** recovery task ***********************************/
   if (taskid == numworkers){

     MPI_Status status;
     int broken_proc, broken_line;
     int i_start;

     MPI_Recv(&offset, 1, MPI_INT, MASTER, RECOVERY_COMM, MPI_COMM_WORLD, &status);
     MPI_Recv(&rows, 1, MPI_INT, MASTER, RECOVERY_COMM, MPI_COMM_WORLD, &status);
     MPI_Recv(&a, rows*NCA, MPI_FLOAT, MASTER, RECOVERY_COMM, MPI_COMM_WORLD, &status);
     MPI_Recv(&b, NCA*NCB, MPI_FLOAT, MASTER, RECOVERY_COMM, MPI_COMM_WORLD, &status);
     MPI_Recv(&c, rows * NCB, MPI_FLOAT, MASTER, RECOVERY_COMM, MPI_COMM_WORLD, &status);
     MPI_Recv(&broken_proc, 1, MPI_INT, MASTER, RECOVERY_COMM, MPI_COMM_WORLD, &status);
     MPI_Recv(&broken_line, 1, MPI_INT, MASTER, RECOVERY_COMM, MPI_COMM_WORLD, &status);

    

     char name[20];
     int lines;
     int for_rec = -3;
     printf("Proc: %d is finding need recovery file....\n", taskid);




     lines = broken_line;
     int i = 0;
     while (lines >= offset){
       snprintf(name, sizeof name, "proc_%d_%d.txt", broken_proc, lines);
       FILE *f = fopen(name, "r");
       if (f == NULL) {
         printf("not_found file proc_%d_%d.txt\n",broken_proc, lines);
         // i_start = lines - 1;

       }
       else{

         for_rec = lines - offset + 1;


         i_start = i + for_rec;

         fclose(f);
         break;
       }
       lines = lines - 1;
     }

     if (lines < offset){
       i_start = 0;
     }
     printf("for_rec = %d\n", for_rec );
     //scanf from recovery files
     float buf;
     while (for_rec > 0){
       printf("I'm HERE!!!!\n" );
       snprintf(name, sizeof name, "proc_%d_%d.txt", broken_proc, (for_rec + offset - 1) );
       FILE *f_rec = fopen(name, "r");



       for (int j = 0; j < NCB; j++) {
         fscanf(f_rec, "%f", &buf);
         printf("scaneed %f\n", buf);
         c[for_rec - 1][j] = buf;
       }
       printf("%s\n", name);
       for_rec--;
       fclose(f_rec);
     }

     printf("Recovery proc started with line %d\n", i_start);
     for (int i = i_start; i < rows; i++){
       for (int k = 0; k < NCB; k++)
       {

           c[i][k] *= BETA;


           for (int j = 0; j < NCA; j++){


              c[i][k] = c[i][k] + ALPHA * a[i][j] * b[j][k];
           }

        }
     }

     mtype = RECOVERY_COMM;
     MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
     MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
     MPI_Send(&c, rows * NCB, MPI_FLOAT, MASTER, mtype, MPI_COMM_WORLD);
   }



   MPI_Finalize();
   return 0;
}
