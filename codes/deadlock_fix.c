#include <stdio.h>
#include <mpi.h>
#include <string.h>

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	int rank, size;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	char buf[100];	
	if (rank == 0)
	{
		sleep(1);
		sprintf(buf, "Hello from &d", rank);
		MPI_Recv(buf, 100, MPI_CHAR, (rank - 1 + size) % size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Send(buf, strlen(buf)+1, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD); // в конце /0, поэтому +1
		printf("#%d: %s\n", rank, buf);
	}
	else 
	{
		sprintf(buf, "Hello from &d", rank);
		MPI_Send(buf, strlen(buf)+1, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD); // в конце /0, поэтому +1
		MPI_Recv(buf, 100, MPI_CHAR, (rank - 1 + size) % size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("#%d: %s\n", rank, buf);
		sleep(1);
	}

	MPI_Finalize();
	return 0;
}
