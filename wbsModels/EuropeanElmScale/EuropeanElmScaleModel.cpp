//**********************************************************************
// 04/04/2018	1.0.0	Rémi Saint-Amant	Create 
//**********************************************************************
#include "EuropeanElmScaleModel.h"
#include "Basic/Statistic.h"
#include <math.h>
#include <crtdbg.h>
#include "ModelBase/EntryPoint.h"
#include "ModelBase/SimulatedAnnealingVector.h"

using namespace std;


namespace WBSF
{

	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CEuropeanElmScaleModel::CreateObject);
	/*
//Male
NbVal=   276	Bias=-14.08771	MAE=18.07775	RMSE=70.26210	CD= 0.72543	R²= 0.73771
Threshold1          	=   1.65
a1                  	= 191.0
b1                  	=   1.49995
a2                  	= 241.1
b2                  	=   0.74280
a3                  	= 367.8
b3                  	=   1.31798
a4                  	= 482.4
b4                  	=   1.49996
a5                  	= 782.4
b5                  	=   0.63299
//female and baby
NbVal=   276	Bias=-3.78197	MAE=16.99951	RMSE=55.30316	CD= 0.82990	R²= 0.83612
Threshold2          	=   4.84008
a6                  	=  93.71502
b6                  	=   2.01212
a7                  	= 610.27338
b7                  	=   1.36273
a8                  	=1164.71147
b8                  	=   2.36267
a9                  	=1246.75056
b9                  	=   5.14439
a10                 	=1279.92535
b10                 	=   5.01932

//Dreistadt model
NbVal=     2	Bias= 0.02225	MAE= 0.03912	RMSE= 0.04500	CD= 1.00000	R²= 1.00000
a11                 	= 229.82435 
a12                 	= 364.27397 
b12                 	=   0.73123 

*/




	enum TOutput { O_M_NYPH2o, O_M_NYPH2, O_M_PREPUPA, O_M_PUPA, O_M_ADULT, O_M_DEADADULT, O_F_NYPH2o, O_F_NYPH2, O_F_ADULT, O_F_DEAD_ADULT, O_NYPH1, O_NYPH2, O_D_NYPH2, O_D_ADULT, O_D_DEAD_ADULT, NB_OUTPUTS };

	const char EuropeanElmScale_header[] = "MaleNyph2o|MaleNyph2|MalePrepupa|MalePupa|MaleAdult|MaleDeadAdult|FemaleNyph2o|FemaleNyph2|FemaleAdult|FemaleDeadAdult|Nyph1|Nyph2|DFemaleNyph2|DFemaleAdult|DFemaleDeadAdult";

	enum TEvaluation { VERTICAL, HORIZONTAL, DIAGONAL };
	static const TEvaluation EVALUATION = VERTICAL;

	const double CEuropeanElmScaleModel::THRESHOLDm = 1.65;
	const double CEuropeanElmScaleModel::THRESHOLDf = 4.8;


	const double CEuropeanElmScaleModel::Am[NB_MALE_PARAMS] =
	{
		191.0,241.1,367.8,482.4,782.4
	};

	const double CEuropeanElmScaleModel::Bm[NB_MALE_PARAMS] =
	{
		1.5,0.7,1.3,1.5,0.6
	};

	const double CEuropeanElmScaleModel::Af[NB_FEMALE_PARAMS] =
	{
		93.7,610.3,1164.7
	};


	const double CEuropeanElmScaleModel::Bf[NB_FEMALE_PARAMS] =
	{
		2.0,1.4,2.4
	};

	const double CEuropeanElmScaleModel::Ab[NB_BABY_PARAMS] =
	{
		1246.8,1279.9
	};


	const double CEuropeanElmScaleModel::Bb[NB_BABY_PARAMS] =
	{
		5.1,4.0
	};


	const double CEuropeanElmScaleModel::Ad[NB_DREISTADT_PARAMS] =
	{
		229.8,364.3
	};


	const double CEuropeanElmScaleModel::Bd =
	{
		0.73123
	};






	CEuropeanElmScaleModel::CEuropeanElmScaleModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2018)";
	}

	CEuropeanElmScaleModel::~CEuropeanElmScaleModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CEuropeanElmScaleModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;

		bool bCumul = parameters[c++].GetBool();
		m_CRm.m_startJday = 0;
		m_CRm.m_lowerThreshold = THRESHOLDm;
		m_CRm.m_bCumul = bCumul;
		m_CRm.m_bMultipleVariance = true;
		m_CRm.m_bPercent = true;
		m_CRm.m_bAdjustFinalProportion = true;
		m_CRm.m_method = CEuropeanElmScaleCRm::DAILY_AVERAGE;
		//m_CRm.m_method = CEuropeanElmScaleCRf::MODIFIED_ALLEN_WAVE;

		m_CRf.m_startJday = 0;
		m_CRf.m_lowerThreshold = THRESHOLDf;
		m_CRf.m_bCumul = bCumul;
		m_CRf.m_bMultipleVariance = true;
		m_CRf.m_bPercent = true;
		m_CRf.m_bAdjustFinalProportion = true;
		m_CRf.m_method = CEuropeanElmScaleCRf::DAILY_AVERAGE;
		//m_CRf.m_method = CEuropeanElmScaleCRf::MODIFIED_ALLEN_WAVE;

		m_CRb.m_startJday = m_CRf.m_startJday;
		m_CRb.m_lowerThreshold = m_CRf.m_lowerThreshold;//same as femelle to keep validity
		m_CRb.m_bCumul = m_CRf.m_bCumul;
		m_CRb.m_bMultipleVariance = m_CRf.m_bMultipleVariance;
		m_CRb.m_bPercent = m_CRf.m_bPercent;
		m_CRb.m_bAdjustFinalProportion = m_CRf.m_bAdjustFinalProportion;
		m_CRb.m_method = m_CRf.m_method;
		//m_CRf.m_method = CEuropeanElmScaleCRf::DAILY_AVERAGE;

		m_CRd.m_startJday = 60;
		m_CRd.m_lowerThreshold = 11;//same as femelle to keep validity
		m_CRd.m_bCumul = bCumul;
		m_CRd.m_bMultipleVariance = false;
		m_CRd.m_bPercent = true;
		m_CRd.m_bAdjustFinalProportion = false;
		m_CRd.m_method = CEuropeanElmScaleCRd::SINGLE_SINE;


		for (size_t i = 0; i < NB_MALE_PARAMS; i++)
		{
			m_CRm.m_a[i] = Am[i];
			m_CRm.m_b[i] = Bm[i];
		}

		for (size_t i = 0; i < NB_FEMALE_PARAMS; i++)
		{
			m_CRf.m_a[i] = Af[i];
			m_CRf.m_b[i] = Bf[i];
		}

		for (size_t i = 0; i < NB_BABY_PARAMS; i++)
		{
			m_CRb.m_a[i] = Ab[i];
			m_CRb.m_b[i] = Bb[i];
		}

		for (size_t i = 0; i < NB_DREISTADT_PARAMS; i++)
		{
			m_CRd.m_a[i] = Ad[i];
			
		}

		m_CRd.m_b_ = Bd;

		if (parameters.size() > 1)
		{
			m_CRm.m_lowerThreshold = parameters[c++].GetFloat();
			m_CRf.m_lowerThreshold = parameters[c++].GetFloat();
			m_CRb.m_lowerThreshold = m_CRf.m_lowerThreshold;

			for (size_t i = 0; i < NB_MALE_PARAMS; i++)
			{
				m_CRm.m_a[i] = parameters[c++].GetFloat();
				m_CRm.m_b[i] = parameters[c++].GetFloat();
			}

			for (size_t i = 0; i < NB_FEMALE_PARAMS; i++)
			{
				m_CRf.m_a[i] = parameters[c++].GetFloat();
				m_CRf.m_b[i] = parameters[c++].GetFloat();
			}

			for (size_t i = 0; i < NB_BABY_PARAMS; i++)
			{
				m_CRb.m_a[i] = parameters[c++].GetFloat();
				m_CRb.m_b[i] = parameters[c++].GetFloat();
			}

			for (size_t i = 0; i < NB_DREISTADT_PARAMS; i++)
			{
				m_CRd.m_a[i] = parameters[c++].GetFloat();
			}

			m_CRd.m_b_ = parameters[c++].GetFloat();

		}

		return msg;
	}


	ERMsg CEuropeanElmScaleModel::OnExecuteDaily()
	{
		ERMsg msg;

		//m_DD.Execute(m_weather, m_output);

		CModelStatVector output_m;
		CModelStatVector output_f;
		CModelStatVector output_b;
		CModelStatVector output_d;
		m_CRm.Execute(m_weather, output_m);
		m_CRf.Execute(m_weather, output_f);
		m_CRb.Execute(m_weather, output_b);
		m_CRd.Execute(m_weather, output_d);

		m_output.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_OUTPUTS);
		ASSERT(output_m.size() == output_m.size());
		for (size_t d = 0; d < output_m.size(); d++)
		{

			m_output[d][O_M_NYPH2o] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 0], 1);
			m_output[d][O_M_NYPH2] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 1], 1);
			m_output[d][O_M_PREPUPA] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 2], 1);
			m_output[d][O_M_PUPA] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 3], 1);
			m_output[d][O_M_ADULT] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 4], 1);
			m_output[d][O_M_DEADADULT] = WBSF::Round(output_m[d][CEuropeanElmScaleCRm::O_FIRST_STAGE + 5], 1);
			m_output[d][O_F_NYPH2o] = WBSF::Round(output_f[d][CEuropeanElmScaleCRf::O_FIRST_STAGE + 0], 1);
			m_output[d][O_F_NYPH2] = WBSF::Round(output_f[d][CEuropeanElmScaleCRf::O_FIRST_STAGE + 1], 1);
			m_output[d][O_F_ADULT] = WBSF::Round(output_f[d][CEuropeanElmScaleCRf::O_FIRST_STAGE + 2], 1);
			m_output[d][O_F_DEAD_ADULT] = WBSF::Round(output_f[d][CEuropeanElmScaleCRf::O_FIRST_STAGE + 3], 1);
			m_output[d][O_NYPH1] = WBSF::Round(output_b[d][CEuropeanElmScaleCRb::O_FIRST_STAGE + 1], 1);
			m_output[d][O_NYPH2] = WBSF::Round(output_b[d][CEuropeanElmScaleCRb::O_FIRST_STAGE + 2], 1);
			m_output[d][O_D_NYPH2] = WBSF::Round(output_d[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 0], 1);
			m_output[d][O_D_ADULT] = WBSF::Round(output_d[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 1], 1);
			m_output[d][O_D_DEAD_ADULT] = WBSF::Round(output_d[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 2], 1);

		}
		//Using Growing Degree Days For Insect Management
		//Nancy E.Adams
		//Extension Educator, Agricultural Resources
		//base 50°F (10°C)
		//GDD data is collected beginning March 1
		//1026 - 1388 (641.25 - 771.1)

		//Dreistadt, S. H. and K.S. Hagen. 1994. European elm scale (Homoptera: Eriococcidae) abundance and parasitism in northern California. Pan-Pacific Entomologist 70:240-252.
		//Location of study : Northern California(field studies)
		//	Developmental threshold
		//	Lower : 11°C
		//	Method of calculation : Single Sine
		//	Degree - day accumulations required for each stage of development
		//	Biofix : March 1
		//	Host : English Elm and Siberian Elm	DD(°C)
		//	Female scale peak	301


		return msg;
	}


	void CEuropeanElmScaleModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		CTRef ref(ToShort(data[1]), ToShort(data[2]) - 1, ToShort(data[3]) - 1);

		ASSERT(header.size() == 15);

		std::vector<double> obs(NB_INPUT);
		for (size_t i = 0; i < NB_INPUT; i++)
		{
			obs[i] = ToDouble(data[i + 4]);//Cumulative
		}

		m_SAResult.push_back(CSAResult(ref, obs));
	}

	void CEuropeanElmScaleModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;

		if (m_SAResult.size() > 0)
		{
			//remove unused years
			/*if (m_firstYear == -999 && m_lastYear == -999)
			{
				CStatistic years;
				for (CSAResultVector::const_iterator p = m_SAResult.begin(); p < m_SAResult.end(); p++)
					years += p->m_ref.GetYear();

				m_firstYear = (int)years[LOWEST];
				m_lastYear = (int)years[HIGHEST];
				while (m_weather.GetNbYears() > 1 && m_weather.GetFirstYear() < m_firstYear)
					m_weather.erase(m_weather.begin());

				while (m_weather.GetNbYears() > 1 && m_weather.GetLastYear() > m_lastYear)
					m_weather.erase(--m_weather.end());

			}*/


			//look to see if all ai are in growing order
			bool bValid = true;

			for (int i = 1; i < NB_MALE_PARAMS&&bValid; i++)
			{
				if (m_CRm.m_a[i] < m_CRm.m_a[i - 1])
					bValid = false;

				if (fabs(m_CRm.m_b[i] - m_CRm.m_b[i - 1]) > 1)
					bValid = false;
			}


			for (int i = 1; i < NB_FEMALE_PARAMS&&bValid; i++)
			{
				if (m_CRf.m_a[i] < m_CRf.m_a[i - 1])
					bValid = false;

				if (fabs(m_CRf.m_b[i] - m_CRf.m_b[i - 1]) > 1)
					bValid = false;
			}

			if (m_CRb.m_a[EGG_NYPH1] < m_CRf.m_a[F_NYPH2_ADULT])
				bValid = false;

			for (int i = 1; i < NB_BABY_PARAMS&&bValid; i++)
			{
				if (m_CRb.m_a[i] < m_CRb.m_a[i - 1])
					bValid = false;

				//if (fabs(m_CRb.m_b[i] - m_CRb.m_b[i - 1]) > 1)
					//bValid = false;
			}
			for (int i = 1; i < NB_DREISTADT_PARAMS&&bValid; i++)
			{
				if (m_CRd.m_a[i] < m_CRd.m_a[i - 1])
					bValid = false;

				if (fabs(m_CRd.m_b[i] - m_CRd.m_b[i - 1]) > 1)
					bValid = false;
			}
			/*if (bValid)
			{
				CModelStatVector test;
				m_CRf.m_bCumul = true;
				m_CRf.Execute(m_weather, test);

				for (int i = 1; i < NB_FEMALE_PARAMS&&bValid; i++)
				{
					double sim = test[m_SAResult[i].m_ref][CEuropeanElmScaleCRf::O_FIRST_STAGE + v];
					if (m_CRf.m_a[i] < m_CRf.m_a[i - 1])
						bValid = false;
				}
			}*/

			if (bValid)
			{
				//******************************************************************************************************************************************************
				//Vertical lookup
				if (EVALUATION == VERTICAL)
				{

					//Male
					//{
					//	CModelStatVector output;
					//	m_CRm.m_bCumul = false;
					//	m_CRm.Execute(m_weather, output);

					//	CStatistic obsStat[NB_MALE_PARAMS];
					//	for (size_t i = 0; i < m_SAResult.size(); i++)
					//	{
					//		for (size_t v = 0; v < NB_MALE_PARAMS; v++)
					//		{
					//			if (m_SAResult[i].m_obs[I_M_NYPH2o + v] > 0)
					//				obsStat[v] += m_SAResult[i].m_obs[I_M_NYPH2o + v];
					//		}
					//	}


					//	CStatistic simStat[NB_MALE_PARAMS];
					//	for (size_t i = 0; i < output.size(); i++)
					//	{
					//		for (size_t v = 0; v < NB_MALE_PARAMS; v++)
					//		{
					//			if (output[i][CEuropeanElmScaleCRm::O_FIRST_STAGE + v] > 0)
					//				simStat[v] += output[i][CEuropeanElmScaleCRm::O_FIRST_STAGE + v];
					//		}
					//	}

					//	if (simStat[NB_MALE_PARAMS - 1][HIGHEST] > 0)
					//	{
					//		for (size_t v = 0; v < NB_MALE_PARAMS; v++)
					//		{
					//			//double c = obsStat[v][HIGHEST] / simStat[v][HIGHEST];
					//			double c = obsStat[v][MEAN] / simStat[v][MEAN];
					//			for (size_t i = 0; i < m_SAResult.size(); i++)
					//			{
					//				double obs = m_SAResult[i].m_obs[I_M_NYPH2o + v];
					//				if (obs > -999 &&
					//					output.IsInside(m_SAResult[i].m_ref))
					//				{
					//					double sim = output[m_SAResult[i].m_ref][CEuropeanElmScaleCRm::O_FIRST_STAGE + v] * c;
					//					stat.Add(sim, obs);
					//				}
					//			}
					//		}
					//	}
					//}

					////Female
					//{

					//	CModelStatVector output;
					//	m_CRf.m_bCumul = false;
					//	m_CRf.Execute(m_weather, output);

					//	CStatistic obsStat[NB_FEMALE_PARAMS];
					//	for (size_t i = 0; i < m_SAResult.size(); i++)
					//	{
					//		for (size_t v = 0; v < NB_FEMALE_PARAMS; v++)
					//		{
					//			if (m_SAResult[i].m_obs[I_F_NYPH2o + v] > 0)
					//				obsStat[v] += m_SAResult[i].m_obs[I_F_NYPH2o + v];
					//		}
					//	}


					//	CStatistic simStat[NB_FEMALE_PARAMS];
					//	for (size_t i = 0; i < output.size(); i++)
					//	{
					//		for (size_t v = 0; v < NB_FEMALE_PARAMS; v++)
					//		{
					//			if (output[i][CEuropeanElmScaleCRm::O_FIRST_STAGE + v] > 0)
					//				simStat[v] += output[i][CEuropeanElmScaleCRf::O_FIRST_STAGE + v];
					//		}
					//	}

					//	if (simStat[NB_FEMALE_PARAMS - 1][HIGHEST])
					//	{
					//		for (size_t v = 0; v < NB_FEMALE_PARAMS; v++)
					//		{
					//			//double c = obsStat[v][HIGHEST] / simStat[v][HIGHEST];
					//			double c = obsStat[v][MEAN] / simStat[v][MEAN];
					//			for (size_t i = 0; i < m_SAResult.size(); i++)
					//			{
					//				double obs = m_SAResult[i].m_obs[I_F_NYPH2o + v];
					//				if (obs > -999 &&
					//					output.IsInside(m_SAResult[i].m_ref))
					//				{
					//					double sim = output[m_SAResult[i].m_ref][CEuropeanElmScaleCRf::O_FIRST_STAGE + v] * c;
					//					stat.Add(sim, obs);
					//				}
					//			}
					//		}
					//	}
					//}


					////Baby
					//{

					//	CModelStatVector output;
					//	m_CRb.m_bCumul = false;
					//	m_CRb.Execute(m_weather, output);

					//	CStatistic obsStat[NB_BABY_PARAMS];
					//	for (size_t i = 0; i < m_SAResult.size(); i++)
					//	{
					//		for (size_t v = 0; v < NB_BABY_PARAMS; v++)
					//		{
					//			if (m_SAResult[i].m_obs[I_NYPH1 + v] > 0)
					//				obsStat[v] += m_SAResult[i].m_obs[I_NYPH1 + v];
					//		}
					//	}


					//	CStatistic simStat[NB_BABY_PARAMS];
					//	for (size_t i = 0; i < output.size(); i++)
					//	{
					//		for (size_t v = 0; v < NB_BABY_PARAMS; v++)
					//		{
					//			if (output[i][CEuropeanElmScaleCRm::O_FIRST_STAGE + v + 1] > 0)
					//				simStat[v] += output[i][CEuropeanElmScaleCRb::O_FIRST_STAGE + v + 1];
					//		}
					//	}

					//	if (simStat[NB_BABY_PARAMS - 1][HIGHEST])
					//	{
					//		for (size_t v = 0; v < NB_BABY_PARAMS; v++)
					//		{
					//			//double c = obsStat[v][HIGHEST] / simStat[v][HIGHEST];
					//			double c = obsStat[v][MEAN] / simStat[v][MEAN];
					//			for (size_t i = 0; i < m_SAResult.size(); i++)
					//			{
					//				double obs = m_SAResult[i].m_obs[I_NYPH1 + v];
					//				if (obs > -999 &&
					//					output.IsInside(m_SAResult[i].m_ref))
					//				{
					//					double sim = output[m_SAResult[i].m_ref][CEuropeanElmScaleCRb::O_FIRST_STAGE + v + 1] * c;
					//					stat.Add(sim, obs);
					//				}
					//			}
					//		}
					//	}
					//}

					{
						CModelStatVector output;
						m_CRd.m_bCumul = false;
						m_CRd.Execute(m_weather, output);

						CStatistic statDD;// [NB_DREISTADT_PARAMS + 1];
						CStatistic::SetVMiss(-999);

						for (size_t d = 0; d < output.size(); d++)
						{
							//for (int s = 0; s < NB_DREISTADT_PARAMS+1; s++)
							//{
								if (output[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 1] > 1 &&
									output[d][CEuropeanElmScaleCRd::O_FIRST_STAGE + 1] < 99)
								{
									double DD = output[d][CEuropeanElmScaleCRd::O_DD];
									statDD += DD;
								}
							//}
						}



						static const double obs[2] = { 301,76 };
						for (size_t t = 0; t < 2; t++)
						{
							double sim = statDD[t == 0 ? MEAN : STD_DEV];
							stat.Add(sim, obs[t]);
						}
					}
				}
			}
		}
	}
}
