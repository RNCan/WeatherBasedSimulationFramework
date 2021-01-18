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


	const char* CTerm::TERMS_NAME[3 * 5] = { "Lon", "Lat", "Elev", "Expo", "Shore", "Lon²", "Lat²", "Elev²", "Expo²", "Shore²", "Lon³", "Lat³", "Elev³", "Expo³", "Shore³" };

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
		std::bitset<3 * 5> v;
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


	//i-Starting with no variables in the model, we fit a separate model including each variable one by one.
	//The variable(assuming there is one) with the most statistically significant P - value below a pre - defined inclusion threshold is selected.
	//ii-We fit separate models including the variable selected at stage 1 and adding each remaining variable one by one.
	//iii-Of the remaining variables, the variable(assuming there is one) with the most statistically significant P - value below the inclusion threshold is selected.
	//iiii-If a variable was added to the model at step(ii), all previously selected variables are checked to see if they still reach the inclusion threshold, and are dropped one by one if not, starting with the least significant.
	//Steps(ii) and (iii)are repeated until none of the remaining variables have a P - value below the inclusion threshold when added to the model including the previously selected variables.
	ERMsg CSpatialRegression::StepWise(const CGridPointVector& calibPts, std::vector<int>& regressionTerm, double criticalR2, CCallback& callback)const
	{
		ERMsg msg;

		bool bUseLatLon = m_param.m_bUseLatLon;
		bool bUseElev = m_param.m_bUseElevation;
		bool bUseExpo = m_param.m_bUseExposition && m_pPts->HaveExposition();
		bool bUseShore = m_param.m_bUseShore;

		callback.PushTask("StepWise regression", NOT_INIT);


		CGeoRegression regression;
		// fits best of MAX_PREDICTORS-predictor multiple 
		// regression model and returns the R2.

		double global_bestR² = -999;

		//add new terms to model until the r² improvement is < threshold

		static const int Term[2][10] =
		{
			{CTerm::LAT, CTerm::ELEV,CTerm::EXPO,CTerm::SHORE, CTerm::LON},
			{CTerm::LAT | CTerm::ELEV,CTerm::LAT | CTerm::EXPO,CTerm::LAT | CTerm::SHORE,
			CTerm::ELEV | CTerm::EXPO,CTerm::ELEV | CTerm::SHORE,
			CTerm::EXPO | CTerm::SHORE,
			CTerm::LON | CTerm::LAT, CTerm::LON | CTerm::ELEV,CTerm::LON | CTerm::EXPO,CTerm::LON | CTerm::SHORE }
		};

		for (int i = 0; i < 3 && msg; i++)
		{
			int last_j = i == 0 ? 5 : i == 1 ? 10 : int(CTerm::GetNbTerms());

			bool bContinueAdd = true;
			while (bContinueAdd&&msg)
			{
				
				double best_R² = -999;
				int best_j = -1;
				//find the next term
#pragma omp parallel for num_threads( m_info.m_nbCPU ) if (m_info.m_bMulti)
				for (int jj = 0; jj < last_j; jj++)
				{
#pragma omp flush(msg)
					if (msg)
					{
						int j = int(i < 2 ? Term[i][jj] : jj);
						if (CTerm::IsValid(j, bUseLatLon, bUseElev, bUseExpo, bUseShore))
						{
							//Verify that term j is not in the model
							bool alreadyIn = std::find(regression.begin(), regression.end(), CTerm(j)) != regression.end();

							//if the term isn't in the model, test the improvement achieved by adding it.
							if (!alreadyIn)
							{
								//add thid term
								CGeoRegression regression2 = regression;
								regression2.push_back(CTerm(j));

								double R² = regression2.Compute(calibPts, m_prePostTransfo);
#pragma omp critical(SPA_REG_SET)
								{
									if (R² > best_R²)
									{
										best_R² = R²;
										best_j = j;
									}
									msg += callback.StepIt(0);
#pragma omp flush(msg)
								}
								//
								//remove this term

							}//if not already in
						}
					}//msg
				} //for all term

				if (best_j != -1 && best_R² - global_bestR² >= criticalR2)
				{
					global_bestR² = best_R²;
					regression.push_back(CTerm(best_j));


					//remove non-relevent term
					bool bContinueRemove = true;
					while (bContinueRemove&&msg)
					{
						double less_imp_R² = 999;
						int less_imp_j = -1;
#pragma omp parallel for num_threads( m_info.m_nbCPU ) if (m_info.m_bMulti)
						for (int j = 0; j < regression.size() - 1; j++)
						{

							CGeoRegression regression2 = regression;
							regression2.erase(regression2.begin() + j);

							double R² = regression2.Compute(calibPts, m_prePostTransfo);
#pragma omp critical(SPA_REG_SET)
							{
								if (R² > less_imp_R²)
								{
									less_imp_R² = R²;
									less_imp_j = j;
								}
								msg += callback.StepIt(0);
#pragma omp flush(msg)
							}
						} //for all term

						if (less_imp_j != -1 && global_bestR² - less_imp_R² < criticalR2)
						{
							CGeoRegression::iterator it = std::find(regression.begin(), regression.end(), CTerm(best_j));
							ASSERT(it != regression.end());

							regression.erase(it);
							global_bestR² = less_imp_R²;
						}
						else
						{
							bContinueRemove = false;
						}


					}
				}
				else
				{
					bContinueAdd = false;
				}
				msg += callback.StepIt(0);
			}
		}
		//transfer parameters
		regressionTerm.resize(regression.size());
		for (size_t i = 0; i < regression.size(); i++)
			regressionTerm[i] = regression[i].m_v;


		callback.PopTask();
		//return global_bestR²;
		return msg;
	}

	ERMsg CSpatialRegression::Initialization(CCallback& callback)
	{
		//		ASSERT(!m_bInit);

		ERMsg msg;

		msg = CGridInterpolBase::Initialization(callback);
		if (msg)
		{
			CGridPointVectorPtr pCalibPts = GetCalibrationPts();
			ASSERT(pCalibPts->size() >= 30);

			//Compute best regression
			msg = StepWise(*pCalibPts, m_param.m_regressionModel, m_param.m_regressCriticalR2, callback);
			if (msg)
			{
				m_regression.resize(m_param.m_regressionModel.size());
				for (size_t i = 0; i < m_param.m_regressionModel.size(); i++)
					m_regression[i] = m_param.m_regressionModel[i];

				//Compute with the real parameters
				m_regression.Compute(*pCalibPts, m_prePostTransfo);
				//			m_bInit = true;
				if (m_regression.empty())
					msg.ajoute("Unable to create spatial regression");
			}
		}


		return msg;
	}


	double CSpatialRegression::Evaluate(const CGridPoint& pt, int iXval)const
	{
		/*if (iXval >= 0 && m_param.m_XvalPoints > 0)
		{
			int l = (int)ceil((iXval) / m_inc);
			if (int(l*m_inc) == iXval)
				return m_param.m_noData;
		}*/

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