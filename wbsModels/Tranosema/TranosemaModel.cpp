//*****************************************************************************
// Class: CTranosemaModel
//
// Description: CTranosemaModel is a BioSIM model of Tranosema
//*****************************************************************************
// 22/01/2016	1.1.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 11/12/2015	1.0.0	Rémi Saint-Amant	Creation
//*****************************************************************************

#include "Basic/UtilStd.h"
#include "ModelBase/EntryPoint.h"
#include "..\SpruceBudworm\SpruceBudworm.h"
#include "TranosemaModel.h"
#include "Tranosema.h"



using namespace std;
using namespace WBSF::Tranosema;

namespace WBSF
{
	static const bool ACTIVATE_PARAMETRIZATION = false;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CTranosemaModel::CreateObject);

	enum{ O_D_EGG, O_D_PUPA, O_D_ADULT, O_D_DEAD_ADULT, O_D_OVIPOSITING_ADULT, O_D_BROOD, O_D_ATTRITION, NB_DAILY_OUTPUT };
	extern char DAILY_HEADER[] = "Egg,Pupa,Adult,DeadAdult,OvipositingAdult,Brood,Attrition";

	enum{ O_A_NB_GENERATION, O_A_GROW_RATE1, O_A_GROW_RATE2, O_A_GROW_RATE3, O_A_GROW_RATE4, O_A_GROW_RATE5, O_A_GROW_RATE6, O_A_GROW_RATE7, O_A_GROW_RATE8, NB_ANNUAL_STAT, NB_MAX_GENERATION = 8 };
	extern char ANNUAL_HEADER[] = "Gmax,GrowRate1,GrowRate2,GrowRate3,GrowRate4,GrowRate5,GrowRate6,GrowRate7,GrowRate8";


	CTranosemaModel::CTranosemaModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 3;
		VERSION = "1.1.1 (2016)";

		// initialize your variables here (optional)
		m_bHaveAttrition = true;
		m_generationAttrition = 0.10;//10% of Attrition
		m_DiapauseAge = 0.0;
	}

	CTranosemaModel::~CTranosemaModel()
	{
	}

	//************************************************************************************************

	//this method is call to load your parameter in your variable
	ERMsg CTranosemaModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_bHaveAttrition = parameters[c++].GetBool();
		m_generationAttrition = parameters[c++].GetReal();
		m_DiapauseAge = parameters[c++].GetReal();
		//m_bFertility = parameters[c++].GetBool();
		ASSERT(m_DiapauseAge >= 0. && m_DiapauseAge <= 2.);

		return msg;
	}

	//************************************************************************************************
	// Daily method
	void CTranosemaModel::GetSpruceBudwormBiology(CWeatherStation& weather, CModelStatVector& stat)
	{
		//This is where the model is actually executed
		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));
		stat.Init(p.GetNbRef(), p.Begin(), SBW::TSpruceBudwormStats::NB_STATS, 0, DAILY_HEADER);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 0; y < weather.size(); y++)
		{
			CTPeriod p = weather[y].GetEntireTPeriod(CTM(CTM::DAILY));


			//Create stand
			CSBWStand stand(this);
			//create tree
			std::shared_ptr<CSBWTree> pHost = make_shared<CSBWTree>(&stand);

			//Init tree
			pHost->m_kind = CSBWTree::BALSAM_FIR;
			pHost->m_nbMinObjects = 100;
			pHost->m_nbMaxObjects = 1000;
			pHost->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, 400, 100, SBW::L2o, NOT_INIT, false));


			//Init stand
			stand.m_bFertilEgg = false;
			stand.m_bApplyAttrition = false;
			stand.m_bStopL22 = true;
			stand.m_host.push_back(pHost);

			for (CTRef d = p.Begin(); d <= p.End(); d++)
			{
				stand.Live(weather.GetDay(d));
				stand.GetStat(d, stat[d]);
				stand.AdjustPopulation();
				HxGridTestConnection();
			}
		}
	}


	//This method is called to compute the solution
	ERMsg CTranosemaModel::OnExecuteDaily()
	{
		ERMsg msg;
		
		//if daily data, compute sub-daily data
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();
		
		//Init spruce budworm data
		CModelStatVector SBWStat;
		
		//GetSpruceBudwormBiology(m_weather, SBWStat);

		//one CModelStatVector by generation
		vector<CModelStatVector> TranosemaStat;
		ExecuteDailyAllGenerations(SBWStat, TranosemaStat);

		//merge generations vector into one output vector (max of 4 generations)
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		size_t maxG = min(size_t(4), TranosemaStat.size());
		m_output.Init(p.size(), p.Begin(), maxG*NB_DAILY_OUTPUT, 0, DAILY_HEADER);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			for (size_t g = 0; g < maxG; g++)
			{
				for (size_t s = 0; s < NB_DAILY_OUTPUT; s++)
					m_output[TRef][g*NB_DAILY_OUTPUT + s] = TranosemaStat[g][TRef][s];
			}
		}


		return msg;
	}

	void CTranosemaModel::ExecuteDailyAllGenerations(CModelStatVector& /*SBWStat*/, vector<CModelStatVector>& stat)
	{
		//This is where the model is actually executed
		CTPeriod entirePeriod = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//get the annual period 
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			//get initial population from spruce budworm L4 instar
//			CInitialPopulation initialPopulation = SBWStat.GetInitialPopulation(SBW::S_L4, 400, 100, ADULT, NOT_INIT, true, 0, p);
			CInitialPopulation initialPopulation(p.Begin(), 0, 400, 100, EGG+m_DiapauseAge, NOT_INIT, true,0);

			//Create stand
			CTranosemaStand stand(this);

			//Create host
			CHostPtr pHost = make_shared<CHost>(&stand);

			//Init host
			pHost->m_nbMinObjects = 100;
			pHost->m_nbMaxObjects = 1000;
			pHost->Initialize<CTranosema>(initialPopulation);
			double nbAlive = pHost->GetNbSpecimenAlive();

			//Init stand
			stand.m_bApplyAttrition = m_bHaveAttrition;
			stand.m_generationAttrition = m_generationAttrition;
			stand.m_host.push_front(pHost);


			//run the model for all days of all years
			for (CTRef d = p.Begin(); d <= p.End(); d++)
			{
				stand.Live(m_weather.GetDay(d));

				size_t nbGenerations = stand.GetFirstHost()->GetNbGeneration();
				if (nbGenerations > stat.size())
					stat.push_back(CModelStatVector(entirePeriod.GetNbRef(), entirePeriod.Begin(), NB_STATS, 0));

				for (size_t g = 0; g < nbGenerations; g++)
					stand.GetStat(d, stat[g][d], g);

				stand.AdjustPopulation();
				HxGridTestConnection();
			}
		}
	}


	//************************************************************************************************
	ERMsg CTranosemaModel::OnExecuteAnnual()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		//Init spruce budworm data
		CModelStatVector SBWStat;

		GetSpruceBudwormBiology(m_weather, SBWStat);

		vector<CModelStatVector> TranosemaStat;
		ExecuteDailyAllGenerations(SBWStat, TranosemaStat);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_STAT, 0, ANNUAL_HEADER);


		//now compute annual grow rates
		//Get last complete generation
		size_t maxG = min(size_t(NB_MAX_GENERATION-1), TranosemaStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));
				m_output[TRef][O_A_NB_GENERATION] = 0;

				//find the number of complete generation (AI>2.9)
				for (size_t g = 0; g < maxG; g++)
				{
					//Get the average instar at the end of the season
					double AI = TranosemaStat[g][season.End()].GetAverageInstar(EGG, 0, DEAD_ADULT, false);
					if (AI>2.9)
						m_output[TRef][O_A_NB_GENERATION] = g + 1;
				}

				size_t Gmax = min(maxG, size_t(m_output[TRef][O_A_NB_GENERATION]));
				if (Gmax > 1)
				{
					double nbAdult­¯¹ = 100;
					for (size_t g = 0; g < Gmax - 1; g++)
					{
						CStatistic adultStat = TranosemaStat[g + 1].GetStat(E_ADULT, season);
						double nbAdult = adultStat[SUM];
						if (nbAdult­¯¹ > 0.01)
							m_output[TRef][O_A_GROW_RATE1 + g] = nbAdult / nbAdult­¯¹;

						nbAdult­¯¹ = nbAdult;
					}
				}
			}
		}


		return msg;
	}
	//************************************************************************************************

	//simulated annaling 
	//void CTranosemaModel::AddDailyResult(const StringVector& header, const StringVector& data)
	//{
	//	//transform value to date/stage
	//	ASSERT( header[3] == "Year");
	//	ASSERT( header[4] == "Month");
	//	ASSERT( header[5] == "Day");
	//	ASSERT( header[12] == "NbInds");
	//	ASSERT( header[13] == "AI");
	//
	//	CTRef ref(ToShort(data[3]), ToShort(data[4]) - 1, ToShort(data[5]) - 1);
	//	std::vector<double> obs;
	//	obs.push_back(ToDouble(data[13]));
	//	obs.push_back(ToDouble(data[12]));
	//	m_SAResult.push_back( CSAResult(ref, obs ) );
	//}
	//
	//void CTranosemaModel::GetFValueDaily(CStatisticXY& stat)
	//{
	//	ERMsg msg;
	//	CModelStatVector statSim;
	//
	//	if( m_SAResult.size() > 0)
	//	{ 
	//		CStatistic years;
	//		for(CSAResultVector::const_iterator p= m_SAResult.begin(); p<m_SAResult.end(); p++)
	//			years += p->m_ref.GetYear();
	// 
	//		int m_firstYear = (int)years[LOWEST];
	//		int m_lastYear = (int)years[HIGHEST];
	//
	//		ASSERT( m_weather.GetFirstYear() == m_firstYear );
	//		ASSERT( m_weather.GetLastYear() == m_lastYear );
	//		ExecuteDaily(statSim);
	// 
	//
	//		for(int i=0; i<(int)m_SAResult.size(); i++)
	//		{
	//			if( statSim.IsInside(m_SAResult[i].m_ref) )
	//			{
	//				double AISim = statSim[ m_SAResult[i].m_ref ][S_AVERAGE_INSTAR];
	//				//_ASSERTE( AISim >= 2&&AISim<=8);
	//				//when all bug die, a value of -9999 can be compute
	//				if( AISim>=2 && AISim<=8)
	//				{
	//					for(int j=0; j<m_SAResult[i].m_obs[1]; j++)
	//					stat.Add(AISim, m_SAResult[i].m_obs[0]);
	//				}
	//			}
	//		}
	//	}
	//}

	//
	//ERMsg CTranosemaModel::OnExecuteAnnual()
	//{
	//	_ASSERTE(m_weather.size() > 1);
	//
	//	ERMsg msg;
	//
	//	//In annual model stop developing of the L22 to get cumulative L22
	//	CModelStatVector stat;
	//	ExecuteDaily(stat, true);
	//
	//	
	//	CAnnualOutput stateA(m_weather.size() - 1, CTRef(m_weather.GetFirstYear()));
	//
	//	for(size_t y=0; y<m_weather.size()-1; y++)
	//	{
	//		CStatistic statL22;
	//		CTPeriod p = m_weather[y + 1].GetEntireTPeriod();
	//		for (CTRef d = p.Begin(); d <= p.End(); d++)
	//			statL22 += stat[d][S_L22];
	//		
	//		double gr = statL22[CFL::HIGHEST];
	//		stateA[y][O_GROWTH_RATE] = gr/100; //initial population is 100 insect
	//	}
	//
	//	SetOutput(stateA);
	//
	//	return msg;
	//}

}