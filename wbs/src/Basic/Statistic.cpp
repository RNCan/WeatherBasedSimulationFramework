//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
// Class:	CStatistic, CStatisticEx, CStatisticXW, CStatisticXY, CStatisticXYEx, CStatisticXYW
//			
//****************************************************************************
// 02/12/2015   Rémi Saint-Amant  Add weathed statistic and quartile
// 22/12/2014   Rémi Saint-Amant  Add RANGE as statistic type
// 24/05/2013	Rémi Saint-Amant  Add standart deviation over population (N) 
// 20/02/2009	Rémi Saint-Amant  Add classification class
// 04/03/2004	Rémi Saint-Amant  Remove the old GetVaporPressureDeficit
// 20/11/2003	Rémi Saint-Amant  Initiale Version
//****************************************************************************

#include "stdafx.h"
#include <math.h>
#include <float.h>
#include <algorithm>
#include <boost/math/distributions/normal.hpp>


#include "Basic/Statistic.h"
#include "Basic/UtilMath.h"
#include "WeatherBasedSimulationString.h"


using namespace std;

namespace WBSF
{

	 
	//this static varaible is share by CStatistic and CStatisticXY
	double STAT_VMISS = -9999999.0;


	const char* CStatistic::NAME[NB_STATXY_TYPE_EX] =
	{ 
		"Lowest", "Mean", "Sum", "Sum²", "StandardDeviation (N-1)", "StandardDeviation (N)", "StandardError", "CoeficientOfVariation", "Variance", "Highest", "TotalSumOfSquares", "QuadraticMean", "Range", "NbValue",
		"MeanAbsoluteDeviation", "Skewness", "Kurtosis", "Median", "Mode", "Lo", "Q¹", "Q³", "Hi", "IQR",
		"MeanX", "MeanY", "Intercept", "Slope", "Covariance", "Correlation", "Bias", "MeanAbsolutError", "RootMeanSquareError", "ResidualSumOfSquare", "CoeficientOfDetermination", "CoeficientOfCorrelation", "R²",
		"TheilSenIntercept", "TheilSenSlope", "LogLikelihood", "LogLikelihood2"
	};

	

	StringVector CStatistic::TITLE;
	const char* CStatistic::GetTitle(size_t s)
	{
		assert(s < NB_STATXY_TYPE_EX);
		if (TITLE.empty())
			TITLE.LoadString(IDS_STR_STATISTIC, "|;");

		assert(TITLE.size() == NB_STATXY_TYPE_EX);

		return TITLE[s].c_str();
	}

	void CStatistic::ReloadString()
	{
		TITLE.clear();
	}

	CStatistic::CStatistic()
	{
		Reset();
	}

	void CStatistic::Reset()
	{
		m_nbValues = 0;

		m_lowest = DBL_MAX;
		m_sum = 0;
		m_sum² = 0;
		m_hightest = -DBL_MAX;
	}

	CStatistic& CStatistic::operator=(const CStatistic& in)
	{
		if (&in != this)
		{
			m_nbValues = in.m_nbValues;

			m_lowest = in.m_lowest;
			m_sum = in.m_sum;
			m_sum² = in.m_sum²;
			m_hightest = in.m_hightest;
		}


		return *this;
	}

	CStatistic& CStatistic::operator+=(double value)
	{
		_ASSERTE(!_isnan(value));

		m_nbValues++;

		if (value < m_lowest)
			m_lowest = value;

		m_sum += value;
		m_sum² += Square(value);
		if (value > m_hightest)
			m_hightest = value;

		return *this;
	}

	CStatistic& CStatistic::operator+=(const CStatistic& statistic)
	{
		m_nbValues += statistic.m_nbValues;

		if (statistic.m_lowest < m_lowest)
			m_lowest = statistic.m_lowest;

		m_sum += statistic.m_sum;
		m_sum² += statistic.m_sum²;
		if (statistic.m_hightest > m_hightest)
			m_hightest = statistic.m_hightest;

		return *this;
	}

	double CStatistic::operator[](size_t type)const
	{
		_ASSERTE(type >= 0 && type < NB_STAT_TYPE);
		const CStatistic& me = *this;

		double value = STAT_VMISS;

		if (m_nbValues > 0 || type == NB_VALUE)
		{
			size_t nbVal = m_nbValues == 1 ? m_nbValues : m_nbValues - 1;
			switch (type)
			{
			case LOWEST: value = m_lowest; break;
			case SUM:value = m_sum; break;
			case SUM²:value = m_sum²; break;
			case MEAN:value = m_sum / m_nbValues; break;
			case STD_DEV:value = sqrt(std::max(0.0, (m_sum² - Square(m_sum) / m_nbValues)) / (nbVal)); break;
			case STD_DEV_OVER_POP:value = sqrt(std::max(0.0, (m_sum² - Square(m_sum) / m_nbValues)) / (m_nbValues)); break;
			case STD_ERR:value = (me[STD_DEV] / sqrt((double)m_nbValues)); break;
			case TSS: value = std::max(0.0, m_sum² - Square(m_sum) / m_nbValues); break;
			case COEF_VAR:
				if (!(m_sum² > 0 && m_sum == 0))
				{
					value = m_sum != 0 ? me[STD_DEV] / me[MEAN] : 0;//est-ce que doit accepter les valeur négative???
					break;
				}
			case VARIANCE:value = std::max(0.0, (m_sum² - Square(m_sum) / m_nbValues) / (nbVal)); break;
			case HIGHEST:value = m_hightest; break;
			case QUADRATIC_MEAN:  value = sqrt(m_sum² / m_nbValues); break;
			case RANGE: value = m_hightest - m_lowest; break;
			case NB_VALUE:value = (double)m_nbValues; break;
			default: _ASSERTE(false);//not supported by this statistic class
			}
		}

		_ASSERTE(!_isnan(value));
		return value;
	}


	double CStatistic::GetVMiss(){ return STAT_VMISS; }
	void CStatistic::SetVMiss(double vmiss){ STAT_VMISS = vmiss; }

	bool CStatistic::operator==(const CStatistic& in)const
	{
		bool bEqual = true;
		if (m_nbValues != in.m_nbValues)bEqual = false;
		if (fabs(m_lowest - in.m_lowest) > EPSILON_DATA)bEqual = false;
		if (fabs(m_sum - in.m_sum) > EPSILON_DATA)bEqual = false;
		if (fabs(m_sum² - in.m_sum²) > EPSILON_DATA)bEqual = false;
		if (fabs(m_hightest - in.m_hightest) > EPSILON_DATA)bEqual = false;

		return bEqual;
	}
	//*************************************************************
	//CStatisticEx : keep in memory all elements for MAD computation

	CStatisticEx::CStatisticEx()
	{
		Reset();
	}

	void CStatisticEx::Reset()
	{
		CStatistic::Reset();
		m_values.clear();
		m_sorted.clear();
	}

	CStatisticEx& CStatisticEx::operator=(const CStatisticEx& in)
	{
		CStatistic::operator=(in);
		m_values = in.m_values;
		m_sorted.clear();

		return *this;
	}

	CStatisticEx&  CStatisticEx::operator+=(double value)
	{
		CStatistic::operator+=(value);
		m_values.push_back(value);
		m_sorted.clear();

		return *this;
	}

	CStatisticEx&  CStatisticEx::operator+=(const CStatisticEx& statistic)
	{
		CStatistic::operator+=(statistic);
		m_values.insert(m_values.end(), statistic.m_values.begin(), statistic.m_values.end());
		m_sorted.clear();

		return *this;
	}

	void CStatisticEx::sort()const
	{
		if (m_sorted.empty())
		{
			CStatisticEx& me = const_cast<CStatisticEx&>(*this);
			me.m_sorted = m_values;
			std::sort(me.m_sorted.begin(), me.m_sorted.end());
		}

		ASSERT(m_sorted.size() == m_values.size());
	}

	double CStatisticEx::operator[](size_t type)const
	{
		_ASSERTE(m_values.size() == m_nbValues);
		_ASSERTE(type >= 0 && type < NB_STAT_TYPE_EX);
		const CStatisticEx& me = *this;

		double value = STAT_VMISS;

		if (m_nbValues > 0 || type == NB_VALUE)
		{
			if (type == MAD)
			{
				value = 0;
				double mean = CStatistic::operator[](MEAN);
				for (int i = 0; i < (int)m_values.size(); i++)
					value += fabs(m_values[i] - mean);

				value /= m_values.size();
			}
			else if (type == SKEWNESS)
			{
				value = 0;
				double mean = CStatistic::operator[](MEAN);
				for (int i = 0; i < (int)m_values.size(); i++)
					value += pow(m_values[i] - mean, 3);

				double SD = CStatistic::operator[](STD_DEV);
				double tmp = ((m_values.size() - 1)*pow(SD, 3));

				if (tmp != 0)
					value /= tmp;
			}
			else if (type == KURTOSIS)
			{
				value = 0;
				double mean = CStatistic::operator[](MEAN);
				for (int i = 0; i < (int)m_values.size(); i++)
					value += pow(m_values[i] - mean, 4);

				double SD = CStatistic::operator[](STD_DEV);
				double tmp = ((m_values.size() - 1)*pow(SD, 4));

				if (tmp != 0)
					value /= tmp;
			}
			else if (type == MEDIAN || type == Q²)
			{
				sort();

				size_t N1 = (m_sorted.size() + 1) / 2 - 1;
				size_t N2 = m_sorted.size() / 2 + 1 - 1;
				ASSERT(N2 == N1 || N2 == N1 + 1);
				
				value = (m_sorted[N1] + m_sorted[N2]) / 2.0;
			}
			else if (type == MODE )
			{
				sort();

				int maxOcc = 0;
				int occ = 0;
				double lastVal = -DBL_MAX;
				for (size_t i = 0; i < m_sorted.size(); i++)
				{
					if (fabs(m_sorted[i] - lastVal) < 0.000001)
					{
						occ++;
					}
					else
					{
						occ = 1;
						lastVal = m_sorted[i];
					}

					if (occ > maxOcc)
					{
						maxOcc = occ;
						value = lastVal;
					}
				}
			}
			else if (type == Qᴸ)
			{
				sort();
				
				double Q1 = me[Q¹];
				double IQR = me[INTER_Q];
				value = max(Q1 - 1.5*IQR, m_sorted.front());
			}
			else if (type == Q¹)
			{
				size_t N1 = (m_sorted.size() + 1) / 4 - 1;
				size_t N2 = m_sorted.size() / 4 + 1 - 1;
				ASSERT(N2 == N1 || N2 == N1 + 1);

				sort();
				value = (m_sorted[N1] + m_sorted[N2]) / 2.0;
			}
			else if (type == Q³)
			{
				size_t N1 = 3 * (m_sorted.size() + 1) / 4 - 1;
				size_t N2 = 3 * m_sorted.size() / 4 + 1 - 1;
				ASSERT(N2 == N1 || N2 == N1 + 1);

				sort();
				value = (m_sorted[N1] + m_sorted[N2]) / 2.0;
			}
			else if (type == Qᴴ)
			{
				sort();

				double Q3 = me[Q³];
				double IQR = me[INTER_Q];
				value = min(Q3 + 1.5*IQR, m_sorted.back());
			}
			else if (type == INTER_Q)
			{
				value = me[Q³] - me[Q¹];
			}
			else
			{
				value = CStatistic::operator[](type);
			}
		}

		_ASSERTE(!_isnan(value));
		return value;
	}

	double CStatisticEx::percentil(double p)const
	{
		ASSERT(p >= 0 && p <= 100);
		sort();//create sorted array
		
		double value = STAT_VMISS;

		if (!m_sorted.empty())
		{
			size_t n = max(size_t(1), size_t(ceil(p / 100 * m_sorted.size()))) - size_t(1);
			value = m_sorted[n];
		}

		return value;
	}

	//*************************************************************
	//Weighted CStatistic
	CStatisticXW::CStatisticXW()
	{
		m_wx = 0;
		m_wx² = 0;
	}


	void CStatisticXW::Reset()
	{
		m_x.clear();
		m_w.clear();
		m_wx = 0;
		m_wx² = 0;
	}

	CStatisticXW& CStatisticXW::operator=(const CStatisticXW& in)
	{
		m_x = in.m_x;
		m_w = in.m_w;
		m_wx = in.m_wx;
		m_wx² = in.m_wx²;

		return *this;
	}

	CStatisticXW& CStatisticXW::Add(double x, double w)
	{
		m_x.insert(x);
		m_w.insert(w);
		m_wx += w*x;
		m_wx² += w*x*x;

		return *this;
	}

	CStatisticXW& CStatisticXW::operator+=(const CStatisticXW& statistic)
	{
		m_x += statistic.m_x;
		m_w += statistic.m_w;
		m_wx += statistic.m_wx;
		m_wx² += statistic.m_wx²;

		return *this;
	}

	double CStatisticXW::operator[](size_t type)const
	{
		ASSERT(m_x[NB_VALUE] == m_w[NB_VALUE]);
		ASSERT(type >= 0 && type < NB_STAT_TYPE);

		double value = STAT_VMISS;

		const CStatisticXW& me = *this;
		double nbValues = m_x[NB_VALUE];

		if (nbValues > 0 || type == NB_VALUE)
		{
			switch (type)
			{
			case LOWEST:	value = m_x[LOWEST]; break;
			case SUM:		value = m_wx; break;
			case SUM²:		value = m_wx²; break;
			case MEAN:		value = m_wx / m_w[SUM]; break;
				//from https://stat.ethz.ch/pipermail/r-help/2008-July/168762.html
			case STD_DEV:
			case STD_DEV_OVER_POP:
			{
				if ((Square(m_w[SUM]) - m_w[SUM²]) != 0)
				{
					double mean = me[MEAN];
					double sum = 0;
					for (size_t i = 0; i < m_x().size(); i++)
						sum += m_w(i) * Square(m_x(i) - mean);

					value = m_w[SUM] / (Square(m_w[SUM]) - m_w[SUM²]) *sum;
				}
				break;
			}
			case STD_ERR:value = (me[STD_DEV] / sqrt(m_w[SUM])); break;
			case TSS: value = std::max(0.0, m_wx² * m_w[SUM] - Square(m_wx)); break;
			case COEF_VAR:
				if (!(m_wx²>0 && m_wx == 0))
				{
					value = m_wx > 0 ? me[STD_DEV] / me[MEAN] : 0;
					break;
				}

			case VARIANCE: value = ((Square(m_w[SUM]) - m_w[SUM²]) != 0) ? (m_wx² * m_w[SUM] - Square(m_wx)) / (Square(m_w[SUM]) - m_w[SUM²]) : VMISS; break;
			case HIGHEST: value = m_x[HIGHEST]; break;
			case QUADRATIC_MEAN:  value = sqrt(m_wx² / m_w[SUM]); break;
			case RANGE: value = m_x[RANGE]; break;
			case NB_VALUE: value = nbValues; break;
			default: ASSERT(false);//not supported by this class
			}
		}

		_ASSERTE(!_isnan(value));

		return value;
	}


	//*************************************************************
	//CStatisticXY: statistic of 2 variables

	CStatisticXY::CStatisticXY()
	{
		Reset();
	}

	CStatisticXY& CStatisticXY::operator=(const CStatisticXY& in)
	{
		if (&in != this)
		{
			m_x = in.m_x;
			m_y = in.m_y;
			m_lowest = in.m_lowest;
			m_xy = in.m_xy;
			m_e = in.m_e;
			m_rs = in.m_rs;
			m_ae = in.m_ae;
			m_hightest = in.m_hightest;
		}

		return *this;
	}

	void CStatisticXY::Reset()
	{
		m_x.clear();
		m_y.clear();

		m_lowest = DBL_MAX;
		m_xy = 0;
		m_e = 0;
		m_rs = 0;
		m_ae = 0;
		m_hightest = -DBL_MAX;
	}

	//x = pred, y = obs
	CStatisticXY& CStatisticXY::Add(double x, double y)
	{
		m_x += x;
		m_y += y;

		double e = (x - y);
		if (e < m_lowest)
			m_lowest = e;

		m_xy += x*y;
		m_e += (x - y);
		m_rs += Square(x - y);
		m_ae += abs(x - y);

		if (e > m_hightest)
			m_hightest = e;

		return *this;
	}

	CStatisticXY& CStatisticXY::operator+=(const CStatisticXY& in)
	{
		m_x += in.m_x;
		m_y += in.m_y;

		if (in.m_lowest < m_lowest)
			m_lowest = in.m_lowest;

		m_xy += in.m_xy;
		m_e += in.m_e;
		m_rs += in.m_rs;
		m_ae += in.m_ae;

		if (in.m_hightest > m_hightest)
			m_hightest = in.m_hightest;

		return *this;
	}

	double CStatisticXY::operator[](size_t type)const
	{
		_ASSERTE(m_x[NB_VALUE] == m_y[NB_VALUE]);
		_ASSERTE(type >= 0 && type < NB_STATXY_TYPE);

		double value = STAT_VMISS;

		const CStatisticXY& me = *this;
		double nbValues = m_x[NB_VALUE];

		if (nbValues > 0 || type == NB_VALUE)
		{
			double nbVal = (nbValues == 1) ? nbValues : nbValues - 1;
			switch (type)
			{
			case LOWEST:	value = m_lowest; break;
			case MEAN:		value = m_e / nbValues; break;//mean of error (bias) (m_y[MEAN] + m_x[MEAN]) / 2; break;
			case MEAN_X:	value = m_x[MEAN]; break;
			case MEAN_Y:	value = m_y[MEAN]; break;
			case TSS:		value = m_y[TSS]; break;
			case INTERCEPT: value = m_y[MEAN] - m_x[MEAN] * me[SLOPE]; break;
			case SLOPE:
				if (m_x[TSS] != 0)
					value = (m_xy - (m_x[SUM] * m_y[SUM] / nbValues)) / m_x[TSS];
				break;
			case COVARIANCE: value = ((m_xy - m_x[SUM] * m_y[MEAN] - m_y[SUM] * m_x[MEAN] + nbValues*m_x[MEAN] * m_y[MEAN])) / nbVal; break;
			case CORRELATION:
			{
				double stdDev = (m_x[STD_DEV] * m_y[STD_DEV]);
				if (stdDev != 0)
					value = me[COVARIANCE] / stdDev;
				break;
			}
			case BIAS:		value = m_e / nbValues; break;
			case MAE:		value = m_ae / nbValues; break;
			case RMSE:		value = sqrt(m_rs / nbValues); break;
			case RSS:		value = m_rs; break;
			case COEF_D:	value = me[TSS] > 0 ? (1 - me[RSS] / me[TSS]) : STAT_VMISS; break;
			case COEF_C:
			{
				double d = sqrt(nbValues*m_x[SUM²] - Square(m_x[SUM]))*sqrt(nbValues*m_y[SUM²] - Square(m_y[SUM]));
				if (d != 0)
					value = (nbValues*m_xy - m_x[SUM] * m_y[SUM]) / d;
				break;
			}
			case STAT_R²:
			{
				double c = me[COEF_C];
				if (c > STAT_VMISS)
					value = Square(c);
				break;
			}
			case HIGHEST:	value = m_hightest; break;
			case RANGE:		value = m_hightest - m_lowest; break;
			case NB_VALUE:	value = nbValues; break;
			default: _ASSERTE(false);
			}
		}

		_ASSERTE(!_isnan(value));

		return value;
	}

	double CStatisticXY::GetVMiss(){ return STAT_VMISS; }
	void CStatisticXY::SetVMiss(double vmiss){ STAT_VMISS = vmiss; }

	//*****************************************************************
	//CStatisticXYEx: statistic of 2 variables, keeps all values in memory
	CStatisticXYEx::CStatisticXYEx()
	{
		Reset();
	}

	void CStatisticXYEx::Reset()
	{
		CStatisticXY::Reset();
		m_xValues.clear();
		m_yValues.clear();
	}

	CStatisticXYEx& CStatisticXYEx::operator=(const CStatisticXYEx& in)
	{
		if (&in != this)
		{
			CStatisticXY::operator=(in);
			m_xValues = m_xValues;
			m_yValues = m_yValues;
		}

		return *this;
	}

	double CStatisticXYEx::operator[](size_t type)const
	{
		_ASSERTE(m_xValues.size() == m_yValues.size());
		_ASSERTE(m_xValues.size() == GetX()[NB_VALUE]);
		_ASSERTE(m_yValues.size() == GetY()[NB_VALUE]);
		_ASSERTE(type >= 0 && type < NB_STATXY_TYPE_EX);

		const CStatisticXYEx& me = *this;
		double value = STAT_VMISS;

		if (m_xValues.size() > 0)
		{
			if (type == INTERCEPT_THEIL_SEN)
			{
				value = m_yValues[MEDIAN] - m_xValues[MEDIAN] * me[SLOPE_THEIL_SEN];
			}
			else if (type == SLOPE_THEIL_SEN)
			{
				CStatisticEx stat;
				ASSERT(m_xValues.size() == m_yValues.size());
				for (size_t i = 0; i < m_xValues.size(); i++)
				{
					for (size_t j = i + 1; j < m_xValues.size(); j++)
					{
						double deltaX = (m_xValues[i] - m_xValues[j]);
						double deltaY = (m_yValues[i] - m_yValues[j]);
						if (deltaX != 0)
							stat += deltaY / deltaX;
					}
				}

				value = stat[MEDIAN];
			}
			else if (type == LOG_LIKELIHOOD1)
			{
				//double xSum = m_x[SUM];
				double ySum = m_y[SUM];
				if (ySum > 0)
				{
					double LL = 0;
					//likelihood with classical sigma hat
					double sigma = sqrt(me[RSS] / (m_xValues.size() - 1))*(m_xValues.size()) / (m_xValues.size() - 1);
					for (size_t i = 0; i < m_xValues.size(); i++)
					{
						double m = m_xValues[i];
						boost::math::normal_distribution<> N(m, sigma);

						double x = m_yValues[i];
						double p = boost::math::pdf(N, x);
						ASSERT(p > 0);
						LL += log(p);
					}

					value = LL;
				}
			}
			else if (type == LOG_LIKELIHOOD2)
			{
				double xSum = m_x[SUM];
				double ySum = m_y[SUM];
				if (ySum > 0)
				{
					double LL = LogFactorial(xSum);
					for (size_t i = 0; i < m_xValues.size(); i++)
					{
						double p = pow(m_yValues[i] / ySum, m_xValues[i]);
						LL += log(p);
						LL -= LogFactorial(m_xValues[i]);
					}

					value = LL;
				}
			}
			else
			{
				value = CStatisticXY::operator[](type);
			}
		}

		_ASSERTE(!_isnan(value) && _finite(value));
		return value;
	}


	//x = pred, y = obs
	CStatisticXYEx& CStatisticXYEx::Add(double x, double y)
	{
		m_CS.Enter();
		CStatisticXY::Add(x, y);

		m_xValues.push_back(x);
		m_yValues.push_back(y);
		m_CS.Leave();

		return *this;
	}

	CStatisticXYEx& CStatisticXYEx::operator+=(const CStatisticXYEx& statistic)
	{
		CStatisticXY::operator+=(statistic);
		m_xValues.insert(m_xValues.end(), statistic.m_xValues.begin(), statistic.m_xValues.end());
		m_yValues.insert(m_yValues.end(), statistic.m_yValues.begin(), statistic.m_yValues.end());

		return *this;
	}


	//*************************************************************
	//Weighted CStatisticXY
	CStatisticXYW::CStatisticXYW()
	{
		m_wxy = 0;
		m_we = 0;
		m_wrs = 0;
		m_wae = 0;
	}


	void CStatisticXYW::Reset()
	{
		m_xw.clear();
		m_yw.clear();
		m_wxy = 0;
		m_we = 0;
		m_wrs = 0;
		m_wae = 0;
	}

	CStatisticXYW& CStatisticXYW::operator=(const CStatisticXYW& in)
	{
		m_xw = in.m_xw;
		m_yw = in.m_yw;
		m_wxy = in.m_wxy;
		m_we = in.m_we;
		m_wrs = in.m_wrs;
		m_wae = in.m_wae;

		return *this;
	}

	CStatisticXYW& CStatisticXYW::Add(double x, double y, double w)
	{
		m_xw.insert(x, w);
		m_yw.insert(y, w);
		double e = x - y;
		m_wxy += w*x*y;
		m_we += w*e;
		m_wrs += w*e*e;
		m_wae += w*fabs(e);

		return *this;
	}

	CStatisticXYW& CStatisticXYW::operator+=(const CStatisticXYW& in)
	{
		m_xw += in.m_xw;
		m_yw += in.m_yw;
		m_wxy += in.m_wxy;
		m_we += in.m_we;
		m_wrs += in.m_wrs;
		m_wae += in.m_wae;

		return *this;
	}

	double CStatisticXYW::operator[](size_t type)const
	{
		ASSERT(m_xw[NB_VALUE] == m_yw[NB_VALUE]);
		ASSERT(type >= 0 && type < NB_STATXY_TYPE);

		double value = STAT_VMISS;

		const CStatisticXYW& me = *this;
		double nbValues = m_xw[NB_VALUE];

		if (nbValues > 0 || type == NB_VALUE)
		{
			switch (type)
			{
			case LOWEST: /*value = m_e[LOWEST];*/ break;
			case MEAN: value = m_we / m_xw.W(SUM); break;
			case MEAN_X: value = m_xw[MEAN]; break;
			case MEAN_Y:value = m_yw[MEAN]; break;
			case TSS: value = m_yw[TSS]; break;
			case INTERCEPT: value = m_yw[MEAN] - m_xw[MEAN] * me[SLOPE]; break;
			case SLOPE:
				if (m_xw[TSS] != 0)
					value = (m_wxy - (m_xw[SUM] * m_yw[SUM] / m_xw.W(SUM))) / m_xw[TSS];
				break;
			case COVARIANCE: value = ((m_wxy - m_xw[SUM] * m_yw[MEAN] - m_yw[SUM] * m_xw[MEAN] + m_xw.W(SUM)*m_xw[MEAN] * m_yw[MEAN])) / m_xw.W(SUM); break;
			case CORRELATION:
			{
				double stdDev = (m_xw[STD_DEV] * m_yw[STD_DEV]);
				if (stdDev != 0)
					value = me[COVARIANCE] / stdDev;
				break;
			}

			case BIAS: value = m_we / m_xw.W(SUM); break;
			case MAE: value = m_wae / m_xw.W(SUM); break;
			case RMSE: value = sqrt(m_wrs / m_xw.W(SUM)); break;//ici il ya une erreur dans le calcul... a faire
			case RSS: value = m_wrs / m_xw.W(SUM); break;
			case COEF_D: value = me[TSS] > 0 ? (1 - me[RSS] / me[TSS]) : STAT_VMISS; break;
			case COEF_C:
			{
				double d = sqrt(max(0.0, m_xw.W(SUM)*m_xw[SUM²] - Square(m_xw[SUM])))*sqrt(max(0.0, m_xw.W(SUM) * m_yw[SUM²] - Square(m_yw[SUM])));
				if (d != 0)
					value = (m_xw.W(SUM)*m_wxy - m_xw[SUM] * m_yw[SUM]) / d;
				break;
			}
			case STAT_R²:
			{
				double c = me[COEF_C];
				if (c > STAT_VMISS)
					value = Square(c);
				break;
			}
			case HIGHEST: value = STAT_VMISS; break;
			case RANGE: value = STAT_VMISS; break;
			case NB_VALUE: value = nbValues; break;
			default: _ASSERTE(false);
			}
		}

		_ASSERTE(!_isnan(value));

		return value;
	}




	//************************************************************

	CClassify::CClassify()
	{
		Reset();
	}

	void CClassify::Reset()
	{
		m_stat.clear();
		m_values.clear();
	}


	void CClassify::Add(double x, double y)
	{
		m_values.push_back(CXY<double, double>(x, y));
	}

	void CClassify::ClassifyManual(vector<double> classLimit)
	{
		_ASSERTE(classLimit.size() >= 2);
		m_stat.clear();
		m_stat.resize(classLimit.size() - 1);

		if (m_values.size() > 0)
		{
			std::sort(m_values.begin(), m_values.end());

			double first = m_values.begin()->m_x;
			double last = m_values.rbegin()->m_x;
			if (classLimit[0] == -1)
				classLimit[0] = first;

			if (classLimit[classLimit.size() - 1] == -1)
				classLimit[classLimit.size() - 1] = last;

			int j = 0;
			for (int i = 0; i<int(classLimit.size() - 1); i++)
			{
				while (j < (int)m_values.size() &&
					m_values[j].m_x >= classLimit[i] &&
					m_values[j].m_x < classLimit[i + 1])
				{
					m_stat[i].m_x += m_values[j].m_x;
					m_stat[i].m_y += m_values[j].m_y;
					j++;
				}
			}
		}
	}


	void CClassify::ClassifyEqualInterval(short nbClass)
	{
		m_stat.clear();
		m_stat.resize(nbClass);

		if (m_values.size() > 0)
		{
			std::sort(m_values.begin(), m_values.end());

			double first = m_values.begin()->m_x;
			double last = m_values.rbegin()->m_x;
			double classSize = (double)(last - first) / nbClass;
			int j = 0;
			for (int i = 0; i < nbClass; i++)
			{
				double limit = first + (i + 1)*classSize;

				while (j < (int)m_values.size() &&
					m_values[j].m_x <= limit)
				{
					m_stat[i].m_x += m_values[j].m_x;
					m_stat[i].m_y += m_values[j].m_y;
					j++;
				}
			}
		}
	}

	void CClassify::ClassifyNaturalBreak(short nbClass)
	{
		m_stat.clear();
		m_stat.resize(nbClass);


		std::sort(m_values.begin(), m_values.end());

		double classSize = (double)m_values.size() / nbClass;
		for (int i = 0; i < nbClass; i++)
		{
			int f = int(i*classSize);
			int l = int((i + 1)*classSize);
			_ASSERTE(f >= 0 && f <= (int)m_values.size());
			_ASSERTE(l >= 0 && l <= (int)m_values.size());

			for (int j = f; j < l; j++)
			{
				m_stat[i].m_x += m_values[j].m_x;
				m_stat[i].m_y += m_values[j].m_y;
			}
		}
	}

}//namespace WBSF