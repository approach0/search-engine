#pragma once
#include "common/common.h"
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

	char *symbinfo_path;
	char *pf_path;
	uint  pf; /* token path frequency of this entry */
};

struct math_index_stats {
	uint N; /* number of token paths in the collection */
	uint n_tex; /* total number of TeX indexed */
};

typedef struct math_index {
	char dir[MAX_DIR_PATH_NAME_LEN];
	char mode[8];
	struct strmap *dict; /* dictionary */
	struct codec_buf_struct_info *cinfo;
	size_t memo_usage; /* of all caching buffers */
	struct math_index_stats stats;
} *math_index_t;

math_index_t math_index_open(const char*, const char*);

void         math_index_flush(math_index_t);
void         math_index_close(math_index_t);

int math_index_load(math_index_t, size_t);

enum math_reader_medium {
	MATH_READER_MEDIUM_NONE,
	MATH_READER_MEDIUM_ONDISK,
	MATH_READER_MEDIUM_INMEMO
};

struct math_invlist_entry_reader {
	invlist_iter_t reader;
	FILE *fh_symbinfo;
	uint pf;
	enum math_reader_medium medium;
};

struct math_invlist_entry_reader
math_index_lookup(math_index_t, const char *);

void math_index_print(math_index_t);

size_t math_index_add(math_index_t, doc_id_t, exp_id_t, struct subpaths);

/* utilities */
int    mk_path_str(struct subpath*, int, char*);
struct codec_buf_struct_info *math_codec_info();

/* math inverted list items */
struct math_invlist_item {
	/* 8-byte global key */
	uint32_t docID; /* 4 bytes */
	union {
		uint32_t secID;
		struct {
			uint16_t sect_root;
			uint16_t expID;
		};
	}; /* 4 bytes */

	uint8_t  sect_width;
	uint8_t  orig_width;
	/* padding 16 bits here */
	uint32_t symbinfo_offset; /* pointing to symbinfo file offset */
	/* 4 bytes */
}; /* 4 x 4 = 16 bytes in total */

#define key2doc(_64key) (_64key >> 32)
#define key2exp(_64key) ((_64key >> 16) & 0xffff)
#define key2rot(_64key) (_64key & 0xffff)

/* field index for math_invlist_item */
enum {
	FI_DOCID,
	FI_EXPID,
	FI_SECT_ROOT,
	FI_SECT_WIDTH,
	FI_ORIG_WIDTH,
	FI_OFFSET,
	N_MATH_INVLIST_ITEM_FIELDS
};

#define MAX_INDEX_EXP_SYMBOL_SPLITS 8

#pragma pack(push, 1)
struct symbsplt {
	uint16_t symbol;
	uint8_t  splt_w;
};

struct symbinfo {
	uint32_t ophash:24;
	uint32_t n_splits:8;

	struct symbsplt split[MAX_INDEX_EXP_SYMBOL_SPLITS];
};
#pragma pack(pop)

#define SYMBINFO_SIZE(_n) offsetof(struct symbinfo, split[_n]) /* 4 + 3n */
