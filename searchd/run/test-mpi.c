#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#define CLUSTER_MASTER_NODE 0

int main(int argc, char* argv[])
{
	int n_nodes, node_rank;
	int qry, res;

	if (argc != 2) {
		printf("bad arg.\n");
		return 1;
	}

	sscanf(argv[1], "%d", &qry);

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &n_nodes);
	MPI_Comm_rank(MPI_COMM_WORLD, &node_rank);

	if (node_rank == CLUSTER_MASTER_NODE)
		printf("query: %d\n", qry);

	MPI_Bcast(&qry, sizeof(int), MPI_BYTE,
	          CLUSTER_MASTER_NODE, MPI_COMM_WORLD);
	// MPI_Barrier(MPI_COMM_WORLD);

	res = qry * (node_rank + 1);

	void *recv_buf = NULL;
	if (node_rank == CLUSTER_MASTER_NODE)
		recv_buf = (int *)malloc(n_nodes * sizeof(int));

	MPI_Gather(&res, sizeof(int), MPI_BYTE,
	           recv_buf, sizeof(int), MPI_BYTE,
	/* Receive count ------^ is the count of elements
	* received per node, not the total summation of
	* counts from all nodes. */
	           CLUSTER_MASTER_NODE, MPI_COMM_WORLD);

	if (node_rank == CLUSTER_MASTER_NODE) {
		for (int i = 0; i < n_nodes; i++) {
			int *r = (int *)recv_buf;
			printf("node[%d] result: %d.\n", i, r[i]);
		}
		free(recv_buf);
	}

	MPI_Finalize();
}
