#include "postmerger.h"

struct postmerger_postlist math_memo_postlist(void*);
struct postmerger_postlist math_memo_postlist_gener(void*);

struct postmerger_postlist math_disk_postlist(void*);
struct postmerger_postlist math_disk_postlist_gener(void*);

struct postmerger_postlist term_memo_postlist(void*);
struct postmerger_postlist term_disk_postlist(void*);
void term_postlist_free(struct postmerger_postlist);

struct postmerger_postlist empty_postlist(void);
