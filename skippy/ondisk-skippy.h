/* test config */
// #define ON_DISK_SKIPPY_LEVELS 3
// #define ON_DISK_SKIPPY_MIN_N_BLOCKS 2
// #define ON_DISK_SKIPPY_SKIPPY_SPANS 6
// #define ON_DISK_SKIPPY_BUF_LEN 3

/* deploy config */
#define ON_DISK_SKIPPY_LEVELS 2
#define ON_DISK_SKIPPY_MIN_N_BLOCKS 2
#define ON_DISK_SKIPPY_SKIPPY_SPANS 128
#define ON_DISK_SKIPPY_BUF_LEN ON_DISK_SKIPPY_SKIPPY_SPANS

#pragma pack(push, 1)
struct skippy_data {
	uint64_t key;
	long     child_offset;
};
#pragma pack(pop)

struct skippy_fh {
	/* book keeping */
	long   n_spans;
	FILE   *fh[ON_DISK_SKIPPY_LEVELS];
	long   len[ON_DISK_SKIPPY_LEVELS];
	/* skip reader */
	struct skippy_data buf[ON_DISK_SKIPPY_LEVELS][ON_DISK_SKIPPY_BUF_LEN];
	size_t buf_cur[ON_DISK_SKIPPY_LEVELS];
	size_t buf_end[ON_DISK_SKIPPY_LEVELS]; /* final available position */
	long   cur_offset[ON_DISK_SKIPPY_LEVELS];
};

struct skippy_node;
typedef struct skippy_data (*skippy_fwrite_hook)(struct skippy_node *, void *);

int skippy_fopen(struct skippy_fh*, const char*, const char*, int);
void skippy_fclose(struct skippy_fh*);

int skippy_fwrite(struct skippy_fh*, struct skippy_node*, skippy_fwrite_hook, void*);

int skippy_fend(struct skippy_fh*);

struct skippy_data skippy_fnext(struct skippy_fh*, int);
long skippy_fskip_to(struct skippy_fh*, uint64_t);

void skippy_fh_buf_print(struct skippy_fh*);
void skippy_fprint(struct skippy_fh*);
