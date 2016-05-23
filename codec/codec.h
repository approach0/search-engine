#include <stdint.h>
#include <stddef.h>

enum codec_method {
	CODEC_FOR_DELTA,
	CODEC_PLAIN /* do nothing */
};

struct for_delta_args {
	int pack_size;
	/* ...... */
	/* whatever needed for FOR-delta compression */
};

struct codec {
	enum codec_method method;
	void             *args; /* algorithm-dependent arguments */
};

size_t codec_compress(struct codec*, const uint32_t*, size_t, void*);

size_t codec_decompress(struct codec*, const void*, uint32_t*, size_t);

size_t encode_struct_arr(void*, const void*, struct codec*, size_t, size_t);

size_t decode_struct_arr(void*, const void*, struct codec*, size_t, size_t);

char *codec_method_str(enum codec_method);
