#pragma once

#include <string>
#include <vector>
#include <valarray>
#include <algorithm>
#include <bitset>
#include <assert.h>

namespace LTR
{

typedef double REAL_TYPE;
typedef std::valarray<REAL_TYPE> CRealArray;
typedef std::valarray<size_t> CVectices;
typedef std::valarray<bool> CBoolArray;// Base;
static const size_t UNKNOWN_POS =-1;


inline size_t sum(const CBoolArray& v)
{
    size_t sum = 0;
    for (size_t i=0; i<v.size(); i++)
    {
        if(v[i])
            sum ++;
    }


    return sum;
}


enum TFitMethod { FIT_EARLY_TO_LATE, FIT_MPFIT, NB_FIT_METHODS };
//typedef std::bitset<NB_FIT_METHODS> TFitMethods;


inline CRealArray convert(const CVectices& v)
{
    CRealArray c(v.size());
    for (size_t i = 0; i < v.size(); i++)
        c[i] = REAL_TYPE(v[i]);

    return c;
}


inline std::slice get_slice(size_t begin, size_t end)
{
    return std::slice(begin, end - begin + 1, 1);
}

inline CVectices allpos(size_t n)
{
    CVectices x(n);
    for (size_t i = 0; i < x.size(); i++)
        x[i] = i;

    return x;
}

inline CVectices get_pos(const CBoolArray& mask)
{
    CVectices pos = allpos(mask.size());
    return pos[mask];
}


template <class T>
std::valarray <T>  subset(const std::valarray <T>& v, size_t begin, size_t end)
{
    assert(begin < v.size());
    assert(end < v.size());

    return v[std::slice(begin, end - begin + 1, 1)];
}

//{
//	return in.size();
//}

inline REAL_TYPE get_slope(const CRealArray& x_endpoints, const CRealArray& y_endpoints)
{
    assert(x_endpoints.size() == 2);
    assert(y_endpoints.size() == 2);

    REAL_TYPE span = x_endpoints[1] - x_endpoints[0];
    return (y_endpoints[1] - y_endpoints[0]) / span;
}

inline REAL_TYPE get_range(const CRealArray& y)
{
    return y.max() - y.min();
}


typedef std::pair<REAL_TYPE, REAL_TYPE> RegressP;
inline RegressP Regress(const CRealArray& x, const CRealArray& y)
{
    assert(x.size() == y.size());
    //compute slope
    const size_t n = x.size();
    const REAL_TYPE s_x = x.sum();
    const REAL_TYPE s_y = y.sum();
    const REAL_TYPE s_xx = (x * x).sum();
    const REAL_TYPE s_xy = (x * y).sum();

    const REAL_TYPE slope = (n * s_xy - s_x * s_y) / (n * s_xx - s_x * s_x);

    //compute intercept
    const REAL_TYPE intercept = (s_y - slope * s_x) / x.size();

    return std::make_pair(slope, intercept);
}

inline RegressP anchored_regression(const CRealArray& xvals, const CRealArray& yvals, REAL_TYPE yanchorval)
{
    // do a simple least - squares regression, but
    //   anchor it so that the zeroth element has the
    //  value "yanchorval"
    CRealArray x = xvals - xvals[0];
    CRealArray y = yvals - yanchorval;

    REAL_TYPE xy = (x * y).sum();
    REAL_TYPE xx = (x * x).sum();
    REAL_TYPE slope = xy / xx;

    return std::make_pair(slope, yanchorval);
}



inline CRealArray FitRegress(const CRealArray& x, RegressP P)
{
    return x * P.first + P.second;
}


inline REAL_TYPE GetRSS(const CRealArray& y1, const CRealArray& y2)
{
    assert(y1.size() == y2.size());

    return ((y1 - y2) * (y1 - y2)).sum();
}


class CBestModelInfo
{
public:

    CBestModelInfo()
    {
        ok = false;
        f_stat = 0;
        p_of_f = 0;
        ms_regr = 0;
        ms_resid = 0;
        AICc = 0;
    }

    size_t n_obs()
    {
        return yfit.size();
    }
    size_t n_segments()
    {
        return vertices.size() - 1;
    }
    size_t n_vertices()
    {
        return vertices.size();
    }

    bool	ok;

    //results
    CVectices vertices;
    CRealArray vertvals;
    CRealArray yfit;
    CRealArray slope;
    CRealArray segment_mse;

    //statistic
    REAL_TYPE f_stat;
    REAL_TYPE p_of_f;
    REAL_TYPE ms_regr;
    REAL_TYPE ms_resid;
    REAL_TYPE AICc;
};

class CCalcFittingStats3
{
public:

    CCalcFittingStats3()
    {
        ok = false;
        mean_y = 0;
        sum_of_squares = 0;
        sum_of_squares_resid = 0;
        sum_of_squares_regression = 0;
        df_regr = 0;
        df_resid = 0;
        residual_variance = 0;
        total_variance = 0;

        adjusted_rsquare = 0;
        f_stat = 0;
        p_of_f = 1;
        ms_regr = 0;
        ms_resid = 0;
        abs_diff = 0;
        AICc = 0;
    }

    bool ok;
    REAL_TYPE mean_y;
    REAL_TYPE sum_of_squares;
    REAL_TYPE sum_of_squares_resid;
    REAL_TYPE sum_of_squares_regression;
    size_t df_regr;
    size_t df_resid;
    REAL_TYPE residual_variance;
    REAL_TYPE total_variance;

    REAL_TYPE adjusted_rsquare;
    REAL_TYPE f_stat;
    REAL_TYPE p_of_f;
    REAL_TYPE ms_regr;	//added these two 6/14/06
    REAL_TYPE ms_resid;
    REAL_TYPE abs_diff;	//added these two 6/14/06
    REAL_TYPE AICc;		//added july 29 2007
};



class CfindCorrection
{
public:

    CRealArray correction;
    CRealArray prop_correction;
};

class CSplitSeries
{
public:

    REAL_TYPE delta;
    size_t vertex;
    bool ok;
};

class CFindBestTrace
{
public:
    CRealArray yfit;
    CRealArray slopes;
    CRealArray vertvals;
};
class CFillFromVertices
{
public:

    CRealArray yfit;
    CRealArray slopes;
};
class CTakeOutWeakest2
{
public:
    CVectices v;
    CRealArray vertvals;
    CRealArray yfit;
    CRealArray slopes;
};

REAL_TYPE angle_diff(const CRealArray& xcoords, const CRealArray& ycoords, REAL_TYPE yrange, REAL_TYPE distweightfactor);
CVectices vet_verts3(const CRealArray& x, const CRealArray& y, const CVectices& vertices, size_t desired_count, REAL_TYPE distweightfactor);
CRealArray desawtooth(CRealArray vals, const CBoolArray& goods, REAL_TYPE stopat = 0.9, CRealArray* output_corr_factore=nullptr);
CRealArray fill_line(const CRealArray& x, const CVectices& x_endpoints, const CRealArray& y_endpoints);
CRealArray fill_verts(const CRealArray& x, const CRealArray& p, const CVectices& vertices);
size_t pick_better_fit(const CRealArray& y, const CRealArray& yfit1, const CRealArray& yfit2);
CSplitSeries split_series(const CRealArray& x, const CRealArray& y, bool endsegment, bool firstsegment, bool disttest);
CRealArray score_segments(const CRealArray& x, const CRealArray& y, const CVectices& vertices);
CVectices find_vertices(const CRealArray& x, const CRealArray& y, size_t max_count, REAL_TYPE distweightfactor = 2);
CFindBestTrace find_best_trace(const CRealArray& x, const CRealArray& y, const CVectices& v);
CFindBestTrace find_best_trace_mpfit(const CRealArray& x, const CRealArray& y, const CVectices& v);
CFillFromVertices fill_from_vertices(const CRealArray& x, const CVectices& v, const CRealArray& vv);

CTakeOutWeakest2 take_out_weakest2(const CBestModelInfo& info, REAL_TYPE threshold, const CRealArray& x, CRealArray y, const CVectices& v, CRealArray vertvals);
size_t pick_best_model6(const std::vector < CBestModelInfo >& info, REAL_TYPE pval, REAL_TYPE bestmodelproportion, bool use_fstat = false);
bool check_slopes(const CBestModelInfo& info, REAL_TYPE threshold);
CCalcFittingStats3 calc_fitting_stats3(const CRealArray& y, const CRealArray& yfit, size_t n_predictors);

}
