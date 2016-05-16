#include <stdint.h>
#include <stddef.h>

enum codec_method {
	CODEC_FOR_DELTA
};

struct for_delta_args {
	int pack_size;
	/* ...... */
	/* whatever needs for FOR-delta compression */
};

struct codec {
	enum codec_method method;
	void             *args; /* algorithm-dependent arguments */
};

size_t codec_compress(struct codec*, const uint32_t*, void*, size_t);

size_t codec_decompress(struct codec*, void*, uint32_t*, size_t);
