//****************************************************************************
//Copyright © 2008-2011 Oregon State University
//All Rights Reserved.
//
//
//Permission to use, copy, modify, && distribute this software && its
//documentation for educational, research && non-profit purposes, without
//fee, && without a written agreement is hereby granted, provided that the
//above copyright notice, this paragraph && the following three paragraphs
//appear in all copies.
//
//
//Permission to incorporate this software into commercial products may be
//obtained by contacting Oregon State University Office of Technology Transfer.
//
//
//This software program && documentation are copyrighted by Oregon State
//University. The software program && documentation are supplied "as is",
//without any accompanying services from Oregon State University. OSU does not
//warrant that the operation of the program will be uninterrupted or
//error-free. The }-user understands that the program was developed for
//research purposes && is advised not to rely exclusively on the program for
//any reason.
//
//
//IN NO EVENT SHALL OREGON STATE UNIVERSITY BE LIABLE TO ANY PARTY FOR DIRECT,
//INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
//PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
//IF OREGON STATE UNIVERSITYHAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
//DAMAGE. OREGON STATE UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES,
//INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE AND ANY STATUTORY WARRANTY OF
//NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS,
//AND OREGON STATE UNIVERSITY HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
//SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
//
//****************************************************************************

//****************************************************************************
//Convert in C++ by Remi Saint-Amant	2023-09-01
//****************************************************************************

#include <boost/math/distributions/fisher_f.hpp>
#include <random>
#include <iostream>
#include <valarray>
#include <cassert>


#include "LandTrendUtil.h"
#include "external/mpfit/mpfit.h"


using namespace std;

namespace LTR
{

REAL_TYPE angle_diff(const CRealArray& xcoords, const CRealArray& ycoords, REAL_TYPE yrange, REAL_TYPE distweightfactor)
{
    //need three points -- middle point is the one that gets the score
    //  the others are the ones preceding && following
    //Note that ycoords needs to be scaled to the range of the whole
    //  trajectory for this to really be meaningful

    if (xcoords.size() != 3 || ycoords.size() != 3)
        return -1;

    //distweightfactor helps determine how much weight is given to angles
    //  that precede a disturbance.  If set to 0, the angle difference is passed straight on.
    if (distweightfactor == 0)
        distweightfactor = 2;


    //get the slope of the prior to current points
    REAL_TYPE ydiff1 = (ycoords[1] - ycoords[0]);
    REAL_TYPE ydiff2 = (ycoords[2] - ycoords[1]);

    REAL_TYPE angle1 = atan(ydiff1 / (xcoords[1] - xcoords[0]));
    REAL_TYPE angle2 = atan(ydiff2 / (xcoords[2] - xcoords[1]));

    REAL_TYPE scaler = std::max(REAL_TYPE(0.0), (ydiff2 * distweightfactor) / yrange) + 1;	//if disturbance (positive), give more weight


    REAL_TYPE diff = std::max(std::abs(angle1), std::abs(angle2)) * scaler;

    return diff;
}




CVectices vet_verts3(const CRealArray& x, const CRealArray& y, const CVectices& vertices, size_t desired_count, REAL_TYPE distweightfactor)
{
    //if we don't have enough, just return what was given
    //get initial slope ratios across all points

    if (desired_count >= vertices.size() || vertices.size() <= 3)
        return vertices;


    size_t n_to_remove = vertices.size() - desired_count;
    assert(n_to_remove > 0);

    CVectices v = vertices;
    size_t n_t = vertices.size() - 2;

    CRealArray slope_ratios(n_t);

    //for angles, need to make it scaled in y like we see in display
    REAL_TYPE xr = x.max() - x.min();
    REAL_TYPE yr = y.max() - y.min();

    CRealArray yscale = ((y - y.min()) / yr) * xr;	//make a square
    REAL_TYPE sc_yr = yscale.max() - yscale.min();

    for (size_t i = 1; i <= vertices.size() - 2; i++)	//i is referenced off of the vertices array
    {
        CVectices ii = { v[i - 1], v[i], v[i + 1] };
        slope_ratios[i - 1] = angle_diff(x[ii], yscale[ii], sc_yr, distweightfactor);
    }


//		REAL_TYPE m = slope_ratios.max();	//this one sticks && is used to mask out ones taken out

    //now go through && iteratively remove them.
    CVectices usable = allpos(v.size());	//start with all possible usable, then take away


    for (size_t i = 0, count = v.size(); i <= n_to_remove - 1; i++, count--)
    {
        //pick from the slope diffs in play (not the ones at the end, which are
        //      shifted there from prior iterations)
        //	   print, 'slope_diffs'
        //	   print, slope_ratios[0:count-1-2]
        //
        CRealArray sub_worst = subset(slope_ratios, 0, count - 1 - 2);
        size_t worst = std::distance(std::begin(sub_worst), std::min_element(std::begin(sub_worst), std::end(sub_worst))); //worst is in terms of triangle area array

        worst = worst + 1;		//now increment so it's in terms of the vertex array

        //shift down to take out bad one
        for (size_t j = worst; j < usable.size() - 1; j++)
            usable[j] = usable[j + 1];	//shift down to take out bad one

        //as long as we're not at the end of the triangle array, then shift down as well.  Note
        //   that the array index for worst is in terms of the vertex array, so the test
        //   of position is vs. n_t instead of n_t-1

        if (worst != n_t)
            for (size_t j = worst - 1; j < slope_ratios.size() - 1; j++)
                slope_ratios[j] = slope_ratios[j + 1];	//same

        //recalculate the ones around the one taken out.
        if (worst != 1)
        {
            CVectices ii = { v[usable[worst - 2]], v[usable[worst - 1]], v[usable[worst]] };

            //if we're not at the beginning, need to recalculate the one to the left
            slope_ratios[worst - 2] = angle_diff(x[ii], yscale[ii], sc_yr, distweightfactor);
        }


        if (worst != n_t)
        {
            CVectices ii = { v[usable[worst - 1]], v[usable[worst]], v[usable[worst + 1]] };
            //if we're not at the end, need to recalc the one to the right
            slope_ratios[worst - 1] = angle_diff(x[ii], yscale[ii], sc_yr, distweightfactor);
        }
    }


    usable = subset(usable, 0, desired_count - 1);
    return  v[usable];

}


CfindCorrection find_correction(const CRealArray& vals)
{
    CfindCorrection out;

    //warning, shift in std seem to be inverse of shift in IDL
    size_t n = vals.size();
    CRealArray diff_2 = abs(vals - vals.shift(2)).shift(-1);

    CRealArray diff_minus1 = vals - vals.shift(1);
    CRealArray diff_plus1 = vals - vals.shift(-1);

    out.correction.resize(vals.size());
    out.prop_correction.resize(vals.size());

    out.correction[0] = 0;
    out.correction[n - 1] = 0;	//no corrections on the end

    for (size_t i = 1; i < n - 1; i++)
    {
        REAL_TYPE  md = max(abs(diff_minus1[i]), abs(diff_plus1[i]));
        if (md == 0)
            md = diff_2[i];	//set this to result in correction of 0

        out.prop_correction[i] = (1. - (diff_2[i] / md));
        out.correction[i] = out.prop_correction[i] * (((vals[i - 1] + vals[i + 1]) / 2) - (vals[i]));

        if (std::isnan(out.prop_correction[i]))//avoid nan in the prop_correction. This cause problem in valarray.
            out.prop_correction[i] = 0;
    }

    return out;


}



CRealArray desawtooth(CRealArray vals, const CBoolArray& goods, REAL_TYPE  stopat)
{
    assert(stopat != 0);

   
    CRealArray v = vals[goods];

    REAL_TYPE  prop = 1.0;
    size_t count = 1;

    do
    {
        CfindCorrection c = find_correction(v);
        size_t wh_max = distance(begin(c.prop_correction), max_element(begin(c.prop_correction), end(c.prop_correction)));
        prop = c.prop_correction[wh_max];

        if (prop > stopat)
        {
            v[wh_max] = v[wh_max] + (c.correction[wh_max]);
            count = count + 1;
        }
    }
    while (prop > stopat);

    vals[goods] = v;

    return vals;

}



CRealArray fill_line(const CRealArray& x, const CVectices& x_endpoints, const CRealArray& y_endpoints)
{
    //called by fill_verts, fill verts2, which are only in f7

    //differs from the version 1 in find_segments6 in that
    //  this one assumes the x_endpoints are x indices, not
    //  the actual x values

    //each is a 2-element array
    //  start and end of the interval
    //  this calculates the y-vals in between

    REAL_TYPE slope = get_slope(x[x_endpoints], y_endpoints);
    CRealArray xvals = subset(x, x_endpoints[0], x_endpoints[1]);
    return ((xvals - xvals[0]) * slope) + y_endpoints[0];
}


CRealArray fill_verts(const CRealArray& x, const CRealArray& p, const CVectices& vertices)
{
    //Only in 7
    //called by find_best_trace3
    CRealArray ymod(0.0, x.size());

    for (size_t i = 0; i < p.size() - 1; i++)
    {
        ymod[get_slice(vertices[i], vertices[i + 1])] = fill_line(x, { vertices[i], vertices[i + 1] }, { p[i], p[i + 1] });
    }

    //assert(ymod.min() > -999);

    return ymod;
}


struct vars_struct
{
    vars_struct()
    {
        x = y = ey = nullptr;
        v = nullptr;
    }

    const CRealArray* x;
    const CRealArray* y;
    const CVectices* v;
    const CRealArray* ey;
};



int mpfit_func(int m, int n, double* p, double* dy, double** dvec, void* vars)
{
    vars_struct* vvars = (vars_struct*)vars;

    const CRealArray& x = *(vvars->x);
    const CRealArray& y = *(vvars->y);
    const CVectices& v = *(vvars->v);
    const CRealArray& ey = *(vvars->ey);

    for (size_t i = 0; i < size_t(n - 1); i++)
    {
        size_t vi0 = v[i];
        size_t vi1 = v[i + 1];
        REAL_TYPE span = x[vi1] - x[vi0];
        REAL_TYPE slope = REAL_TYPE(p[i + 1] - p[i]) / span;

        for (size_t ii = vi0; ii <= vi1; ii++)
        {
            REAL_TYPE yfit = ((x[ii] - x[vi0]) * slope) + p[i];
            dy[ii] = (y[ii] - yfit) / ey[i];
        }

    }

    return 0;
}


size_t pick_better_fit(const CRealArray& y, const CRealArray& yfit1, const CRealArray& yfit2)
{
    //this assumes same number of parameters to have created the yfits
    REAL_TYPE diff1 = GetRSS(y, yfit1);
    REAL_TYPE diff2 = GetRSS(y, yfit2);

    return diff1 > diff2 ? 1 : 0;
}



CSplitSeries split_series(const CRealArray& x, const CRealArray& y, bool endsegment, bool firstsegment, bool disttest)
{
    assert(x.size() == y.size());
    assert(x.size() > 2);
    //given an x && y  split the series into two smaller segments
    //However, invoke a rule where there can be no series where the value
    //   decreases (implication of recovery) for only 1 or 2 years --
    //   this will help with the overfitting, since this is not
    //   really a prominent type of phenomenon, && if it's minor
    //   anyway a coarser fit would make more sense.
    //endsegment is a flag set to 1 if this segment is at the end
    //  of the time period, when we don't invoke the
    //  recovery restriction rule -- it will get thrown out
    //  later if it's really extreme


    RegressP P = Regress(x, y);
    CRealArray yfit = FitRegress(x, P);
    CRealArray diff = abs(yfit - y);


    diff[0] = diff[diff.size() - 1] = 0;  //endpoints are already vertices, so take them out of consideration


    //the first segment test was causing so many problems that I just took it out 1/2/09
    //    if it causes a lot of false positives, then look at comparing the
    //    slope of y[1]-y[0]/x[1]-x[0] vs. the slope of the regression (r) -- if the
    //    regression is greater (in the opposite direction) than the this one,then
    //    let it go.

    //we accept vertices near edge if disturbance, but not if recovery
    if (disttest && endsegment)
    {
        REAL_TYPE f = (y[y.size() - 1] > y[y.size() - 2]) ? 1 : 0;
        diff[diff.size() - 2] = diff[diff.size() - 2] * f;	//we know there are three elements at least, because
        // assured in calling program
    }


    size_t maxdiff = distance(begin(diff), std::max_element(begin(diff), end(diff)));

    //if maxdiff is 0, then we
    // know there were no segments
    // that met the rule of no 1-yr recovery

    return CSplitSeries({ diff.max(), maxdiff, maxdiff > 0 });


} //split series

CRealArray score_segments(const CRealArray& x, const CRealArray& y, const CVectices& vertices)
{
    assert(vertices.size() >= 2);

    CRealArray segment_scores(vertices.size() - 1);  	//# segments always # vertices -1

    for (size_t i = 0; i < segment_scores.size(); i++)
    {
        assert(vertices[i] < vertices[i + 1]);

        REAL_TYPE span = REAL_TYPE(vertices[i + 1] - vertices[i] + 1);
        if (span > 2)
        {

            CRealArray yy = subset(y, vertices[i], vertices[i + 1]);
            //if we've done desawtooth, it's possible that all of the
            //  values in a segment have same value, in which case regress
            //  would choke, so deal with that.

            if (get_range(yy) > 0)
            {
                CRealArray xx = subset(x, vertices[i], vertices[i + 1]);
                RegressP P = Regress(xx, yy);
                CRealArray yfit = FitRegress(xx, P);

                segment_scores[i] = GetRSS(yy, yfit) / span;
            }
            else
            {
                segment_scores[i] = 0;
            }

            //then check to make sure that we don't have 3 or 4 element with in
        }

    }


    return segment_scores;
}




CVectices find_vertices(const CRealArray& x, const CRealArray& y, size_t max_count, REAL_TYPE distweightfactor)
{
    assert(max_count > 0);
    assert(y.size() > 2);
    assert(distweightfactor >= 0);

    size_t n = y.size();
    size_t m = min(max_count, n - 2);		//just in case

    CBoolArray vertex_flags(false, n);
    //size_t nb = sum(vertex_flags);

    vertex_flags[0] = vertex_flags[n - 1] = true;//set ends to vertices
    //nb = sum(vertex_flags);

    //vertex_flags[32] = true;
    //nb = sum(vertex_flags);

    CVectices VERTICES = allpos(n);

    //grab a few with big changes in convolve value
    CVectices vertices = VERTICES[vertex_flags];


    size_t count = 0;
    size_t i = 0;
    while (vertices.size() < m)
    {
        //while vertex count less than max
        i++;
        CRealArray mses = score_segments(x, y, vertices);		//# segments always vertex_count - 1
        bool ok = false;
        size_t s = -1;
        CSplitSeries v;

        size_t insidecount = 0;
        while (ok == false)
        {
            i++;
            REAL_TYPE max_mse = mses.max();	//find the segment with the most residual variation
            if (max_mse == 0)
                return vertices;		//fail if we can't get high enough without breaking recovery rule

            s = std::distance(begin(mses), std::max_element(begin(mses), end(mses)));
            assert(s >= 0 && s <= mses.size() - 1);

            bool endsegment = s == (mses.size() - 1);
            bool firstsegment = (s == 0);
            bool disttest = distweightfactor != 0;//just use distweightfactor to determine if disturbance should be considered in initial segments

            CRealArray xx = subset(x, vertices[s], vertices[s + 1]);
            CRealArray yy = subset(y, vertices[s], vertices[s + 1]);
            v = split_series(xx, yy, endsegment, firstsegment, disttest);

            insidecount = insidecount + 1;


            ok = v.ok;
            if (ok != true)
                mses[s] = 0;		//reset
        }		//finding the segments that are legit in terms of recovery rule

        size_t new_pos = vertices[s]+v.vertex;
        vertex_flags[new_pos] = true;		//the vertex picked by split series, but modified by the
        //   count indicating the beginning of the piece
        //   of the series that was sent to split_series


        //nb = sum(vertex_flags);
        //vertex_flags[31] = true;
        //nb = sum(vertex_flags);
        vertices = CVectices(VERTICES[vertex_flags]);

        count = count + 1;

        if (count > 20)
            return vertices;
    }


    return vertices;
}


CFindBestTrace find_best_trace(const CRealArray& x, const CRealArray& y, const CVectices& v)
{

    //for a given set of vertices (x-vals), find the
    //   the combo of vertex y-vals that results
    //   in best fit for each segment
    //x && y are the original values
    //  v is the list of vertices (in terms of array position, not the x-value
    //  n_segments is the number of segments -- passed just
    //     to save calc time
    //This is used only on the first run, with all of the
    //   segments.  From here on out, we just eliminate
    //   each one && calc the vals

    CRealArray vx = x[v];
    CRealArray vv = y[v];	//initially, the vertex vals are the y-values at the x-vals
    //these will be modified as we regress our way through the series

    CRealArray yfit_final(y.size());	//set up array of fitted vals
    CRealArray slope(v.size() - 1);	//set up array to capture slopes

    for (size_t s = 0; s < v.size() - 1; s++)
    {
        if (v[s + 1] - v[s] > 0)
        {
            CRealArray dot_way = fill_line(x, { v[s], v[s + 1] }, { vv[s], vv[s + 1] });
            REAL_TYPE dot_slope = get_slope({ vx[s], vx[s + 1] }, { vv[s], vv[s + 1] });

            CRealArray xx = subset(x, v[s], v[s + 1]);
            CRealArray yy = subset(y, v[s], v[s + 1]);

            RegressP P = (s == 0) ? Regress(xx, yy) : anchored_regression(xx, yy, vv[s]);
            REAL_TYPE regress_slope = P.first;
            CRealArray regree_way = FitRegress(xx - xx[0], P);

            size_t choice = pick_better_fit(yy, dot_way, regree_way);
            CRealArray yfit_better = choice == 0 ? dot_way : regree_way;
            assert(v[s + 1] - v[s] == yfit_better.size() - 1);
            assert((v[s + 1] - v[s] + 1) == yfit_better.size());

            vv[s] = yfit_better[0];
            vv[s + 1] = yfit_better[yfit_better.size() - 1];
            slope[s] = choice == 0 ? dot_slope : regress_slope;
            yfit_final[get_slice(v[s], v[s + 1])] = yfit_better;
        }

    }

    return CFindBestTrace({ yfit_final, slope, vv });

}



CFindBestTrace find_best_trace_mpfit(const CRealArray& x, const CRealArray& y, const CVectices& v)
{
    assert(x.size() == y.size());
    //for a given set of vertices (x-vals), find the
    //   the combo of vertex y-vals that results
    //   in best fit for each segment
    //x && y are the original values
    //  v is the list of vertices (in terms of array position, not the x-value
    //  n_segments is the number of segments -- passed just
    //     to save calc time

    //use mpfitfun to get this right.


    size_t n_yrs = x.size();
    size_t num_params = v.size();


    if (num_params == 0)
        return CFindBestTrace();//then stop



    REAL_TYPE m = y.sum() / y.size();
    REAL_TYPE sdev = ((y - m) * (y - m)).sum() / (y.size() - 1);
    if (sdev == 0)
        sdev = 1;		//if it's a bunch of zeros, just set error to 1 so it'll pass through


    CRealArray vals_sterr(sdev, n_yrs);

    //Set initial guess as the mean
    vector<double> p(num_params, m);

    vars_struct param;
    param.x = &x;
    param.y = &y;
    param.v = &v;
    param.ey = &vals_sterr;

    mp_result result;
    memset(&result, 0, sizeof(result));       /* Zero results structure */

    mpfit(mpfit_func, int(x.size()), int(v.size()), p.data(), nullptr, nullptr, (void*)&param, &result);

    CRealArray vertvals(p.data(), p.size());
    CRealArray yfit = fill_verts(x, vertvals, v);

    CRealArray spans = convert(v.shift(1) - v);
    CRealArray ydiff = vertvals.shift(1) - vertvals;
    CRealArray slopes = subset(ydiff, 1, v.size() - 1) / subset(spans, 1, v.size() - 1);

    return CFindBestTrace({ yfit, slopes, vertvals });
}


CFillFromVertices fill_from_vertices(const CRealArray& x, const CVectices& v, const CRealArray& vv)
{
    assert(v.size() == vv.size());

    CRealArray yfit(x.size());	//set up
    CRealArray slopes(v.size() - 1);

    for (size_t i = 0; i < v.size() - 1; i++)
    {
        CRealArray k = fill_line(x, { v[i], v[i + 1] }, { vv[i], vv[i + 1] });
        REAL_TYPE slope = get_slope({ x[v[i]], x[v[i + 1]] }, { vv[i], vv[i + 1] });
        assert(k.size() == v[i + 1] - v[i] + 1);

        yfit[get_slice(v[i], v[i + 1])] = k;
        slopes[i] = slope;
    }

    return CFillFromVertices({ yfit, slopes });
}




CTakeOutWeakest2 take_out_weakest2(const CBestModelInfo& info, REAL_TYPE threshold, const CRealArray& x, CRealArray y, const CVectices& v, CRealArray vertvals)
{
    assert(x.size() == y.size());
    assert(v.size() == vertvals.size());

    CBoolArray use(true, v.size());

    //first, check to see if there are any segments that
    //break the slope criterion.  If so, take them out first.
    //assert(info.n_segments == info.slope.size());
    //size_t n_slopes = info.slope.size();

    //we operate under the knowledge that
    //  disturbance is always considered to have a positive
    //  slope, && recovery a negative slope (based on band5 type indicators).

    CVectices slope_pos = allpos(info.slope.size());

    CVectices negatives = slope_pos[info.slope < 0.0 && info.slope != -1.0];
    size_t n_negatives = negatives.size();

    REAL_TYPE range_of_vals = get_range(info.yfit);//no need of subset here
    bool run_mse = true;

    CRealArray scaled_slope(threshold - 1, info.slope.size());		//set so it won't be > threshold
    if (n_negatives > 0)
        scaled_slope = std::abs(info.slope[negatives]) / range_of_vals;

    if (scaled_slope.max() > threshold)
    {
        assert(negatives.size() == scaled_slope.size());
        //if we have one that violates, take it out
        size_t violator = negatives[distance(begin(scaled_slope), std::max_element(begin(scaled_slope), end(scaled_slope)))];

        //the violator is a segment -- which vertex to remove?   Since we
        //   are tracking through time, we assume that it is the latter
        //   vertex that is causing the problem && take it out. This will
        //   be violated only if there are spikes in brightness that are
        //   not fixed by desawtooth, but that situation would be no better
        //   removing the first vertex anyway, so we stick with this approach
        //   since it will take out more shadow problems. the only

        //now interpolate to get rid of this point, so it doesn't mess
        //   up the fits later
        if (violator + 1 == vertvals.size() - 1)
        {
            y[v[violator + 1]] = y[v[violator + 1] - 1];
            vertvals[vertvals.size() - 1] = y[v[violator + 1]];

            run_mse = true;	//since the violating point was at }, need to run mse instead after fixing
        }
        else
        {
            //just set == to prior
            use[violator + 1] = 0;
            REAL_TYPE lefty = y[v[violator + 1] - 1];
            REAL_TYPE righty = y[v[violator + 1] + 1];
            REAL_TYPE leftx = x[v[violator + 1] - 1];
            REAL_TYPE rightx = x[v[violator + 1] + 1];
            REAL_TYPE thisx = x[v[violator + 1]];
            REAL_TYPE slope = REAL_TYPE(righty - lefty) / (rightx - leftx);
            REAL_TYPE interpy = ((thisx - leftx) * slope) + lefty;
            y[v[violator + 1]] = interpy;
            run_mse = false;
        }
    }

    if (run_mse)
    {
        //take out the vertex whose elimination results in the least penalty
        CRealArray mse(v.size());

        for (size_t i = 1; i < v.size() - 1; i++)//for each vertex
        {
            //look at mse for region defined by prior && subsequent vertex
            //  the one with least is the least important

            CRealArray yfit = fill_line(x, { v[i - 1], v[i + 1] }, { vertvals[i - 1], vertvals[i + 1] });
            assert(yfit.size() == v[i + 1] - v[i - 1] + 1);
            mse[i] = GetRSS(yfit, subset(y, v[i - 1], v[i + 1])) / REAL_TYPE(x[v[i + 1]] - x[v[i - 1]]);
        }

        size_t weakest = distance(begin(mse), std::min_element(begin(mse) + 1, end(mse) - 1));

        use[weakest] = false;

    }

    CFillFromVertices ok = fill_from_vertices(x, v[use], vertvals[use]);

    return CTakeOutWeakest2({ v[use], vertvals[use], ok.yfit, ok.slopes });


}


size_t pick_best_model6(const vector < CBestModelInfo >& info, REAL_TYPE pval, REAL_TYPE bestmodelproportion, bool use_fstat)
{
    assert(bestmodelproportion < 1.0);

    CVectices n_segments(info.size());
    CRealArray p_of_f(info.size());
    CRealArray f_stat(info.size());

    for (size_t i = 0; i < info.size(); i++)
    {
        n_segments[i] = info[i].slope.size();
        p_of_f[i] = info[i].p_of_f;
        f_stat[i] = info[i].f_stat;
    }

    if ((n_segments != size_t(0)).size() == 0)
        return -1;//then stop;

    //now pick the best one
    if (use_fstat == false)
    {
        REAL_TYPE mx = p_of_f.min();
        CBoolArray le = p_of_f <= ((2 - bestmodelproportion) * mx);
        CVectices valid_p_of_f = get_pos(le);

        size_t best = std::distance(begin(valid_p_of_f), std::max_element(begin(valid_p_of_f), end(valid_p_of_f))); //get the fist valid p_of_f

        return valid_p_of_f[best];
    }
    else
    {
        //check to see whether any with p < 0.001
        if (CRealArray(p_of_f[p_of_f <= pval]).size() == 0)
            return -1;

        //otherwise, find the best f-stat
        REAL_TYPE mx = f_stat.max();

        CBoolArray ge = f_stat >= (bestmodelproportion * mx);
        CVectices valid_f_stat = get_pos(ge);
        assert(valid_f_stat.size() > 0);

        size_t best = std::distance(begin(valid_f_stat), std::min_element(begin(valid_f_stat), end(valid_f_stat)));//get the first valid f_stat
        return valid_f_stat[best];
    }
}


bool check_slopes(const CBestModelInfo& info, REAL_TYPE threshold)
{
    //given one model, look at its slopes
    //filter out if recovery happens quicker than quickest disturbance --
    //  a value-free way to get unreasonable things out.
    // but of course don't do if all we have is recovery.  no way to tell for sure then.

    bool ok = true;


    //size_t n_slopes = info.slope.size();

    //all of these operate under the knowledge that
    //  disturbance is always considered to have a positive
    //  slope, && recovery a negative slope (based on band5 type indicators).
    //  Always make sure that he adjustment factor that happens
    //   upstream of find_segments6 ensures that recovery is negative

    CBoolArray negatives = info.slope < 0.0;

    if (info.slope[negatives].size() > 0)
    {
        //assert(info.n_obs == info.yfit.size());
        REAL_TYPE range_of_vals = get_range(subset(info.yfit, 0, info.yfit.size() - 1));
        CRealArray scaled_slope = abs(info.slope[negatives]) / range_of_vals;

        if (scaled_slope.max() > threshold)
            return false;
    }


    return ok;
}

//INPUT :
//	X : cutoff
//	DFN : numerator degrees of freedom
//	DFD : denominator degrees of freedom
//	OUTPUT :The probability of a value greater than X.
REAL_TYPE f_test1(REAL_TYPE f_regr, REAL_TYPE df_regr, REAL_TYPE df_resid)
{
    //F_test1 returns the probability of an observed value greater than X
    //from an F distribution with DFN and DFD numerator and denominator
    //degrees of freedom.

    if (!::isfinite(f_regr))
        return 1;

    // define a standard fisher distribution:
    boost::math::fisher_f dist(df_regr, df_resid);

    //get probability to get f_regr
    return cdf(dist, f_regr);
}


CCalcFittingStats3 calc_fitting_stats3(const CRealArray& y, const CRealArray& yfit, size_t n_predictors)
{
    assert(y.size() == yfit.size());

    CCalcFittingStats3 out;
    //   a count for the number of predictor variables used
    //   to get the predicted vals, return stuff
    //  Assumes equal weights

    REAL_TYPE mean_y = y.sum() / y.size();
    REAL_TYPE ss = ((y - mean_y) * (y - mean_y)).sum();		//sum of squares

    std::valarray <REAL_TYPE> resid = y - yfit;
    REAL_TYPE abs_diff = abs(resid).sum();

    REAL_TYPE ss_resid = (resid * resid).sum();		//sum of squares of the residuals
    if (ss_resid > ss)
        ss_resid = ss; //to handle small rounding error when no trend

    REAL_TYPE ss_regr = ss - ss_resid;					//sum of squares of the regression

    size_t df_regr = n_predictors;
    int df_resid = int(y.size()) - int(n_predictors) - 1;		//take away the number of times we have exact matches at vertices


    if (df_resid > 0)
    {
        REAL_TYPE residual_variance = ss_resid / df_resid;
        REAL_TYPE total_variance = ss / (y.size() - 1);
        REAL_TYPE adjusted_rsquare = 1 - (residual_variance / total_variance);	//terms from Jongman et al. pg 37


        REAL_TYPE ms_regr = ss_regr / df_regr;			//mean square error of regression
        REAL_TYPE ms_resid = ss_resid / df_resid;		//mean square error of resids

        REAL_TYPE f_regr = ms_regr / ms_resid;

        REAL_TYPE p_of_f = 1 - f_test1(f_regr, REAL_TYPE(df_regr), REAL_TYPE(df_resid));

        //calc the AIC
        REAL_TYPE AIC = (2 * n_predictors) + (y.size() * log(ss_resid / y.size()));
        REAL_TYPE AICc = AIC + ((2 * n_predictors * (n_predictors + 1)) / (y.size() - n_predictors - 1));
        if (!::isfinite(AICc))
            AICc = -1;


        out.ok = true;
        out.mean_y = mean_y;
        out.sum_of_squares = ss;
        out.sum_of_squares_resid = ss_resid;
        out.sum_of_squares_regression = ss_regr;
        out.df_regr = df_regr;
        out.df_resid = df_resid;
        out.residual_variance = residual_variance;
        out.total_variance = total_variance;
        out.adjusted_rsquare = adjusted_rsquare;
        out.f_stat = f_regr;
        out.p_of_f = p_of_f;
        out.ms_regr = ms_regr;		//added these two 6/14/06
        out.ms_resid = ms_resid;
        out.abs_diff = abs_diff;	//added these two 6/14/06
        out.AICc = AICc;			//added july 29 2007
    }
    else
    {
        //not enough degree of freedom
        out.ok = true;

        out.mean_y = mean_y;
        out.sum_of_squares = ss;
        out.sum_of_squares_resid = ss_resid;
        out.sum_of_squares_regression = ss_regr;
        out.df_regr = df_regr;
        out.df_resid = df_resid;
        out.residual_variance = 0;
        out.total_variance = 0;
        out.adjusted_rsquare = 0;
        out.f_stat = 0;
        out.p_of_f = 1.0;
        out.ms_regr = 0;				//added these two 6/14/06
        out.ms_resid = 0;
        out.AICc = 0;
        out.abs_diff = abs_diff;
    }



    return out;
}


}
