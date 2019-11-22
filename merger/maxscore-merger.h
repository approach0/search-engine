#include <stdint.h>
#include "config.h"

/* MaxScore merger */
struct ms_merger {
	merge_set_t  set;
	int          map[MAX_MERGE_SET_SZ]; /* set.prop[map[i]] */
	float        acc_upp[MAX_MERGE_SET_SZ]; /* MaxScore A[] */
	uint64_t     min, size; /* minimal ID and active number */
	/* pivot: separate between requirement and skipping set */
	int          pivot;
};

int  ms_merger_lift_up_pivot(struct ms_merger*, float,
                             merger_upp_relax_fun, void*);

void ms_merger_update_acc_upp(struct ms_merger*);
int  ms_merger_map_remove(struct ms_merger*, int);
int  ms_merger_iter_follow(struct ms_merger*, int);
uint64_t ms_merger_min(struct ms_merger*);

struct ms_merger *ms_merger_iterator(merge_set_t*);
void  ms_merger_iter_free(struct ms_merger*);
int   ms_merger_iter_next(struct ms_merger*);
void  ms_merger_iter_print(struct ms_merger*, merger_keyprint_fun);
