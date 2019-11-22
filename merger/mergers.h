#pragma once

/* merge set data structure */
#include "merge-set.h"

/* general merger operations on merge set */
#define MERGER_ITER_CALL(_merger, _fun, _i, ...) \
	((_merger)->set._fun[_i])((_merger)->set.iter[_i], ##__VA_ARGS__)

#define merger_map_call(_merger, _fun, _j, ...) \
	MERGER_ITER_CALL(_merger, _fun, (_merger)->map[_j], ##__VA_ARGS__)

#define merger_map_prop(_merger, _name, _j) \
	((_merger)->set._name[_merger->map[_j]])

/* merger no-relax upperbound */
float no_upp_relax(void*, float);

/* merger iteration printing callback type */
typedef float (*merger_upp_relax_fun)(void *, float);

/* merger iteration printing callback type */
typedef void  (*merger_keyprint_fun)(uint64_t);

/* different merge strategies */
#include "maxscore-merger.h"
#include "podium-merger.h"
