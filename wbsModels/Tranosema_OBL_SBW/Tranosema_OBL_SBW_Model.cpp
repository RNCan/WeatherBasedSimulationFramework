//*****************************************************************************
// Class: CTranosemaOBLModel
//
// Description: CTranosema_OBL_SBW_Model is a BioSIM model of Tranosema and ObliqueBandedLeafroller model
//*****************************************************************************
// 17/01/2018	1.0.2	Rémi Saint-Amant	Chnage in annual model
// 29/11/2017	1.0.1	Rémi Saint-Amant	Add CulmulAdult
// 21/11/2017	1.0.0	Rémi Saint-Amant	Creation
//*****************************************************************************

#include "Basic/UtilStd.h"
#include "Basic/SnowAnalysis.h"
#include "ModelBase/EntryPoint.h"
#include "Tranosema_OBL_SBW_Model.h"



using namespace std;
using namespace WBSF::Tranosema;

namespace WBSF
{
	static const bool ACTIVATE_PARAMETRIZATION = false;
	static const size_t NB_GENERATIONS = 6;



	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CTranosema_OBL_SBW_Model::CreateObject);

	
	enum{ O_D_EGG, O_D_PUPA, O_D_ADULT, O_D_DEAD_ADULT, O_D_OVIPOSITING_ADULT, O_D_BROOD, O_D_ATTRITION, O_D_CUMUL_REATCH_ADULT, O_D_CUMUL_DIAPAUSE, O_D_TOTAL, NB_DAILY_OUTPUT, O_D_DAY_LENGTH = NB_DAILY_OUTPUT*NB_GENERATIONS, O_D_NB_OBL, O_D_NB_OBL_L3D, O_D_NB_SBW, O_D_DIAPAUSE_AGE, NB_DAILY_OUTPUT_EX };
	extern char DAILY_HEADER[] = "Egg,Pupa,Adult,DeadAdult,OvipositingAdult,Brood,Attrition,CumulAdult,CumulDiapause";

	//	
	enum{ O_A_NB_GENERATION, O_A_MEAN_GENERATION, O_A_GROWTH_RATE, O_A_ALIVE1, NB_ANNUAL_OUTPUT = O_A_ALIVE1 + NB_GENERATIONS - 1 };
	extern char ANNUAL_HEADER[] = "Gmax,MeanGeneration,GrowthRate,Alive1,Alive2,Alive3,Alive4,Alive5,Alive6";
	
	enum{ O_G_YEAR, O_G_GENERATION, O_G_EGG, O_G_PUPA, O_G_ADULT, O_G_DEAD_ADULT, O_G_BROOD, O_G_ATTRITION, O_G_FROZEN, O_G_HOST_DIE, O_G_DIAPAUSE, O_G_FECONDITY, O_G_EGG_GROWTH, O_G_ADULT_GROW, NB_GENERATION_OUTPUT };
	extern char GENERATION_HEADER[] = "Year,Generation,Eggs,Pupa,Adults,DeadAdults,Broods,Attrition,Diapause,Fecondity,EggGrowth,AdultGrowth";

	

	CTranosema_OBL_SBW_Model::CTranosema_OBL_SBW_Model()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 6;
		VERSION = "1.0.2 (2018)";

		// initialize your variables here (optimal values obtained by sensitivity analysis)
		m_bHaveAttrition = true;
		m_generationAttrition = 0.025;//Attrition survival (cull in the egg stage, before creation)
		m_diapauseAge = EGG + 0.1;
		m_lethalTemp = -5.0;
		m_criticalDaylength = 13.5;
		m_bOnGround = false;
	}

	CTranosema_OBL_SBW_Model::~CTranosema_OBL_SBW_Model()
	{
	}

	//************************************************************************************************

	//this method is call to load your parameter in your variable
	ERMsg CTranosema_OBL_SBW_Model::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_bHaveAttrition = parameters[c++].GetBool();
		m_generationAttrition = parameters[c++].GetReal();
		m_diapauseAge = parameters[c++].GetReal();
		m_lethalTemp = parameters[c++].GetReal();
		m_criticalDaylength = parameters[c++].GetReal();
		m_bOnGround = parameters[c++].GetBool();
		ASSERT(m_diapauseAge >= 0. && m_diapauseAge <= 1.);

		return msg;
	}

	class CTest
	{
	public:

		CTest(double var)
		{
			m_var = var;
		}
		~CTest()
		{
			m_var = -1;
		}

		double m_var;
	};
	//************************************************************************************************
	// Daily method
	
	//This method is called to compute the solution
	ERMsg CTranosema_OBL_SBW_Model::OnExecuteDaily()
	{
		ERMsg msg;
		


		//if daily data, compute sub-daily data
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();
		
		
		//one CModelStatVector by generation
		vector<CModelStatVector> TranosemaStat;
		ExecuteDailyAllGenerations(TranosemaStat);

		//merge generations vector into one output vector (max of 5 generations)
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		size_t maxG = min(NB_GENERATIONS, TranosemaStat.size()); 
		m_output.Init(p.size(), p.Begin(), NB_DAILY_OUTPUT_EX, 0, DAILY_HEADER);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			CStatistic diapauseAge;
			for (size_t g = 0; g < maxG; g++)
			{
				for (size_t s = 0; s < NB_DAILY_OUTPUT; s++)
					m_output[TRef][g*NB_DAILY_OUTPUT + s] = TranosemaStat[g][TRef][s];

				if (TranosemaStat[g][TRef][E_DIAPAUSE] > 0)
					diapauseAge += TranosemaStat[g][TRef][E_DIAPAUSE_AGE] / TranosemaStat[g][TRef][E_DIAPAUSE];
			}
			m_output[TRef][O_D_DAY_LENGTH] = m_weather.GetDayLength(TRef) / 3600.;
			m_output[TRef][O_D_NB_OBL] = TranosemaStat[0][TRef][S_NB_OBL];
			m_output[TRef][O_D_NB_OBL_L3D] = TranosemaStat[0][TRef][S_NB_OBL_L3D];
			m_output[TRef][O_D_NB_SBW] = TranosemaStat[0][TRef][S_NB_SBW];
			m_output[TRef][O_D_DIAPAUSE_AGE] = diapauseAge;
			 
		}


		return msg;
	}

	void CTranosema_OBL_SBW_Model::ExecuteDailyAllGenerations(vector<CModelStatVector>& stat)
	{
		//This is where the model is actually executed
		CTPeriod entirePeriod = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CSnowAnalysis snowA;

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//get the annual period 
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			CTRef TRef = snowA.GetLastSnowTRef(m_weather[y]).as(CTM::DAILY);
			if (!TRef.IsInit() || !m_bOnGround)
				TRef = p.Begin(); //no snow 


			//Create stand
			CTranosema_OBL_SBW_Stand stand(this);

			//OBL init
			std::shared_ptr<CHost> pHostOBL = make_shared<CHost>(&stand.m_OBLStand);
			pHostOBL->Initialize<CObliqueBandedLeafroller>(CInitialPopulation(p.Begin(), 0, 250, 100, OBL::L3D, RANDOM_SEX, true, 0));
			stand.m_OBLStand.m_host.push_front(pHostOBL);

			//SBW init
			stand.m_SBWStand.m_bFertilEgg = false;
			stand.m_SBWStand.m_bApplyAttrition = false;
			stand.m_SBWStand.m_defoliation = 0;
			stand.m_SBWStand.m_bStopL22 = true;
			std::shared_ptr<CSBWTree> pHostSBW = make_shared<CSBWTree>(&stand.m_SBWStand);
			pHostSBW->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, 250, 100, SBW::L2o, RANDOM_SEX, false, 0));
			stand.m_SBWStand.m_host.push_front(pHostSBW);

			//init tranosema

			//Create host
			std::shared_ptr<CTranosema_OBL_SBW_Host> pHostTranosema = make_shared<CTranosema_OBL_SBW_Host>(&stand);

			//Init host
			pHostTranosema->m_nbMinObjects = 100;
			pHostTranosema->m_nbMaxObjects = 1250;
			pHostTranosema->Initialize(CInitialPopulation(TRef, 0, 500, 100, m_diapauseAge, FEMALE, true, 0));
			//pHostTranosema->Initialize(CInitialPopulation(p.Begin(), 0, 1000, 100, m_diapauseAge, FEMALE, true, 0));
			

			//Init stand
			stand.m_bApplyAttrition = m_bHaveAttrition;
			stand.m_generationAttrition = m_generationAttrition;
			stand.m_bAutoComputeDiapause = false;
			stand.m_diapauseAge = m_diapauseAge;
			stand.m_criticalDaylength = m_criticalDaylength;
			stand.m_lethalTemp = m_lethalTemp;
			stand.m_host.push_front(pHostTranosema);


			
			//run the model for all days of all years
			for (CTRef d = p.Begin(); d <= p.End(); d++)
			{
				stand.Live(m_weather.GetDay(d));

				size_t nbGenerations = stand.GetFirstHost()->GetNbGeneration();
				if (nbGenerations > stat.size())
					stat.push_back(CModelStatVector(entirePeriod.GetNbRef(), entirePeriod.Begin(), NB_STATS_EX, 0));

				for (size_t g = 0; g < nbGenerations; g++)
					stand.GetStat(d, stat[g][d], g);

				stand.AdjustPopulation();
				HxGridTestConnection();
			}
		}
	}


	//************************************************************************************************
	ERMsg CTranosema_OBL_SBW_Model::OnExecuteAnnual()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		
		vector<CModelStatVector> TranosemaStat;
		ExecuteDailyAllGenerations(TranosemaStat);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_OUTPUT, 0, ANNUAL_HEADER);
		

		//now compute annual growth rates
		//Get last complete generation
		size_t maxG = min(NB_GENERATIONS, TranosemaStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));

				m_output[TRef][O_A_NB_GENERATION] = maxG;

				CStatistic meanG;
				CStatistic diapause;
				double diapauseBegin = 100;
				for (size_t g = 1; g < maxG; g++)
				{
					
					CStatistic diapauseStat = TranosemaStat[g].GetStat(E_DIAPAUSE, season);
					
					diapause += diapauseStat[SUM];
					meanG += g *diapauseStat[SUM];
					m_output[TRef][O_A_ALIVE1 + (g - 1)] = diapauseStat[SUM];
					

					if (g == maxG - 1)
					{
						ASSERT(diapause.IsInit());
						double diapauseEnd = TranosemaStat[g][season.End()][size_t(m_diapauseAge)];
						//m_output[TRef][O_A_GROW_GENERATION] = ;
						//m_output[TRef][O_A_FECONDITY] = ;
						m_output[TRef][O_A_GROWTH_RATE] = diapauseEnd / diapauseBegin;
						m_output[TRef][O_A_MEAN_GENERATION] = meanG[SUM] / diapause[SUM];
					}
				}

				
			}
		}

			

		return msg;
	}
	//************************************************************************************************
	ERMsg CTranosema_OBL_SBW_Model::OnExecuteAtemporal()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		
		vector<CModelStatVector> TranosemaStat;
		ExecuteDailyAllGenerations(TranosemaStat);
		

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p.size()*NB_GENERATIONS, CTRef(0,0,0,0,CTM(CTM::ATEMPORAL)), NB_GENERATION_OUTPUT, -999, GENERATION_HEADER);


		//now compute generation growth rates
		size_t maxG = min(NB_GENERATIONS, TranosemaStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				size_t y = TRef - p.Begin();
				CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));


				double eggsBegin = 100;
				double adultBegin = 0;
				for (size_t g = 0; g < maxG; g++)
				{
					size_t gg = y*NB_GENERATIONS + g;
					m_output[gg][O_G_YEAR] = TRef.GetYear();
					m_output[gg][O_G_GENERATION] = g;
					for (size_t i = 0; i < 4; i++)
					{
						if (g == 0 && i == 0)
						{
							m_output[gg][O_G_EGG] = 100;
						}
						else
						{
							CStatistic stat = TranosemaStat[g].GetStat(E_EGG + i, season);
							m_output[gg][O_G_EGG + i] = stat[SUM];
						}
					}

					CStatistic broodStat = TranosemaStat[g].GetStat(E_BROOD, season);
					ASSERT(broodStat.IsInit());
					m_output[gg][O_G_BROOD] = broodStat[SUM];
					
						
					CStatistic diapauseStat = TranosemaStat[g].GetStat(E_DIAPAUSE, season);
					ASSERT(diapauseStat.IsInit());
					m_output[gg][O_G_DIAPAUSE] = diapauseStat[SUM];

					CStatistic attritionStat = TranosemaStat[g].GetStat(E_ATTRITION, season);
					ASSERT(attritionStat.IsInit());
					m_output[gg][O_G_ATTRITION] = attritionStat[SUM];

					CStatistic frozenStat = TranosemaStat[g].GetStat(E_FROZEN, season);
					ASSERT(frozenStat.IsInit());
					m_output[gg][O_G_FROZEN] = frozenStat[SUM];

					CStatistic hostDieStat = TranosemaStat[g].GetStat(E_HOST_DIE, season);
					ASSERT(hostDieStat.IsInit());
					m_output[gg][O_G_HOST_DIE] = hostDieStat[SUM];

					if (m_output[gg][O_G_ADULT]>0)
						m_output[gg][O_G_FECONDITY] = m_output[gg][O_G_BROOD] / m_output[gg][O_G_ADULT];
					 
					m_output[gg][O_G_EGG_GROWTH] = (eggsBegin>0)?m_output[gg][O_G_BROOD] / eggsBegin : -999;
					m_output[gg][O_G_ADULT_GROW] = (adultBegin>0)?m_output[gg][O_G_ADULT] / adultBegin : -999;
					
					eggsBegin = m_output[gg][O_G_BROOD];
					adultBegin = m_output[gg][O_G_ADULT];

				}
			}
		}


		return msg;
	}
	

}