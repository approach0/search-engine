#include <assert.h>
#include "common/common.h"
#include "tex-parser/head.h"
#include "config.h"
#include "math-search.h"

struct math_l2_invlist
*math_l2_invlist(math_index_t index, const char* tex, float *threshold)
{
	/* initialize math query */
	struct math_qry mq;
	if (0 != math_qry_prepare(index, tex, &mq)) {
		/* parsing failed */
		math_qry_release(&mq);
		return NULL;
	}

	/* allocate memory */
	struct math_l2_invlist *ret = malloc(sizeof *ret);

	/* save math query structure */
	ret->mq = mq;

	/* initialize scorer */
	math_score_precalc(&ret->msf);

	/* copy threshold address */
	ret->threshold = threshold;
	return ret;
}

void math_l2_invlist_free(struct math_l2_invlist *inv)
{
	math_qry_release(&inv->mq);
	free(inv);
}

/* use MaxScore merger */
typedef struct ms_merger *merger_set_iter_t;
#define merger_set_iterator   ms_merger_iterator
#define merger_set_iter_next  ms_merger_iter_next
#define merger_set_iter_free  ms_merger_iter_free
#define merger_set_map_follow ms_merger_map_follow

#ifdef DEBUG_MATH_SEARCH
static void keyprint(uint64_t k)
{
	printf("#%u, #%u, r%u", key2doc(k), key2exp(k), key2rot(k));
}

static int inspect(uint64_t k)
{
	uint d = key2doc(k);
	uint e = key2exp(k);
	uint r = key2rot(k);
	(void)d; (void)e; (void)r;
	//return (d == 116 && e == 228 && r == 20);
	return (d == 593 && e == 26);
}

static void print_symbinfo(struct symbinfo *symbinfo)
{
	printf("ophash: 0x%x: ", symbinfo->ophash);
	for (int i = 0; i < symbinfo->n_splits; i++) {
		int symbol = symbinfo->symbol[i];
		int width  = symbinfo->splt_w[i];
		printf("%s/%d ", trans_symbol(symbol), width);
	}
	printf("\n");
}
#endif

static FILE **duplicate_entries_fh_array(struct math_qry *mq)
{
	const int n = mq->merge_set.n;
	FILE **ret = malloc(n * sizeof(FILE*));
	for	(int i = 0; i < n; i++) {
		FILE *fh_symbinfo = mq->entry[i].fh_symbinfo;
		if (fh_symbinfo == NULL) {
			ret[i] = NULL;
		} else {
			int fd = fileno(fh_symbinfo);
			ret[i] = fdopen(dup(fd), "r");
			rewind(ret[i]);
		}
	}
	return ret;
}

math_l2_invlist_iter_t math_l2_invlist_iterator(struct math_l2_invlist *inv)
{
	if (merger_set_empty(&inv->mq.merge_set))
		return NULL;

	math_l2_invlist_iter_t l2_iter = malloc(sizeof *l2_iter);

	l2_iter->n_qnodes       = inv->mq.n_qnodes;
	l2_iter->ipf            = inv->mq.ipf;
	l2_iter->msf            = &inv->msf;
	l2_iter->fh_symbinfo    = duplicate_entries_fh_array(&inv->mq);
	l2_iter->ele            = inv->mq.ele;
	l2_iter->merge_iter     = merger_set_iterator(&inv->mq.merge_set);
	l2_iter->future_docID   = 0;
	l2_iter->last_threshold = -1.f;
	l2_iter->threshold      = inv->threshold;

	l2_iter->pruner = math_pruner_init(&inv->mq, &inv->msf, *inv->threshold);

	/* run level-2 next() **twice** to forward the current
	 * ID from zero to future_docID (also zero initially)
	 * and then forward to the first item docID. */
	if (0 != math_l2_invlist_iter_next(l2_iter))
		math_l2_invlist_iter_next(l2_iter);

	return l2_iter;
}

void math_l2_invlist_iter_free(math_l2_invlist_iter_t l2_iter)
{
	if (l2_iter->pruner)
		math_pruner_free(l2_iter->pruner);

	for (int i = 0; i < l2_iter->merge_iter->size; i++)
		if (l2_iter->fh_symbinfo[i])
			fclose(l2_iter->fh_symbinfo[i]);
	free(l2_iter->fh_symbinfo);

	merger_set_iter_free(l2_iter->merge_iter);
	free(l2_iter);
}

uint64_t math_l2_invlist_iter_cur(math_l2_invlist_iter_t l2_iter)
{
	if (l2_iter->item.docID == UINT32_MAX)
		return UINT64_MAX;
	else
		return l2_iter->item.docID;
}

static inline int
hit_nodes(struct math_pruner *pruner, int N, merger_set_iter_t iter, int *out)
{
	int n = 0, max_n = N;

	/* create a simple bitmap */
	int hits[max_n];
	memset(hits, 0, sizeof hits);

	/* find all hit node indexs */
	for (int i = 0; i <= iter->pivot; i++) {
		uint64_t cur = merger_map_call(iter, cur, i);
		if (cur == iter->min) {
			struct math_pruner_backref brefs;
			int iid = iter->map[i];
			brefs = pruner->backrefs[iid];
			/* for all nodes linked to a hit inverted list */
			for (int j = 0; j < brefs.cnt; j++) {
				int idx = brefs.idx[j];
				if (!hits[idx]) {
					hits[idx] = 1;
					out[n++] = idx;
				}
			}
		}
	}

	return n;
}

static inline float
struct_score(merger_set_iter_t iter, struct math_pruner_qnode *qnode,
	math_l2_invlist_iter_t l2_iter, float best, float threshold)
{
	float *ipf = l2_iter->ipf;
	struct math_score_factors *msf = l2_iter->msf;

	float estimate, score = 0, leftover = qnode->sum_ipf;
	/* for each sector tree under qnode */
	for (int i = 0; i < qnode->n; i++) {
		/* sector tree variables */
		int iid = qnode->invlist_id[i];
		int ref = qnode->secttr_w[i];

		/* skip set iterators must follow up */
		if (i > iter->pivot)
			if (!merger_set_map_follow(iter, i))
				goto skip;

		uint64_t cur = MERGER_ITER_CALL(iter, cur, iid);
		if (cur == iter->min) {
			/* read hit inverted list item */
			struct math_invlist_item item;
			MERGER_ITER_CALL(iter, read, iid, &item, sizeof(item));
			/* accumulate preceise partial score */
			score += MIN(ref, item.sect_width) * ipf[iid];
#ifdef DEBUG_MATH_SEARCH__STRUCT_SCORING
			if (inspect(iter->min)) {
				printf("node score += min(q=%u, d=%u) * %.2f from [%3u]"
					" = %.2f \n", ref, item.sect_width, ipf[iid], iid, score);
			}
#endif
		}
skip:
#ifndef MATH_PRUNING_STRATEGY_NONE
		leftover = leftover - ref * ipf[iid];
		estimate = score + leftover; /* new score estimation */

#ifdef DEBUG_MATH_SEARCH__STRUCT_SCORING
		if (inspect(iter->min))
			printf("+= leftover=%.2f = %.2f\n", leftover, estimate);
#endif
		if (estimate <= best || math_score_upp(msf, estimate) <= threshold)
			return 0;
#endif
	}

	return score;
}

static inline void
acc_symbol_subscore(struct subpath_ele *ele, int j, struct symbinfo *symbinfo)
{
	uint16_t q_ophash = ele->secttr[j].ophash;
	uint16_t d_ophash = symbinfo->ophash;

	for (int u = 0; u < ele->n_splits[j]; u++) {
		uint16_t q_symbol = ele->symbol[j][u];
		uint16_t q_splt_w = ele->splt_w[j][u];
		for (int v = 0; v < symbinfo->n_splits; v++) {
			uint16_t d_symbol = symbinfo->symbol[v];
			uint16_t d_splt_w = symbinfo->splt_w[v];

			int sym_equal = (q_symbol == d_symbol) ? 0x2 : 0x0;
			int fnp_equal = (q_ophash == d_ophash) ? 0x1 : 0x0;

			float match_score = 0.f;
			switch (sym_equal | fnp_equal) {
				case 0x3: /* exact match */
					match_score = SYMBOL_SUBSCORE_FULL;
					break;
				case 0x2: /* match except for fingerprint */
					match_score = SYMBOL_SUBSCORE_HALF;
					break;
				case 0x1: /* match fingerprint but not symbol */
				default:
					match_score = SYMBOL_SUBSCORE_BASE;
					break;
			}

			uint16_t min_w = MIN(q_splt_w, d_splt_w);
			match_score = match_score * min_w;
			// q_hash_table[q_symbol][d_symbol] += match_score;
		}
	}
}

static inline float
symbol_score(merger_set_iter_t iter, struct math_pruner_qnode *qnode,
	math_l2_invlist_iter_t l2_iter, float best, float threshold, int *dl)
{
	FILE **fhs = l2_iter->fh_symbinfo;
	struct subpath_ele **eles = l2_iter->ele;

	/* for each sector tree under evaluating qnode */
	for (int i = 0; i < qnode->n; i++) {
		int iid = qnode->invlist_id[i];
		uint64_t cur = MERGER_ITER_CALL(iter, cur, iid);
		if (cur == iter->min) {
			/* read hit inverted list item */
			struct math_invlist_item item;
			MERGER_ITER_CALL(iter, read, iid, &item, sizeof(item));

			/* output document original length */
			*dl = item.orig_width;

			/* seek to symbol info file offset */
			uint32_t offset = item.symbinfo_offset;
			if (0 != fseek(fhs[iid], offset, SEEK_SET)) {
				prerr("fh_symbinfo cannot seek to %u", offset);
				return 0;
			}

			/* read document symbol information */
			struct symbinfo symbinfo;
			size_t rd_sz;
			rd_sz = fread(&symbinfo, 1, sizeof symbinfo, fhs[iid]);
			assert(rd_sz == sizeof symbinfo); (void)rd_sz;
			// print_symbinfo(&symbinfo);

			/* accumulate qry-doc symbol pair scores */
			int j = qnode->ele_splt_idx[i];
			acc_symbol_subscore(eles[iid], j, &symbinfo);
		}
	}

	float mnc_score = 0;
//	for i in q_hash_table:
//		d_symbol = argmax(q_hash_table[i], cross)
//		mnc_score += q_hash_table[i][d_symbol]
//		cross[d_symbol] = 1
	return mnc_score / (float)qnode->sum_w;
}

int math_l2_invlist_iter_next(math_l2_invlist_iter_t l2_iter)
{
	struct math_pruner *pruner = l2_iter->pruner;
	merger_set_iter_t iter = l2_iter->merge_iter;
	float threshold = *l2_iter->threshold;

#ifndef MATH_PRUNING_STRATEGY_NONE
	/* handle threshold update */
	if (threshold != l2_iter->last_threshold) {
		if (math_pruner_update(pruner, threshold)) {
			;
#ifdef DEBUG_MATH_SEARCH
			printf(C_BLUE "[node dropped]\n" C_RST);
#endif
		}

		math_pruner_iters_drop(pruner, iter);

#if defined(MATH_PRUNING_STRATEGY_MAXREF)
		math_pruner_iters_sort_by_maxref(pruner, iter);

#elif defined(MATH_PRUNING_STRATEGY_GBP_NUM)
		math_pruner_iters_gbp_assign(pruner, iter, 0);

#elif defined(MATH_PRUNING_STRATEGY_GBP_LEN)
		math_pruner_iters_gbp_assign(pruner, iter, 1);

#else
	#error("no math pruning strategy specified.")
#endif

		l2_iter->last_threshold = threshold;

		/* if there is no element in requirement set */
		if (iter->pivot < 0)
			goto terminated;
	}
#endif

	/* reset item values */
	l2_iter->item.score = 0.f;
	l2_iter->item.n_occurs = 0;

	/* safe guard */
	if (l2_iter->future_docID == UINT32_MAX)
		goto terminated;

	/* best expression (w/ highest structural score) within a document */
	float best = 0;
	struct math_pruner_qnode *best_qn;

	/* merge to the next */
	int n_max_nodes = l2_iter->n_qnodes;
	do {
		/* set current candidate */
		uint32_t cur_docID = key2doc(iter->min);

#ifdef DEBUG_MATH_SEARCH
		if (inspect(iter->min)) {
			printf(C_BROWN);
			printf("[math merge iteration] future_docID=%u\n",
				l2_iter->future_docID);
			printf(C_RST);
			math_pruner_print(pruner);
			ms_merger_iter_print(iter, keyprint);
		}
#endif

		/* test the termination of level-2 merge iteration */
		if (cur_docID != l2_iter->future_docID) {
			/* passed through all maths in future_docID. */
			l2_iter->item.docID = l2_iter->future_docID;
			l2_iter->future_docID = cur_docID;
			return 1;
		}

		/* get hits query nodes of current candidate */
		int out[n_max_nodes];
		int n_hit_nodes = hit_nodes(pruner, n_max_nodes, iter, out);

#ifdef DEBUG_MATH_SEARCH
		if (inspect(iter->min)) {
			printf("[hit nodes] ");
			for (int i = 0; i < n_hit_nodes; i++) {
				struct math_pruner_qnode *qnode = pruner->qnodes + out[i];
				printf("#%u/%u upp(%.2f)", qnode->root,
					qnode->sum_w, qnode->sum_ipf);
			}
			printf("\n");
		}
#endif

		/* calculate structural score */
		int best_updated = 0;
		for (int i = 0; i < n_hit_nodes; i++) {
			struct math_pruner_qnode *qnode = pruner->qnodes + out[i];

			/* prune qnode if its rooted tree is smaller than that of `best' */
#ifndef MATH_PRUNING_STRATEGY_NONE
			if (qnode->sum_ipf <= best) {
#ifdef DEBUG_MATH_SEARCH
				if (inspect(iter->min))
					printf("qnode#%u pruned (less than best)\n", qnode->root);
#endif
				continue;
			}
#endif
			/* get structural score of matched tree rooted at qnode */
			float s;
			s = struct_score(iter, qnode, l2_iter, best, threshold);
			if (s > best) {
				best = s;
				best_qn = qnode;
				best_updated = 1;
			}

#ifdef DEBUG_MATH_SEARCH
			if (inspect(iter->min))
				printf("qnode#%u sum_ipf: actual=%.2f,upper=%.2f,best=%.2f\n",
					qnode->root, s, qnode->sum_ipf, best);
#endif
		}

		/* if structural score is non-zero */
		if (best_updated) {
			/* calculate symbol score */
			int orig_width = 0;
			float symbol_sim = symbol_score(
				iter, best_qn, l2_iter, best, threshold, &orig_width
			);

			/* get overall score */
			struct math_score_factors *msf = l2_iter->msf;
			msf->symbol_sim = symbol_sim;
			msf->struct_sim = best;
			msf->doc_lr_paths = (float)orig_width;

			float score = math_score_calc(msf);
			//float score = best;

			/* take max expression score as math score for this docID */
			if (score > l2_iter->item.score) {
				/* update the returning item values for current docID */
				l2_iter->item.score = score;
				l2_iter->item.n_occurs = 1;
				l2_iter->item.occur[0] = key2exp(iter->min);

#ifdef DEBUG_MATH_SEARCH
				if (inspect(iter->min)) {
					printf(C_GREEN);
					printf("Propose ");
					printf("[doc#%u exp#%u match %d<->%d]: %.2f/%.0f=>%.2f\n",
						cur_docID, key2exp(iter->min), best_qn->root,
						key2rot(iter->min), best, msf->doc_lr_paths, score);
					printf(C_RST);
				}
#endif
			}
		}

		/* termination should be judged by future_docID, not cur_docID */
		merger_set_iter_next(iter);
	} while (1 /* only terminate when future_docID is UINT64_MAX */);

terminated: /* when iterator reaches the end */

	l2_iter->item.docID = UINT32_MAX;
	return 0;
}

size_t
math_l2_invlist_iter_read(math_l2_invlist_iter_t l2_iter, void *dst, size_t sz)
{
	memcpy(dst, &l2_iter->item, sizeof(struct math_l2_iter_item));
	return sizeof(struct math_l2_iter_item);
}

float math_l2_invlist_iter_upp(math_l2_invlist_iter_t l2_iter)
{
	struct math_pruner* pruner = l2_iter->pruner;
	struct math_score_factors *msf = l2_iter->msf;

	float max_sum_ipf = 0;
	for (int i = 0; i < pruner->n_qnodes; i++) {
		struct math_pruner_qnode *qnode = pruner->qnodes + i;
		if (max_sum_ipf < qnode->sum_ipf)
			max_sum_ipf = qnode->sum_ipf;
	}
	return math_score_upp(msf, max_sum_ipf);
}
