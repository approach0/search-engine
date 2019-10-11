#include <stdint.h>
#include <stdlib.h>

size_t for32_decompress(uint32_t*, uint32_t*, size_t, size_t*);
size_t for32_compress(uint32_t*, size_t, uint32_t*, size_t*);

size_t for32_delta_compress(const uint32_t*, size_t, uint32_t*, size_t*);
size_t for32_delta_decompress(const uint32_t*, uint32_t*, size_t, size_t*);
