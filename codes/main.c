#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	int rank, size, len;
	char name[MPI_MAX_PROCESSOR_NAME];

	MPI_Get_processor_name(name, &len);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	printf("Hello, world!\n");
	printf("name = %s, rank = %d, size = %d\n", name, rank, size);

	MPI_Finalize();
	return 0;
}
