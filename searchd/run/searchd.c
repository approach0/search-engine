#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <mpi.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "timer/timer.h"

#include "search-v3/search.h"
#include "txt-seg/txt-seg.h"
#include "httpd/httpd.h"
#include "flock.h"

#include "config.h"
#include "json-utils.h"
#include "trec-res.h"

/*
 * request handler
 */
struct searchd_args {
	int  n_nodes, node_rank;
	struct indices *indices;
	FILE *log_fh;
	int trec_log;
	int topk;
};

static void print_index_stats(indices_run_sync_t *stats, FILE *fh)
{
	fprintf(fh, "docN=%u, avgDocLen=%u, pathN=%u\n",
		stats->docN, stats->avgDocLen, stats->pathN);

	for (int i = 0; i < MAX_MERGE_SET_SZ; i++) {
		uint32_t doc_freq = stats->doc_freq[i];
		fprintf(fh, "%u ", doc_freq);
	}
	fprintf(fh, "\n");
}

static void reduce_index_stats(indices_run_sync_t arr[], uint32_t n,
                               indices_run_sync_t *out)
{
	PTR_CAST(p, uint32_t, out);
	for (int i = 0; i < MAX_SYNC_ARR_LEN; i++) {
		uint32_t sum = 0;
		for (int j = 0; j < n; j++) {
			sum += *((uint32_t*)&arr[j] + i);
		}
		*p = sum;
		p++;
	}
	out->avgDocLen = out->avgDocLen / n;
}

static const char *
httpd_on_recv(const char *req, void *arg_)
{
	P_CAST(args, struct searchd_args, arg_);
	const char      *ret = NULL;
	struct query     qry = QUERY_NEW;
	int              page;
	ranked_results_t res; /* search results */
	struct timer     timer;
	FILE            *log_fh = args->log_fh;

	/* start timer */
	timer_reset(&timer);

	/* log raw query */
	fprintf(log_fh, "%s\n", req);
	fflush(log_fh);

	/* parse JSON query into local query structure */
	page = parse_json_qry(req, &qry);

	if (page == 0) {
		ret = search_errcode_json(SEARCHD_RET_BAD_QRY_JSON);
		fprintf(log_fh, "%s\n", "Bad JSON format (page = 0).");
		goto reply;

	} else if (qry.len == 0) {
		ret = search_errcode_json(SEARCHD_RET_EMPTY_QRY);
		fprintf(log_fh, "%s\n", "Empty query.");
		goto reply;

	} else if (qry.n_math > MAX_SEARCHD_MATH_KEYWORDS) {
		ret = search_errcode_json(SEARCHD_RET_TOO_MANY_MATH_KW);
		fprintf(log_fh, "%s\n", "Too many math keywords");
		goto reply;

	} else if (qry.n_term > MAX_SEARCHD_TERM_KEYWORDS) {
		ret = search_errcode_json(SEARCHD_RET_TOO_MANY_TERM_KW);
		fprintf(log_fh, "%s\n", "Too many text keywords");
		goto reply;
	}

	/* is there a cluster and this is the master node? */
	if (args->n_nodes > 1 && args->node_rank == CLUSTER_MASTER_NODE) {
		/* copy and broadcast to slave nodes */
		char *send_buf = malloc(CLUSTER_MAX_QRY_BUF_SZ);
		strncpy(send_buf, req, CLUSTER_MAX_QRY_BUF_SZ);
		send_buf[CLUSTER_MAX_QRY_BUF_SZ - 1] = '\0';

		MPI_Bcast(send_buf, CLUSTER_MAX_QRY_BUF_SZ, MPI_BYTE,
			CLUSTER_MASTER_NODE, MPI_COMM_WORLD);
		free(send_buf);
	}

	/* log parsed query */
	query_print(qry, log_fh);
	fflush(log_fh);

	/* if it is cluster, synchronize and sum index stats */
	indices_run_sync_t sync = {0};
	if (args->n_nodes > 1) {
		/* get this node index stats */
		res = indices_run_query(args->indices, &qry, 1, &sync, 1, log_fh);
		free_ranked_results(&res);
#ifdef DEBUG_LOG_STATS_SYNC
		fprintf(log_fh, "node[%d] stats: ", args->node_rank);
		print_index_stats(&sync, log_fh);
#endif

		/* gather each node stats and reduce to single global stats */
		indices_run_sync_t *gather_sync = NULL;
		if (args->node_rank == CLUSTER_MASTER_NODE)
			gather_sync = malloc(args->n_nodes * sizeof(indices_run_sync_t));

		MPI_Gather(&sync, MAX_SYNC_ARR_LEN, MPI_UINT32_T,
		           gather_sync, MAX_SYNC_ARR_LEN, MPI_UINT32_T,
		           CLUSTER_MASTER_NODE, MPI_COMM_WORLD);

		if (args->node_rank == CLUSTER_MASTER_NODE) {
			/* reduce index stats */
			reduce_index_stats(gather_sync, args->n_nodes, &sync);
#ifdef DEBUG_LOG_STATS_SYNC
			fprintf(log_fh, "reduced: ");
			print_index_stats(&sync, log_fh);
#endif
			free(gather_sync);
		}

		/* now distribute the global index stats among nodes */
		MPI_Bcast(&sync, MAX_SYNC_ARR_LEN, MPI_UINT32_T,
			CLUSTER_MASTER_NODE, MPI_COMM_WORLD);
	}

	/* actually perform search */
	res = indices_run_query(args->indices, &qry, args->topk, &sync, 0, log_fh);

	//////// TREC LOG ////////
	if (args->trec_log)
		search_results_trec_log(&res, args->indices);
	//////////////////////////

	if (args->n_nodes > 1) {
		/* ask engine to return all top K results */
		ret = search_results_json(&res, -1, args->indices);
	} else {
		/* generate response JSON for specific page only */
		ret = search_results_json(&res, page - 1, args->indices);
	}

	/* since results are converted to JSON string, we can free them now. */
	free_ranked_results(&res);

	/* is there a cluster? */
	if (args->n_nodes > 1) {
		/* allocate results receving buffer if this is master node */
		char *gather_buf = NULL;
		if (args->node_rank == CLUSTER_MASTER_NODE)
			gather_buf = malloc(args->n_nodes * MAX_SEARCHD_RESPONSE_JSON_SZ);

		/* search results (in ret) from each node (including master node) will
		 * be gathered in gather_buf */
		MPI_Gather(ret, MAX_SEARCHD_RESPONSE_JSON_SZ, MPI_BYTE,
		           gather_buf, MAX_SEARCHD_RESPONSE_JSON_SZ, MPI_BYTE,
		           CLUSTER_MASTER_NODE, MPI_COMM_WORLD);

		if (args->node_rank == CLUSTER_MASTER_NODE) {
			/* merge gather results and return */
			ret = json_results_merge(gather_buf, args->n_nodes, page);
			free(gather_buf);
		}
	}

reply:
	query_delete(qry);

	long time_cost = timer_tot_msec(&timer);
	fprintf(log_fh, "Query handle cost: %ld msec.\n", time_cost);
	fprintf(log_fh, "unfree allocs: %ld.\n\n", mhook_unfree());
	fflush(log_fh);

	return ret;
}

static void slave_run(struct searchd_args *args)
{
	char *recv_buf = malloc(CLUSTER_MAX_QRY_BUF_SZ);

	fprintf(args->log_fh, "Slave[%d] ready.\n", args->node_rank);
	fflush(args->log_fh);

	while (1) {
		/* receive request JSON from master node */
		MPI_Bcast(recv_buf, CLUSTER_MAX_QRY_BUF_SZ, MPI_BYTE,
				CLUSTER_MASTER_NODE, MPI_COMM_WORLD);

		/* master quits */
		if (recv_buf[0] == '\0')
			break;

		/* simulate http request */
		(void)httpd_on_recv(recv_buf, args);
	}

	free(recv_buf);
}

/* very unpleasant function name, but very descriptive :P */
static void signal_slaves_to_die()
{
	char *send_buf = malloc(CLUSTER_MAX_QRY_BUF_SZ);
	send_buf[0] = '\0'; /* empty buffer to signal die command */
	MPI_Bcast(send_buf, CLUSTER_MAX_QRY_BUF_SZ, MPI_BYTE,
			  CLUSTER_MASTER_NODE, MPI_COMM_WORLD);
	free(send_buf);
}

/*
 * Program main
 */
int main(int argc, char *argv[])
{
	int                   opt;
	struct indices        indices;
	struct searchd_args   searchd_args;
	FILE                 *log_fh = NULL;

	/* command line arguments */
	int                   trec_log = 0;
	char                 *index_path = NULL;
	char                 *lock_file = NULL;
	int                   lock_fd = -1;
	char                  dict_path[1024] = ".";
	unsigned short        port = SEARCHD_DEFAULT_PORT;
	size_t                mi_cache_limit = DEFAULT_MATH_INDEX_CACHE_SZ;
	size_t                ti_cache_limit = DEFAULT_TERM_INDEX_CACHE_SZ;
	int                   topk = DEFAULT_K_TOP_RESULTS;

	/* initialize MPI */
	MPI_Init(&argc, &argv);

	/* open searchd log file */
	log_fh = fopen(SEARCHD_LOG_FILE, "a");
	if (log_fh == NULL) {
		fprintf(stderr, "Cannot open %s.\n", SEARCHD_LOG_FILE);
		log_fh = fopen("/dev/null", "a");
	}

	/* parse program arguments */
	while ((opt = getopt(argc, argv, "hTi:L:d:p:c:C:k:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("search daemon.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s \n"
			       " -h (help) \n"
			       " -i <index path> (required) \n"
			       " -L <lock file> \n"
			       " -T (TREC log) \n"
			       " -d <dict path> \n"
			       " -p <port> \n"
			       " -c <term cache size (MB), default: %u MB> \n"
			       " -C <math cache size (MB), default: %u MB> \n"
			       " -k <keep top-K results> \n"
			       "\n", argv[0],
			       DEFAULT_TERM_INDEX_CACHE_SZ, DEFAULT_MATH_INDEX_CACHE_SZ);
			goto exit;

		case 'i':
			index_path = strdup(optarg);
			break;

		case 'L':
			lock_file = strdup(optarg);
			break;

		case 'T':
			trec_log = 1;
			break;

		case 'd':
			strcpy(dict_path, optarg);
			break;

		case 'p':
			sscanf(optarg, "%hu", &port);
			break;

		case 'c':
			sscanf(optarg, "%lu", &ti_cache_limit);
			break;

		case 'C':
			sscanf(optarg, "%lu", &mi_cache_limit);
			break;

		case 'k':
			sscanf(optarg, "%d", &topk);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/* get cluster information */
	MPI_Comm_size(MPI_COMM_WORLD, &searchd_args.n_nodes);
	MPI_Comm_rank(MPI_COMM_WORLD, &searchd_args.node_rank);

	/* check index path argument */
	if (index_path == NULL) {
		fprintf(stderr, "Indice path not specified.\n");
		goto exit;
	}

	/* obtain lock (if specified to) before opening indices */
	if (lock_file) {
		printf("Obtaining file lock at `%s' ...\n", lock_file);
		lock_fd = flock_exlock(lock_file);
		if (lock_fd == -1) {
			fprintf(stderr, "Unable to get lock file descriptor.\n");
			goto exit;
		}
		printf("Lock obtained.\n");
	}

	/* open indices */
	printf("node[%d] Opening index at: %s\n", searchd_args.node_rank, index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("Index open failed.\n");
		goto close;
	}

	/* open segmentation dictionary */
	printf("Open segment dictionary at [%s]\n", dict_path);
	text_segment_init(dict_path);

	/* setup cache */
	indices.ti_cache_limit = ti_cache_limit MB;
	indices.mi_cache_limit = mi_cache_limit MB;
	printf("Caching index (term %lu MiB, math %lu MiB) ...\n",
		ti_cache_limit, mi_cache_limit);
	indices_cache(&indices);
	indices_print_summary(&indices);
	printf("\n");

	/* set searchd args */
	searchd_args.indices  = &indices;
	searchd_args.trec_log = trec_log;
	searchd_args.log_fh   = log_fh;
	searchd_args.topk     = topk;

	/* should wait everyone before serving */
	MPI_Barrier(MPI_COMM_WORLD);

	if (searchd_args.node_rank == CLUSTER_MASTER_NODE) {
		/* master node, run httpd */
		struct uri_handler uri_handlers[] = {
			{SEARCHD_DEFAULT_URI, &httpd_on_recv}
		};

		printf("Listening on port %hu ...\n", port);
		fflush(stdout); /* notify others (expect script) */

		fprintf(log_fh, "Master node ready.\n"); fflush(log_fh);

		if (0 != httpd_run(port, uri_handlers, 1, &searchd_args))
			printf("Port %hu is occupied\n", port);

		signal_slaves_to_die();
	} else {
		/* slave node */
		signal(SIGUSR1, SIG_IGN);
		slave_run(&searchd_args);
	}

	/* free text segment */
	text_segment_free();

close:
	/* close indices */
	printf("node[%d] closing index...\n", searchd_args.node_rank);
	indices_close(&indices);

exit:
	/*
	 * free program arguments
	 */
	free(index_path);
	free(lock_file);
	flock_unlock(lock_fd);

	/* print unfree memory counter */
	mhook_print_unfree();
	fflush(stdout);

	/* close MPI  */
	MPI_Finalize();

	/* final log */
	fprintf(log_fh, "Exit, unfree allocs: %ld.\n\n", mhook_unfree());
	fclose(log_fh);
	return 0;
}
