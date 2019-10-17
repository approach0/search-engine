#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for dup() */
#include <limits.h>

#include "dir-util/dir-util.h" /* MAX_DIR_PATH_NAME_LEN */

#include "skippy.h"
#include "ondisk-skippy.h"

/* Circular buffer of on-disk skippy, explained.
 *
 * The first (rebuf-ed) state:
 *   s                   e
 * [-1] [0] [1] [2] ... [5]
 * where start position `s' is empty and has a negative offset
 * to the real file beginning, and end `e' is an unstable position
 * because the next of which is unavailable.
 *
 * After the next rebuf (i.e., the next frame), the state then
 * looks like:
 *                       e
 * [5] [6] [7] [8] ... [11]
 * where the first position becomes the end one in previous state.
 */

static void frebuf(struct skippy_fh *sfh, int level)
{
	FILE *fh = sfh->fh[level];
	size_t *cur = sfh->buf_cur + level;
	size_t *end = sfh->buf_end + level;
	long   *offset = sfh->cur_offset + level;
	struct skippy_data *buf = sfh->buf[level];

	/* update current pointer and offset (offset can be negative as in
	 * the start position) */
	*cur = 0;
	*offset = ftell(fh) - sizeof(struct skippy_data);

	/* next frame of the circular buffer */
	buf[*cur] = buf[*end];
	*end = fread(buf + 1, sizeof(struct skippy_data),
	             ON_DISK_SKIPPY_BUF_LEN - 1, fh);
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
	printf(" [%lu / %lu] ", cur, end);
}

void skippy_fh_buf_print(struct skippy_fh *sfh)
{
	for (int i = ON_DISK_SKIPPY_LEVELS - 1; i >= 0; i--) {
		printf("level[%d] (@%ld): ", i, sfh->cur_offset[i]);

		level_buf_print(sfh, i);
		printf("\n");
	}
}

static long fbuf_seek(struct skippy_fh *sfh, int level, long offset)
{
	FILE *fh = sfh->fh[level];

	/* seek to specified offset and do rebuf */
	fseek(fh, offset, SEEK_SET);
	frebuf(sfh, level);

	/* at this point the first position is unknown. */
	sfh->buf[level][0].key = 0;
	sfh->buf[level][0].child_offset = 0;

	/* advance one step to be positioned at a valid start.
	 * (after advance, the first position should not be used) */
	skippy_fnext(sfh, level);
	return 0;
}

int skippy_fopen(struct skippy_fh *sfh, const char *path,
                 const char *mode, int n_spans)
{
	char fullname[ON_DISK_SKIPPY_LEVELS][MAX_DIR_PATH_NAME_LEN];
	int fail_cnt = 0;

	sfh->n_spans = n_spans; /* set span */

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

		/* setup start position (only used for reader) */
		if (mode[0] == 'r') {
			sfh->buf_end[i] = 0;
			fbuf_seek(sfh, i, 0);
		}
	}

	return fail_cnt;
}

void skippy_fclose(struct skippy_fh *sfh)
{
	for (int i = 0; i < ON_DISK_SKIPPY_LEVELS; i++)
		if (sfh->fh[i]) fclose(sfh->fh[i]);
}

int skippy_fend(struct skippy_fh *sfh)
{
	size_t cur = sfh->buf_cur[0];
	size_t end = sfh->buf_end[0];
	return (cur > end); /* termination state */
}

struct skippy_data skippy_fnext(struct skippy_fh *sfh, int level)
{
	size_t *cur = sfh->buf_cur + level;
	size_t end = sfh->buf_end[level];
	long   *cur_offset = sfh->cur_offset + level;
	struct skippy_data *buf = sfh->buf[level];

	/* advance */
	(*cur) += 1;
	(*cur_offset) += sizeof(struct skippy_data);

	if (*cur > end) {
		/* passed the final one, i.e., termination state */
		return ((struct skippy_data){0, 0});
	} else if (*cur == end) {
		/* unstable state (next one unavailable) */
		frebuf(sfh, level);
	}

	return buf[*cur];
}

struct skippy_data skippy_fcur(struct skippy_fh *sfh, int level)
{
	size_t cur = sfh->buf_cur[level];
	struct skippy_data *buf = sfh->buf[level];
	return buf[cur];
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

/* skip to the skippy data who is just less than or equal to `key' */
struct skippy_data skippy_fskip(struct skippy_fh *sfh, uint64_t key)
{
	int level = ON_DISK_SKIPPY_LEVELS - 1;

	while (level >= 0) {
//		printf("[skipping] level= %d\n", level);
//		skippy_fh_buf_print(sfh);

		long next = peek_next_key(sfh, level);

		if (next <= key) {
			skippy_fnext(sfh, level);
		} else {
			size_t cur = sfh->buf_cur[level];
			struct skippy_data *buf = sfh->buf[level];
			long offset = buf[cur].child_offset;

			/* the lower level (child) advances to child_offset */
			if (level > 0 && offset > sfh->cur_offset[level - 1])
				fbuf_seek(sfh, level - 1, offset);

			level --;
		}
	}

	size_t cur = sfh->buf_cur[0];
	return sfh->buf[0][cur];
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
		/* for writer, we avoid using cur_offset */
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

void skippy_fflush(struct skippy_fh *sfh)
{
	for (int i = 0; i < ON_DISK_SKIPPY_LEVELS; i++) {
		fflush(sfh->fh[i]);
	}
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
