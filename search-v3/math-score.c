#include <math.h>
#include "common/common.h"
#include "math-score.h"

const float eta = MATH_SCORE_ETA;

#define DOC_LEN_PENALTY ((1.f - eta) + eta * (1.f / logf(1.f + dl)))

void math_score_precalc(struct math_score_factors *msf)
{
	float dl;

	dl = 1.f;
	msf->upp_sf = 1.f * DOC_LEN_PENALTY;

	dl = (float)MAX_MATCHED_PATHS;
	msf->low_sf = .5f * DOC_LEN_PENALTY;

	for (int i = 0; i < MAX_MATCHED_PATHS; i++) {
		dl = (float)i;
		msf->penalty_tab[i] = DOC_LEN_PENALTY;
	}
}

float math_score_ipf(float N, float pf)
{
	return logf(N / pf);
}

float math_score_calc(struct math_score_factors *msf)
{
	/* rescale symbol similarity score */
	//float rescale = (0.5f + msf->symbol_sim / 2.f);
	float rescale = 1.f / (1.f + powf(1.f - msf->symbol_sim, 2.f));

	float sf = rescale * msf->penalty_tab[msf->doc_lr_paths];

	return msf->struct_sim * sf;
}

float math_score_upp(void *msf_, float sum_ipf)
{
	P_CAST(msf, struct math_score_factors, msf_);
	return sum_ipf * msf->upp_sf;
}

float math_score_low(void *msf_, float sum_ipf)
{
	P_CAST(msf, struct math_score_factors, msf_);
	return sum_ipf * msf->low_sf;
}
