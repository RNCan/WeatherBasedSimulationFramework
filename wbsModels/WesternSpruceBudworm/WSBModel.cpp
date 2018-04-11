//*****************************************************************************
// Individual-based model of Western Spruce Busdworm (WSB)
// 
// Jacques Régnière
// Canadian Forest Service
// 
// Programmer: Rémi St-Amant
// 
// Winter 2009
// 
//*****************************************************************************
//*****************************************************************************
// File: WSBModel.cpp
//
// Class: CWSBModel
//
// Description: CWSBModel is a BioSIM model that computes Western Spruce Busdworm 
//              seasonal biology. There are two model, one base on individual 
//				model. The other is base developement rate.(a revoir)
//
//*****************************************************************************
// 06/04/2018   3.2.1   Rémi Saint-Amant	Annual model bug correction
// 01/04/2018	3.2.0	Rémi Saint-Amant    Compile with VS 2017
// 29/08/2017	3.1.3	Rémi Saint-Amant    Revised model
// 04/05/2017	3.1.2	Rémi Saint-Amant    New hourly generation
// 23/12/2016	3.1.1	Rémi Saint-Amant    Correctiopn on overheating 
// 20/09/2016	3.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 21/01/2016	3.0.0	Rémi Saint-Amant	Update with BioSIM 11.0
// 19/09/2013	2.10	Rémi Saint-Amant	Add Shoot Devel as output variable
// 19/06/2013	2.9		Rémi Saint-Amant	Correction in model base that affect m_totalBrood (verify broods)
// 05/04/2013	2.8		Rémi Saint-Amant	Update with new Simulated Annealing
// 18/02/2013			Rémi Saint-Amant	Update with new BioSIM Model Base (better autoBalance object)
// 29/06/2011			Rémi Saint-Amant	New simulated annealing
// 27/01/2011			Rémi Saint-Amant	Update with new BioSIM Model Base
// 23/01/2009			Rémi Saint-Amant    Creation
//*****************************************************************************

#include "Basic/timeStep.h"
#include "ModelBase/EntryPoint.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include "WSpruceBudworm.h"
#include "WSBModel.h"

using namespace std;

namespace WBSF
{
	//uncomment this line to activate version for simulated annealing
	static const bool ACTIVATE_PARAMETRIZATION = false;
	static const bool BY_AI = false;



	//**************************
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CWSBModel::CreateObject);


	enum TOuput { O_L2o, O_L2, O_L3, O_L4, O_L5, O_L6, O_PUPAE, O_ADULT, O_DEAD_ADULT, O_OVIPOSITING_ADULT, O_BROOD, O_EGG2, O_L2o2, O_L22, O_AVERAGE_INSTAR, O_DD_BUD, O_DD_SHOOT, O_DEAD_ATTRITION, O_DEAD_FROZEN_EGG, O_DEAD_FROZEN_LARVA, O_DEAD_FROZEN_ADULT, O_DEAD_CLEANUP, O_DEAD_MISSING_ENERGY, O_DEAD_SYNCH, O_DEAD_WINDOW, O_E_L2, O_E_L3, O_E_L4, O_E_L5, O_E_L6, O_E_PUPAE, O_E_ADULT, O_E_DEAD_ADULT, NB_OUTPUT_D };
	enum TOuputA { O_A_FEMALE, O_A_EGG2, O_A_L2o2, O_A_DEAD_ATTRITION, O_A_DEAD_FROZEN_EGG, O_A_DEAD_FROZEN_LARVA, O_A_DEAD_FROZEN_ADULT, O_A_DEAD_CLEANUP, O_A_DEAD_SYNCH, O_A_DEAD_WINDOW, O_A_DEAD_MISSING_ENERGY, O_A_GROWTH_RATE, NB_OUTPUT_A };

	CWSBModel::CWSBModel()
	{
		//**************************
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface

		NB_INPUT_PARAMETER = ACTIVATE_PARAMETRIZATION ? 22 : 4;

		VERSION = "3.2.1 (2018)";

		m_bApplyMortality = true;
		m_bFertilEgg = false;	//If female is fertile, eggs will be added to the developement
		m_survivalRate = 0.2;
		m_defoliation = 0.5;


		//developer parameters
		m_initialPopulation = 100;
		m_bApplyAttrition = true;
		m_bApplyWinterMortality = true;
		m_bApplyAsynchronyMortality = true;
		m_bApplyWindowMortality = true;

		m_nbObjects = 400;       //Number of females in the initial attack 
		m_nbMinObjects = 100;
		m_nbMaxObjects = 1000;
		memset(m_rho25Factor, 0, NB_STAGES * sizeof(m_rho25Factor[0]));
		m_bCumulatif = false;

		//Simulated Annealing data
		m_dataType = DATA_UNKNOWN;
		m_bInit = false;
	}

	CWSBModel::~CWSBModel()
	{}

	//**************************
	//this method is called to load parameters in variables
	ERMsg CWSBModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_weather.GetNbYears() > 0);

		ERMsg msg;

		int c = 0;

		m_bApplyMortality = parameters[c++].GetBool();
		m_bFertilEgg = parameters[c++].GetBool();
		m_survivalRate = parameters[c++].GetReal() / 100;
		m_defoliation = parameters[c++].GetReal() / 100;

		if (ACTIVATE_PARAMETRIZATION)
		{
			m_initialPopulation = parameters[c++].GetInt();
			m_bApplyAttrition = parameters[c++].GetBool();
			m_bApplyWinterMortality = parameters[c++].GetBool();
			m_bApplyAsynchronyMortality = parameters[c++].GetBool();
			m_bApplyWindowMortality = parameters[c++].GetBool();
			m_nbMinObjects = parameters[c++].GetInt();
			m_nbMaxObjects = parameters[c++].GetInt();
			m_nbObjects = parameters[c++].GetInt();

			for (int s = 0; s < NB_STAGES; s++)
				m_rho25Factor[s] = parameters[c++].GetReal();
		}


		ASSERT(m_defoliation >= 0 && m_defoliation <= 1);
		return msg;
	}


	ERMsg CWSBModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Actual model execution
		CModelStatVector stat;
		GetDailyStat(stat);


		//fill ouptut matrix
		ComputeRegularStat(stat, m_output);

		return msg;
	}

	ERMsg CWSBModel::OnExecuteAnnual()
	{
		ASSERT(m_weather.size() > 1);

		ERMsg msg;


		//In annual model stop developing of the L22 to get cumulative L22
		CModelStatVector output;
		GetDailyStat(output);


		m_output.Init(m_weather.size(), CTRef(m_weather.GetFirstYear()), NB_OUTPUT_A, -999);


		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//estimate of missing energy and grow rate : take the last day of the current year
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			CTRef lastDay = p.End();

			double L2o22 = output.GetStat(E_L2o2, p)[SUM];
			double missE = output[lastDay][S_DEAD_MISSING_ENERGY];
			m_output[y][O_A_DEAD_MISSING_ENERGY] = missE;
			m_output[y][O_A_GROWTH_RATE] = (L2o22 - missE) / 100;

			if (y < m_weather.size() - 1)
			{
				//Get the number of individuals that complete the winter L2o -> L2 (next year)
				CTPeriod p = m_weather[y + 1].GetEntireTPeriod(CTM(CTM::DAILY));
				CTRef lastDay = output.GetFirstTRef(S_L2o2, "==", 0, 0, p);
				if (lastDay.IsInit())
					m_output[y][O_A_DEAD_MISSING_ENERGY] = output[lastDay][S_DEAD_MISSING_ENERGY];

				CStatistic gr = output.GetStat(E_L22, p);

				if (gr.IsInit())
					m_output[y][O_A_GROWTH_RATE] = gr[SUM] / 100; //initial population is 100 insect
			}

			for (size_t i = O_A_FEMALE; i < O_A_DEAD_MISSING_ENERGY; i++)
			{
				static size_t VAR_POS[O_A_DEAD_MISSING_ENERGY] = { E_TOTAL_FEMALE, E_EGG2, E_L2o2, S_DEAD_ATTRITION, S_DEAD_FROZEN_EGG, S_DEAD_FROZEN_LARVA, S_DEAD_FROZEN_ADULT, S_DEAD_CLEANUP, S_DEAD_SYNCH, S_DEAD_WINDOW };
				CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

				CStatistic stat = output.GetStat(VAR_POS[i], p);
				m_output[y][i] = stat[((i < O_A_DEAD_ATTRITION) ? SUM : HIGHEST)];
			}
		}


		return msg;
	}

	void CWSBModel::GetDailyStat(CModelStatVector& stat)
	{
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		//This is where the model is actually executed
		stat.Init(p, NB_WSB_STAT);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year 
		for (size_t y1 = 0; y1 < m_weather.GetNbYears(); y1++)
		{
			CTPeriod p = m_weather[y1].GetEntireTPeriod(CTM(CTM::DAILY));

			//Create stand
			CWSBStand stand(this);

			stand.m_bFertilEgg = m_bFertilEgg;
			stand.m_survivalRate = m_survivalRate;
			stand.m_defoliation = m_defoliation;


			stand.m_bApplyAttrition = m_bApplyMortality && m_bApplyAttrition;
			stand.m_bApplyWinterMortality = m_bApplyMortality && m_bApplyWinterMortality;
			stand.m_bApplyAsynchronyMortality = m_bApplyMortality && m_bApplyAsynchronyMortality;
			stand.m_bApplyWindowMortality = m_bApplyMortality && m_bApplyWindowMortality;

			//Create the initial attack
			CHostPtr pTree = make_shared<CWSBTree>(&stand);
			pTree->m_nbMinObjects = m_nbMinObjects;
			pTree->m_nbMaxObjects = m_nbMaxObjects;
			pTree->Initialize<CWSpruceBudworm>(CInitialPopulation(p.Begin(), 0, m_nbObjects, m_initialPopulation, L2o, NOT_INIT, m_bFertilEgg, 0));

			stand.m_host.push_back(pTree);

			//if Simulated Annealing, set parameters
			if (ACTIVATE_PARAMETRIZATION)
			{
				stand.m_equations.SetRho25(m_rho25Factor);
				//stand.m_rates.Save("D:\\Rates.csv");
			}


			int nbYear = m_bFertilEgg ? 2 : 1;
			for (size_t y = 0; y < nbYear && y1 + y < m_weather.GetNbYears(); y++)
			{
				size_t yy = y1 + y;

				CTPeriod pp = m_weather[yy].GetEntireTPeriod(CTM(CTM::DAILY));
				for (CTRef d = pp.Begin(); d <= pp.End(); d++)
				{
					stand.Live(m_weather.GetDay(d));
					stand.GetStat(d, stat[d]);

					if (y == 1 && stat[d][S_L2o2] == 0 && stat[d][S_L22] == 0)
					{
						d = pp.End();//end the simulation here
					}

					stand.AdjustPopulation();
					HxGridTestConnection();
				}

				stand.HappyNewYear();
			}
		}
	}

	void CWSBModel::ComputeRegularStat(CModelStatVector& stat, CModelStatVector& output)
	{
		output.Init(stat.GetTPeriod(), NB_OUTPUT_D);


		for (size_t d = 0; d < stat.size(); d++)
		{
			for (size_t i = 0; i < O_L22 - O_L2o + 1; i++)
				output[d][O_L2o + i] = stat[d][S_L2o + i];

			for (size_t i = 0; i < O_E_DEAD_ADULT - O_E_L2 + 1; i++)
				output[d][O_E_L2 + i] = stat[d][E_L2 + i];

			output[d][O_AVERAGE_INSTAR] = stat[d][S_AVERAGE_INSTAR];
			output[d][O_DD_BUD] = stat[d][S_DD_BUD];
			output[d][O_DD_SHOOT] = stat[d][S_DD_SHOOT];

			output[d][O_DEAD_ATTRITION] = stat[d][S_DEAD_ATTRITION];
			output[d][O_DEAD_FROZEN_EGG] = stat[d][S_DEAD_FROZEN_EGG];
			output[d][O_DEAD_FROZEN_LARVA] = stat[d][S_DEAD_FROZEN_LARVA];
			output[d][O_DEAD_FROZEN_ADULT] = stat[d][S_DEAD_FROZEN_ADULT];
			output[d][O_DEAD_CLEANUP] = stat[d][S_DEAD_CLEANUP];
			output[d][O_DEAD_MISSING_ENERGY] = stat[d][S_DEAD_MISSING_ENERGY];
			output[d][O_DEAD_SYNCH] = stat[d][S_DEAD_SYNCH];
			output[d][O_DEAD_WINDOW] = stat[d][S_DEAD_WINDOW];

		}


		//	m_bCumulatif=true;
		//if( m_bCumulatif)
		//{
		//	for (int d=0; d<output.size(); d++)
		//	{
		//		CStatistic dayStat;
		//		for(int i=O_L2; i<=O_DEAD_ADULT; i++)
		//			dayStat+=output[d][i];

		//		if( dayStat[SUM]>0)
		//		{
		//			output[d][O_L2]+=output[d][O_L2o];
		//			for(int i=O_L2; i<=O_ADULT; i++)
		//			{
		//				for(int j=i+1; j<=O_DEAD_ADULT; j++)
		//					output[d][i] += output[d][j];

		//				output[d][i] = output[d][i]/dayStat[SUM]*100;
		//			}
		//		}
		//	}
		//}

	}



	//simulated annaling 
	void CWSBModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		if (header.size() == NB_DATA_EMERGENCE)
		{
			ASSERT(header[DE_YEAR] == "Year");
			ASSERT(header[DE_MONTH] == "Month");
			ASSERT(header[DE_DAY] == "Day");
			ASSERT(header[DE_JDAY] == "Jday");
			ASSERT(header[DE_L2o] == "L2o_(%)");
			ASSERT(header[DE_N] == "n");

			m_dataType = OPT_EMERGENCE;
			CTRef ref(ToInt(data[DE_YEAR]), ToSizeT(data[DE_MONTH]) - 1, ToSizeT(data[DE_DAY]) - 1);

			std::vector<double> obs;
			obs.push_back(ToDouble(data[DE_L2o]));
			obs.push_back(ToDouble(data[DE_N]));

			ASSERT(obs.size() == NB_OBS_EMERGENCE);
			m_SAResult.push_back(CSAResult(ref, obs));
		}
		else if (header.size() == NB_DATA_STAGE)
		{
			//transform value to date/stage
			ASSERT(header[DS_YEAR] == "Year");
			ASSERT(header[DS_MONTH] == "Month");
			ASSERT(header[DS_DAY] == "Day");
			ASSERT(header[DS_JDAY] == "Jday");
			ASSERT(header[DS_L2] == "L2");
			ASSERT(header[DS_L3] == "L3");
			ASSERT(header[DS_L4] == "L4");
			ASSERT(header[DS_L5] == "L5");
			ASSERT(header[DS_L6] == "L6");
			ASSERT(header[DS_PUPEA] == "Pupae");
			ASSERT(header[DS_ADULT] == "Adults");
			ASSERT(header[DS_N] == "total_SBW");
			ASSERT(header[DS_AI] == "AI");



			if (BY_AI)
			{
				m_dataType = OPT_AI;
				CTRef ref(ToInt(data[DS_YEAR]), ToSizeT(data[DS_MONTH]) - 1, ToSizeT(data[DS_DAY]) - 1);
				std::vector<double> obs;
				for (int i = DS_AI; i <= DS_N; i++)
					obs.push_back(ToDouble(data[i]));

				ASSERT(obs.size() == NB_OBS_AI);
				m_SAResult.push_back(CSAResult(ref, obs));
			}
			else
			{
				m_dataType = OPT_STAGE;
				CTRef ref(ToInt(data[DS_YEAR]), ToSizeT(data[DS_MONTH]) - 1, ToSizeT(data[DS_DAY]) - 1);
				std::vector<double> obs;

				for (int i = DS_L2; i <= DS_ADULT; i++)
					obs.push_back(ToDouble(data[i]));

				ASSERT(obs.size() == NB_OBS_STAGE);
				m_SAResult.push_back(CSAResult(ref, obs));
			}
		}
		else ASSERT(false);


	}

	void CWSBModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;


		if (m_SAResult.size() > 0)
		{

			if (!m_bInit)
			{
				CStatistic years;
				for (CSAResultVector::const_iterator p = m_SAResult.begin(); p < m_SAResult.end(); p++)
					years += p->m_ref.GetYear();

				int m_firstYear = (int)years[LOWEST];
				int m_lastYear = (int)years[HIGHEST];
				ASSERT(false);//a faire
				/*while (m_weather.GetNbYears() > 1 && m_weather.GetFirstYear() < m_firstYear)
					m_weather.RemoveYear(0);

				while (m_weather.GetNbYear() > 1 && m_weather.GetLastYear() > m_lastYear)
					m_weather.RemoveYear(m_weather.GetNbYear() - 1);
*/

				ASSERT(m_weather.GetFirstYear() == m_firstYear);
				ASSERT(m_weather.GetLastYear() == m_lastYear);
				m_bInit = true;
			}

			switch (m_dataType)
			{
			case OPT_EMERGENCE: GetFValueDailyEmergence(stat); break;
			case OPT_STAGE: GetFValueDailyStage(stat); break;
			case OPT_AI: GetFValueDailyAI(stat); break;
			default: ASSERT(false);
			}
		}
	}

	void CWSBModel::GetFValueDailyEmergence(CStatisticXY& stat)
	{

		CModelStatVector statSim;
		GetDailyStat(statSim);

		for (int i = 0; i < (int)m_SAResult.size(); i++)
		{
			if (statSim.IsInside(m_SAResult[i].m_ref))
			{
				ASSERT(m_SAResult[i].m_obs.size() == NB_OBS_EMERGENCE);

				double obsPropL2 = m_SAResult[i].m_obs[OE_L2o];
				double simPropL2 = statSim[m_SAResult[i].m_ref][S_L2o];

				ASSERT(obsPropL2 >= 0 && obsPropL2 <= 100);
				double n = Round(sqrt(m_SAResult[i].m_obs[OE_N]));
				for (int j = 0; j < n; j++)
					stat.Add(simPropL2, obsPropL2);

			}
		}
	}


	void CWSBModel::GetFValueDailyStage(CStatisticXY& stat)
	{

		CModelStatVector statSim;
		GetDailyStat(statSim);

		for (int i = 0; i < (int)m_SAResult.size(); i++)
		{
			if (statSim.IsInside(m_SAResult[i].m_ref))
			{
				ASSERT(m_SAResult[i].m_obs.size() == NB_OBS_STAGE);
				//CWSBStat& dayStat = (CWSBStat&)statSim[m_SAResult[i].m_ref];

				CStatisticXYEx statLH;
				for (int l = OS_L2; l <= OS_ADULT; l++)
				{
					double obs = m_SAResult[i].m_obs[l];
					double sim = statSim[m_SAResult[i].m_ref][S_L2 + l];

					statLH.Add(sim, obs);

				}

				double NLL = statLH[NEGATIVE_LOG_LIKELIHOOD];
				//Try to maximize MAE of log(LH)
				if (NLL > -9999 && !_isnan(NLL) && _finite(NLL))
					stat.Add(NLL, 0);
			}
		}
	}


	void CWSBModel::GetFValueDailyAI(CStatisticXY& stat)
	{

		CModelStatVector statSim;
		GetDailyStat(statSim);

		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			if (statSim.IsInside(m_SAResult[i].m_ref))
			{
				ASSERT(m_SAResult[i].m_obs.size() == NB_OBS_AI);

				double obs = m_SAResult[i].m_obs[OA_AI];

				//				CWSBStat& dayStat = (CWSBStat&)statSim[m_SAResult[i].m_ref];
				double sim = 0;// dayStat.GetAverageInstar(false);
				ASSERT(sim = -9999 || (sim >= 2 && sim <= 8));

				//some obs 0 or 100 were set to -9999 
				if (obs >= 2 && obs <= 8 &&
					sim >= 2 && sim <= 8)
				{
					stat.Add(sim, obs);
				}
			}
		}
	}
}


