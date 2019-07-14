#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "skippy/skippy.h"
#include "mhook/mhook.h"
#include "dir-util/dir-util.h"

struct test_block {
	struct skippy_node sn;
	char  *payload;
	size_t sz;
};

static long test_block_payload_hook(char *path, void *blk_)
{
	char fullname[MAX_DIR_PATH_NAME_LEN];
	size_t n;
	FILE *fh;

	P_CAST(blk, struct test_block, blk_);
	sprintf(fullname, "%s.bin", path);
	fh = fopen(fullname, "a");

	n = fwrite(&blk->sz, sizeof(blk->sz), 1, fh);
	if (n != 1) return 0;

	fwrite(blk->payload, sizeof(blk->sz), 1, fh);
	n = ftell(fh);
	fclose(fh);

	return n;
}

static char test_payload[][1024] = {
	"an individual keeping track of the amount of mail they receive each day",
	"a reasonable assumption is that the number of pieces of mail received in a day obeys a Poisson distribution",
	"On a particular river, overflow floods occur once every 100 years on average.",
	"The number of magnitude 5 earthquakes per year in a country may not follow a Poisson distribution",
	"De Mensura Sortis seu; de Probabilitate Eventuum in Ludis a Casu Fortuito Pendentibus",
	"k! is the factorial of k",
	"distribution are known and are sharp",
	"sum of two independent random variables is Poisson-distributed",
	"Poisson races",
	"Biology example: the number of mutations on a strand of DNA per unit length.",
	"Management example: customers arriving at a counter or call centre",
	"Finance and insurance example: number of losses or claims occurring in a given period of time",
	"the number of days that the patients spend in the ICU is not Poisson distributed"
};

#define N (sizeof(test_payload) / 1024)

// ---

#pragma pack(push, 1)
struct skippy_data {
	uint64_t key;
	long     child_offset;
};
#pragma pack(pop)

struct skippy_fh {
	long   n_spans;
	char   path[MAX_DIR_PATH_NAME_LEN];

	FILE   *fh[SKIPPY_TOTAL_LEVELS];
	long   len[SKIPPY_TOTAL_LEVELS];
};

typedef long (*skippy_payload_hook)(char *, void *);

int skippy_fopen(struct skippy_fh *sfh, const char *path,
                 const char *mode, int n_spans)
{
	char fullname[SKIPPY_TOTAL_LEVELS][MAX_DIR_PATH_NAME_LEN];
	int fail_cnt = 0;

	sfh->n_spans = n_spans;
	strcpy(sfh->path, path);
	for (int i = 0; i < SKIPPY_TOTAL_LEVELS; i++) {
		FILE *fh;
		sprintf(fullname[i], "%s.skp-l%d.bin", path, i);
		fh = fopen(fullname[i], mode);
		sfh->fh[i] = fh;
		if (fh == NULL) {
			fail_cnt ++;
			sfh->len[i] = 0L;
		}
	
		fseek(fh, 0, SEEK_END);
		sfh->len[i] = ftell(fh) / sizeof(struct skippy_data);
		rewind(fh);
	}

	return fail_cnt;
}

void skippy_fseek_end(struct skippy_fh *sfh)
{
	for (int i = 0; i < SKIPPY_TOTAL_LEVELS; i++) {
		FILE *fh = sfh->fh[i];
		fseek(fh, 0, SEEK_END);
	}
}

void skippy_frewind(struct skippy_fh *sfh)
{
	for (int i = 0; i < SKIPPY_TOTAL_LEVELS; i++) {
		FILE *fh = sfh->fh[i];
		rewind(fh);
	}
}

static int
fappend_level(struct skippy_fh *sfh, int level, uint64_t key, long pay_offset)
{
	long len, child_offset;
	if (level == 0) {
		/* bottom level will always be appended */
		len = 1;
		child_offset = pay_offset;
	} else {
		/* other levels depend on lower level */
		len = sfh->len[level - 1];
		child_offset = (len - 1) * sizeof(struct skippy_data);
	}

	if (len % sfh->n_spans == 1) {
		struct skippy_data sd = {key, child_offset};
		return fwrite(&sd, sizeof(struct skippy_data), 1, sfh->fh[level]);
	} else {
		return 0;
	}
}

int skippy_fappend(struct skippy_fh *sfh, struct skippy_node *from,
                   skippy_payload_hook payload_hook)
{
	struct skippy_node *cur = from;

	/* ensure file written at the end */
	skippy_fseek_end(sfh);

	for (; cur != NULL; cur = cur->next[0]) {
		/* get last payload writing position */
		long pay_offset = payload_hook(sfh->path, cur);
		if (pay_offset == 0)
			break;

		for (int i = 0; i < SKIPPY_TOTAL_LEVELS; i++) {
			int incr = fappend_level(sfh, i, cur->key, pay_offset);
			sfh->len[i] += incr;
			if (incr == 0) break;
		}
	}
	return 0;
}

void level_print(struct skippy_fh *sfh, int level)
{
	long offset = 0;
	for (int i = 0; i < sfh->len[level]; i++) {
		struct skippy_data sd = {0};

		fpos_t pos;
		fgetpos(sfh->fh[level], &pos);

		size_t sz = fread(&sd, 1, sizeof(struct skippy_data), sfh->fh[level]);
		if (sz == 0) break;
		printf("@%lu[#%lu,@%lu] ", offset, sd.key, sd.child_offset);
		offset += sz;
	}
}

void skippy_fprint(struct skippy_fh *sfh)
{
	printf("on-disk skippy (span=%lu) of %s:\n", sfh->n_spans, sfh->path);

	skippy_frewind(sfh);

	for (int i = SKIPPY_TOTAL_LEVELS - 1; i >= 0; i--) {
		printf("level[%d], len=%lu: ", i, sfh->len[i]);

		level_print(sfh, i);
		printf("\n");
	}
}

void skippy_fclose(struct skippy_fh *sfh)
{
	for (int i = 0; i < SKIPPY_TOTAL_LEVELS; i++) {
		fclose(sfh->fh[i]);
	}
}

int main()
{
	struct skippy skippy;

	skippy_init(&skippy, 3);

	for (int i = 0; i < N; i++) {
		struct test_block *blk = malloc(sizeof(struct test_block));
		blk->sz = strlen(test_payload[i]) + 1;
		blk->payload = malloc(blk->sz);
		strcpy(blk->payload, test_payload[i]);
		skippy_node_init(&blk->sn, i);
		skippy_append(&skippy, &blk->sn);
	}
	
	skippy_print(&skippy, true);

	struct skippy_node *cur, *save;
	skippy_foreach(cur, save, &skippy, 0) {
		P_CAST(p, struct test_block, cur);
		printf("[%u] %s\n", cur->key, p->payload);
	}

	do {
		struct skippy_fh sfh;
		if (skippy_fopen(&sfh, "test", "a+", DEFAULT_SKIPPY_SPANS))
			break;

		skippy_fappend(&sfh, skippy.head[0], test_block_payload_hook);
		skippy_fprint(&sfh);

		skippy_fclose(&sfh);
	} while (0);

	skippy_free(&skippy, struct test_block, sn, free(p->payload); free(p));

	mhook_print_unfree();
	return 0;
}
