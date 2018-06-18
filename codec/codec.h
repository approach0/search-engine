#pragma once
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
	CODEC_GZ,
	CODEC_PLAIN /* do nothing */
};

#define CODEC_DEFAULT_ARGS NULL

struct codec {
	enum codec_method method;
	void             *args; /* algorithm-dependent arguments */
};

struct codec *codec_new(enum codec_method, void*);
void          codec_free(struct codec*);

size_t codec_compress_ints(struct codec*, const uint32_t*, size_t, void*);

size_t codec_decompress_ints(struct codec*, const void*, uint32_t*, size_t);

size_t codec_compress(struct codec*, const void*, size_t, void**);

size_t codec_decompress(struct codec*, const void*, size_t, void*, size_t);

char *codec_method_str(enum codec_method);

/* codec array constructor/deconstructor */
struct codec **codec_new_array(int, ...);
void codec_array_free(int, struct codec **);
