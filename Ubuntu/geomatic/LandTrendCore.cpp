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


#include <algorithm>
#include <iostream>
#include <numeric>

#include "LandTrendCore.h"


using namespace std;
namespace LTR
{

//*******************************************************************
//
//MAIN ROUTINE:   fit_trajectory_v2
//
//*******************************************************************


CBestModelInfo fit_trajectory_v2(const CRealArray& all_years, const CRealArray& vvals, const CBoolArray& goods,
                                 size_t minneeded, int background, int modifier,
                                 REAL_TYPE  desawtooth_val, REAL_TYPE  pval, size_t max_segments, REAL_TYPE  recovery_threshold,
                                 REAL_TYPE  distweightfactor, size_t  vertexcountovershoot, REAL_TYPE  bestmodelproportion, TFitMethod fit_method)
{
    //CBestModelInfo best_model;
    //February 28, 2008
    //given a set of years && observed values,
    //   fit the trajectory using segmentation.
    //  Use a simple segmentation approach where the segments
    //      vertices are assigned in order from early to latest
    //      year.
    //  If that doesn't produce a reasonable fit at pval threshold,
    //	   then try a segmentation that allows all vertex values
    //	   to float independently (more timeconsuming)
    //Ouputs:
    //	 A structure with enough room for information on the
    //		year && the value of 6 vertices (which is currently the
    //		most that are reasonable in the landsat record).

    //Years:  the year values that are to be used in the fitting.
    // 		These should have the same number of elements as the vvals
    //		array.  These should not include years that have clouds
    //		or any other filtered values.
    //VVals:  The values (spectral, biomass, etc.) for the Years.
    //Allyears:  The years for the entire sequence of images, including
    //		the years that have been masked out for the fitting.
    //		This allows us to build a true fitted image even for
    //		years where there's a cloud, etc.

    assert(minneeded >= 3);
    assert(pval >= 0 && pval <= 1);
    assert(desawtooth_val >= 0 && desawtooth_val <= 1);
    assert(max_segments > 0);
    assert(CRealArray(all_years[goods]).size() > 0);

    CBestModelInfo best_model;

    //get the offset of years.  add back in at the

    REAL_TYPE minimum_x_year = all_years.min();
    assert(minimum_x_year == all_years[0]);

    CRealArray all_x = all_years - minimum_x_year;

    //avoid little dribs && drabs

    //check if a bunch of zeros -- this indicates off the edge
    size_t n_zeroes = CRealArray(vvals[abs(vvals - REAL_TYPE(background * modifier)) < 0.1]).size();
    if (n_zeroes > (0.3 * all_years.size()))
        return best_model;

    
    if (CRealArray(all_x[goods]).size() < minneeded)
    {
        //not enough data to run the fitting, then set all to flat line
        best_model.ok = true;
        best_model.yfit.resize(vvals.size(), vvals.sum() / vvals.size());
        best_model.vertvals.resize(2, vvals.sum() / vvals.size());
        best_model.vertices = { 0, vvals.size() - 1 };
        best_model.slope.resize(1, 0);
        best_model.segment_mse.resize(1, 0);

        return best_model;
    }


    //Do prep work

    //Take out spikes that start && end at same value (to get rid of weird years
    //			left over after cloud filtering)

    CRealArray all_y = (desawtooth_val < 1.0) ? desawtooth(vvals, goods, desawtooth_val) : vvals;
    all_y *= modifier;  //this sets everything so disturbance is always positive


    size_t max_count = 0;
    if (max_segments != 0)
        max_count = max_segments + 1;
    else
        max_count = min(7, int(round(all_years.size() / 2.5)));	//maximum number of vertices is 7



    //****************************
    //Do the trajectory fitting!  This uses one of two approaches
    //   to identify the best fit of the

    best_model = tbcd_v2(all_x, all_y, goods, max_count, pval,
                         recovery_threshold, distweightfactor, vertexcountovershoot, bestmodelproportion, fit_method);	//max_count is the number of segments + 1

    //************************


    //***********
    //  Now assign the vert valsvals.
    //assert(best_model.n_segments == best_model.vertices.size());
    //assert(best_model.vertices.size() == best_model.segment_mse.size());
    if (best_model.ok == true)
    {
        best_model.yfit *= modifier;
        best_model.vertvals *= modifier;
        best_model.slope *= modifier;
        best_model.vertices = best_model.vertices + size_t(minimum_x_year);
    }

    //need to use the vertvals to do this.

    return best_model;
}


vector<CBestModelInfo> get_all_model(const CRealArray& x, const CRealArray& y, CVectices v, REAL_TYPE recovery_threshold, TFitMethod fit_method)
{

    //********************************
    //FIND BEST TRACE WITH ALL THOSE VERTICES
    //
    //The best trace is the set of y-values that best
    //    works with those vertices.  This approach
    //    uses find_best_trace, which uses a simple
    //    && relatively quick way to do this.  Starting
    //    at the left of the sequence, it allows segments
    //    to either track the two vertices directly,
    //    or to use a simple linear least-squares value
    //    anchored on the left vertex to connect to the
    //    next vertex.  This forces the sequence in order
    //    from left to right (oldest to newest).

    //********************************
    //THEN SEQUENTIALLY REMOVE VERTICES
    //
    //now go through each vertex, take it out, &&
    // see what the fit is.

    vector<CBestModelInfo> info(v.size() - 1);


    for (size_t i = 0; i < info.size(); i++)
    {
        //n_vertices-2 because always need the first && last
        //take out all vertices, check them out, pick the
        //   one that results best overall fit

        //first, find the trace through the x-vals of the vertices
        //   that minimizes the squared difference between the
        //   fit && the observed values for each segment separately.
        //   this can be done using either a floating regression
        //   or a dot-to-dot regression

        CFindBestTrace best_fit = (fit_method == FIT_EARLY_TO_LATE) ? find_best_trace(x, y, v) : find_best_trace_mpfit(x, y, v);

        CCalcFittingStats3 ok = calc_fitting_stats3(y, best_fit.yfit, (v.size() * 2) - 2);

        info[i].ok = ok.ok;
        info[i].f_stat = ok.f_stat;
        info[i].p_of_f = ok.p_of_f;
        info[i].ms_regr = ok.ms_regr;
        info[i].ms_resid = ok.ms_resid;
        info[i].AICc = ok.AICc;

        info[i].vertices = v;		//vertices are the actual index in the array, not the year
        info[i].vertvals = best_fit.vertvals;
        info[i].yfit = best_fit.yfit;	//added [0:n_obs-1] 2/29 to allow for space in
        info[i].slope = best_fit.slopes;


        //compute next step
        if (i < info.size() - 1)//don't compute the last one
        {
            CTakeOutWeakest2 rr = take_out_weakest2(info[i], recovery_threshold, x, y, v, best_fit.vertvals);
            v = rr.v;
        }

    }

    return info;
}


//************************************
//PICK THE BEST ONE
//
//Along the way, though, we check to make
//   sure that the best one doesn't violate
//   the slopes  rule (where recovery can't
//   be faster than a prescribed speed)
//Note:  This section requires that recovery
//   always result in a decrease in the y-values
//   If the y-values are of a type where recovery
//   actually results in an increase (e.g. biomass),
//   you need to set the modifier value to -1 before
//   running tbcd, so this recovery criterion will
//   be applied appropriately.
CBestModelInfo pick_best(vector<CBestModelInfo> info, REAL_TYPE pval, REAL_TYPE bestmodelproportion, REAL_TYPE recovery_threshold)
{
    bool notdone = true;
    CRealArray fstats(info.size());
    for (size_t i = 0; i < info.size(); i++)
        fstats[i] = info[i].f_stat;


    size_t increment = 0;
    size_t best = -1;

    while (notdone)
    {
        //	i++;
        increment = increment + 1;

        best = pick_best_model6(info, pval, bestmodelproportion);

        if (best != UNKNOWN_POS)
        {
            //check on the slopes
            bool ok = check_slopes(info[best], recovery_threshold);
            if (ok == false)
            {
                info[best].p_of_f = 1;
                info[best].f_stat = 0;
                info[best].AICc = 0;
            }



            assert(increment <= info.size());
            notdone = !(ok || (increment > info.size()));	//once ok = 1,
            //or if we've gone through this as many times as there are vertices, we move on
        }
        else
        {
            //if we get here, it means none of the
            //  aicc's were valid &&/or that
            //  none of the segments worked.

            best = distance(begin(fstats), std::min_element(begin(fstats), end(fstats)));
            notdone = false;
        }
    }

    assert(best != UNKNOWN_POS);

    return info[best];
}



void reduce_vertice(CVectices& new_vertices, CRealArray& new_vertvals, CRealArray& new_slope, CRealArray& new_segment_mse)
{
    assert(new_vertices.size() == new_vertvals.size());
    assert(new_slope.size() == new_segment_mse.size());

    CRealArray slope_ratios(new_slope.size() - 1);
    REAL_TYPE sc_yr = get_range(new_vertvals);

    for (size_t i = 1; i < slope_ratios.size() - 1; i++)
    {
        slope_ratios[i - 1] = angle_diff(convert(subset(new_vertices, i - 1, i + 1)), subset(new_vertvals, i - 1, i + 1), sc_yr, 2);
    }

    //find the vertex that has the least "bend"
    size_t minv = std::distance(std::begin(slope_ratios), std::min_element(std::begin(slope_ratios), std::end(slope_ratios))); //worst is in terms of triangle area array
    minv = minv + 1;   //because the zeroth is not in slope_ratios

    //take out the worst one
    CBoolArray flag(true, new_vertices.size());
    flag[minv] = false;
    CVectices use = get_pos(flag);
    new_vertices = CVectices(new_vertices[use]);
    new_vertvals = CRealArray(new_vertvals[use]);

    CBoolArray mse_flag(true, new_segment_mse.size());
    mse_flag[minv] = false;
    CVectices mse_use = get_pos(mse_flag);

    new_segment_mse = CRealArray(new_segment_mse[mse_use]);


    //calculate slope
    CRealArray dy = new_vertvals.shift(1) - new_vertvals;
    CRealArray dx = convert(new_vertices.shift(1) - new_vertices);
    new_slope = subset(CRealArray(dy / dx), 1, new_slope.size() - 1);

    assert(new_vertices.size() == new_vertvals.size());
    assert(new_slope.size() == new_segment_mse.size());
}

//*******************************************************************
//
//SUB MAIN ROUTINE:   TBCD_V2
//
//*******************************************************************

CBestModelInfo tbcd_v2(const CRealArray& all_x, const CRealArray& all_y, const CBoolArray& goods,
                       size_t max_count, REAL_TYPE pval, REAL_TYPE recovery_threshold,
                       REAL_TYPE distweightfactor, size_t vertexcountovershoot, REAL_TYPE bestmodelproportion, TFitMethod fit_method)
{
    assert(all_x.size() == goods.size());
    assert(all_x.size() == all_y.size());

    CBestModelInfo best_model;

    //February 28, 2007. REK.
    //Derived from find_segments6 && 7.

    CRealArray x = all_x[goods];
    CRealArray y = all_y[goods];

    assert(x.size() >= 2);
    //*********************************
    //FIND ALL POTENTIAL VERTICES FIRST
    //
    //given a series of vals y, find the logical segments of
    //  straight lines.  Use the actual values of the
    //  curve for the fits

    CVectices v1 = find_vertices(x, y, max_count + vertexcountovershoot, distweightfactor);//MAKE INTO A PARAMETER
    const CVectices v = vet_verts3(x, y, v1, max_count, distweightfactor);

    assert(v.size() <= max_count);


    //catch the case where all values are same -- happens
    //  if the mask isn't quite right && the background value
    //  is set wrong.
    if (v.size() == 2)
        return best_model;



    //********************************
    //FIND BEST TRACE WITH ALL THOSE VERTICES
    //
    //The best trace is the set of y-values that best
    //    works with those vertices.  This approach
    //    uses find_best_trace, which uses a simple
    //    && relatively quick way to do this.  Starting
    //    at the left of the sequence, it allows segments
    //    to either track the two vertices directly,
    //    or to use a simple linear least-squares value
    //    anchored on the left vertex to connect to the
    //    next vertex.  This forces the sequence in order
    //    from left to right (oldest to newest).

    vector < CBestModelInfo > info;

    if (fit_method == FIT_EARLY_TO_LATE)
        info = get_all_model(x, y, v, recovery_threshold, FIT_EARLY_TO_LATE);
    else
        info = get_all_model(x, y, v, recovery_threshold, FIT_MPFIT);


    //************************************
    //PICK THE BEST ONE
    //
    //Along the way, though, we check to make
    //   sure that the best one doesn't violate
    //   the slopes  rule (where recovery can't
    //   be faster than a prescribed speed)
    //Note:  This section requires that recovery
    //   always result in a decrease in the y-values
    //   If the y-values are of a type where recovery
    //   actually results in an increase (e.g. biomass),
    //   you need to set the modifier value to -1 before
    //   running tbcd, so this recovery criterion will
    //   be applied appropriately.

    best_model = pick_best(info, pval, bestmodelproportion, recovery_threshold);


    //*******************************
    //IF NO GOOD FIT FOUND, TRY THE MPFIT APPROACH
    if (fit_method == FIT_EARLY_TO_LATE && best_model.p_of_f > pval)
    {
        //*********
        //Find the best fit using the marquardt approach (F7)

        //************************************
        //FIND BEST TRACE WITH ALL THOSE VERTICES
        info = get_all_model(x, y, v, recovery_threshold, FIT_MPFIT);

        //************************************
        //PICK THE BEST ONE
        best_model = pick_best(info, pval, bestmodelproportion, recovery_threshold);

    } // doing F7 if F6 didn't work

    //**************************************************************
    //Calculate the MSE of each segment, for use in masking ag later

    //assert(best != -1);
    best_model.segment_mse = score_segments(x, y, best_model.vertices);



    //************************
    //First, fill in the vertices. right now, the vertex values in the info structure are based
    //  on the x values passed to the routine (the ones that have been
    //  filtered), so we need to extend in case those first or last years are
    //  missing for this pixel.

    //first the middles

    if (all_x.size() != x.size())
    {
        CVectices G = get_pos(goods);
        best_model.vertices = G[best_model.vertices];

        //front end
        if (all_x[0] != x[0])
        {
            //originally, I had extended the segment forward. But this causes
            //  problems for disturbances that occur in year 3 (where year 1 has a cloud, year 2
            //   is pre-disturbance, && year 3 is the disturbance).  In those cases
            //   the disturbance is propagated back, doubling the intensity && shifting
            //    the year back one.  Therefore, the safer thing to do is to just
            //    add a new vertex in front, make it the same y-value.  For the
            //    long disturbance or recovery situations, this shouldn't add too
            //    much error, && it seems likely that they will get collapsed later
            //The one hitch is if there are already 6 segments, so we have to
            //   handle that

            size_t new_n_segments = best_model.slope.size() + 1;	//we're going to increment by 1
            size_t new_n_vertices = new_n_segments + 1;

            CVectices new_vertices(new_n_vertices);
            new_vertices[get_slice(1, new_vertices.size() - 1)] = best_model.vertices;// [get_slice(0, best_model.n_segments)] ;
            new_vertices[0] = 0;

            CRealArray new_vertvals(new_n_vertices);
            new_vertvals[get_slice(1, new_vertvals.size() - 1)] = best_model.vertvals;// [get_slice(0, best_model.n_segments)] ;
            new_vertvals[0] = new_vertvals[1];	//just copy the same y-val

            CRealArray new_slope(new_n_segments);
            new_slope[get_slice(1, new_slope.size() - 1)] = best_model.slope;// [get_slice(0, best_model.n_segments - 1)] ;//THIS COULD CAUSE POTENTIAL ERROR IN THE SLOPE!!!!
            new_slope[0] = 0;  //it's flat!

            CRealArray new_segment_mse(new_n_segments);
            new_segment_mse[get_slice(1, new_segment_mse.size() - 1)] = best_model.segment_mse;// [get_slice(0, best_model.n_segments - 1)] ;
            new_segment_mse[0] = 0;

            //check for number of segments, && take out if too many
            if (new_n_segments > max_count - 1)
            {
                //need to find out which one is least problematic to take out
                reduce_vertice(new_vertices, new_vertvals, new_slope, new_segment_mse);
            }		//confirmed 2/29/08 that this works.


            best_model.vertices = new_vertices;
            best_model.vertvals = new_vertvals;
            best_model.slope = new_slope;
            best_model.segment_mse = new_segment_mse;

        }

        //now do the other end
        //size_t n_all_x = all_x.size();		//n_obs is the number of filtered x's


        if (all_x[all_x.size() - 1] != x[x.size() - 1])
        {
            //if the last year is not the same

            size_t new_n_segments = best_model.slope.size() + 1;
            size_t new_n_vertices = new_n_segments + 1;

            CVectices new_vertices(new_n_vertices);
            new_vertices[get_slice(0, new_vertices.size() - 2)] = best_model.vertices;
            new_vertices[new_vertices.size() - 1] = all_x.size() - 1;

            CRealArray new_vertvals(new_n_vertices);
            new_vertvals[get_slice(0, new_vertvals.size() - 2)] = best_model.vertvals;
            new_vertvals[new_vertvals.size() - 1] = new_vertvals[new_vertvals.size() - 2];	//just copy the same y-val

            CRealArray new_slope(new_n_segments);
            new_slope[get_slice(0, new_slope.size() - 2)] = best_model.slope;
            new_slope[new_slope.size() - 1] = 0;  //it's flat!

            CRealArray new_segment_mse(new_n_segments);
            new_segment_mse[get_slice(0, new_segment_mse.size() - 2)] = best_model.segment_mse;
            new_segment_mse[new_segment_mse.size() - 1] = 0;


            //Assuming on
            if (new_n_segments > max_count - 1)
            {
                reduce_vertice(new_vertices, new_vertvals, new_slope, new_segment_mse);
            }

            best_model.vertices = new_vertices;
            best_model.vertvals = new_vertvals;
            best_model.slope = new_slope;
            best_model.segment_mse = new_segment_mse;

        }
    }



    if (best_model.p_of_f <= pval)
    {
        //***********************
        //Now fill in the yfit values for the "allyears" range, to get a
        //  true yfit even for years that were missing from this pixel


        //should be able to use "fill_from_vertices"
        //   need to pass the allyears, but that needs to be in
        //    the same year units as the x vals, so that means that I need
        //    to subtract the min of the allyears from both.
        //

        CFillFromVertices ok = fill_from_vertices(all_x, best_model.vertices, best_model.vertvals);
        best_model.yfit = ok.yfit;
    }
    else
    {
        //if not good, then set all to flat line
        best_model.ok = true;
        best_model.yfit.resize(all_y.size(), y.sum() / y.size());
        best_model.vertvals.resize(2, y.sum() / y.size());
        best_model.vertices = { 0, all_y.size() - 1 };
        best_model.slope.resize(1, 0);
        best_model.segment_mse.resize(1, 0);
    }

    return best_model;

}

}
