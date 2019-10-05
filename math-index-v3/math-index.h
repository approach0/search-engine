#include "common/types.h"
#include "strmap/strmap.h"
#include "codec/codec.h"
#include "invlist/invlist.h"

#include "dir-util/dir-util.h" /* for MAX_DIR_PATH_NAME_LEN */
#include "term-index/term-index.h" /* for doc_id_t and position_t */
#include "tex-parser/tex-parser.h" /* for subpaths */

typedef position_t exp_id_t;
#define MAX_EXP_ID UINT_MAX

/* invlist entry stored in dictionary */
struct math_invlist_entry {
	struct invlist *invlist;
	invlist_iter_t iterator;

	FILE *fh_symbinfo; /* used by writer */
	long  offset; /* symbol info offset, used by writer */

	FILE *fh_pf; /* used by writer */
	uint  pf; /* token path frequency of this entry */
};

typedef struct math_index {
	char dir[MAX_DIR_PATH_NAME_LEN];
	char mode[8];
	struct strmap *dict; /* dictionary */
	struct codec_buf_struct_info *cinfo;

	uint N; /* number of token paths in the collection */
} *math_index_t;

math_index_t math_index_open(const char*, const char*);
void         math_index_close(math_index_t);

int math_index_add(math_index_t, doc_id_t, exp_id_t, struct subpaths);

/* utilities */
int    mk_path_str(struct subpath*, int, char*);
struct codec_buf_struct_info *math_codec_info();

/* math inverted list items */
struct math_invlist_item {
	uint32_t docID; /* 4 bytes */
	union {
		uint32_t secID;
		struct {
			uint32_t expID:20;
			uint32_t sect_root:12;
		};
	}; /* 4 bytes */
	uint8_t  sect_width;
	uint8_t  orig_width;
	/* padding 16 bits here */
	uint32_t symbinfo_offset; /* pointing to symbinfo file offset */
	/* 4 bytes */
}; /* 4 x 4 = 16 bytes in total */

/* field index for math_invlist_item */
enum {
	FI_DOCID,
	FI_SECID,
	FI_SECT_WIDTH,
	FI_ORIG_WIDTH,
	FI_OFFSET
};

#define MAX_INDEX_EXP_SYMBOL_SPLITS 8

#pragma pack(push, 1)
struct symbinfo {
	uint32_t ophash:24;
	uint32_t n_splits:8;
	uint16_t symbol[MAX_INDEX_EXP_SYMBOL_SPLITS];
	uint8_t  splt_w[MAX_INDEX_EXP_SYMBOL_SPLITS];
}; /* 28 bytes */
#pragma pack(pop)
