#include <stdint.h>
#include <stddef.h>

/* import frame of reference */
#include "for.h"

struct for_delta_args {
	size_t b;
};
/* import END */

enum codec_method {
	CODEC_FOR,
	CODEC_FOR_DELTA,
	CODEC_PLAIN /* do nothing */
};

#define CODEC_DEFAULT_ARGS NULL

struct codec {
	enum codec_method method;
	void             *args; /* algorithm-dependent arguments */
};

struct codec *codec_new(enum codec_method, void*);
void          codec_free(struct codec*);

size_t codec_compress(struct codec*, const uint32_t*, size_t, void*);

size_t codec_decompress(struct codec*, const void*, uint32_t*, size_t);

size_t encode_struct_arr(void*, const void*, struct codec**, size_t, size_t);

size_t decode_struct_arr(void*, const void*, struct codec**, size_t, size_t);

char *codec_method_str(enum codec_method);
