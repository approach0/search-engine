#pragma once

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
/* ROUND_UP(12, 5) should be 5 * (12/5 + 1) = 15 */
/* ROUND_UP(10, 5) should be 10 * (10/5 + 0) = 10 */

#define PTR_CAST(_a, _type, _b) \
	_type *_a = (_type *) _b

#undef offsetof
#define offsetof(_type, _member) ((uintptr_t)&((_type*)0)->_member)

#include <stdint.h>
#define container_of(_member_addr, _type, _member_name) \
	(_member_addr == NULL) ? NULL : \
	(_type*)((uintptr_t)(_member_addr) - offsetof(_type, _member_name))

#include <stddef.h> /* for size_t */
#include <stdio.h>  /* for printf */
static inline void print_size(size_t _sz)
{
	float sz = (float)_sz;
	if (_sz >= (1024 << 20))
		printf("%.3f GB", sz / (1024.f * 1024.f * 1024.f));
	else if (_sz >= (1024 << 10))
		printf("%.3f MB", sz / (1024.f * 1024.f));
	else if (_sz > 1024)
		printf("%.3f KB", sz / 1024.f);
	else
		printf("%lu Byte(s)", _sz);
}

#define __1MB__ (1024 << 10)
#define MB * __1MB__

#define __1KB__ 1024
#define KB * __1KB__

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
