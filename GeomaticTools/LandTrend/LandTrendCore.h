#pragma once

#include "LandTrendUtil.h"

namespace LTR
{


	CBestModelInfo fit_trajectory_v2(const CRealArray& all_years, const CRealArray& vvals, const CBoolArray& goods,
		size_t minneeded, int background, int modifier,
		REAL_TYPE  desawtooth_val, REAL_TYPE  pval, size_t max_segments, REAL_TYPE  recovery_threshold,
		REAL_TYPE  distweightfactor, size_t  vertexcountovershoot, REAL_TYPE  bestmodelproportion, TFitMethod fit_method);


	CBestModelInfo tbcd_v2(const CRealArray& all_x, const CRealArray& y, const CBoolArray& goods,
		size_t max_count, REAL_TYPE pval, REAL_TYPE recovery_threshold,
		REAL_TYPE distweightfactor, size_t vertexcountovershoot, REAL_TYPE bestmodelproportion, TFitMethod fit_method);



}