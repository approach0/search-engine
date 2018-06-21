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
