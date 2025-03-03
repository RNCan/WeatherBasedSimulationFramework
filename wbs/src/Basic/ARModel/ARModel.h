// Copyright (C) 2012, 2013 Rhys Ulerich
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once


#ifndef AR_SUPPRESS_DOXYGEN_MAINPAGE
/**
 * @mainpage
 *
 * ARModel implements CARModel "modeling tools" for autoregressive
 * processes in C++.
 *
 * This code was adapted from http://github.com/RhysU/ar
 *
 * If you find these tools useful towards publishing research, please consider
 * citing:
 * \li Todd A. Oliver, Nicholas Malaya, Rhys Ulerich, and Robert D. Moser.
 *     "Estimating uncertainties in statistics computed from direct numerical
 *     simulation." Physics of Fluids  26 (March 2014): 035101+.
 *     http://dx.doi.org/10.1063/1.4866813
 */
#endif /* AR_SUPPRESS_DOXYGEN_MAINPAGE */

/** @file
 * Simple autoregressive process modeling tools in C++.
 */

#include <vector>
#include <cmath>
#include <string>

namespace WBSF
{


/**
 * A parent type for autoregressive model selection criterion.
 *
 * Each subclass should have an <tt>overfit_penalty(N, p)</tt> method
 * following Broersen, P. M. and H. E. Wensink. "On Finite Sample Theory for
 * Autoregressive Model Order Selection." IEEE Transactions on Signal
 * Processing 41 (January 1993): 194+.
 * http://dx.doi.org/10.1109/TSP.1993.193138.
 */
struct criterion
{
    /** Compute the underfit penalty given \f$\sigma^2_\epsilon\f$. */
    template <typename Result, typename Input>
    static Result underfit_penalty(Input sigma2e)
    {
        using std::log;
        return log(Result(sigma2e));
    }
};

/**
 * Evaluate a given \ref criterion for \c N samples and model order \c p.
 *
 * @param[in] sigma2e The residual \f$\sigma^2_\epsilon\f$
 * @param[in] N       Sample count used to compute \f$\sigma^2_\epsilon\f$
 * @param[in] p       The model order use to compute \f$sigma^2_\epsilon\f$
 *
 * @return the evaluated criterion.
 */
template <class    Criterion,
          typename Result,
          typename Integer1,
          typename Integer2>
Result evaluate(Result sigma2e, Integer1 N, Integer2 p)
{
    Result underfit = Criterion::template underfit_penalty<Result>(sigma2e);
    Result overfit  = Criterion::template overfit_penalty<Result>(N, p);
    return underfit + overfit;
}

/**
 * Represents the generalized information criterion (GIC).  The penalty factor
 * \f$\alpha\f$ is controlled by <tt>AlphaNumerator / AlphaDenominator</tt>.
 */
template <int AlphaNumerator = 3, int AlphaDenominator = 1>
struct GIC : public criterion
{
    /** Compute overfit penalty given \c N observations at model order \c p. */
    template <typename Result, typename Integer1, typename Integer2>
    static Result overfit_penalty(Integer1 N, Integer2 p)
    {
        return Result(AlphaNumerator) * p / (N * AlphaDenominator);
    }
};

/** Represents the Akaike information criterion (AIC). */
struct AIC : public GIC<2> {};

/** Represents the consistent criterion BIC. */
struct BIC : public criterion
{
    /** Compute overfit penalty given \c N observations at model order \c p. */
    template <typename Result, typename Integer1, typename Integer2>
    static Result overfit_penalty(Integer1 N, Integer2 p)
    {
        using std::log;
        return log(Result(N)) * p / N;
    }
};

/** Represents the minimally consistent criterion (MCC). */
struct MCC : public criterion
{
    /** Compute overfit penalty given \c N observations at model order \c p. */
    template <typename Result, typename Integer1, typename Integer2>
    static Result overfit_penalty(Integer1 N, Integer2 p)
    {
        using std::log;
        return 2*log(log(Result(N))) * p / N;
    }
};

/**
 * Represents the asymptotically-corrected Akaike information criterion (AICC).
 */
struct AICC : public criterion
{
    /** Compute overfit penalty given \c N observations at model order \c p. */
    template <typename Result, typename Integer1, typename Integer2>
    static Result overfit_penalty(Integer1 N, Integer2 p)
    {
        return 2 * Result(p) / (N - p - 1);
    }
};



/**
 * \class CARModel
 * Implementation of a simple autoregressive process modeling tools in C++.
 * Let fit and predict
 */
class CARModel
{

public:


    /**
     * A test harness for Cedrick Collomb's Burg algorithm variant.
     *
     * Taken from Cedrick Collomb. "Burg's method, algorithm, and recursion",
     * November 2009 available at http://www.emptyloop.com/technotes/.
     */

    /**
     * @param[in]  order     Exclusive end of the input data range.
     * @param[in]  x         Input source data
     * @param[out] coeffs    The population variance of the data.
     *
     * @returns the variance of the residual \f$\sigma_\epsilon\f$
     */
    static double burg_method(std::size_t order, const std::vector<double>& x, std::vector<double>& coeffs,
                              std::vector<double>& gain, std::vector<double>& ac);

    /**
    * Compute the mean and population variance using Welford's algorithm.
    * The latter quantity is effectively the centered
    * sum of squares. The algorithm is found in Knuth's TAOCP volume 2 section
    * 4.2.2.A on page 232.  The implementation follows
    * http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance.
    *
    * @param[in]  x     input data
    *
    * @returns The population variance of the data.
    */
    static double welford_variance_population(const std::vector<double>& x);

    /**
    * Robustly compute negative one half the reflection coefficient assuming
    * \f$\vec{a}\f$ and \f$\vec{b}\f$ contain real-valued backward and forward
    * prediction error sequences, respectively.  Zero is returned whenever the
    * reflection coefficient numerator is identically zero, as otherwise
    * constant zero signals produce undesired NaN reflection coefficients.
    * The constant zero special case does not defeat NaN detection as any data
    * introducing NaN into the denominator must introduce NaN into the numerator.
    *
    * @param[in] a_first Beginning of the first input range \f$\vec{a}\f$.
    * @param[in] a_last  Exclusive end of first input range \f$\vec{a}\f$.
    * @param[in] b_first Beginning of the second input range \f$\vec{b}\f$.
     *
    * @return \f$\frac{\vec{a}\cdot\vec{b}}
    *                 {\vec{a}\cdot\vec{a} + \vec{b}\cdot\vec{b}}\f$
    *         when that numerator is nonzero, else zero.
    *
    * @see Wikipedia's article on <a href="">Kahan summation</a> for
    *      background on how the accumulation error is reduced in the result.
    */
    static double negative_half_reflection_coefficient( std::vector<double>::const_iterator a_first, std::vector<double>::const_iterator a_last, std::vector<double>::const_iterator b_first);

    /*! Set warmup*/
    static void set_warmup(std::size_t N)
    {
        N_WARM_UP=N;
    }

    /*! Get warmup*/
    static std::size_t get_warmup()
    {
        return N_WARM_UP;
    }


    /**
     * Constructor of a model
     * @param[in]   coeffs   Model parameters \f$a_i\f$, \dots
     * @param[in]   mean     Mean of the input. Will be add to prediction
     * @param[in]   sigma    Standard deviation discrepancy \f$\sigma_\epsilon\f$
     *
     */
    CARModel( const std::vector<double>& coeffs=std::vector<double>(), double mean=0, double sigma=0):
        m_coeffs(coeffs),
        m_mean(mean),
        m_sigma(sigma)
    {
    }

    /**
    * Fit an autoregressive model to stationary time series data using %Burg's
    * method.  That is, find coefficients \f$a_i\f$ such that the sum of the
    * squared errors in the forward predictions \f$x_n = -a_1 x_{n-1} - \dots -
    * a_p x_{n-p}\f$ and backward predictions \f$x_n = -a_1 x_{n+1} - \dots - a_p
    * x_{n+p}\f$ are both minimized.  This code will generate single model of
    * given order
    *
    * \f$a_\text{order}\f$ parameters for an AR(<tt>order</tt>) model.
    * The mean of the input data \f$\vec{x}\f$ is computed and store
    * when \c subtract_mean is true. Otherwise mean = 0. The estimated model
    * parameters \f$a_1, \dots, a_p\f$ and standard deviation discrepancy
    * \f$\sigma_\epsilon\f$ are store internally.
    *
    * The classical algorithm, rather than the variant using denominator recursion, due
    * to Andersen (http://dx.doi.org/10.1109/PROC.1978.11160), has been chosen as
    * the latter can be numerically unstable.
    *
    * @param[in]     order         autoregressive model order desired
    * @param[in]     x             input data
    * @param[in]     subtract_mean Should \c mean be subtracted from the data?
    *
    */
    void fit(std::size_t order, std::vector<double> x, bool subtract_mean=false);


    //void autofit(std::size_t max_order, std::vector<double> x, bool subtract_mean=false);

    /**
    * Simulate an autoregressive model process with optionally the initial state.
    * Random number will be distributed with \f$N\left(0, \sigma^2_\epsilon\right)\f$.
    *
    * @param[in] N       Number of values to genarate starting with \f$a_1\f$.
    * @param[in] x       The observed data to be completed
    *
    * @returns predicted values (<tt>y</tt>).
    */

    std::vector<double> predict(std::size_t N, const std::vector<double>& x=std::vector<double>())const;

    /*! Get coefficients*/
    const std::vector<double>& coeffs()const
    {
        return m_coeffs;
    }

    /*! Get size of observations*/
    const double N()const
    {
        return (double)m_N;
    }

    /*! Get mean of the model*/
    const double mean()const
    {
        return m_mean;
    }
    /*! Get standard deviation of residuals*/
    const double sigma()const
    {
        return m_sigma;
    }

    /**
    * Helper method returning criterion.
    * Intended for use within interactive APIs, this method handles much of the
    * ugliness of resolving overloaded criterion logic.
    * Criterion abbreviations can be:
    * <dl>
    * <dt>AIC </dt><dd>Akaike information criterion</dd>
    * <dt>AICC</dt><dd>asymptotically-corrected Akaike information criterion</dd>
    * <dt>GIC </dt><dd>generalized information criterion</dd>
    * <dt>BIC </dt><dd>consistent criterion BIC</dd>
    * <dt>MCC </dt><dd>minimally consistent criterion</dd>
    * </dl>
    * Leading or trailing whitespace as well as capitalization differences are
    * ignored.
    */
    double get_metrics(const std::string& criterion)const;

    /**
    * Returning metrics of a criterion.
    */
    template <class Criterion>
    double get_metrics()const
    {
        return evaluate<Criterion>(m_sigma*m_sigma, m_N, m_coeffs.size());
    }

protected:

    /*! The coefficients of the AR model */
    std::vector<double> m_coeffs;

    /*! Number of observation to create the model*/
    std::size_t m_N;
    /*! The mean of the model */
    double m_mean;
    /*! The standard deviation of the residual of the model*/
    double m_sigma;

    /*! gain for each order*/
    std::vector<double> m_gain;

    /*! auto correlation factor foe each order*/
    std::vector<double> m_ac;


    /*! warm up is the number of generation before computing prediction */
    static std::size_t N_WARM_UP;
};





}
