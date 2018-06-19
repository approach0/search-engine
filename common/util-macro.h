#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
/* ROUND_UP(12, 5) should be 5 * (12/5 + 1) = 15 */
/* ROUND_UP(10, 5) should be 10 * (10/5 + 0) = 10 */

#define PTR_CAST(_a, _type, _b) \
	_type *_a = (_type *) _b
