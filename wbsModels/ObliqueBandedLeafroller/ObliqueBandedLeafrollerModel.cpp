//*****************************************************************************
// Class: CObliqueBandedLeafrollerModel
//
// Description: CObliqueBandedLeafrollerModel is a BioSIM model of ObliqueBandedLeafroller
//*****************************************************************************
// 27/10/2017	1.0.0	Rémi Saint-Amant	Creation
//*****************************************************************************

#include "Basic/UtilStd.h"
#include "Basic/SnowAnalysis.h"
#include "ModelBase/EntryPoint.h"
#include "ObliqueBandedLeafrollerModel.h"
#include "ObliqueBandedLeafroller.h"



using namespace std;

namespace WBSF
{
	static const bool ACTIVATE_PARAMETRIZATION = false;
	static const size_t NB_GENERATIONS = 6;



	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CObliqueBandedLeafrollerModel::CreateObject);

	enum{ O_D_EGG, O_D_L1, O_D_L2, O_D_L3, O_D_L3D, O_D_L4, O_D_L5, O_D_L6, O_D_PUPA, O_D_ADULT_PREOVIP, O_D_ADULT, O_D_DEAD_ADULT, O_D_OVIPOSITING_ADULT, O_D_BROOD, O_D_FROZEN, NB_DAILY_OUTPUT };
	extern char DAILY_HEADER[] = "Egg,L1,L2,L3,L3D,L4,L5,L6,Pupa,Adult,DeadAdult,OvipositingAdult,Brood,Frozen";

	//	
	enum{ O_A_NB_GENERATION, O_A_MEAN_GENERATION, O_A_GROW_RATE, O_A_ALIVE1, NB_ANNUAL_OUTPUT = O_A_ALIVE1 + NB_GENERATIONS-1 };
	extern char ANNUAL_HEADER[] = "Gmax,MeanGeneration, GrowRate,Alive1,Alive2,Alive3,Alive4,Alive5,Alive6";

	enum{ O_G_DIAPAUSE, O_G_GROW_RATE, NB_GENERATION_OUTPUT};
	extern char GENERATION_HEADER[] = "Diapause, GrowRate";

	

	CObliqueBandedLeafrollerModel::CObliqueBandedLeafrollerModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2017)";

		// initialize your variables here (optimal values obtained by sensitivity analysis)
		//m_bHaveAttrition = true;
		//m_generationAttrition = 0.025;//Attrition survival (cull in the egg stage, before creation)
		//m_diapauseAge = EGG + 0.0;
		//m_lethalTemp = -5.;
		//m_criticalDaylength = 13.5;
		//m_startDateShift = 15;
	}

	CObliqueBandedLeafrollerModel::~CObliqueBandedLeafrollerModel()
	{
	}

	//************************************************************************************************

	//this method is call to load your parameter in your variable
	ERMsg CObliqueBandedLeafrollerModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		/*m_bHaveAttrition = parameters[c++].GetBool();
		m_generationAttrition = parameters[c++].GetReal();
		m_diapauseAge = parameters[c++].GetReal();
		m_lethalTemp = parameters[c++].GetReal();
		m_criticalDaylength = parameters[c++].GetReal();
		m_startDateShift = parameters[c++].GetInt();
		ASSERT(m_diapauseAge >= 0. && m_diapauseAge <= 2.);*/

		return msg;
	}

	//************************************************************************************************
	// Daily method
	//void CObliqueBandedLeafrollerModel::GetSpruceBudwormBiology(CWeatherStation& weather, CModelStatVector& stat)
	//{
	//	//This is where the model is actually executed
	//	CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));
	//	stat.Init(p.GetNbRef(), p.Begin(), SBW::TSpruceBudwormStats::NB_STATS, 0, DAILY_HEADER);

	//	//we simulate 2 years at a time. 
	//	//we also manager the possibility to have only one year
	//	for (size_t y = 0; y < weather.size(); y++)
	//	{
	//		CTPeriod p = weather[y].GetEntireTPeriod(CTM(CTM::DAILY));


	//		//Create stand
	//		CSBWStand stand(this);
	//		//create tree
	//		std::shared_ptr<CSBWTree> pHost = make_shared<CSBWTree>(&stand);

	//		//Init tree
	//		pHost->m_kind = CSBWTree::BALSAM_FIR;
	//		pHost->m_nbMinObjects = 100;
	//		pHost->m_nbMaxObjects = 1000;
	//		pHost->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, 400, 100, SBW::L2o, NOT_INIT, false));


	//		//Init stand
	//		stand.m_bFertilEgg = false;
	//		stand.m_bApplyAttrition = false;
	//		stand.m_bStopL22 = true;
	//		stand.m_host.push_back(pHost);

	//		for (CTRef d = p.Begin(); d <= p.End(); d++)
	//		{
	//			stand.Live(weather.GetDay(d));
	//			stand.GetStat(d, stat[d]);
	//			stand.AdjustPopulation();
	//			HxGridTestConnection();
	//		}
	//	}
	//}


	//This method is called to compute the solution
	ERMsg CObliqueBandedLeafrollerModel::OnExecuteDaily()
	{
		ERMsg msg;
		
		//if daily data, compute sub-daily data
		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();
		
		//Init spruce budworm data
		CModelStatVector SBWStat;
		
		//GetSpruceBudwormBiology(m_weather, SBWStat);

		//one CModelStatVector by generation
		vector<CModelStatVector> ObliqueBandedLeafrollerStat;
		ExecuteDailyAllGenerations(SBWStat, ObliqueBandedLeafrollerStat);

		//merge generations vector into one output vector (max of 5 generations)
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		size_t maxG = min(NB_GENERATIONS, ObliqueBandedLeafrollerStat.size()); 
		//m_output.Init(p.size(), p.Begin(), NB_GENERATIONS*NB_DAILY_OUTPUT, 0, DAILY_HEADER);
		m_output.Init(p.size(), p.Begin(), NB_DAILY_OUTPUT, 0, DAILY_HEADER);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			for (size_t g = 0; g < maxG; g++)
			{
				for (size_t s = 0; s < NB_DAILY_OUTPUT; s++)
					m_output[TRef][s] += ObliqueBandedLeafrollerStat[g][TRef][s];
					//m_output[TRef][g*NB_DAILY_OUTPUT + s] = ObliqueBandedLeafrollerStat[g][TRef][s];
			}
			//m_output[TRef][O_D_DAY_LENGTH] = m_weather.GetDayLength(TRef) / 3600.;
		}


		return msg;
	}

	void CObliqueBandedLeafrollerModel::ExecuteDailyAllGenerations(CModelStatVector& /*SBWStat*/, vector<CModelStatVector>& stat)
	{
		if (m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		CTPeriod entirePeriod = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//This is where the model is actually executed
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			CInitialPopulation initialPopulation(p.Begin(), 0, 1000, 100, L3D, RANDOM_SEX, true, 0);

			//Create stand
			CObliqueBandedLeafrollerStand stand(this);

			//Create host
			CHostPtr pHost = make_shared<CHost>(&stand);

			//Init host
			pHost->m_nbMinObjects = 100;
			pHost->m_nbMaxObjects = 2500;
			pHost->Initialize<CObliqueBandedLeafroller>(initialPopulation);
			stand.m_host.push_front(pHost);

			//run the model for all days of all years
			//get the annual period 
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

			stand.HappyNewYear();
		}
	}


	//************************************************************************************************
	ERMsg CObliqueBandedLeafrollerModel::OnExecuteAnnual()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		//Init spruce budworm data
		CModelStatVector SBWStat;

		vector<CModelStatVector> ObliqueBandedLeafrollerStat;
		ExecuteDailyAllGenerations(SBWStat, ObliqueBandedLeafrollerStat);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_OUTPUT, 0, ANNUAL_HEADER);
		

		//now compute annual grow rates
		//Get last complete generation
		size_t maxG = min(NB_GENERATIONS, ObliqueBandedLeafrollerStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));

				m_output[TRef][O_A_NB_GENERATION] = maxG;

				CStatistic meanG;
				CStatistic alive;
				double pupaBegin = 100;
				for (size_t g = 1; g < maxG; g++)
				{
					double pupaEnd = ObliqueBandedLeafrollerStat[g][season.End()][S_PUPA];
						
					alive += pupaEnd;
					m_output[TRef][O_A_ALIVE1 + (g-1)] = pupaEnd;
					meanG += g *pupaEnd;
				}

				ASSERT(alive.IsInit());
				m_output[TRef][O_A_GROW_RATE] = alive[SUM] / pupaBegin;
				m_output[TRef][O_A_MEAN_GENERATION] = meanG[SUM]/alive[SUM];
			}
		}


		return msg;
	}
	//************************************************************************************************
	ERMsg CObliqueBandedLeafrollerModel::OnExecuteAtemporal()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		//Init spruce budworm data
		CModelStatVector SBWStat;

		vector<CModelStatVector> ObliqueBandedLeafrollerStat;
		ExecuteDailyAllGenerations(SBWStat, ObliqueBandedLeafrollerStat);
		

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p.size()*(NB_GENERATIONS - 1), CTRef(0,0,0,0,CTM(CTM::ATEMPORAL)), NB_GENERATION_OUTPUT, 0, GENERATION_HEADER);


		//now compute generation grow rates
		size_t maxG = min(NB_GENERATIONS, ObliqueBandedLeafrollerStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				double diapauseBegin = 100;
				for (size_t g = 1; g < maxG; g++)
				{
					CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));

					CStatistic diapauseStat = ObliqueBandedLeafrollerStat[g].GetStat(E_DIAPAUSE, season);
					if (diapauseStat.IsInit() && diapauseBegin>0)
					{
						size_t y = TRef - p.Begin();
						size_t gg = y*(NB_GENERATIONS-1) + (g - 1);
						m_output[gg][O_G_DIAPAUSE] = diapauseStat[SUM];
						m_output[gg][O_G_GROW_RATE] = diapauseStat[SUM] / diapauseBegin;

						diapauseBegin = diapauseStat[SUM];
					}
				}
			}
		}


		return msg;
	}
	//************************************************************************************************

	//simulated annaling 
	//void CObliqueBandedLeafrollerModel::AddDailyResult(const StringVector& header, const StringVector& data)
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
	//void CObliqueBandedLeafrollerModel::GetFValueDaily(CStatisticXY& stat)
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
	//ERMsg CObliqueBandedLeafrollerModel::OnExecuteAnnual()
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