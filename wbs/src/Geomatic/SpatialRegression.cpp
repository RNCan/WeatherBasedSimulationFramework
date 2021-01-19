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



	std::vector<int> CTerm::GetOrder(size_t level)
	{
		ASSERT(level < NB_LEVEL);

		static const int ORDER1[5] = { LAT, ELEV, EXPO, SHORE, LON };
		static const int ORDER2[5] = { LAT², ELEV², EXPO², SHORE², LON² };


		std::vector<int> order;

		if (level >= SIMPLE_FIRST_ORDER)
		{
			//add term
			for (int i = 0; i < 5; i++)
				order.push_back(ORDER1[i]);
		}

		if (level >= MULTI_FIRST_ORDER)
		{
			//add multiplication
			for (int i = 0; i < 5; i++)
				for (int j = i + 1; j < 5; j++)
					order.push_back(ORDER1[i] | ORDER1[j]);
		}

		if (level >= SIMPLE_SECOND_ORDER)
		{
			//add square
			for (int j = 0; j < 5; j++)
				order.push_back(ORDER2[j]);
		}

		if (level >= MULTI_SECOND_ORDER1)
		{
			//add multiplication square
			for (int i = 0; i < 5; i++)
				for (int j = i + 1; j < 5; j++)
					order.push_back(ORDER1[i] | ORDER2[j]);
		}

		if (level >= MULTI_SECOND_ORDER2)
		{
			//add multiplication square*square
			for (int i = 0; i < 5; i++)
				for (int j = i + 1; j < 5; j++)
					order.push_back(ORDER2[i] | ORDER2[j]);
		}

		if (level >= SIMPLE_THIRTH_ORDER)
		{
			//add cube
			for (int i = 0; i < 5; i++)
				order.push_back(ORDER1[i] | ORDER2[i]);
		}

		if (level >= ALL_ORDER)
		{
			//add all others
			for (int i = 0; i < GetNbTerms(); i++)
				if (find(order.begin(), order.end(), i) == order.end())
					order.push_back(i);
		}


		return order;
	}


	size_t CTerm::GetNbItem()const
	{
		size_t n = 0;
		for (int i = 0; i < NB_TERMS; i++)
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
		const double var[NB_TERMS] = { x , y , elev, expo, shore, x*x, y*y, elev*elev, expo*expo, shore*shore };
		//const double var[NB_TERMS] = { x/1000.0, y / 1000.0, elev, expo, shore/1000.0, x*x / 1000000.0, y*y / 1000000.0, elev*elev, expo*expo, shore*shore/1000000.0 };
//		const double var[NB_TERMS] = { Signe(x)*sqrt(abs(x)), Signe(y)*sqrt(abs(y)), Signe(elev)*sqrt(abs(elev)), Signe(expo)*sqrt(abs(expo)), Signe(shore)*sqrt(abs(shore)), x, y, elev, expo, shore , pow(x,1.5), pow(y,1.5), pow(elev,1.5), pow(expo,1.5), pow(shore,1.5)};

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
	ERMsg CSpatialRegression::StraightForeward(const CGridPointVector& calibPts, std::vector<int>& regressionTerm, double criticalR2, size_t maxLevel, CCallback& callback)const
	{
		ERMsg msg;

		bool bUseLatLon = m_param.m_bUseLatLon;
		bool bUseElev = m_param.m_bUseElevation;
		bool bUseExpo = m_param.m_bUseExposition && m_pPts->HaveExposition();
		bool bUseShore = m_param.m_bUseShore;

		callback.PushTask("Straight foreward optimization", NOT_INIT);

		CGeoRegression regression;

		double bestR² = -FLT_MAX;

		vector<int> order = CTerm::GetOrder(maxLevel);
		//add new terms to model until the r² improvement is < threshold
		bool bContinueLoop1 = true;
		while (bContinueLoop1&&msg)
		{
			bool bContinueLoop2 = true;
			//find the next term
			for (size_t j = 0; j < order.size() && bContinueLoop2&&msg; j++)
			{
				if (CTerm::IsValid(order[j], bUseLatLon, bUseElev, bUseExpo, bUseShore))
				{
					//Verify that term i is not in the model
					bool alreadyIn = std::find(regression.begin(), regression.end(), CTerm(order[j])) != regression.end();

					//if the term isn't in the model, test the improvement achieved by adding it.
					if (!alreadyIn)
					{
						CGeoRegression regression2 = regression;
						regression2.push_back(CTerm(order[j]));

						double R² = regression2.Compute(calibPts, m_prePostTransfo);
						double adjR² = 1 - ((1 - R²)*(calibPts.size() - 1)) / (calibPts.size() - regression2.size() - 1);
						
						if (adjR² - bestR² > criticalR2)
						{
							//add the term and restart from beginning
							regression.push_back(CTerm(order[j]));
							bestR² = adjR²;
							bContinueLoop2 = false;
						}
					}//if not already in
				}

				msg += callback.StepIt(0);
			} //for all term

			bContinueLoop1 = !bContinueLoop2;
		}

		//transfer parameters
		regressionTerm = regression.GetOrder();


		callback.PopTask();

		return msg;
	}

	ERMsg CSpatialRegression::Foreward(const CGridPointVector& calibPts, std::vector<int>& regressionTerm, double criticalR2, size_t maxLevel, CCallback& callback)const
	{
		ERMsg msg;

		bool bUseLatLon = m_param.m_bUseLatLon;
		bool bUseElev = m_param.m_bUseElevation;
		bool bUseExpo = m_param.m_bUseExposition && m_pPts->HaveExposition();
		bool bUseShore = m_param.m_bUseShore;

		callback.PushTask("Foreward optimization", NOT_INIT);


		CGeoRegression regression;


		double global_bestR² = -FLT_MAX;

		for (size_t level = 0; level <= maxLevel; level++)
		{
			vector<int> order = CTerm::GetOrder(level);

			bool bContinueAdd = true;
			while (bContinueAdd&&msg)
			{

				double best_R² = -FLT_MAX;
				int best_j = -1;
				//find the next term
#pragma omp parallel for num_threads( m_info.m_nbCPU ) if (m_info.m_bMulti)
				for (int i = 0; i < int(order.size()); i++)
				{
#pragma omp flush(msg)
					if (msg)
					{
						if (CTerm::IsValid(order[i], bUseLatLon, bUseElev, bUseExpo, bUseShore))
						{
							//Verify that term j is not in the model
							bool alreadyIn = std::find(regression.begin(), regression.end(), CTerm(order[i])) != regression.end();

							//if the term isn't in the model, test the improvement achieved by adding it.
							if (!alreadyIn)
							{
								//add thid term
								CGeoRegression regression2 = regression;
								regression2.push_back(CTerm(order[i]));

								double R² = regression2.Compute(calibPts, m_prePostTransfo);
								double adjR² = 1 - ((1 - R²)*(calibPts.size() - 1)) / (calibPts.size() - regression2.size() - 1);
#pragma omp critical(SPA_REG_SET)
								{
									if (adjR² > best_R²)
									{
										best_R² = adjR²;
										best_j = order[i];
									}
									msg += callback.StepIt(0);
#pragma omp flush(msg)
								}
							}//if not already in
						}
					}//msg
				} //for all term

				if (best_j != -1 && best_R² - global_bestR² > criticalR2)
				{
					global_bestR² = best_R²;
					regression.push_back(CTerm(best_j));
				}
				else
				{
					bContinueAdd = false;
				}
				msg += callback.StepIt(0);
			}
		}

		//transfer parameters
		regressionTerm = regression.GetOrder();


		callback.PopTask();


		return msg;
	}

	ERMsg CSpatialRegression::Backward(const CGridPointVector& calibPts, std::vector<int>& regressionTerm, double criticalR2, size_t maxLevel, CCallback& callback)const
	{
		ERMsg msg;


		bool bUseLatLon = m_param.m_bUseLatLon;
		bool bUseElev = m_param.m_bUseElevation;
		bool bUseExpo = m_param.m_bUseExposition && m_pPts->HaveExposition();
		bool bUseShore = m_param.m_bUseShore;

		callback.PushTask("Backward optimization", NOT_INIT);

		//start with all terms that have positive R²
		msg = Foreward(calibPts, regressionTerm, criticalR2/10.0, maxLevel, callback);
		if (!msg)
			return msg;


		CGeoRegression regression = regressionTerm;

		double R² = regression.Compute(calibPts, m_prePostTransfo);
		double adjR² = 1 - ((1 - R²)*(calibPts.size() - 1)) / (calibPts.size() - regression.size() - 1);
		double global_bestR² = adjR²;


		//remove non-relevent term
		bool bContinueRemove = true;
		while (bContinueRemove&&msg)
		{
			double less_imp_R² = -FLT_MAX;
			int less_imp_j = -1;
#pragma omp parallel for num_threads( m_info.m_nbCPU ) if (m_info.m_bMulti)
			for (int j = 0; j < regression.size(); j++)
			{
#pragma omp flush(msg)
				if (msg)
				{
					CGeoRegression regression2 = regression;
					regression2.erase(regression2.begin() + j);

					double R² = regression2.Compute(calibPts, m_prePostTransfo);
					double adjR² = 1 - ((1 - R²)*(calibPts.size() - 1)) / (calibPts.size() - regression2.size() - 1);
#pragma omp critical(SPA_REG_SET)
					{
						if (adjR² > less_imp_R²)
						{
							less_imp_R² = adjR²;
							less_imp_j = j;
						}
						msg += callback.StepIt(0);
#pragma omp flush(msg)
					}
				}//if msg
			} //for all term

			if (less_imp_j != -1 && global_bestR² - less_imp_R² < criticalR2)
			{
				regression.erase(regression.begin() + less_imp_j);
				global_bestR² = less_imp_R²;
			}
			else
			{
				bContinueRemove = false;
			}
		}

		//transfer parameters
		regressionTerm = regression.GetOrder();


		callback.PopTask();

		return msg;
	}

	//i-Starting with no variables in the model, we fit a separate model including each variable one by one.
//The variable(assuming there is one) with the most statistically significant P - value below a pre - defined inclusion threshold is selected.
//ii-We fit separate models including the variable selected at stage 1 and adding each remaining variable one by one.
//Of the remaining variables, the variable(assuming there is one) with the most statistically significant P - value below the inclusion threshold is selected.
//iii-If a variable was added to the model at step(ii), all previously selected variables are checked to see if they still reach the inclusion threshold, and are dropped one by one if not, starting with the least significant.
//iiii-Steps(ii) and (iii)are repeated until none of the remaining variables have a P - value below the inclusion threshold when added to the model including the previously selected variables.
	ERMsg CSpatialRegression::StepWise(const CGridPointVector& calibPts, std::vector<int>& regressionTerm, double criticalR2, size_t maxLevel, CCallback& callback)const
	{
		ERMsg msg;

		bool bUseLatLon = m_param.m_bUseLatLon;
		bool bUseElev = m_param.m_bUseElevation;
		bool bUseExpo = m_param.m_bUseExposition && m_pPts->HaveExposition();
		bool bUseShore = m_param.m_bUseShore;

		callback.PushTask("StepWise optimization", NOT_INIT);

		CGeoRegression regression;

		double global_bestR² = -FLT_MAX;

		for (size_t level = 0; level <= maxLevel; level++)
		{
			vector<int> order = CTerm::GetOrder(level);

			bool bContinueAdd = true;
			while (bContinueAdd&&msg)
			{

				double best_R² = -FLT_MAX;
				int best_j = -1;
				//find the next term
#pragma omp parallel for num_threads( m_info.m_nbCPU ) if (m_info.m_bMulti)
				for (int i = 0; i < int(order.size()); i++)
				{
#pragma omp flush(msg)
					if (msg)
					{
						if (CTerm::IsValid(order[i], bUseLatLon, bUseElev, bUseExpo, bUseShore))
						{
							//Verify that term j is not in the model
							bool alreadyIn = std::find(regression.begin(), regression.end(), CTerm(order[i])) != regression.end();

							//if the term isn't in the model, test the improvement achieved by adding it.
							if (!alreadyIn)
							{
								//add thid term
								CGeoRegression regression2 = regression;
								regression2.push_back(CTerm(order[i]));

								double R² = regression2.Compute(calibPts, m_prePostTransfo);
								double adjR² = 1 - ((1 - R²)*(calibPts.size() - 1)) / (calibPts.size() - regression2.size() - 1);
#pragma omp critical(SPA_REG_SET)
								{
									if (adjR² > best_R²)
									{
										best_R² = adjR²;
										best_j = order[i];
									}
									msg += callback.StepIt(0);
#pragma omp flush(msg)
								}
							}//if not already in
						}
					}//msg
				} //for all term

				if (best_j != -1 && best_R² - global_bestR² > criticalR2)
				{
					global_bestR² = best_R²;
					regression.push_back(CTerm(best_j));

					//remove non-relevent term
					bool bContinueRemove = true;
					while (bContinueRemove&&msg)
					{
						double less_imp_R² = -FLT_MAX;
						int less_imp_j = -1;
#pragma omp parallel for num_threads( m_info.m_nbCPU ) if (m_info.m_bMulti)
						for (int j = 0; j < regression.size() - 1; j++)
						{
#pragma omp flush(msg)
							if (msg)
							{

								CGeoRegression regression2 = regression;
								regression2.erase(regression2.begin() + j);

								double R² = regression2.Compute(calibPts, m_prePostTransfo);
								double adjR² = 1 - ((1 - R²)*(calibPts.size() - 1)) / (calibPts.size() - regression2.size() - 1);
#pragma omp critical(SPA_REG_SET)
								{
									if (adjR² > less_imp_R²)
									{
										less_imp_R² = adjR²;
										less_imp_j = j;
									}
									msg += callback.StepIt(0);
#pragma omp flush(msg)
								}
							}
						} //for all term

						if (less_imp_j != -1 && global_bestR² - less_imp_R² < criticalR2)
						{
							regression.erase(regression.begin() + less_imp_j);
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
		}//level

		//transfer parameters
		regressionTerm = regression.GetOrder();


		callback.PopTask();

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
			switch (m_param.m_regressOptimization)
			{
			case CGeoRegression::STRAIGHT_FOREWARD: msg = StraightForeward(*pCalibPts, m_param.m_regressionModel, m_param.m_regressCriticalR2, CTerm::ALL_ORDER, callback); break;
			case CGeoRegression::SW_FOREWARD: msg = Foreward(*pCalibPts, m_param.m_regressionModel, m_param.m_regressCriticalR2, CTerm::ALL_ORDER, callback); break;
			case CGeoRegression::SW_BACKWARD: msg = Backward(*pCalibPts, m_param.m_regressionModel, m_param.m_regressCriticalR2, CTerm::ALL_ORDER, callback); break;
			case CGeoRegression::SW_STEPWISE: msg = StepWise(*pCalibPts, m_param.m_regressionModel, m_param.m_regressCriticalR2, CTerm::ALL_ORDER, callback); break;
			default: ASSERT(false);
			}


			if (msg)
			{

				//Compute with the real parameters
				m_regression = m_param.m_regressionModel;
				m_regression.Compute(*pCalibPts, m_prePostTransfo);

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
		string str = "Nb temrs=" + to_string(m_regression.size()) + "\n";
		str += string(CGridInterpolParam::GetMemberName(CGridInterpolParam::REGRESSION_MODEL)) + " = " + m_regression.GetEquation();

		return str;
	};



	void CSpatialRegression::GetParamterset(CGridInterpolParamVector& parameterset)
	{
		parameterset.clear();
	}



}