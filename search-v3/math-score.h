#include <stdint.h>
#include "config.h"

struct math_score_factors {
	float symbol_sim; /* normalized */
	float struct_sim; /* sum of ipf */
	float doc_lr_paths;

	/* precomputed */
	float upp_sf;
	float low_sf;
};

void math_score_precalc(struct math_score_factors *);
float math_score_ipf(float, float);
float math_score_calc(struct math_score_factors *);

float math_score_upp(void *, float);
float math_score_low(void *, float);
