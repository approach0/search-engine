#include <assert.h>
#include <float.h> /* for FLT_MAX */
#include <unistd.h> /* for dup() */

#include "common/common.h"
#include "tex-parser/head.h"
#include "config.h"
#include "math-search.h"

struct math_l2_invlist
*math_l2_invlist(math_index_t index, const char* tex,
	float *threshold, float *dynamic_threshold)
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

	/* copy threshold addresses */
	ret->threshold = threshold;
	ret->dynm_threshold = dynamic_threshold;
	return ret;
}

void math_l2_invlist_free(struct math_l2_invlist *inv)
{
	math_qry_release(&inv->mq);
	free(inv);
}

/* use MaxScore merger */
typedef struct ms_merger *merger_set_iter_t;
#define merger_set_iterator    ms_merger_iterator
#define merger_set_iter_next   ms_merger_iter_next
#define merger_set_iter_free   ms_merger_iter_free
#define merger_set_iter_follow ms_merger_iter_follow

#ifdef DEBUG_MATH_SEARCH
static void keyprint(uint64_t k)
{
	printf("#%u, #%u, r%u", key2doc(k), key2exp(k), key2rot(k));
}

static int do_inspect;

static int inspect(uint64_t k)
{
	uint d = key2doc(k);
	uint e = key2exp(k);
	uint r = key2rot(k);
	(void)d; (void)e; (void)r; (void)do_inspect;
	return (d == 1 || d == 2);
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
	/* merge set can be empty when you get no subpath from tex, for example,
	 * a single NIL node alone. */
	if (merger_set_empty(&inv->mq.merge_set))
		return NULL;

	math_l2_invlist_iter_t l2_iter = malloc(sizeof *l2_iter);

	l2_iter->n_qnodes       = inv->mq.n_qnodes;
	l2_iter->ipf            = inv->mq.merge_set.upp;
	l2_iter->msf            = &inv->msf;
	l2_iter->fh_symbinfo    = duplicate_entries_fh_array(&inv->mq);
	l2_iter->ele            = inv->mq.ele;
	l2_iter->mnc            = &inv->mq.mnc;
	l2_iter->tex            = inv->mq.tex;

	l2_iter->merge_iter     = merger_set_iterator(&inv->mq.merge_set);
	/* now, pointing to the first REAL item */
	l2_iter->real_curID     = key2doc(l2_iter->merge_iter->min);
	l2_iter->item.docID     = l2_iter->real_curID;
	l2_iter->last_threshold = -FLT_MAX;
	l2_iter->threshold      = inv->threshold;
	l2_iter->dynm_threshold = inv->dynm_threshold;

	l2_iter->pruner = math_pruner_init(&inv->mq, &inv->msf, *inv->threshold);

	return l2_iter;
}

void math_l2_invlist_iter_free(math_l2_invlist_iter_t l2_iter)
{
	if (l2_iter->pruner)
		math_pruner_free(l2_iter->pruner);

	for (int i = 0; i < l2_iter->merge_iter->set.n; i++)
		if (l2_iter->fh_symbinfo[i])
			fclose(l2_iter->fh_symbinfo[i]);
	free(l2_iter->fh_symbinfo);

	merger_set_iter_free(l2_iter->merge_iter);
	free(l2_iter);
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
	uint32_t dl = 1;

	float estimate, score = 0, leftover = qnode->sum_ipf;
	/* for each sector tree under qnode */
	for (int i = 0; i < qnode->n; i++) {
		/* sector tree variables */
		int iid = qnode->invlist_id[i];
		int ref = qnode->secttr_w[i];

		/* skip set iterators must follow up */
		if (!merger_set_iter_follow(iter, iid))
			goto skip;

#ifdef DEBUG_MATH_SEARCH__STRUCT_SCORING
		if (inspect(iter->min)) {
			printf("after skip [%u]:\n", iid);
			ms_merger_iter_print(iter, keyprint);
		}
#endif
		/* read item if it is a candiate */
		uint64_t cur = MERGER_ITER_CALL(iter, cur, iid);
		if (cur == iter->min) {
			/* read hit inverted list item */
			struct math_invlist_item item;
			MERGER_ITER_CALL(iter, read, iid, &item, sizeof(item));
			dl = item.orig_width;
			/* accumulate preceise partial score */
			score += MIN(ref, item.sect_width) * ipf[iid];
#ifdef DEBUG_MATH_SEARCH__STRUCT_SCORING
			if (inspect(iter->min)) {
				printf("node score += min(q=%u, d=%u) * %.2f from [%3u]"
					" = %.2f \n", ref, item.sect_width, ipf[iid], iid, score);
			}
#endif
		}
skip:;
#ifndef MATH_PRUNING_STRATEGY_NONE
		leftover = leftover - ref * ipf[iid];
		estimate = score + leftover; /* new score estimation */

#ifdef DEBUG_MATH_SEARCH__STRUCT_SCORING
		if (inspect(iter->min))
			printf("+= leftover=%.2f = %.2f\n", leftover, estimate);
#endif
		if (estimate <= best ||
		    math_score_upp_tight(msf, estimate, dl) <= threshold)
			return 0;
#endif
	}

	return score;
}

static inline void
acc_symbol_subscore(struct mnc_scorer *mnc, struct subpath_ele *ele,
                    int j, struct symbinfo *symbinfo)
{
	uint16_t q_ophash = ele->secttr[j].ophash;
	uint16_t d_ophash = symbinfo->ophash;

	for (int u = 0; u < ele->n_splits[j]; u++) {
		uint16_t q_symbol = ele->symbol[j][u];
		uint16_t q_splt_w = ele->splt_w[j][u];
		for (int v = 0; v < symbinfo->n_splits; v++) {
			uint16_t d_symbol = symbinfo->split[v].symbol;
			uint16_t d_splt_w = symbinfo->split[v].splt_w;
			uint16_t min_w = MIN(q_splt_w, d_splt_w);

			int sym_equal = (q_symbol == d_symbol) ? 0x2 : 0x0;
			int fnp_equal = (q_ophash == d_ophash) ? 0x1 : 0x0;

			float score = 0.f;
			switch (sym_equal | fnp_equal) {
				case 0x3: /* exact match */
					score = SYMBOL_SUBSCORE_FULL;
					break;
				case 0x2: /* match except for fingerprint */
					score = SYMBOL_SUBSCORE_HALF;
					break;
				case 0x1: /* match fingerprint but not symbol */
				default:
					score = SYMBOL_SUBSCORE_BASE;
					break;
			}
#ifdef DEBUG_MATH_SEARCH__SYMBOL_SCORING
			if (do_inspect) {
				printf("q=%s|0x%x * %u\n", trans_symbol(q_symbol),
					q_ophash, q_splt_w);
				printf("d=%s|0x%x * %u\n", trans_symbol(d_symbol),
					d_ophash, d_splt_w);
				printf("+= %.2f * %u = %.2f\n", score, min_w,
					score * min_w);
				do_inspect = 0;
			}
#endif
			score = score * min_w;
			mnc_score_doc_path_add(mnc, q_symbol, d_symbol, score);
		}
	}
}

static inline float
symbol_score(math_l2_invlist_iter_t l2_iter, merger_set_iter_t iter,
	struct math_pruner_qnode *qnode, int *dl)
{
	float score;
	FILE **fhs = l2_iter->fh_symbinfo;
	struct subpath_ele **eles = l2_iter->ele;
	struct mnc_scorer *mnc = l2_iter->mnc;

	/* reset mnc */
	mnc_score_doc_reset(mnc);

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
#ifdef IGNORE_MATH_SYMBOL_SCORE
			return 1.f;
#endif
			/* seek to symbol info file offset */
			uint32_t offset = item.symbinfo_offset;
			if (0 != fseek(fhs[iid], offset, SEEK_SET)) {
				prerr("fh_symbinfo cannot seek to %u", offset);
				return 0;
			}

			/* read document symbol information */
			struct symbinfo symbinfo;
			math_index_read_symbinfo(&symbinfo, fhs[iid]);

#ifdef DEBUG_MATH_SEARCH__SYMBOL_SCORING
			if (inspect(iter->min)) {
				math_index_print_symbinfo(&symbinfo);
				do_inspect = 1;
			}
#endif
			/* accumulate qry-doc symbol pair scores */
			int j = qnode->ele_splt_idx[i];
			acc_symbol_subscore(mnc, eles[iid], j, &symbinfo);
		}
	}

	score = mnc_score_align(mnc);
#ifdef DEBUG_MATH_SEARCH__SYMBOL_SCORING
	if (inspect(iter->min)) {
		printf("[mnc table]\n");
		mnc_score_print(mnc, 0);
		printf("symbol score = %.2f / %d = %.2f\n", score, qnode->sum_w,
			score / qnode->sum_w);
	}
#endif
	return score / qnode->sum_w;
}

float math_l2_invlist_iter_upp(math_l2_invlist_iter_t l2_iter)
{
	struct math_score_factors *msf = l2_iter->msf;
	float max_sum_ipf = math_pruner_max_sum_ipf(l2_iter->pruner);

	return math_score_upp(msf, max_sum_ipf);
}

/*
 * Iterator positional functions
 */
uint64_t math_l2_invlist_iter_cur(math_l2_invlist_iter_t l2_iter)
{
	if (l2_iter->item.docID == UINT32_MAX)
		return UINT64_MAX;
	else
		return l2_iter->item.docID;
}

inline static int update_pruner(math_l2_invlist_iter_t l2_iter)
{
	struct math_pruner *pruner = l2_iter->pruner;
	merger_set_iter_t iter = l2_iter->merge_iter;
	float threshold = *l2_iter->threshold;

#ifndef MATH_PRUNING_STRATEGY_NONE
	/* handle threshold update */
	if (threshold != l2_iter->last_threshold) {
		if (math_pruner_update(pruner, threshold)) {
			;
#ifdef DEBUG_MATH_SEARCH__PRUNER_UPDATE
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

#ifdef DEBUG_MATH_SEARCH__PRUNER_UPDATE
		printf("[pruner update] for `%s'\n", l2_iter->tex);
		math_pruner_print(pruner);
		math_pruner_print_stats(pruner);
#endif

		/* if there is no element in requirement set */
		if (iter->pivot < 0)
			return 0;
	}
#endif

	return 1;
}

/*
 * read_and_future_next() will read the current unread level-2 item and
 * forward real_curID to the next docID.
 *
 * Before:
 * +---------------+   +---------------+
 * ||              |   |               |
 * +---------------+   +---------------+
 *  ^
 *  `---- real_curID, item.docID
 *
 * After:
 * +---------------+   +---------------+
 * |||||||||||||||||   ||              |
 * +---------------+   +---------------+
 *  ^                   ^
 *  `---- item.docID     `---- real_curID
 */
inline static int read_and_future_next(math_l2_invlist_iter_t l2_iter)
{
	merger_set_iter_t iter = l2_iter->merge_iter;
	struct math_pruner *pruner = l2_iter->pruner;

	/* best expression (w/ highest structural score) within a document */
	float best = 0;
	struct math_pruner_qnode *best_qn;

	/* dynamic threshold */
	float dynm_threshold = *l2_iter->dynm_threshold;

	/* merge to the next */
	int n_max_nodes = l2_iter->n_qnodes;
	do {
		/* set current candidate */
		uint32_t cur_docID = key2doc(iter->min);

#ifdef DEBUG_MATH_SEARCH
		if (inspect(iter->min)) {
			printf("[l2_read_next] docID=%u\n", cur_docID);
			ms_merger_iter_print(iter, keyprint);
		}
#endif

		/* read until current ID becomes different */
		if (cur_docID != l2_iter->real_curID) {
			/* now real_curID is async with item.docID */
			l2_iter->real_curID = cur_docID;
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
					printf("qnode#%u skipped (less than best)\n", qnode->root);
#endif
				continue;
			}
#endif
			/* get structural score of matched tree rooted at qnode */
			float s;
			s = struct_score(iter, qnode, l2_iter, best, dynm_threshold);
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
			float symb = symbol_score(l2_iter, iter, best_qn, &orig_width);

			/* get overall score */
			struct math_score_factors *msf = l2_iter->msf;
			msf->symbol_sim   = symb;
			msf->struct_sim   = best;
			msf->doc_lr_paths = orig_width;

			float score = math_score_calc(msf);

			/* take max expression score as math score for this docID */
			if (score > l2_iter->item.score) {
				/* update the returning item values for current docID */
				l2_iter->item.score = score;
				l2_iter->item.n_occurs = 1;
				l2_iter->item.occur[0] = key2exp(iter->min);

#ifdef DEBUG_MATH_SEARCH
				if (inspect(iter->min)) {
					printf("Propose ");
					printf(C_GREEN);
					printf("[doc#%u exp#%u %d<->%d] %.2f,%.2f/%d=>%.2f\n",
						cur_docID, key2exp(iter->min),
						best_qn->root, key2rot(iter->min),
						best, symb, msf->doc_lr_paths, score);
					printf(C_RST);
				}
#endif
			}
		}

	} while (merger_set_iter_next(iter));

	l2_iter->real_curID = UINT32_MAX;
	return 0;
}

/*
 * pass_through_next() will simply fast forward to the next docID,
 * skip reading passing elements.
 *
 * Before:
 * +---------------+   +---------------+
 * ||              |   |               |
 * +---------------+   +---------------+
 *  ^
 *  `---- real_curID, item.docID
 *
 * After:
 * +---------------+   +---------------+
 * |               |   ||              |
 * +---------------+   +---------------+
 *                      ^
 *                      `---- real_curID, item.docID
 */
inline static int pass_through_next(math_l2_invlist_iter_t l2_iter)
{
	merger_set_iter_t iter = l2_iter->merge_iter;

	do {
		/* set current candidate */
		uint32_t cur_docID = key2doc(iter->min);

#ifdef DEBUG_MATH_SEARCH
		if (inspect(iter->min)) {
			printf("[l2_next] docID=%u\n", cur_docID);
			ms_merger_iter_print(iter, keyprint);
		}
#endif
		/* read until current ID becomes different */
		if (cur_docID != l2_iter->item.docID) {
			l2_iter->item.docID = cur_docID;
			l2_iter->real_curID = cur_docID;
			return 1;
		}

	} while (merger_set_iter_next(iter));

	l2_iter->item.docID = UINT32_MAX;
	l2_iter->real_curID = UINT32_MAX;
	return 0;
}

int math_l2_invlist_iter_next(math_l2_invlist_iter_t l2_iter)
{
	if (l2_iter->item.docID < l2_iter->real_curID) {
		/* since iterator is already passed, just need to fast forward */
		l2_iter->item.docID = l2_iter->real_curID;
		return (l2_iter->item.docID != UINT32_MAX);
	} else {
		/* iterator is at begining of this docID, let's pass through it */
		if (unlikely(0 == update_pruner(l2_iter)))
			return 0;
		return pass_through_next(l2_iter);
	}
}

size_t
math_l2_invlist_iter_read(math_l2_invlist_iter_t l2_iter, void *dst, size_t sz)
{
	if (l2_iter->item.docID == l2_iter->real_curID) {
		/* reset item values */
		l2_iter->item.score = 0.f;
		l2_iter->item.n_occurs = 0;

		/* let's read this item */
		if (unlikely(0 == update_pruner(l2_iter)))
			return 0;
		read_and_future_next(l2_iter);
	}

	/* copy and return item */
	memcpy(dst, &l2_iter->item, sizeof(struct math_l2_iter_item));
	return sizeof(struct math_l2_iter_item);
}

int math_l2_invlist_iter_skip(math_l2_invlist_iter_t l2_iter, uint64_t key_)
{
	merger_set_iter_t iter = l2_iter->merge_iter;

	/* refuse to skip if target key is in the back */
	uint64_t cur_docID = math_l2_invlist_iter_cur(l2_iter);
	if (key_ <= cur_docID)
		return (cur_docID != UINT64_MAX);

	/* convert to level-1 key */
	uint64_t key = doc2key(key_);

	/* update pruner */
	if (unlikely(0 == update_pruner(l2_iter)))
		return 0;

	/* skip to key for every iterator in required set */
	for (int i = 0; i <= iter->pivot; i++) {
		uint64_t cur = merger_map_call(iter, cur, i);
		if (cur < key)
			merger_map_call(iter, skip, i, key);
	}

	/* assign updated current ID */
	iter->min = ms_merger_min(iter);
	l2_iter->real_curID = key2doc(iter->min);
	l2_iter->item.docID = l2_iter->real_curID;

#ifdef DEBUG_MATH_SEARCH
	if (inspect(iter->min)) {
		printf("[after level-2 skipping]\n");
		ms_merger_iter_print(iter, keyprint);
	}
#endif
	return (l2_iter->item.docID != UINT32_MAX);
}
