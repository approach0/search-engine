#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "skippy.h"
#include "ondisk-skippy.h"

#include "mhook/mhook.h"
#include "common/common.h" /* PTR_CAST */

#define N (sizeof(test_payload) / 1024)

struct test_block {
	struct skippy_node sn;
	char  *payload;
	size_t sz;
};

static struct skippy_data
test_block_write_hook(struct skippy_node *blk_, void *args)
{
	struct skippy_data sd;
	PTR_CAST(fh, FILE, args);
	PTR_CAST(blk, struct test_block, blk_);

	fseek(fh, 0, SEEK_END);
	sd.child_offset = ftell(fh);

	fwrite(&blk->sz, 1, sizeof(blk->sz), fh);
	fwrite(blk->payload, 1, blk->sz, fh);

	sd.key = sd.child_offset + 1;
	return sd;
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

void test_write(struct skippy *skippy)
{
	FILE *fh = fopen("postinglist.bin", "a+");

	struct skippy_fh sfh;
	do {
		if (skippy_fopen(&sfh, "test", "a", ON_DISK_SKIPPY_SKIPPY_SPANS))
			break;
		skippy_fwrite(&sfh, skippy->head[0], test_block_write_hook, fh);
		skippy_fclose(&sfh);

		if (skippy_fopen(&sfh, "test", "r", ON_DISK_SKIPPY_SKIPPY_SPANS))
			break;
		/* print file content */
		skippy_fprint(&sfh);
		printf("\n");
		skippy_fclose(&sfh);

	} while (0);

	fclose(fh);
}

void test_next_iterator()
{
	FILE *fh = fopen("postinglist.bin", "a+");

	struct skippy_fh sfh;
	do {
		if (skippy_fopen(&sfh, "test", "r", ON_DISK_SKIPPY_SKIPPY_SPANS))
			break;
		skippy_fprint(&sfh);
		printf("\n");

		do {
			struct skippy_data sd = skippy_fnext(&sfh, 0);
			if (0 == sd.key) break;
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
}

void test_jump_iterator()
{
	struct skippy_fh sfh;
	do {
		/* reading test (skipping) */
		if (skippy_fopen(&sfh, "test", "r", ON_DISK_SKIPPY_SKIPPY_SPANS))
			break;
		skippy_fprint(&sfh);
		printf("\n");

		uint64_t from = 916, to = 2500;

//		printf("enter from, to ...\n");
//		scanf("%lu, %lu", &from, &to);
		printf("\n");
		printf("from %lu to %lu:\n", from, to);
		do {
			struct skippy_data sd = skippy_fnext(&sfh, 0);
			if (0 == sd.key) break;

			if (sd.key == from) {
				printf("\n FROM \n");
				skippy_fh_buf_print(&sfh);

				skippy_fskip(&sfh, to);
				printf("\n AFTER JUMP \n");
				skippy_fh_buf_print(&sfh);

				printf("\n REPEAT \n");
				skippy_fskip(&sfh, to);
				skippy_fh_buf_print(&sfh);
			}
		} while (1);

		skippy_fclose(&sfh);
	} while (0);
}

int main(int argc, char *argv[])
{
	int opt;
	struct skippy skippy;

	while ((opt = getopt(argc, argv, "hwrj")) != -1) {
		switch (opt) {
		case 'h':
			printf("OPTIONS:\n");
			printf("%s -h |"
			       " -w (writing test) | "
			       " -r (reading test) | "
			       " -j (jump test) "
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'w':
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
				PTR_CAST(p, struct test_block, cur);
				printf("[%u] %s\n", cur->key, p->payload);
			}

			test_write(&skippy);
			skippy_free(&skippy, struct test_block, sn, free(p->payload); free(p));
			break;

		case 'r':
			test_next_iterator();
			break;

		case 'j':
			test_jump_iterator();
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

exit:
	mhook_print_unfree();
	return 0;
}
