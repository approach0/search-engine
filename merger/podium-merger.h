#include <stdint.h>
#include "config.h"

/* Medal podium merger */
struct pd_merger {
	merge_set_t  set;
	int          map[MAX_MERGE_SET_SZ]; /* set.prop[map[i]] */
	float        acc_upp[MAX_MERGE_SET_SZ]; /* MaxScore A[] */
	float        acc_max[MAX_MERGE_SET_SZ]; /* Podium   M[] */
	uint64_t     min, size; /* minimal ID and active number */
	/* pivot: separate between requirement and skipping set */
	int          pivot;
};

int  pd_merger_lift_up_pivot(struct pd_merger*, float,
                             merger_upp_relax_fun, void*);

void pd_merger_update_acc_upp(struct pd_merger*);
int  pd_merger_map_remove(struct pd_merger*, int);
int  pd_merger_iter_follow(struct pd_merger*, int);
uint64_t pd_merger_min(struct pd_merger*);

struct pd_merger *pd_merger_iterator(merge_set_t*);
void  pd_merger_iter_free(struct pd_merger*);
int   pd_merger_iter_next(struct pd_merger*);
void  pd_merger_iter_print(struct pd_merger*, merger_keyprint_fun);
