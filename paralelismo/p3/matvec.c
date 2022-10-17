#include <stdio.h>
#include <sys/time.h>
#include <mpi.h>
#define DEBUG 1

#define N 1024

int main(int argc, char *argv[] ) {
  
  MPI_Init(&argc,&argv);
  int rank,nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  int i,j;
  int cnt= N/nprocs;
  float matrix[N][N],matrix2[cnt][N];
  float vector[N];
  float result[N],result2[cnt];
  struct timeval  tv1_computacion, tv2_computacion,tv1_comunicacion,tv2_comunicacion;

  /* Initialize Matrix and Vector */
  if(rank==0){
  for(i=0;i<N;i++) {
    vector[i] = i;
    for(j=0;j<N;j++) {
      matrix[i][j] = i+j;
    }
  }
  }
  gettimeofday(&tv1_comunicacion, NULL);
  
  MPI_Scatter(matrix,cnt*N,MPI_FLOAT,matrix2,cnt*N,MPI_FLOAT,0,MPI_COMM_WORLD);
  MPI_Bcast(vector,N, MPI_FLOAT,0,MPI_COMM_WORLD);

  gettimeofday(&tv1_computacion, NULL);

  for(i=0;i<cnt;i++) {
    result2[i]=0;
    for(j=0;j<N;j++) {
      result2[i] += matrix2[i][j]*vector[j];
    }
  }
  gettimeofday(&tv2_computacion, NULL);
  
  MPI_Gather(result2,cnt,MPI_FLOAT,result,cnt,MPI_FLOAT,0,MPI_COMM_WORLD);
  
  gettimeofday(&tv2_comunicacion, NULL);
    
  int microseconds_computacion = (tv2_computacion.tv_usec - tv1_computacion.tv_usec)+ 1000000 * (tv2_computacion.tv_sec - tv1_computacion.tv_sec);
  int microseconds_comunicacion = (tv2_comunicacion.tv_usec - tv1_comunicacion.tv_usec)+ 1000000 * (tv2_comunicacion.tv_sec - tv1_comunicacion.tv_sec);

  /*Display result */
  if (DEBUG){
    if(rank==0){
     for(i=0;i<N;i++) {
      printf(" %f \t ",result[i]);
     }
    }
  } else {
    printf ("Computation Time (seconds) = %lf\n", (double) microseconds_computacion/1E6);
    
    printf ("Communication Time (seconds) = %lf\n", (double) microseconds_comunicacion/1E6);
  }    
  MPI_Finalize();
  return 0;
}

