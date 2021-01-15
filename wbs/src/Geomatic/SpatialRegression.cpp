//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 30-08-2018	Rémi Saint-Amant	Add shore as term
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include "Geomatic/SpatialRegression.h"
#include "newmat/Regression.h"
#include "Basic/UtilStd.h"
#include <bitset>

namespace WBSF
{

	//**********************************************************************


	const char* CTerm::TERMS_NAME[3*5] = { "Lon", "Lat", "Elev", "Expo", "Shore", "Lon²", "Lat²", "Elev²", "Expo²", "Shore²", "Lon³", "Lat³", "Elev³", "Expo³", "Shore³" };

	int CTerm::GetNbItem()const
	{
		int n = 0;
		for (size_t i = 0; i < NB_TERMS; i++)
			if (m_v & (1 << i))
				n++;

		return n;
	}

	//Get estimator
	double CTerm::GetE(double x, double y, double elev, double expo, double shore)const
	{
		ASSERT(IsInit());

		if (!IsInit())
			return 0;


		double E = 1;
		const double var[NB_TERMS] = { x, y, elev, expo, shore, x*x, y*y, elev*elev, expo*expo, shore*shore };

		for (size_t i = 0; i < NB_TERMS; i++)
		{
			if (m_v & (1 << i))
				E *= var[i];
		}

		return E;
	}

	string CTerm::GetName()const
	{
		string str;

		//replace x*x² by x³
		std::bitset<3*5> v;
		for (size_t i = 0; i < 5; i++)
		{
			bool bCube = (m_v & (1 << i)) && (m_v & (1 << (i + 5)));
			if (bCube)
			{
				v.set(10 + i, 1);
			}
			else
			{
				v.set(0 + i, m_v & (1 << i));
				v.set(5 + i, m_v & (1 << (i + 5)));
			}
				
		}


		for (size_t i = 0; i < v.size(); i++)
		{
			if (v.test(i))
			{
				if (!str.empty())
					str += "*";

				str += GetName(i);
			}
		}
/*
		for (size_t i = 0; i < NB_TERMS; i++)
		{
			if (m_v & (1 << i))
			{
				if (!str.empty())
					str += "*";

				str += GetName(i);
			}
		}*/

		return str;
	}


	//**********************************************************************

	//evaluate result from regression
	double CGeoRegression::GetF(double x, double y, double elev, double expo, double shore)const
	{
		ASSERT(size() == m_param.size());

		double F = 0;
		if (!empty())
		{
			F = m_intercept;
			for (size_t i = 0; i < size(); i++)
			{
				F += at(i).GetE(x, y, elev, expo, shore)*m_param[i];
			}
		}

		return F;
	}

	//compute regression parameters
	double CGeoRegression::Compute(const CGridPointVector& pts, const CPrePostTransfo& transfo)
	{
		m_param.clear();

		if (!empty())
		{
			try
			{
				NEWMAT::Matrix X((int)pts.size(), (int)size());
				NEWMAT::ColumnVector Y((int)pts.size());

				for (int i = 0; i < pts.size(); i++)
				{
					for (int j = 0; j < size(); j++)
					{
						X[i][j] = at(j).GetE(pts[i]);
					}

					Y[i] = transfo.Transform(pts[i].m_event);
				}

				NEWMAT::ColumnVector result;
				NEWMAT::ColumnVector Fitted;
				m_R² = DoRegression(X, Y, result, Fitted);

				m_intercept = result[0];//intercept

				for (int i = 1; i < result.size(); i++)
					m_param.push_back(result[i]);



			}
			catch (...)
			{
				m_R² = -999;
				m_intercept = 0;
				m_param.resize(size(), 0);
			}

			ASSERT(m_param.size() == size());

		}

		return m_R²;
	}

	string CGeoRegression::GetEquation()const
	{
		ASSERT(size() == m_param.size());

		string str = "----";

		if (size() > 0)
		{
			str = ToString(m_intercept, 8);
			for (size_t i = 0; i < m_param.size(); i++)
			{
				str += (m_param[i] > 0) ? " + " : " - ";
				str += ToString(abs(m_param[i]), 8) + "*" + at(i).GetName();
			}
		}

		return str;
	}

	string CGeoRegression::GetTerms()const
	{
		string str;
		//out put only term
		for (size_t i = 0; i < size(); i++)
		{
			if (i > 0)str += ", ";
			str += at(i).GetName();
		}

		return str;
	}


	//*************************************************************************
	//const int CSpatialRegression::TERMS[NB_TERMS_CATEGORY][NB_TERM_MAX] =
	//{
	//	//NO_EXPO_NO_SHORE
	//	{
	//		CTerm::LAT, CTerm::LON, CTerm::ELEV,
	//		CTerm::LAT², CTerm::LON², CTerm::ELEV²,
	//		CTerm::LAT | CTerm::LON, CTerm::ELEV | CTerm::LON, CTerm::LAT | CTerm::ELEV, CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LON²,
	//		CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LON, CTerm::ELEV | CTerm::LAT², CTerm::ELEV² | CTerm::LAT,
	//		CTerm::ELEV | CTerm::LAT | CTerm::LON, CTerm::LAT² | CTerm::LON², CTerm::ELEV² | CTerm::LON², CTerm::ELEV² | CTerm::LAT², CTerm::ELEV | CTerm::LAT | CTerm::LON²,
	//		CTerm::ELEV | CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LAT | CTerm::LON, CTerm::ELEV | CTerm::LAT² | CTerm::LON², CTerm::ELEV² | CTerm::LAT | CTerm::LON²,
	//		CTerm::ELEV² | CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LAT² | CTerm::LON²
	//	},
	//	//EXPO_NO_SHORE
	//	{
	//		CTerm::LAT, CTerm::LON, CTerm::ELEV, CTerm::EXPO,
	//		CTerm::LAT², CTerm::LON², CTerm::ELEV², CTerm::EXPO²,
	//		CTerm::LAT | CTerm::LON, CTerm::ELEV | CTerm::LON, CTerm::LAT | CTerm::ELEV, CTerm::LAT | CTerm::EXPO, CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LON²,
	//		CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LON, CTerm::ELEV | CTerm::LAT², CTerm::ELEV² | CTerm::LAT, CTerm::EXPO² | CTerm::LAT,
	//		CTerm::ELEV | CTerm::LAT | CTerm::LON, CTerm::LAT² | CTerm::LON², CTerm::ELEV² | CTerm::LON², CTerm::ELEV² | CTerm::LAT²,
	//		CTerm::ELEV | CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LAT | CTerm::LON
	//	},

	//	//NO_EXPO_SHORE
	//	{
	//		CTerm::LAT, CTerm::LON, CTerm::ELEV, CTerm::SHORE,
	//		CTerm::LAT², CTerm::LON², CTerm::ELEV², CTerm::SHORE²,
	//		CTerm::LAT | CTerm::LON, CTerm::LAT | CTerm::ELEV, CTerm::LAT | CTerm::SHORE,
	//		CTerm::LON | CTerm::ELEV, CTerm::LON | CTerm::SHORE, 
	//		CTerm::ELEV| CTerm::SHORE,
	//		CTerm::LAT | CTerm::LON², CTerm::LAT | CTerm::ELEV², CTerm::LAT | CTerm::SHORE²,
	//		CTerm::LON | CTerm::ELEV², CTerm::LON | CTerm::SHORE²,
	//		CTerm::ELEV | CTerm::SHORE²,
	//		CTerm::LAT² | CTerm::LON, CTerm::LAT² | CTerm::ELEV, CTerm::LAT² | CTerm::SHORE,
	//		CTerm::LON² | CTerm::ELEV, CTerm::LON² | CTerm::SHORE,
	//		CTerm::ELEV² | CTerm::SHORE,
	//		
	//		CTerm::ELEV | CTerm::LAT | CTerm::LON, CTerm::LAT² | CTerm::LON², CTerm::ELEV² | CTerm::LON², CTerm::ELEV² | CTerm::LAT²,
	//		CTerm::ELEV | CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LAT | CTerm::LON
	//	},

	//	//EXPO_SHORE
	//	{
	//		CTerm::LAT, CTerm::LON, CTerm::ELEV, CTerm::EXPO, CTerm::SHORE,
	//		CTerm::LAT², CTerm::LON², CTerm::ELEV², CTerm::EXPO², CTerm::SHORE²,
	//		CTerm::LAT | CTerm::LON, CTerm::ELEV | CTerm::LON, CTerm::LAT | CTerm::ELEV, CTerm::LAT | CTerm::EXPO, CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LON²,
	//		CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LON, CTerm::ELEV | CTerm::LAT², CTerm::ELEV² | CTerm::LAT, CTerm::EXPO² | CTerm::LAT,
	//		CTerm::ELEV | CTerm::LAT | CTerm::LON, CTerm::LAT² | CTerm::LON², CTerm::ELEV² | CTerm::LON², CTerm::ELEV² | CTerm::LAT²,
	//		CTerm::ELEV | CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LAT | CTerm::LON
	//	},

	//};
	//	const int CSpatialRegression::TERM_WITH_EXPO[NB_TERM] =
	//{
	//	CTerm::LAT, CTerm::LON, CTerm::ELEV, CTerm::EXPO,
	//	CTerm::LAT², CTerm::LON², CTerm::ELEV², CTerm::EXPO²,
	//	CTerm::LAT | CTerm::LON, CTerm::ELEV | CTerm::LON, CTerm::LAT | CTerm::ELEV, CTerm::LAT | CTerm::EXPO, CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LON²,
	//	CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LON, CTerm::ELEV | CTerm::LAT², CTerm::ELEV² | CTerm::LAT, CTerm::EXPO² | CTerm::LAT,
	//	CTerm::ELEV | CTerm::LAT | CTerm::LON, CTerm::LAT² | CTerm::LON², CTerm::ELEV² | CTerm::LON², CTerm::ELEV² | CTerm::LAT²,
	//	CTerm::ELEV | CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LAT | CTerm::LON,
	//};


	//const int CSpatialRegression::TERM_WITHOUT_EXPO[] =
	//{
	//	CTerm::LAT, CTerm::LON, CTerm::ELEV,
	//	CTerm::LAT², CTerm::LON², CTerm::ELEV²,
	//	CTerm::LAT | CTerm::LON, CTerm::ELEV | CTerm::LON, CTerm::LAT | CTerm::ELEV, CTerm::LAT | CTerm::LON², CTerm::ELEV | CTerm::LON²,
	//	CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LON, CTerm::ELEV | CTerm::LAT², CTerm::ELEV² | CTerm::LAT,
	//	CTerm::ELEV | CTerm::LAT | CTerm::LON, CTerm::LAT² | CTerm::LON², CTerm::ELEV² | CTerm::LON², CTerm::ELEV² | CTerm::LAT², CTerm::ELEV | CTerm::LAT | CTerm::LON²,
	//	CTerm::ELEV | CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LAT | CTerm::LON, CTerm::ELEV | CTerm::LAT² | CTerm::LON², CTerm::ELEV² | CTerm::LAT | CTerm::LON²,
	//	CTerm::ELEV² | CTerm::LAT² | CTerm::LON, CTerm::ELEV² | CTerm::LAT² | CTerm::LON²,
	//};

	//size_t CSpatialRegression::GetNbTerms(bool bExpo, bool bShore)
	//{
	//	//size_t nb_vars = 2*(3 + (bExpo?1:0) + (bShore?1:0));

	//	//return 1024;// pow(2, (nb_vars + 1));
	//	return 1 << CTerm::NB_TERMS;
	//}
	//int GetTerms(int i, bool bExpo, bool bShore)const;/// { ASSERT(i >= 0 && i < NB_TERM_MAX); return bExpo ? TERM_WITH_EXPO[i] : TERM_WITHOUT_EXPO[i]; }

	//int CSpatialRegression::GetTerms(size_t i, bool bExpo, bool bShore)
	//{
	//	//size_t nb_vars = (3 + (bExpo ? 1 : 0) + (bShore ? 1 : 0))*CTerm::NB_POWERS;

	//	int v=0;
	//	//if (i < nb_vars)
	//		//v = i << 1;
	//	for(size_t ii=0; ii<1024&&i<1024; ii++)
	//}

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CSpatialRegression::CSpatialRegression()
	{
		Reset();
	}

	void CSpatialRegression::Reset()
	{
		m_regression.Reset();
	}

	CSpatialRegression::~CSpatialRegression()
	{}



	//**************************************************************************
	// permet de trouver les termes de régressions qui seront utilisé dans les
	// équations 
	//**************************************************************************
	double CSpatialRegression::StepWise(std::vector<int>& regressionTerm, double criticalR2)const
	{
		bool bUseElev = m_param.m_bUseElevation;
		bool bUseExpo = m_param.m_bUseExposition && m_pPts->HaveExposition();
		bool bUseShore = m_param.m_bUseShore;


		CGeoRegression regression;
		// fits best of MAX_PREDICTORS-predictor multiple 
		// regression model and returns the R2.

		double bestR² = -999;

		//add new terms to model until the r² improvement is < threshold
		bool bContinueLoop1 = true;
		size_t nbTerms = CTerm::GetNbTerms();
		for (int i = 0; i < nbTerms&&bContinueLoop1; i++)
		{
			if (CTerm::IsValid(i, bUseElev, bUseExpo, bUseShore))
			{
				bool bContinueLoop2 = true;
				//find the next term
				for (int j = 0; j < nbTerms&&bContinueLoop2; j++)
				{
					if (CTerm::IsValid(j, bUseElev, bUseExpo, bUseShore))
					{
						//Verify that term i is not in the model
						//bool alreadyIn = std::find(regression.begin(), regression.end(), GetTerm(j, bExpo)) != regression.end();
						bool alreadyIn = std::find(regression.begin(), regression.end(), CTerm(j)) != regression.end();

						//if the term isn't in the model, test the improvement achieved by adding it.
						if (!alreadyIn)
						{
							regression.push_back(CTerm(j));
							double R² = regression.Compute(*m_pPts, m_prePostTransfo);
							if (R² > -999)
							{
								if (R² - bestR² > criticalR2)
								{
									bestR² = R²;
									bContinueLoop2 = false;
								}
								else
								{
									//discard this term
									regression.pop_back();
								}
							}
						}//if not already in
					}
				} //for all term

				bContinueLoop1 = !bContinueLoop2;
			}
		}

		//transfer parameters
		regressionTerm.resize(regression.size());
		for (size_t i = 0; i < regression.size(); i++)
			regressionTerm[i] = regression[i].m_v;

		return bestR²;
	}

	ERMsg CSpatialRegression::Initialization(CCallback& callback)
	{
		ERMsg msg = CGridInterpolBase::Initialization(callback);

		if (!m_bInit)
		{
			//Compute best regression
			StepWise(m_param.m_regressionModel, m_param.m_regressCriticalR2);

			m_regression.resize(m_param.m_regressionModel.size());
			for (size_t i = 0; i < m_param.m_regressionModel.size(); i++)
				m_regression[i] = m_param.m_regressionModel[i];

			//Compute with the real parameters
			m_regression.Compute(*m_pPts, m_prePostTransfo);
			m_bInit = true;

		}


		if (m_regression.empty())
		{
			msg.ajoute("Unable to create spatial regression");
		}

		return msg;
	}


	double CSpatialRegression::Evaluate(const CGridPoint& pt, int iXval)const
	{
		if (iXval >= 0 && m_param.m_XvalPoints > 0)
		{
			int l = (int)ceil((iXval) / m_inc);
			if (int(l*m_inc) == iXval)
				return m_param.m_noData;
		}

		double value = m_prePostTransfo.InvertTransform(m_regression.GetF(pt), m_param.m_noData);

		if ( /*iXval<0 &&*/ m_param.m_bGlobalLimit && value > m_param.m_noData)
		{
			bool bOutside = value<m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV] || value>m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV];
			if (bOutside)
			{
				if (m_param.m_bGlobalLimitToBound)
					value = min(m_stat[HIGHEST] + m_param.m_globalLimitSD*m_stat[STD_DEV], max(m_stat[LOWEST] - m_param.m_globalLimitSD*m_stat[STD_DEV], value));
				else
					value = m_param.m_noData;
			}
		}

		if ( /*iXval<0 &&*/ m_param.m_bGlobalMinMaxLimit && value > m_param.m_noData)
		{
			bool bOutside = value<m_param.m_globalMinLimit || value>m_param.m_globalMaxLimit;
			if (bOutside)
			{
				if (m_param.m_bGlobalMinMaxLimitToBound)
					value = min(m_param.m_globalMaxLimit, max(m_param.m_globalMinLimit, value));
				else
					value = m_param.m_noData;
			}
		}

		return value;
	}

	string CSpatialRegression::GetFeedbackBestParam()const
	{
		string str = string(CGridInterpolParam::GetMemberName(CGridInterpolParam::REGRESSION_MODEL)) + " = " + m_regression.GetEquation();

		//str.Format( "%s = %s\n", 
		//	CGridInterpolParam::GetMemberName(CGridInterpolParam::REGRESSION_MODEL),
		//	//m_param.GetMember(CGridInterpolParam::REGRESSION_MODEL),
		//	(LPCTSTR)m_regression.GetEquation()
		//	);

		return str;
	};

	//double CSpatialRegression::GetOptimizedR²()const
	//{
	//	//Don't call parent because we don't want to do Xvalidation
	//	return 1;
	//}

	void CSpatialRegression::GetParamterset(CGridInterpolParamVector& parameterset)
	{
		parameterset.clear();

		//if (m_param.m_regressionModel.empty())
		//{
		//	//this is to call GetOptimiseR² and initialise term with StepWise
		//	parameterset.resize(1, m_param);

		//	//Compute best regression
		//	StepWise(parameterset[0].m_regressionModel);
		//}
	}



}