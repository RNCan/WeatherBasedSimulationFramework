//*****************************************************************************
// Individual-based model of Hemlock looper (HL)
// 
// Jacques Régnière
// Canadian Forest Service
// 
// Programmer: Rémi Saint-Amant
// 
// Spring 2014
// 
//*****************************************************************************
//*****************************************************************************
// File: HLModel.cpp
//
// Class: CHLModel
//
// Description: CHLModel is a BioSIM model that computes Hemmlock Looper
//              seasonal biology. 
//
//*****************************************************************************
// 06/04/2017	1.1.3	Rémi Saint-Amant	Recompile
// 22/01/2016	1.1.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 04/03/2015	1.0.1	Rémi Saint-Amant	Update with BioSIM 11
// 17/04/2014	1.0.0	Jacques Regniere	Start
//*****************************************************************************

#include "basic/timeStep.h" 
#include "ModelBase/EntryPoint.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include "HemlockLooper.h"
#include "HemlockLooperModel.h"

using namespace std;
using namespace WBSF::HemlockLooper;

namespace WBSF
{
	//true to activate simulated annealing, false otherwhise
	static const bool ACTIVATE_PARAMETRIZATION = false;
	static const bool BY_AI = false;



	//**************************
	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CHLModel::CreateObject);


	//temporary output vector
	typedef CModelStatVectorTemplate<NB_HL_STAT> CHLStatVector;


	//final output vector


	enum TOuput{ O_EGGS, O_L1, O_L2, O_L3, O_L4, O_PUPA, O_ADULTS, O_DEAD_ADULTS, O_AVERAGE_INSTAR, O_NB_FEMALES, O_BROODS, O_DEAD_ATTRITION, O_DEAD_FROZEN, O_DEAD_OVERWINTER, O_S_WEIGHT, O_S_ENERGY, O_S_COLD, O_S_HATCH, NB_DAILY_OUTPUTS };
	extern const char DAILY_HEADER[] = "Eggs,L1,L2,L3,L4,Pupa,Adults,DeadAdults,AverageInstar,NbFemale,Broods,DeadAttrition,DeadFrozen,DeadOverwinter,Sw,Se,Sc,Sh";

	enum TOuputA{ O_GROWTH_RATE, NB_OUTPUT_A };
	extern const char ANNUAL_HEADER[] = "GrowthRate";
	typedef CModelStatVectorTemplate<NB_OUTPUT_A, ANNUAL_HEADER> CAnnualOutputVector;

	CHLModel::CHLModel()
	{
		//**************************
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface

		NB_INPUT_PARAMETER = ACTIVATE_PARAMETRIZATION ? 8 : 2;
		VERSION = "1.0.4 (2017)";

		m_timeStep = 4;


		m_bApplyMortality = true;

		//developer parameters
		m_bApplyAttrition = true;

		memset(m_rho25Factor, 0, (NB_STAGES - 1)*sizeof(m_rho25Factor[0]));
		memset(m_eggsParam, 0, (HemlockLooperEquations::NB_PARAM)*sizeof(m_eggsParam[0]));


		//Simulated Annealing data type
		m_dataType = DATA_UNKNOWN;
		m_bInit = false;
	}

	CHLModel::~CHLModel()
	{
	}

	//this method is called to load parameters into variables
	ERMsg CHLModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(!m_weather.empty());

		ERMsg msg;

		int c = 0;

		m_bApplyMortality = parameters[c++].GetBool();
		m_bComputeWinterMortality = parameters[c++].GetBool();

		if (ACTIVATE_PARAMETRIZATION)
		{
			for (int s = 0; s < HemlockLooperEquations::NB_PARAM; s++)
				m_eggsParam[s] = parameters[c++].GetReal();
		}

		return msg;
	}

	ERMsg CHLModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Actual model execution
		CHLStatVector stat;
		GetDailyStat(stat);

		//fill output result
		//CDailyOutputVector output;
		ComputeRegularStat(stat, m_output);

		return msg;
	}

	ERMsg CHLModel::OnExecuteAnnual()
	{
		ERMsg msg;

		//Actual model execution
		CHLStatVector stat;
		GetDailyStat(stat);



		if (m_weather.size() > 1)
		{
			//fill ouptut matrix
			CAnnualOutputVector output(m_weather.size() - 1, CTRef(m_weather.GetFirstYear()));

			//for(size_t y=0; y<m_weather.size()-1; y++)
			//{
			//CStatistic s = stat.GetStat( E_L12, m_weather[y+1].GetTPeriod() );
			//output[y][O_GROWTH_RATE] = s[SUM]/100; 
			//}

			SetOutput(output);
		}





		return msg;
	}


	void CHLModel::GetDailyStat(CModelStatVector& stat)
	{
		//ASSERT(m_weather.GetNbYears() > 1);

		if (!m_weather.IsHourly())
		{
			//Generate hourly values
			m_weather.ComputeHourlyVariables(); 
		}

		

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::DAILY);
		//p.Begin().m_year++;//skip the first year (initialization)

		//This is where the model is actually executed
		stat.Init(p, NB_HL_STAT);

		//CInitialPopulation inititialPopulation = GetFirstOviposition();
		//CInitialPopulation inititialPopulation(CTRef(m_weather.GetFirstYear(), SEPTEMBER, DAY_15), 5);
		
		CInitialPopulation inititialPopulation;

		//for (size_t y1 = 0; y1 < m_weather.size() - 1; y1++)
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			

			//Create stand
			int year = m_weather[y].GetTRef().GetYear();
			if (!m_bComputeWinterMortality || y==0)
				inititialPopulation = CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0);
			

			CHLStand stand(this);

			stand.m_bApplyMortality = m_bApplyMortality;
			stand.m_bFertilEgg = false;

			//always add trees in stand first
			CHLTreePtr pTree = make_shared<CHLTree>(&stand);
			stand.m_host.push_front(pTree);
			


			//Create the initial population
			pTree->Initialize<CHemlockLooper>(inititialPopulation);

			//if Simulated Annealing, set 
			if (ACTIVATE_PARAMETRIZATION)
			{
				stand.m_development.SetEggParam(m_eggsParam);
				//stand.m_development.SetRho25(m_rho25Factor);
				//stand.m_rates.Save("D:\\Rates.csv"); 
			}


			
			if (m_bComputeWinterMortality && y>0)
			{
				for (size_t yy = y; yy < y+2; yy++)
				{
					for (size_t m = 0; m < m_weather[yy].size(); m++)
					{
						for (size_t d = 0; d < m_weather[yy][m].size(); d++)
						{ 
							CTRef TRef = m_weather[yy][m][d].GetTRef();
							stand.Live(m_weather[yy][m][d]);
							if (stat.IsInside(TRef) && yy == y+1)
								stand.GetStat(TRef, stat[TRef]); 
							//	stat[TRef][E_FEMALES] = stand.GetNbObjectAlive();
							//	stat[TRef][S_DEAD_ATTRITION] = stand.GetFirstHost()->size();
							//}

							stand.AdjustPopulation();
							HxGridTestConnection();
						}
					}

					stand.HappyNewYear();
				}
			}
			else
			{
				for (size_t m = 0; m < m_weather[y].size(); m++)
				{
					for (size_t d = 0; d < m_weather[y][m].size(); d++)
					{
						CTRef TRef = m_weather[y][m][d].GetTRef();
						stand.Live(m_weather[y][m][d]);
						stand.GetStat(TRef, stat[TRef]);

						stand.AdjustPopulation();
						HxGridTestConnection();
					}
				}
			}
			
			if (m_bComputeWinterMortality)
			{
				inititialPopulation = stat.GetInitialPopulation(E_BROODS, m_weather[y + 1].GetEntireTPeriod(CTM::DAILY));
				if (inititialPopulation.empty())
					inititialPopulation.Initialize(CTRef(year + 1, JANUARY, DAY_01), 5);
			}
		}
	}

	CInitialPopulation CHLModel::GetFirstOviposition()
	{
		int year = m_weather.GetFirstYear();
		CInitialPopulation inititialPopulation(CTRef(year, SEPTEMBER, DAY_15), 5);


		//Create stand
		CHLStand stand(this);
		stand.m_bApplyMortality = false;
		stand.m_bFertilEgg = false;

		//Create and add the tree first
		CHLTreePtr pTree = make_shared<CHLTree>(&stand);
		stand.m_host.push_front(pTree);


		//Create the initial population
		pTree->Initialize<CHemlockLooper>(inititialPopulation);


		//size_t y = 0ull;
		CHLStatVector stat(366, m_weather[size_t(1)][FIRST_MONTH][FIRST_DAY].GetTRef());

		for (size_t y = 0; y < 2; y++)
		{
			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					CTRef TRef = m_weather[y][m][d].GetTRef();
					stand.Live(m_weather[y][m][d]);
					if (y==1)
						stand.GetStat(TRef, stat[TRef]);

					HxGridTestConnection();
				}
			}
		}

		inititialPopulation = stat.GetInitialPopulation(E_BROODS);
		inititialPopulation.UpdateYear(year);


		if (inititialPopulation.empty())
			inititialPopulation.Initialize(CTRef(year, SEPTEMBER, DAY_15), 5);

		return inititialPopulation;
	}

	void CHLModel::ComputeRegularStat(CModelStatVector& stat, CModelStatVector& output)
	{
		output.Init(stat.GetTPeriod(), NB_DAILY_OUTPUTS, 0, DAILY_HEADER);
		//output.SetFirstTRef(stat.GetFirstTRef());

		for (size_t d = 0; d < stat.size(); d++)
		{
			for (size_t i = 0; i <= O_DEAD_ADULTS; i++)
				output[d][O_EGGS + i] = stat[d][S_EGGS + i];

			output[d][O_AVERAGE_INSTAR] = stat[d][S_AVERAGE_INSTAR];
			output[d][O_BROODS] = stat[d][E_BROODS];
			output[d][O_NB_FEMALES] = stat[d][E_FEMALES];
			output[d][O_DEAD_ATTRITION] = stat[d][S_DEAD_ATTRITION];
			output[d][O_DEAD_FROZEN] = stat[d][S_DEAD_FROZEN];
			output[d][O_DEAD_OVERWINTER] = stat[d][S_DEAD_OVERWINTER];
			if (stat[d][E_NB_HATCH] > 0)
			{
				output[d][O_S_WEIGHT] = stat[d][E_SWEIGHT] / stat[d][E_NB_HATCH];
				output[d][O_S_ENERGY] = stat[d][E_SENERGY] / stat[d][E_NB_HATCH];
				output[d][O_S_COLD] = stat[d][E_SCOLD] / stat[d][E_NB_HATCH];
				output[d][O_S_HATCH] = stat[d][E_SHATCH] / stat[d][E_NB_HATCH];
			}
		}
	}


	//**************************
	

	//simulated annaling 
	void CHLModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		if (header.size() == NB_DATA_ECLOSION)
		{
			ASSERT(header[DE_YEAR] == "Year");
			ASSERT(header[DE_MONTH] == "Month");
			ASSERT(header[DE_DAY] == "Day");
			ASSERT(header[DE_JDAY] == "Jday");
			ASSERT(header[DE_ECLOSION_CUMUL] == "Eclosion Cumul(%)");

			m_dataType = OPT_ECLOSION;
			CTRef ref(ToInt(data[DE_YEAR]), ToSizeT(data[DE_MONTH]) - 1, ToSizeT(data[DE_DAY]) - 1);

			std::vector<double> obs;
			obs.push_back(ToDouble(data[DE_ECLOSION_CUMUL]));
			//obs.push_back(ToDouble(data[DE_N]));

			ASSERT(obs.size() == NB_OBS_ECLOSION);
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
			ASSERT(header[DS_PUPA] == "Pupae");
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

	void CHLModel::GetFValueDaily(CStatisticXY& stat)
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
				for (CWeatherStation::iterator it = m_weather.begin(); it != m_weather.end();)
				{
					if (it->first < m_firstYear || it->first > m_lastYear)
						it = m_weather.erase(it);
					else
						it++;
				}
				//while (m_weather.size() > 1 && m_weather.GetFirstYear() < m_firstYear)
				//m_weather.erase(0);// .RemoveYear(0);

				//while( m_weather.size() > 1 && m_weather.GetLastYear() > m_lastYear )
				//m_weather.RemoveYear(m_weather.GetNbYear()-1);


				ASSERT(m_weather.GetFirstYear() == m_firstYear);
				ASSERT(m_weather.GetLastYear() == m_lastYear);
				m_bInit = true;
			}

			switch (m_dataType)
			{
			case OPT_ECLOSION: GetFValueDailyEclosion(stat); break;
			case OPT_STAGE: GetFValueDailyStage(stat); break;
			case OPT_AI: GetFValueDailyAI(stat); break;
			default: ASSERT(false);
			}
		}
	}

	void CHLModel::GetFValueDailyEclosion(CStatisticXY& stat)
	{

		CHLStatVector statSim;
		GetDailyStat(statSim);

		for (int i = 0; i < (int)m_SAResult.size(); i++)
		{
			if (statSim.IsInside(m_SAResult[i].m_ref))
			{
				ASSERT(m_SAResult[i].m_obs.size() == NB_OBS_ECLOSION);

				double obs = m_SAResult[i].m_obs[OE_ECLOSION];
				double sim = 100 - statSim[m_SAResult[i].m_ref][S_EGGS];

				//ASSERT(obsPropL2 >= 0 && obsPropL2 <= 100);
				//double n = (double)Round(sqrt(m_SAResult[i].m_obs[OE_N]));
				//for (int j = 0; j < n; j++)
				stat.Add(sim, obs);
			}
		}
	}


	void CHLModel::GetFValueDailyStage(CStatisticXY& stat)
	{

		CHLStatVector statSim;
		GetDailyStat(statSim);

		for (int i = 0; i<(int)m_SAResult.size(); i++)
		{
			if (statSim.IsInside(m_SAResult[i].m_ref))
			{
				ASSERT(m_SAResult[i].m_obs.size() == NB_OBS_STAGE);
				CModelStat& dayStat = statSim[m_SAResult[i].m_ref];

				CStatisticXYEx statLH;
				for (int l = OS_L2; l <= OS_ADULT; l++)
				{
					double obs = m_SAResult[i].m_obs[l];
					double sim = statSim[m_SAResult[i].m_ref][S_L2 + l];

					statLH.Add(sim, obs);

				}

				double NLL = statLH[NEGATIVE_LOG_LIKELIHOOD];
				//Try to maximize MAE of log(LH)
				if (NLL > -999 && !_isnan(NLL) && _finite(NLL))
					stat.Add(NLL, 0);
			}
		}
	}


	void CHLModel::GetFValueDailyAI(CStatisticXY& stat)
	{

		CHLStatVector statSim;
		GetDailyStat(statSim);

		for (int i = 0; i < (int)m_SAResult.size(); i++)
		{
			if (statSim.IsInside(m_SAResult[i].m_ref))
			{
				ASSERT(m_SAResult[i].m_obs.size() == NB_OBS_AI);

				double obs = m_SAResult[i].m_obs[OA_AI];

				CModelStat& dayStat = statSim[m_SAResult[i].m_ref];
				double sim = dayStat.GetAverageInstar(S_EGGS, S_DEAD_ADULTS, EGGS, true);
				ASSERT(sim = -999 || (sim >= 2 && sim <= 8));

				//some obs 0 or 100 were set to -999 
				if (obs >= 2 && obs <= 8 &&
					sim >= 2 && sim <= 8)
				{
					stat.Add(sim, obs);
				}
			}
		}
	}
}