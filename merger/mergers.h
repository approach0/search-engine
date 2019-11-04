#pragma once

/* merge set data structure */
#include "merge-set.h"

/* general merger operations on merge set */
#define MERGER_ITER_CALL(_merger, _fun, _i, ...) \
	((_merger)->set._fun[_i])((_merger)->set.iter[_i], ##__VA_ARGS__)

#define merger_map_call(_merger, _fun, _j, ...) \
	MERGER_ITER_CALL(_merger, _fun, (_merger)->map[_j], ##__VA_ARGS__)

#define merger_map_upp(_merger, _j) \
	((_merger)->set.upp[_merger->map[_j]])

/* different merge strategies */
#include "maxscore-merger.h"
