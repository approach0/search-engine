#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "mhook/mhook.h"
#include "timer/timer.h"

#include "search/config.h"
#include "search/search.h"
#include "httpd/httpd.h"

#include "config.h"
#include "utils.h"

const char *httpd_on_recv(const char* req, void* arg_)
{
	P_CAST(args, struct searcher_args, arg_);
	const char      *ret = NULL;
	struct query     qry;
	uint32_t         page;
	ranked_results_t srch_res; /* search results */
	struct timer     timer;

#ifdef SEARCHD_LOG_ENABLE
	FILE *log_fh = fopen(SEARCHD_LOG_FILE, "a");

	if (log_fh == NULL) {
		fprintf(stderr, "cannot open %s.\n", SEARCHD_LOG_FILE);
		log_fh = fopen("/dev/null", "a");
	}

	fprintf(log_fh, "JSON query: ");
	fprintf(log_fh, "%s\n", req);
	fflush(log_fh);

	fprintf(log_fh, "relay from: ");
	log_json_qry_ip(log_fh, req);
	fprintf(log_fh, "\n");
#endif

	/* start timer */
	timer_reset(&timer);

	/* parse JSON query into local query structure */
	qry = query_new();
	page = parse_json_qry(req, args->lex, &qry);

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
	fprintf(log_fh, "requested page: %u\n", page);
	fprintf(log_fh, "parsed query: \n");
	query_print_to(qry, log_fh);
	fprintf(log_fh, "\n");
	fflush(log_fh);
#endif

	/* search query */
#ifdef SEARCHD_LOG_ENABLE
	fprintf(log_fh, "run query...\n");
	fflush(log_fh);
#endif

	srch_res = indices_run_query(args->indices, &qry);

	/* generate response JSON */
#ifdef SEARCHD_LOG_ENABLE
	fprintf(log_fh, "return results...\n");
	fflush(log_fh);
#endif
	ret = search_results_json(&srch_res, page - 1, args);

	/* free ranked results */
#ifdef SEARCHD_LOG_ENABLE
	fprintf(log_fh, "free results...\n");
	fflush(log_fh);
#endif
	free_ranked_results(&srch_res);

reply:
#ifdef SEARCHD_LOG_ENABLE
	fprintf(log_fh, "delete query...\n");
	fflush(log_fh);
#endif
	query_delete(qry);

#ifdef SEARCHD_LOG_ENABLE
	fprintf(log_fh, "query handled, "
	        "unfree allocs: %ld.\n", mhook_unfree());
	fflush(log_fh);

	fprintf(log_fh, "query handle time cost: %ld msec.\n",
	        timer_tot_msec(&timer));

	fprintf(log_fh, "\n");
	fclose(log_fh);
#endif
	return ret;
}

int main(int argc, char *argv[])
{
	int                   opt;
	char                 *index_path = NULL;
	struct indices        indices;
	unsigned short        cache_sz = SEARCHD_DEFAULT_CACHE_MB;
	unsigned short        port = SEARCHD_DEFAULT_PORT;
	text_lexer            lex = lex_eng_file;
	char                 *dict_path = NULL;
	struct searcher_args  searcher_args;

	/* parse program arguments */
	while ((opt = getopt(argc, argv, "hi:t:p:c:d:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("search daemon.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -i <index path> |"
			       " -p <port> | "
			       " -c <cache size (MB)> | "
			       " -d <dict> "
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'i':
			index_path = strdup(optarg);
			break;

		case 'p':
			sscanf(optarg, "%hu", &port);
			break;

		case 'c':
			sscanf(optarg, "%hu", &cache_sz);
			break;

		case 'd':
			dict_path = strdup(optarg);
			lex = lex_mix_file;
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/* check program arguments */
	if (index_path == NULL) {
		fprintf(stderr, "indices path not specified.\n");
		goto exit;
	}

	/* open text-segment dictionary if needed */
	if (lex == lex_mix_file) {
		printf("opening dictionary...\n");
		if (text_segment_init(dict_path)) {
			fprintf(stderr, "cannot open dict.\n");
			goto exit;
		}
	}

	/* open indices */
	printf("opening index at: `%s' ...\n", index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	/* setup cache */
	printf("setup cache size: %hu MB\n", cache_sz);
	indices_cache(&indices, cache_sz MB);

	/* run httpd */
	printf("listen on port %hu\n", port);

	searcher_args.indices = &indices;
	searcher_args.lex     = lex;
	struct uri_handler uri_handlers[] = {{"/search" , &httpd_on_recv}};
	httpd_run(port, uri_handlers, 1, &searcher_args);

close:
	/* close indices */
	printf("closing index...\n");
	indices_close(&indices);

	/* close text-segment dictionary if opened */
	if (lex == lex_mix_file) {
		printf("closing dictionary...\n");
		text_segment_free();
	}

exit:
	/*
	 * free program arguments
	 */
	free(index_path);
	free(dict_path);

	mhook_print_unfree();
	return 0;
}
