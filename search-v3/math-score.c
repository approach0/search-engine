#include <math.h>
#include "common/common.h"
#include "math-score.h"

void math_score_precalc(struct math_score_factors *msf)
{
	const float eta = MATH_SCORE_ETA;
	float dn;

	dn = 1.f;
	msf->upp_sf = 1.f * ((1.f - eta) + eta * (1.f / logf(1.f + dn)));
	dn = (float)MAX_MATCHED_PATHS;
	msf->low_sf = .5f * ((1.f - eta) + eta * (1.f / logf(1.f + dn)));
}

float math_score_calc(struct math_score_factors *msf)
{
	float dn = msf->doc_lr_paths;
	float eta = MATH_SCORE_ETA;

	float sf = msf->symbol_sim * ((1.f - eta) + eta * (1.f / logf(1.f + dn)));

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
