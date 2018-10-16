struct sector {
	int node_id;
	int width;
};

struct posting {
	struct sector *(*get_sectors)();
	int (*get_min)();
	void (*advance)();
	int expID;
	int refcnt;
};

struct pruner_node {
	int width; // each node is ordered by this key
	struct sector   sect[64]; // each sect node_id is the same here
	struct posting *posting[64];
};

int main() // simulate one merge stage iteration
{
	struct posting all_post[64];
	struct pruner_node pruner_nodes[64];
	int theta = 123; /* best matched number of leaves */
	int widest_match = 0;
	for (int i = 0; i < 64 /* |T_q| */; i++) {
		struct pruner_node *node = pruner_nodes + i;
		int qw_upperbound = node->width;
		int qw_match = 0;
		int vector[128] = {0};

		if (qw_upperbound <= theta) {
			for (int j = 0; j < 64 /* |l(T_q)| */; j++) {
				struct posting *post = node->posting[j];
				post->refcnt --;
			}
			/* delete this pruner_node */;
			break;
		} else if (widest_match >= qw_upperbound) {
			break;
		}

		/* qnode best match calculation */
		for (int j = 0; j < 64 /* |l(T_q)| */; j++) {
			struct sector *q_sect = node->sect + j;
			struct posting *post = node->posting[j];
			struct sector *d_sect = post->get_sectors();
			int qw = q_sect->width;
			int max = 0;

			/* skip non-hit postings */
			if (post->get_min() != post->expID)
				continue;

			/* vector addition */
			for (int k = 0; k < 64 /* |l(T_d)| */; k++) {
				int dw = d_sect[k].width;
				int rn = d_sect[k].node_id;
				if (qw > dw) {
					vector[rn] += dw;
					if (dw > max) max = dw;
				} else {
					vector[rn] += qw;
					if (qw > max) max = qw;
				}

				if (vector[rn] > qw_match) {
					qw_match = vector[rn];
					if (qw_match >= qw_upperbound)
						goto term_early;
				}
			}

			if (max < qw) {
				qw_upperbound -= (qw - max);
				if (qw_upperbound <= theta)
					break;
			}
		}

term_early:
		if (qw_match > widest_match)
			widest_match = qw_match;
	}

	for (int i = 0; i < 64 /* cur number of live postings */; i++) {
		if (all_post[i].refcnt <= 0) {
			/* delete this posting list */;
			continue;
		}

		if (all_post[i].get_min() == all_post[i].expID)
			all_post[i].advance();
	}

	if (widest_match > theta)
		return widest_match; /* put widest_match scored into heap and update theta */
	else
		return 0;
}
