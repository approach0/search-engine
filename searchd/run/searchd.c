#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <mpi.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "timer/timer.h"

#include "search/config.h"
#include "search/search.h"
#include "httpd/httpd.h"
#include "wstring/wstring.h"

#include "config.h"
#include "utils.h"

struct searchd_args {
	int  n_nodes, node_rank;
	struct indices *indices;
	int trec_log;
};

static const char *
httpd_on_recv(const char *req, void *arg_)
{
	P_CAST(args, struct searchd_args, arg_);
	const char      *ret = NULL;
	struct query     qry;
	int              page;
	ranked_results_t srch_res; /* search results */
	struct timer     timer;

	/* start timer */
	timer_reset(&timer);

#ifdef SEARCHD_LOG_ENABLE
	FILE *log_fh = fopen(SEARCHD_LOG_FILE, "a");

	if (log_fh == NULL) {
		fprintf(stderr, "cannot open %s.\n", SEARCHD_LOG_FILE);
		log_fh = fopen("/dev/null", "a");
	}

	fprintf(log_fh, "%s\n", req); fflush(log_fh);
	log_json_qry_ip(log_fh, req); fprintf(log_fh, "\n");
#endif

	/* parse JSON query into local query structure */
	qry  = query_new();
	page = parse_json_qry(req, &qry);

	if (page == 0) {
#ifdef SEARCHD_LOG_ENABLE
		fprintf(log_fh, "requested JSON parse error.\n");
#endif
		ret = search_errcode_json(SEARCHD_RET_BAD_QRY_JSON);
		goto reply;

	} else if (qry.len == 0) {
#ifdef SEARCHD_LOG_ENABLE
		fprintf(log_fh, "resulted qry of length zero.\n");
#endif
		ret = search_errcode_json(SEARCHD_RET_EMPTY_QRY);
		goto reply;

	} else if (qry.n_math > MAX_ACCEPTABLE_MATH_KEYWORDS) {
#ifdef SEARCHD_LOG_ENABLE
		fprintf(log_fh, "qry contains too many math keywords.\n");
#endif
		ret = search_errcode_json(SEARCHD_RET_TOO_MANY_MATH_KW);
		goto reply;

	} else if (qry.n_term > MAX_ACCEPTABLE_TERM_KEYWORDS) {
#ifdef SEARCHD_LOG_ENABLE
		fprintf(log_fh, "qry contains too many term keywords.\n");
#endif
		ret = search_errcode_json(SEARCHD_RET_TOO_MANY_TERM_KW);
		goto reply;
	}

#ifdef SEARCHD_LOG_ENABLE
	fprintf(log_fh, "requested page: %d\n", page);
	fprintf(log_fh, "parsed query: \n");
	query_print(qry, log_fh);
	fprintf(log_fh, "\n");
#endif

	/* is there a cluster? */
	if (args->n_nodes > 1) {

		/*  and this is the master node? */
		if (args->node_rank == CLUSTER_MASTER_NODE) {
			/* broadcast to slave nodes */
			void *send_buf = malloc(CLUSTER_MAX_QRY_BUF_SZ);
			strcpy(send_buf, req);
			MPI_Bcast(send_buf, CLUSTER_MAX_QRY_BUF_SZ, MPI_BYTE,
					  CLUSTER_MASTER_NODE, MPI_COMM_WORLD);
			free(send_buf);
		}

		/* ask engine to return all top K results */
		page = 0;
	}

	/* search query */
	srch_res = indices_run_query(args->indices, &qry);

	//////// TREC LOG ////////
	if (args->trec_log) {
		printf("generating TREC log ...\n");
		search_results_trec_log(&srch_res, args->indices);
	}
	//////////////////////////

	/* generate response JSON */
	ret = search_results_json(&srch_res, page - 1, args->indices);
	free_ranked_results(&srch_res);

	/* is there a cluster? */
	if (args->n_nodes > 1) {
		/* allocate results receving buffer if this is master node */
		char *gather_buf = NULL;
		if (args->node_rank == CLUSTER_MASTER_NODE)
			gather_buf = malloc(args->n_nodes * MAX_SEARCHD_RESPONSE_JSON_SZ);

		/* send and gather search results from each node */
		MPI_Gather(ret, MAX_SEARCHD_RESPONSE_JSON_SZ, MPI_BYTE,
		           gather_buf, MAX_SEARCHD_RESPONSE_JSON_SZ, MPI_BYTE,
		           CLUSTER_MASTER_NODE, MPI_COMM_WORLD);
		/* blocks until all nodes have reached here */
		MPI_Barrier(MPI_COMM_WORLD);

		if (args->node_rank == CLUSTER_MASTER_NODE) {
			/* merge gather results and return */
			ret = json_results_merge(gather_buf, args->n_nodes);
			free(gather_buf);
		}
	}

reply:
	query_delete(qry);

#ifdef SEARCHD_LOG_ENABLE
	fprintf(log_fh, "query handle time cost: %ld msec.\n",
	        timer_tot_msec(&timer));
	fprintf(log_fh, "unfree allocs: %ld.\n", mhook_unfree());

	fprintf(log_fh, "\n");
	fclose(log_fh);
#endif

	return ret;
}

static void slave_run(struct searchd_args *args)
{
	void *recv_buf = malloc(CLUSTER_MAX_QRY_BUF_SZ);
	printf("slave#%d ready.\n", args->node_rank);

	/* receive query from master node */
	MPI_Bcast(recv_buf, CLUSTER_MAX_QRY_BUF_SZ, MPI_BYTE,
	          CLUSTER_MASTER_NODE, MPI_COMM_WORLD);

	/* simulate http request */
	(void)httpd_on_recv(recv_buf, args);
	free(recv_buf);
}

int main(int argc, char *argv[])
{
	int                   opt;
	char                 *index_path = NULL;
	char                 *index_path_i = NULL;
	char                 *index_path_j = NULL;
	struct indices        indices;
	size_t                cache_sz = SEARCHD_DEFAULT_CACHE_MB;
	unsigned short        port = SEARCHD_DEFAULT_PORT;
	struct searchd_args   searchd_args;
	int                   trec_log = 0;

	/* parse program arguments */
	while ((opt = getopt(argc, argv, "hTi:j:t:p:c:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("search daemon.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -p <port> | "
			       " -i <index path> |"
			       " -j <index path> |"
			       " -c <cache size (MB)> | "
			       " -T (TREC log) |"
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'T':
			trec_log = 1;
			break;

		case 'i':
			index_path_i = strdup(optarg);
			break;

		case 'j':
			index_path_j = strdup(optarg);
			break;

		case 'p':
			sscanf(optarg, "%hu", &port);
			break;

		case 'c':
			sscanf(optarg, "%lu", &cache_sz);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/* initialize MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &searchd_args.n_nodes);
	MPI_Comm_rank(MPI_COMM_WORLD, &searchd_args.node_rank);

	/* check number of cluster nodes */
	if (searchd_args.n_nodes > 2) {
		printf("Cluster: Too many nodes!\n");
		goto final;
	}

	/* choose index path if there is a slave node */
	switch (searchd_args.node_rank) {
	case CLUSTER_MASTER_NODE + 0:
		index_path = index_path_i;
		break;

	case CLUSTER_MASTER_NODE + 1:
		index_path = index_path_j;
		break;

	default:
		break;
	}

	/* check index path argument */
	if (index_path == NULL) {
		fprintf(stderr, "indices path not specified.\n");
		goto final;
	}

	/* open indices */
	printf("opening index at: `%s' ...\n", index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	/* setup cache */
	printf("setup cache size: %lu MB\n", cache_sz);
	postlist_cache_set_limit(&indices.ci, cache_sz MB, 0);;
	indices_cache(&indices);

	/* set searchd args */
	searchd_args.indices  = &indices;
	searchd_args.trec_log = trec_log;

	if (searchd_args.node_rank == CLUSTER_MASTER_NODE) {
		/* master node, run httpd */
		struct uri_handler uri_handlers[] = {{SEARCHD_DEFAULT_URI, &httpd_on_recv}};

		printf("listening on port %hu ...\n", port);
		fflush(stdout); /* notify others (expect script) */

		if (0 != httpd_run(port, uri_handlers, 1, &searchd_args))
			printf("port %hu is occupied\n", port);

	} else {
		/* slave node */
		slave_run(&searchd_args);
	}

close:
	/* close indices */
	printf("closing index...\n");
	indices_close(&indices);

final:
	/* close MPI  */
	MPI_Finalize();

exit:
	/*
	 * free program arguments
	 */
	free(index_path_i);
	free(index_path_j);

	mhook_print_unfree();
	fflush(stdout);
	return 0;
}
