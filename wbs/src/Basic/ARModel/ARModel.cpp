// Except for any way in which it interferes with Cedrick Collomb's 2009
// copyright assertion in the article "Burg’s Method, Algorithm and Recursion":
//
// Copyright (C) 2012, 2013 Rhys Ulerich
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

/** @file
 * Class to fit and predict autoregression model (AR) using burg_method .
 */

#include "ARModel.h"

#include <random>
#include <chrono>
#include <algorithm>
#include <cassert>
#include <list>
#include <functional>
#include <numeric>



using namespace std;

namespace WBSF
{

	size_t CARModel::N_WARM_UP = 50;


	void CARModel::fit(size_t order, vector<double> x, bool subtract_mean)
	{
		assert(order < x.size());//order must alway be smaller that the data size.

		m_mean = 0;
		if (subtract_mean)
		{
			m_mean = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
			//transform(x.begin(), x.end(), x.begin(), bind2nd(minus<double>(), m_mean));
			transform(x.begin(), x.end(), x.begin(), bind(minus<double>(), std::placeholders::_1, this->m_mean));
		}


		m_sigma = burg_method(order, x, m_coeffs, m_gain, m_ac);
	}



	vector<double> CARModel::predict(size_t N, const vector<double>& x)const
	{
		assert(x.empty() || x.size() > m_coeffs.size());

		vector<double> coeffs = m_coeffs;
		std::transform(coeffs.begin(), coeffs.end(), coeffs.begin(), [](double a)
			{
				return -a;
			});//to be equivalent to R result

		//initialize output vector with the number of desired values
		vector<double> y(N);


		const unsigned seed = (unsigned)chrono::system_clock::now().time_since_epoch().count();
		default_random_engine generator(seed);
		normal_distribution<double> distribution(0.0, m_sigma);


		list<double> actual;
		if (x.size() > m_coeffs.size())
		{
			//use last sample observation
			for (size_t i = 0; i < m_coeffs.size(); ++i)
				actual.push_back(*(x.end() - m_coeffs.size() + i) - m_mean);
		}
		else
		{
			//generate random values
			for (size_t i = 0; i < m_coeffs.size(); ++i)
				actual.push_back(distribution(generator));

			// warm up
			for (size_t i = 0; i < N_WARM_UP; ++i)
			{
				//product of previous observation and coeficient
				double e = inner_product(actual.rbegin(), actual.rend(), m_coeffs.begin(), -distribution(generator));
				//update actual list with the new value and remove the last one
				actual.push_back(e);
				actual.pop_front();
			}
		}


		//generate predicted values
		for (size_t i = 0; i < y.size(); ++i)
		{
			//product of previous observations and coefficients
			double e = inner_product(actual.rbegin(), actual.rend(), m_coeffs.begin(), -distribution(generator));
			//update actual list with the new value and remove the last one
			actual.push_back(e);
			actual.pop_front();

			//store y for output
			y[i] = e;
		}



		//add mean (when subtract_mean is true in the fit) to all elements
		for (auto& e : y)
		{
			e += m_mean;
		}


		return y;
	}









	double CARModel::welford_variance_population(const std::vector<double>& x)
	{
		size_t N = 1;  // Running next sample number
		double  m = 0;  // Running mean of data thus far
		double  nv = 0;  // Running variance times the number of samples

		//while (first != last)
		for (auto const& e : x)
		{
			//value x  = *first++;
			double d = e - m;
			m += d / N++;
			nv += d * (e - m);
		}

		return nv /= (N - 1);

	}


	/*
	double CARModel::BurgAlgorithm( size_t order, const vector<double>& x, vector<double>& coeffs )
	{
		//resize output coeficients
		coeffs.resize(order, 0);

		double sigma2e =  welford_variance_population(x);

		// Get size from input vectors
		size_t N = x.size() - 1;


		// Initialize Ak
		vector<double> Ak( order + 1, 0.0 );
		Ak[ 0 ] = 1.0;

		// Initialize f and b
		vector<double> f( x );
		vector<double> b( x );

		// Initialize Dk
		double Dk = 0.0;
		for ( size_t j = 0; j <= N; j++ )
		{
			Dk += 2.0 * f[ j ] * f[ j ];
		}
		Dk -= f[ 0 ] * f[ 0 ] + b[ N ] * b[ N ];

		// Burg recursion
		for ( size_t k = 0; k < order; k++ )
		{
			// Compute MU
			double mu = 0.0;
			for ( size_t n = 0; n <= N - k - 1; n++ )
			{
				mu += f[ n + k + 1 ] * b[ n ];
			}
			mu *= -2.0 / Dk;

			// Update Ak
			for ( size_t n = 0; n <= ( k + 1 ) / 2; n++ )
			{
				double t1 = Ak[ n ] + mu * Ak[ k + 1 - n ];
				double t2 = Ak[ k + 1 - n ] + mu * Ak[ n ];
				Ak[ n ] = t1;
				Ak[ k + 1 - n ] = t2;
			}

			// UPDATE f and b
			for ( size_t n = 0; n <= N - k - 1; n++ )
			{
				double t1 = f[ n + k + 1 ] + mu * b[ n ];
				double t2 = b[ n ] + mu * f[ n + k + 1 ];
				f[ n + k + 1 ] = t1;
				b[ n ] = t2;
			}

			// UPDATE Dk
			Dk = ( 1.0 - mu * mu ) * Dk - f[ k + 1 ] * f[ k + 1 ] - b[ N - k - 1 ] * b[ N - k - 1 ];

			sigma2e *= ( 1.0 - mu * mu );
		}

		// Assign coefficients
		coeffs.assign( ++Ak.begin(), Ak.end() );

		//Change sign of coefficients to be equivalent to R arima result
		std::transform(coeffs.begin(), coeffs.end(), coeffs.begin(),[](double a) { return -a; });

		return sqrt(sigma2e);
	}
	*/

	double CARModel::negative_half_reflection_coefficient(std::vector<double>::const_iterator a_first,
		std::vector<double>::const_iterator a_last,
		std::vector<double>::const_iterator b_first)
	{
		//assert(b.size() == a.size());

		double ns = 0, nt, nc = 0, ny;  // Kahan numerator accumulation
		double ds = 0, dt, dc = 0, dy;  // Kahan denominator accumulation

		while (a_first != a_last)
		{
			double xa = *a_first++;     // Denominator: \vec{a}\cdot\vec{a}
			dy = (xa * xa) - dc;
			dt = ds + dy;
			dc = (dt - ds) - dy;
			ds = dt;

			double xb = *b_first++;     // Denominator: \vec{b}\cdot\vec{b}
			dy = (xb * xb) - dc;
			dt = ds + dy;
			dc = (dt - ds) - dy;
			ds = dt;

			ny = (xa * xb) - nc;           // Numerator:   \vec{a}\cdot\vec{b}
			nt = ns + ny;
			nc = (nt - ns) - ny;
			ns = nt;
		}

		return ns + nc == 0                // Does special zero case apply?
			? 0                            // Yes, to avoid NaN from 0 / 0
			: (ns + nc) / (ds + dc);       // No, correct final sums and form ratio
	}

	double CARModel::burg_method(size_t order, const vector<double>& x, vector<double>& coeffs, vector<double>& all_gain, vector<double>& ac)
	{
		// Initialize f from [data_first, data_last) and fix number of samples
		vector<double> f = x;
		const size_t N = f.size();

		// Stably compute the incoming data's mean and population variance
		double mean = std::accumulate(f.begin(), f.end(), 0.0) / f.size();
		double sigma2e = welford_variance_population(f);

		// When requested, subtract the just-computed mean from the data.
		// Adjust, if necessary, to make sigma2e the second moment.
		sigma2e += mean * mean;

		// Output sigma2e and gain for a zeroth order model, if requested.
		double gain = 1.0;

		// Initialize and perform Burg recursion

		vector<double> b = f;
		vector<double> Ak(order + 1, 0.0);
		Ak[0] = 1;
		//vector<double> ac;
		ac.clear();
		ac.reserve(order);
		for (size_t kp1 = 1; kp1 <= order; ++kp1)
		{
			// Compute mu from f, b, and Dk and then update sigma2e and Ak using mu
			// Afterwards, Ak[1:kp1] contains AR(k) coefficients by the recurrence
			// Must treat mu result of 0 / 0 as 0 to avoid NaNs on constant signals
			// By the recurrence, Ak[kp1] will also be the reflection coefficient
			double mu = -2 * negative_half_reflection_coefficient(f.begin() + kp1, f.end(), b.begin());

			sigma2e *= (1 - mu * mu);
			for (size_t n = 0; n <= kp1 / 2; ++n)
			{
				double t1 = Ak[n] + mu * Ak[kp1 - n];
				double t2 = Ak[kp1 - n] + mu * Ak[n];
				Ak[n] = t1;
				Ak[kp1 - n] = t2;
			}

			// Update the gain per Broersen 2006 equation (5.25)
			gain *= 1 / (1 - Ak[kp1] * Ak[kp1]);
			all_gain.push_back(gain);


			// Compute and output the next autocorrelation coefficient
			// See Broersen 2006 equations (5.28) and (5.31) for details
			ac.push_back(-inner_product(ac.rbegin(), ac.rend(), Ak.begin() + 1, Ak[kp1]));

			// Output parameters and the input and output variances when requested

	//        params_first = copy(Ak.begin() + 1, Ak.begin() + kp1 + 1, params_first);
			// *sigma2e_first++ = sigma2e;
			// *gain_first++    = gain;


			// Update f and b for the next iteration if another remains
			if (kp1 < order)
			{
				for (size_t n = 0; n < N - kp1; ++n)
				{
					double t1 = f[n + kp1] + mu * b[n];
					double t2 = b[n] + mu * f[n + kp1];
					f[n + kp1] = t1;
					b[n] = t2;
				}
			}
		}

		// Assign coefficients
		coeffs.assign(++Ak.begin(), Ak.end());


		//Change sign of coefficients to be equivalent to R arima result
		std::transform(coeffs.begin(), coeffs.end(), coeffs.begin(), [](double a)
			{
				return -a;
			});
		//std::bind1st(std::multiplies<double>(), -1.0));


		// Output the lag [0,maxorder] autocorrelation coefficients in single pass
		//*autocor_first++ = 1;
		//copy(ac.begin(), ac.end(), autocor_first);
		ac.insert(ac.begin(), 1);

		// Return the number of values processed in [data_first, data_last)
		return sqrt(sigma2e);
	}



	double CARModel::get_metrics(const std::string& criterion)const
	{
		double metrics = 0;

		if (0 == criterion.compare("AIC"))
		{
			metrics = get_metrics<AIC>();
		}
		else if (0 == criterion.compare("AICC"))
		{
			metrics = get_metrics<AICC>();
		}
		else if (0 == criterion.compare("BIC"))
		{
			metrics = get_metrics<BIC>();
		}
		else if (0 == criterion.compare("GIC"))
		{
			metrics = get_metrics<GIC<>>();
		}
		else if (0 == criterion.compare("MCC"))
		{
			metrics = get_metrics<MCC>();
		}

		return metrics;
	}


	//cmake "/ucrt64/lib/cmake/UnitTest++" -D CMAKE_BUILD_TYPE=Release -D CMAKE_C_COMPILER="/ucrt64/bin/gcc.exe" -D CMAKE_CXX_COMPILER="/ucrt64/bin/g++.exe" -D CMAKE_MAKE_PROGRAM="/ucrt64/bin/mingw32-make.exe"
	//cmake --build .




}//namespace WBSF
