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

static long test_block_payload_hook(void *blk_, size_t *sz, void *args)
{
	size_t offset;
	P_CAST(blk, struct test_block, blk_);
	P_CAST(fh, FILE, args);

	*sz = fwrite(&blk->sz, 1, sizeof(blk->sz), fh);
	*sz += fwrite(blk->payload, 1, blk->sz, fh);

	offset = ftell(fh);

	return offset;
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

#define ON_DISK_SKIPPY_MIN_N_BLOCKS 2
#define ON_DISK_SKIPPY_SKIPPY_SPANS 6
#define ON_DISK_SKIPPY_BUF_LEN 3

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

	struct skippy_data buf[SKIPPY_TOTAL_LEVELS][ON_DISK_SKIPPY_BUF_LEN];
	size_t buf_cur[SKIPPY_TOTAL_LEVELS];
	size_t buf_end[SKIPPY_TOTAL_LEVELS]; /* final available position */
	long   cur_offset[SKIPPY_TOTAL_LEVELS];
};

typedef long (*skippy_payload_hook)(void *, size_t *, void *);

static long frebuf(struct skippy_fh *sfh, int level)
{
	FILE *fh = sfh->fh[level];
	size_t *cur = sfh->buf_cur + level;
	size_t *end = sfh->buf_end + level;
	struct skippy_data *buf = sfh->buf[level];
	long cur_offset = ftell(fh) - sizeof(struct skippy_data);

	buf[0] = buf[*cur];
	*end = fread(buf + 1, sizeof(struct skippy_data),
	             ON_DISK_SKIPPY_BUF_LEN - 1, fh);
	return cur_offset;
}

static void frst_buf(struct skippy_fh *sfh, int level)
{
	sfh->buf[level][0].key = 0;
	sfh->buf[level][0].child_offset = 0;
	sfh->buf_cur[level] = 0;
	sfh->buf_end[level] = 0;
	sfh->cur_offset[level] = frebuf(sfh, level);
}

static void level_buf_print(struct skippy_fh *sfh, int level)
{
	size_t cur = sfh->buf_cur[level];
	size_t end = sfh->buf_end[level];
	struct skippy_data *buf = sfh->buf[level];

	for (int i = 0; i <= end; i++)
		if (i == cur)
			printf("[#%lu,@%lu] ", buf[i].key, buf[i].child_offset);
		else
			printf(" #%lu,@%lu  ", buf[i].key, buf[i].child_offset);
}

void skippy_fh_buf_print(struct skippy_fh *sfh)
{
	for (int i = SKIPPY_TOTAL_LEVELS - 1; i >= 0; i--) {
		printf("level[%d] (@%ld): ", i, sfh->cur_offset[i]);

		level_buf_print(sfh, i);
		printf("\n");
	}
}

int skippy_fopen(struct skippy_fh *sfh, const char *path,
                 const char *mode, int n_spans)
{
	char fullname[SKIPPY_TOTAL_LEVELS][MAX_DIR_PATH_NAME_LEN];
	int fail_cnt = 0;

	memset(sfh, 0, sizeof(struct skippy_fh));

	sfh->n_spans = n_spans;
	strcpy(sfh->path, path);
	for (int i = 0; i < SKIPPY_TOTAL_LEVELS; i++) {
		/* set fh */
		FILE *fh;
		sprintf(fullname[i], "%s.skp-l%d.bin", path, i);
		fh = fopen(fullname[i], mode);
		sfh->fh[i] = fh;
		if (fh == NULL) {
			fail_cnt ++;
			continue;
		}

		/* set len */
		fseek(fh, 0, SEEK_END);
		sfh->len[i] = ftell(fh) / sizeof(struct skippy_data);
		rewind(fh);

		/* reset reader buf */
		frst_buf(sfh, i);
	}

	return fail_cnt;
}

void skippy_fclose(struct skippy_fh *sfh)
{
	for (int i = 0; i < SKIPPY_TOTAL_LEVELS; i++)
		if (sfh->fh[i]) fclose(sfh->fh[i]);
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

struct skippy_data skippy_fnext(struct skippy_fh *sfh, int level)
{
	size_t *cur = sfh->buf_cur + level;
	size_t *end = sfh->buf_end + level;
	long   *cur_offset = sfh->cur_offset + level;
	struct skippy_data *buf = sfh->buf[level];

	(*cur) += 1;
	(*cur_offset) += sizeof(struct skippy_data);

	if (*cur > *end) {
		/* passed the final one */
		return ((struct skippy_data){0, 0});
	} else if (*cur == *end) {
		/* unstable state (next one unavailable) */
		frebuf(sfh, level);
		*cur = 0;
	}

	return buf[*cur];
}

static long peek_next_key(struct skippy_fh *sfh, int level)
{
	size_t cur = sfh->buf_cur[level];
	size_t end = sfh->buf_end[level];
	struct skippy_data *buf = sfh->buf[level];

	if (cur + 1 > end)
		return LONG_MAX;
	else
		return buf[cur + 1].key;
}

static long fbuf_seek(struct skippy_fh *sfh, int level, long offset)
{
	FILE *fh = sfh->fh[level];
	fseek(fh, offset, SEEK_SET);

	frst_buf(sfh, level);
	skippy_fnext(sfh, level);
	return 0;
}

long skippy_fskip_to(struct skippy_fh *sfh, uint64_t key)
{
	int level = SKIPPY_TOTAL_LEVELS - 1;

	while (level >= 0) {
		long next = peek_next_key(sfh, level);

		if (next <= key) {
			skippy_fnext(sfh, level);
		} else {
			size_t cur = sfh->buf_cur[level];
			struct skippy_data *buf = sfh->buf[level];
			long offset = buf[cur].child_offset;

			if (offset != sfh->cur_offset[level])
				fbuf_seek(sfh, level, offset);

			level --;
		}
	}

	return 0;
}

static int
fappend_level(struct skippy_fh *sfh, int level, uint64_t key, long pay_offset)
{
	long len, child_offset;
	if (level == 0) {
		/* bottom level will always be appended */
		len = ON_DISK_SKIPPY_MIN_N_BLOCKS;
		child_offset = pay_offset;
	} else {
		/* other levels depend on lower level */
		len = sfh->len[level - 1];
		child_offset = (len - 1) * sizeof(struct skippy_data);
	}

	if (len % sfh->n_spans == ON_DISK_SKIPPY_MIN_N_BLOCKS) {
		struct skippy_data sd = {key, child_offset};
		return fwrite(&sd, sizeof(struct skippy_data), 1, sfh->fh[level]);
	} else {
		return 0;
	}
}

int skippy_fappend(struct skippy_fh *sfh, struct skippy_node *from,
                   skippy_payload_hook payload_hook, void *args)
{
	struct skippy_node *cur = from;

	/* ensure file written at the end */
	skippy_fseek_end(sfh);

	for (; cur != NULL; cur = cur->next[0]) {
		/* get last payload writing position */
		size_t wr_sz;
		long pay_offset = payload_hook(cur, &wr_sz, args);
		if (pay_offset == 0)
			break;

		for (int i = 0; i < SKIPPY_TOTAL_LEVELS; i++) {
			int incr = fappend_level(sfh, i, cur->key, pay_offset - wr_sz);
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

int main()
{
	struct skippy skippy;

	skippy_init(&skippy, 3);

	for (int i = 0; i < N; i++) {
		struct test_block *blk = malloc(sizeof(struct test_block));
		blk->sz = strlen(test_payload[i]) + 1;
		blk->payload = malloc(blk->sz);
		strcpy(blk->payload, test_payload[i]);
		skippy_node_init(&blk->sn, i + 1);
		skippy_append(&skippy, &blk->sn);
	}

	skippy_print(&skippy, true);

	struct skippy_node *cur, *save;
	skippy_foreach(cur, save, &skippy, 0) {
		P_CAST(p, struct test_block, cur);
		printf("[%u] %s\n", cur->key, p->payload);
	}


	FILE *fh = fopen("postinglist.bin", "a+");

	do {
		struct skippy_fh sfh;

		/* writing test */
		if (skippy_fopen(&sfh, "test", "a+", ON_DISK_SKIPPY_SKIPPY_SPANS))
			break;
		skippy_fappend(&sfh, skippy.head[0], test_block_payload_hook, fh);
		skippy_fprint(&sfh);
		printf("\n");
		skippy_fclose(&sfh);

		/* reading test */
		if (skippy_fopen(&sfh, "test", "r", ON_DISK_SKIPPY_SKIPPY_SPANS))
			break;

		struct skippy_data sd;
		do {
			sd = skippy_fnext(&sfh, 0);
			if (0 == sd.key) break;
			// printf("---\n");
			skippy_fh_buf_print(&sfh);

			size_t sz;
			char buf[1024];
			fseek(fh, sd.child_offset, SEEK_SET);
			fread(&sz, sizeof(size_t), 1, fh);
			if ((sz = fread(buf, 1, sz, fh))) {
				buf[sz] = '\0';
				printf("<<%s>>\n", buf);
			}
		} while (1);

		skippy_fclose(&sfh);
	} while (0);

	fclose(fh);

	skippy_free(&skippy, struct test_block, sn, free(p->payload); free(p));

	mhook_print_unfree();
	return 0;
}
