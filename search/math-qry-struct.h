#include "indices/indices.h"
#include "math-prefix-qry.h"

struct math_qry_struct {
	struct math_prefix_qry pq;
	struct subpaths subpaths;
};

int math_qry_prepare(struct indices*, char*, struct math_qry_struct*);

void math_qry_free(struct math_qry_struct*);
