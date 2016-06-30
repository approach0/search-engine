#include <stdint.h>
#include <stdlib.h>

size_t for_decompress(uint32_t*, uint32_t*, size_t, size_t*);
size_t for_compress(uint32_t*, size_t, uint32_t*, size_t*);

size_t for_delta_compress(const uint32_t*, size_t, uint32_t*, size_t*);
size_t for_delta_decompress(const uint32_t*, uint32_t*, size_t, size_t*);
