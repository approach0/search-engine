#include <stdint.h>
#include "config.h"

/* MaxScore merger */
struct ms_merger {
	merge_set_t  set;
	int          map[MAX_MERGE_SET_SZ]; /* set.prop[map[i]] */
	int          acc_upp[MAX_MERGE_SET_SZ]; /* MaxScore A[] */
	uint64_t     min, size; /* minimal ID and active number */
	/* pivot: separate between requirement and skipping set */
	int          pivot;
};

float no_upp_relax(void*, float);
int  ms_merger_lift_up_pivot(struct ms_merger*, float,
                             upp_relax_callbk, void*);

void ms_merger_update_acc_upp(struct ms_merger*);
void ms_merger_map_remove(struct ms_merger*, int);
int  ms_merger_skipset_follow(struct ms_merger*);
void ms_merger_iter_sort_by_upp(struct ms_merger*);
