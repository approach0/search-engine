struct sector_tr {
	int rnode;
	int width;
};

struct posting {
	struct sector_tr *(*get_sector_trees)();
	int   (*get_min)();
	void  (*advance)();
	int expID;
	int refcnt;
};

struct pruner_node {
	int width; // each node is ordered by this key
	struct sector_tr secttr[64]; // each sect rnode is the same here
	struct posting *posting[64];
};

int main() // simulate one merge stage iteration
{
	struct posting all_post[64];
	struct pruner_node q_nodes[64];
	int theta = 123; /* best matched number of leaves */
	int q_widest_match = 0;
	for (int i = 0; i < 64 /* |T_q| */; i++) {
		struct pruner_node *q_node = q_nodes + i;
		int q_upperbound = q_node->width;
		int q_node_match = 0;
		int sumvec[128] = {0};

		if (q_upperbound <= theta) {
			for (int j = 0; j < 64 /* |l(T_q)| */; j++) {
				struct posting *post = q_node->posting[j];
				post->refcnt --;
			}
			/* delete this pruner_node */;
			continue;
		}

		/* qnode best match calculation */
		for (int j = 0; j < 64 /* |l(T_q)| */; j++) {
			struct sector_tr *q_secttr = q_node->secttr + j;
			struct posting *post = q_node->posting[j];
			struct sector_tr *d_secttr = post->get_sector_trees();
			int qw = q_secttr->width;
			int max = 0;

			/* skip non-hit postings */
			if (post->get_min() != post->expID)
				continue;

			/* vector addition */
			for (int k = 0; k < 64 /* |l(T_d)| */; k++) {
				int dw = d_secttr[k].width;
				int rn = d_secttr[k].rnode;
				if (qw > dw) {
					sumvec[rn] += dw;
					if (dw > max) max = dw;
				} else {
					sumvec[rn] += qw;
					if (qw > max) max = qw;
				}

				if (sumvec[rn] > q_node_match) {
					q_node_match = sumvec[rn];
				}
			}

			/* update upperbound for each vector addition */
			if (max < qw) {
				q_upperbound -= (qw - max);
				if (q_upperbound <= theta)
					break;
			}
		}

		if (q_node_match > q_widest_match)
			q_widest_match = q_node_match;
	}

	for (int i = 0; i < 64 /* cur number of live postings */; i++) {
		if (all_post[i].refcnt <= 0) {
			/* delete this posting list */;
			continue;
		}

		if (all_post[i].get_min() == all_post[i].expID)
			all_post[i].advance();
	}

	if (q_widest_match > theta)
		return q_widest_match; /* put q_widest_match scored into heap and update theta */
	else
		return 0;
}
