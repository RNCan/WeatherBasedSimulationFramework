//*****************************************************************************
// Class: CActiaInterruptaModel
//
// Description: CActiaInterruptaModel is a BioSIM model of ActiaInterrupta
//*****************************************************************************
// 25/02/2020	1.0.0	R�mi Saint-Amant	Creation
//*****************************************************************************

#include "Basic/UtilStd.h"
#include "Basic/SnowAnalysis.h"
#include "ModelBase/EntryPoint.h"
#include "ActiaInterruptaModel.h"
#include "ActiaInterrupta.h"



using namespace std;
using namespace WBSF::ActiaInterrupta;

namespace WBSF
{
	static const bool ACTIVATE_PARAMETRIZATION = false;
	static const size_t NB_GENERATIONS = 2;



	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CActiaInterruptaModel::CreateObject);

	enum{ O_D_EGG, O_D_PUPA, O_D_ADULT, O_D_DEAD_ADULT, O_D_OVIPOSITING_ADULT, O_D_BROOD, O_D_ATTRITION, O_D_CUMUL_REATCH_ADULT, O_D_CUMUL_DIAPAUSE, NB_DAILY_OUTPUT, O_D_DAY_LENGTH = NB_DAILY_OUTPUT*NB_GENERATIONS, NB_DAILY_OUTPUT_EX };
	extern char DAILY_HEADER[] = "Egg,Pupa,Adult,DeadAdult,OvipositingAdult,Brood,Attrition,CumulAdult";

	//	
	enum{ O_A_NB_GENERATION, O_A_MEAN_GENERATION, O_A_GROWTH_RATE, O_A_ALIVE1, NB_ANNUAL_OUTPUT = O_A_ALIVE1 + NB_GENERATIONS-1 };
	extern char ANNUAL_HEADER[] = "Gmax,MeanGeneration, GrowthRate,Alive1,Alive2,Alive3,Alive4,Alive5,Alive6";

	enum{ O_G_YEAR, O_G_DIAPAUSE, O_G_GROWTH_RATE, NB_GENERATION_OUTPUT};
	extern char GENERATION_HEADER[] = "Year, Diapause, GrowthRate";

	

	CActiaInterruptaModel::CActiaInterruptaModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2020)";

		// initialize your variables here (optimal values obtained by sensitivity analysis)
		//m_bHaveAttrition = true;
		//m_generationAttrition = 0.025;//Attrition survival (cull in the egg stage, before creation)
		//m_diapauseAge = EGG + 0.0;
		//m_lethalTemp = -5.;
		//m_criticalDaylength = 13.5;
		//m_bOnGround = FALSE;
	}

	CActiaInterruptaModel::~CActiaInterruptaModel()
	{
	}

	//************************************************************************************************

	//this method is call to load your parameter in your variable
	ERMsg CActiaInterruptaModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		/*m_bHaveAttrition = parameters[c++].GetBool();
		m_generationAttrition = parameters[c++].GetReal();
		m_diapauseAge = parameters[c++].GetReal();
		m_lethalTemp = parameters[c++].GetReal();
		m_criticalDaylength = parameters[c++].GetReal();
		m_bOnGround = parameters[c++].GetBool();
		ASSERT(m_diapauseAge >= 0. && m_diapauseAge <= 1.);*/

		return msg;
	}

	//************************************************************************************************
	// Daily method
	


	//This method is called to compute the solution
	ERMsg CActiaInterruptaModel::OnExecuteDaily()
	{
		ERMsg msg;
		
		//if daily data, compute sub-daily data
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();
		
		//Init spruce budworm data
		CModelStatVector SBWStat;

		//one CModelStatVector by generation
		vector<CModelStatVector> ActiaInterruptaStat;
		ExecuteDailyAllGenerations(SBWStat, ActiaInterruptaStat);

		//merge generations vector into one output vector (max of 5 generations)
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		size_t maxG = min(NB_GENERATIONS, ActiaInterruptaStat.size()); 
		m_output.Init(p.size(), p.Begin(), NB_DAILY_OUTPUT_EX, 0, DAILY_HEADER);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			for (size_t g = 0; g < maxG; g++)
			{
				for (size_t s = 0; s < NB_DAILY_OUTPUT; s++)
					m_output[TRef][g*NB_DAILY_OUTPUT + s] = ActiaInterruptaStat[g][TRef][s];
			}
			m_output[TRef][O_D_DAY_LENGTH] = m_weather.GetDayLength(TRef) / 3600.;
		}


		return msg;
	}

	void CActiaInterruptaModel::ExecuteDailyAllGenerations(CModelStatVector& /*SBWStat*/, vector<CModelStatVector>& stat)
	{
		//This is where the model is actually executed
		CTPeriod entirePeriod = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CSnowAnalysis snowA;

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//get the annual period 
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			//CTRef TRef = snowA.GetLastSnowTRef(m_weather[y]);
			//if (!TRef.IsInit() || !m_bOnGround)
			//	TRef = p.Begin(); //no snow 

			//get initial population 
			CInitialPopulation initialPopulation(p.Begin(), 0, 1000, 100, MAGGOT, FEMALE, true, 0);

			//Create stand
			CActiaInterruptaStand stand(this);

			//Create host
			CHostPtr pHost = make_shared<CHost>(&stand);

			//Init host
			pHost->m_nbMinObjects = 100;
			pHost->m_nbMaxObjects = 2500;
			pHost->Initialize<CActiaInterrupta>(initialPopulation);
			//double nbAlive = pHost->GetNbSpecimenAlive();

			//Init stand
			/*stand.m_bApplyAttrition = m_bHaveAttrition;
			stand.m_generationAttrition = m_generationAttrition;
			stand.m_diapauseAge = m_diapauseAge;
			stand.m_criticalDaylength = m_criticalDaylength;
			stand.m_lethalTemp = m_lethalTemp;*/
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
	ERMsg CActiaInterruptaModel::OnExecuteAnnual()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		//Init spruce budworm data
		CModelStatVector SBWStat;

		vector<CModelStatVector> ActiaInterruptaStat;
		ExecuteDailyAllGenerations(SBWStat, ActiaInterruptaStat);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_OUTPUT, 0, ANNUAL_HEADER);
		

		//now compute annual growth rates
		//Get last complete generation
		size_t maxG = min(NB_GENERATIONS, ActiaInterruptaStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));

				m_output[TRef][O_A_NB_GENERATION] = maxG;

				/*CStatistic meanG;
				CStatistic diapause;
				double diapauseBegin = 100;
				for (size_t g = 1; g < maxG; g++)
				{
					
					CStatistic diapauseStat = ActiaInterruptaStat[g].GetStat(M_DIAPAUSE, season);
					
					diapause += diapauseStat[SUM];
					meanG += g *diapauseStat[SUM];
					m_output[TRef][O_A_ALIVE1 + (g - 1)] = diapauseStat[SUM];
					

					if (g == maxG - 1)
					{
						ASSERT(diapause.IsInit());
						double diapauseEnd = ActiaInterruptaStat[g][season.End()][size_t(m_diapauseAge)];
						m_output[TRef][O_A_GROWTH_RATE] = diapauseEnd / diapauseBegin;
						m_output[TRef][O_A_MEAN_GENERATION] = meanG[SUM] / diapause[SUM];
					}
				}*/

				
			}
		}

			

		return msg;
	}
	//************************************************************************************************
	ERMsg CActiaInterruptaModel::OnExecuteAtemporal()
	{
		ERMsg msg;

		//if (m_weather.IsDaily())//if daily data, compute sub-daily data
		//	m_weather.ComputeHourlyVariables();

		////Init spruce budworm data
		//CModelStatVector SBWStat;

		//vector<CModelStatVector> ActiaInterruptaStat;
		//ExecuteDailyAllGenerations(SBWStat, ActiaInterruptaStat);
		//

		//CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		//m_output.Init(p.size()*(NB_GENERATIONS - 1), CTRef(0,0,0,0,CTM(CTM::ATEMPORAL)), NB_GENERATION_OUTPUT, 0, GENERATION_HEADER);


		////now compute generation growth rates
		//size_t maxG = min(NB_GENERATIONS, ActiaInterruptaStat.size());
		//if (maxG > 0)
		//{
		//	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		//	{
		//		double diapauseBegin = 100;
		//		for (size_t g = 1; g < maxG; g++)
		//		{
		//			CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));

		//			CStatistic diapauseStat = ActiaInterruptaStat[g].GetStat(E_DIAPAUSE, season);
		//			if (diapauseStat.IsInit() && diapauseBegin>0)
		//			{
		//				size_t y = TRef - p.Begin();
		//				size_t gg = y*(NB_GENERATIONS-1) + (g - 1);
		//				m_output[gg][O_G_DIAPAUSE] = diapauseStat[SUM];
		//				m_output[gg][O_G_GROWTH_RATE] = diapauseStat[SUM] / diapauseBegin;

		//				diapauseBegin = diapauseStat[SUM];
		//			}
		//		}
		//	}
		//}


		return msg;
	}
	//************************************************************************************************

	//simulated annaling 
	//void CActiaInterruptaModel::AddDailyResult(const StringVector& header, const StringVector& data)
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
	//void CActiaInterruptaModel::GetFValueDaily(CStatisticXY& stat)
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
	//ERMsg CActiaInterruptaModel::OnExecuteAnnual()
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