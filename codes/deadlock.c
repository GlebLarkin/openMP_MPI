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

	sprintf(buf, "Hello from &d", rank);
	MPI_Ssend(buf, strlen(buf)+1, MPI_CHAR, (rank + 1) % size, 0, MPI_COMM_WORLD); // в конце /0, поэтому +1
	MPI_Recv(buf, 100, MPI_CHAR, (rank - 1 + size) % size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);// почему можно в тот же buf?
	printf("#%d: %s\n", rank, buf);

	MPI_Finalize();
	return 0;
}
