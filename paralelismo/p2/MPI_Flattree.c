
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
int MPI_Flattree(void * buffer, void *rec_buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
        int numprocs, rank;
        int *rcbuf = rec_buffer;
        MPI_Status error_st;
        MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        int *count_sum[numprocs - 1];

        if (rank != 0) {
            MPI_Send(buffer, count, datatype, root, 100, comm);
        }
        else if (rank == 0){ 
            count_sum[0] = buffer;
            *rcbuf = *count_sum[0];
            for (int i = 1; i < numprocs; i++) {  
                MPI_Recv(buffer, count, datatype, i, 100, comm, &error_st);
                count_sum[i] = buffer;
                *rcbuf += *count_sum[i];
            }
        }
        return MPI_SUCCESS;
}  
int MPI_BinomialBcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int error, i;
    int numprocs, rank;
    int receiver, sender;

    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i = 1; i <=ceil(log2(numprocs)); i++)
        if (rank < pow(2, i-1))
        {
            receiver = rank + pow(2, i-1);
            if (receiver < numprocs)
            {
                error = MPI_Send(buf, count, datatype, receiver, 0, comm);
                if (error != MPI_SUCCESS) return error;
            }
        } else
        {
            if (rank < pow(2, i))
            {
                sender = rank - pow(2, i-1);
                error = MPI_Recv(buf, count, datatype, sender, 0, comm, &status);
                if (error != MPI_SUCCESS) return error;
            }
        }
    return MPI_SUCCESS;
}   
int main(int argc, char *argv[])
{
    MPI_Init(&argc,&argv);
    int i, done = 0, n, count,aux;
    int rank,nprocs;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    while (1)
    {
        if(rank==0) {
            printf("Enter the number of points: (0 quits) \n");
            scanf("%d", &n);
        }
        MPI_BinomialBcast(&n,1, MPI_INT,0,MPI_COMM_WORLD);
        if (n == 0) break;

        count = 0;

        for (i = rank+1; i <= n; i+=nprocs) {
            // Get the random numbers between 0 and 1
            x = ((double) rand()) / ((double) RAND_MAX);
            y = ((double) rand()) / ((double) RAND_MAX);

            // Calculate the square root of the squares
            z = sqrt((x*x)+(y*y));

            // Check whether z is within the circle
            if(z <= 1.0)
                count++;
        }
        MPI_Flattree(&count,&aux,1,MPI_INT,0,MPI_COMM_WORLD); 
        if(rank==0){  
           pi = ((double) aux/(double) n)*4.0;

           printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        }
    }
    MPI_Finalize();
}
