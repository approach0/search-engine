#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for dup() */
#include <limits.h>

#include "dir-util/dir-util.h" /* MAX_DIR_PATH_NAME_LEN */

#include "skippy.h"
#include "ondisk-skippy.h"

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
	for (int i = ON_DISK_SKIPPY_LEVELS - 1; i >= 0; i--) {
		printf("level[%d] (@%ld): ", i, sfh->cur_offset[i]);

		level_buf_print(sfh, i);
		printf("\n");
	}
}

int skippy_fopen(struct skippy_fh *sfh, const char *path,
                 const char *mode, int n_spans)
{
	char fullname[ON_DISK_SKIPPY_LEVELS][MAX_DIR_PATH_NAME_LEN];
	int fail_cnt = 0;

	memset(sfh, 0, sizeof(struct skippy_fh));

	sfh->n_spans = n_spans;
	for (int i = 0; i < ON_DISK_SKIPPY_LEVELS; i++) {
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
	for (int i = 0; i < ON_DISK_SKIPPY_LEVELS; i++)
		if (sfh->fh[i]) fclose(sfh->fh[i]);
}

struct skippy_data skippy_fnext(struct skippy_fh *sfh, int level)
{
	size_t *cur = sfh->buf_cur + level;
	size_t end = sfh->buf_end[level];
	long   *cur_offset = sfh->cur_offset + level;
	struct skippy_data *buf = sfh->buf[level];

	(*cur) += 1;
	(*cur_offset) += sizeof(struct skippy_data);

	if (*cur > end) {
		/* passed the final one */
		return ((struct skippy_data){0, 0});
	} else if (*cur == end) {
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
	int level = ON_DISK_SKIPPY_LEVELS - 1;

	while (level >= 0) {
		// printf("[skipping] level= %d\n", level);
		// skippy_fh_buf_print(sfh);

		long next = peek_next_key(sfh, level);

		if (next <= key) {
			skippy_fnext(sfh, level);
		} else {
			size_t cur = sfh->buf_cur[level];
			struct skippy_data *buf = sfh->buf[level];
			long offset = buf[cur].child_offset;

			if (level > 0 && offset > sfh->cur_offset[level - 1])
				fbuf_seek(sfh, level - 1, offset);

			level --;
		}
	}

	return 0;
}

static int
level_write(struct skippy_fh *sfh, int level, uint64_t key, long pay_offset)
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

int skippy_fwrite(struct skippy_fh *sfh, struct skippy_node *from,
                  skippy_fwrite_hook wr_hook, void *args)
{
	struct skippy_node *cur = from;

	for (; cur != NULL; cur = cur->next[0]) {
		/* get last payload writing position */
		struct skippy_data sd = wr_hook(cur, args);
		if (sd.key == 0)
			break;

		for (int i = 0; i < ON_DISK_SKIPPY_LEVELS; i++) {
			int incr = level_write(sfh, i, sd.key, sd.child_offset);
			sfh->len[i] += incr;
			if (incr == 0) break;
		}
	}
	return 0;
}

static void level_fprint(long len, FILE *fh)
{
	long offset = 0;
	for (int i = 0; i < len; i++) {
		struct skippy_data sd = {0};
		fpos_t pos;
		fgetpos(fh, &pos);

		size_t sz = fread(&sd, 1, sizeof(struct skippy_data), fh);
		if (sz == 0) break;
		printf("@%lu[#%lu,@%lu] ", offset, sd.key, sd.child_offset);
		offset += sz;
	}
}

void skippy_fprint(struct skippy_fh *sfh)
{
	printf("on-disk skippy (span=%lu):\n", sfh->n_spans);

	for (int i = ON_DISK_SKIPPY_LEVELS - 1; i >= 0; i--) {
		int fd = fileno(sfh->fh[i]);
		FILE *fh = fdopen(dup(fd), "r");

		printf("level[%d], len=%lu: ", i, sfh->len[i]);

		rewind(fh);
		level_fprint(sfh->len[i], fh);

		printf("\n");
	}
}
