//*****************************************************************************
// Class: CTranosemaModel
//
// Description: CTranosemaModel is a BioSIM model of Tranosema
//*****************************************************************************
// 28/11/2017	1.2.0	Rémi Saint-Amant    New compilation
// 28/11/2017	1.1.9	Rémi Saint-Amant    Add Cumul Adult
// 14/11/2017	1.1.8	Rémi Saint-Amant    Change in annual behavior
// 04/05/2017	1.1.7	Rémi Saint-Amant    New hourly generation
// 25/05/2016	1.1.6	Rémi Saint-Amant	Some correction in annual model
// 17/05/2016	1.1.4	Rémi Saint-Amant	Add annual variables
// 22/01/2016	1.1.2	Rémi Saint-Amant	Add snomeld date as strarting date
// 22/01/2016	1.1.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 11/12/2015	1.0.0	Rémi Saint-Amant	Creation
//*****************************************************************************

#include "Basic/UtilStd.h"
#include "Basic/SnowAnalysis.h"
#include "ModelBase/EntryPoint.h"
//#include "..\SpruceBudworm\SpruceBudworm.h"
#include "TranosemaModel.h"
#include "Tranosema.h"



using namespace std;
using namespace WBSF::Tranosema;

namespace WBSF
{
	static const bool ACTIVATE_PARAMETRIZATION = false;
	static const size_t NB_GENERATIONS = 6;



	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CTranosemaModel::CreateObject);

	enum TDailyOutput{ O_D_EGG, O_D_PUPA, O_D_ADULT, O_D_DEAD_ADULT, O_D_OVIPOSITING_ADULT, O_D_BROOD, O_D_ATTRITION, O_D_CUMUL_REATCH_ADULT, O_D_CUMUL_DIAPAUSE, NB_DAILY_OUTPUT, O_D_DAY_LENGTH = NB_DAILY_OUTPUT*NB_GENERATIONS, NB_DAILY_OUTPUT_EX };
	extern char DAILY_HEADER[] = "Egg,Pupa,Adult,DeadAdult,OvipositingAdult,Brood,Attrition,CumulAdult";

	//	
	enum TAnnualOutput{ O_A_NB_GENERATION, O_A_MEAN_GENERATION, O_A_GROWTH_RATE, O_A_ALIVE1, NB_ANNUAL_OUTPUT = O_A_ALIVE1 + NB_GENERATIONS-1 };
	extern char ANNUAL_HEADER[] = "Gmax,MeanGeneration, GrowthRate,Alive1,Alive2,Alive3,Alive4,Alive5,Alive6";

	enum  TGenerationOutput { O_G_YEAR, O_G_GENERATION, O_G_DIAPAUSE, O_G_GROWTH_RATE, NB_GENERATION_OUTPUT};
	extern char GENERATION_HEADER[] = "Year,Generation,Diapause,GrowthRate";

	

	CTranosemaModel::CTranosemaModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 6;
		VERSION = "1.2.0 (2020)";

		// initialize your variables here (optimal values obtained by sensitivity analysis)
		m_bHaveAttrition = true;
		m_generationAttrition = 0.025;//Attrition survival (cull in the egg stage, before creation)
		m_diapauseAge = EGG + 0.0;
		m_lethalTemp = -5.;
		m_criticalDaylength = 13.5;
		m_bOnGround = FALSE;
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
		m_diapauseAge = parameters[c++].GetReal();
		m_lethalTemp = parameters[c++].GetReal();
		m_criticalDaylength = parameters[c++].GetReal();
		m_bOnGround = parameters[c++].GetBool();
		ASSERT(m_diapauseAge >= 0. && m_diapauseAge <= 1.);

		return msg;
	}

	//************************************************************************************************
	// Daily method
	//void CTranosemaModel::GetSpruceBudwormBiology(CWeatherStation& weather, CModelStatVector& stat)
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
	ERMsg CTranosemaModel::OnExecuteDaily()
	{
		ERMsg msg;
		
		//if daily data, compute sub-daily data
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();
		
		//Init spruce budworm data
		CModelStatVector SBWStat;
		
		//QUESTION pour SRA: on aurait accès à quoi au juste? J'ai enlevé les dépendances SBW du projet... faudrait les remettre.
		//QUESTION pour RSA: on ferait la même chose avec PBL?
		
		//GetSpruceBudwormBiology(m_weather, SBWStat);

		//one CModelStatVector by generation
		vector<CModelStatVector> TranosemaStat;
		ExecuteDailyAllGenerations(SBWStat, TranosemaStat);

		//merge generations vector into one output vector (max of 5 generations)
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		size_t maxG = min(NB_GENERATIONS, TranosemaStat.size()); 
		m_output.Init(p.size(), p.Begin(), NB_DAILY_OUTPUT_EX, 0, DAILY_HEADER);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			for (size_t g = 0; g < maxG; g++)
			{
				for (size_t s = 0; s < NB_DAILY_OUTPUT; s++)
					m_output[TRef][g*NB_DAILY_OUTPUT + s] = TranosemaStat[g][TRef][s];
			}
			m_output[TRef][O_D_DAY_LENGTH] = m_weather.GetDayLength(TRef) / 3600.;
		}


		return msg;
	}

	void CTranosemaModel::ExecuteDailyAllGenerations(CModelStatVector& /*SBWStat*/, vector<CModelStatVector>& stat)
	{
		//This is where the model is actually executed
		CTPeriod entirePeriod = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CSnowAnalysis snowA;

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//get the annual period 
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			CTRef TRef = snowA.GetLastSnowTRef(m_weather[y]);
			if (!TRef.IsInit() || !m_bOnGround)
				TRef = p.Begin(); //no snow 

			//get initial population 
			CInitialPopulation initialPopulation(TRef.Transform(CTM(CTM::DAILY)), 0, 1000, 100, m_diapauseAge, FEMALE, true, 0);

			//Create stand
			CTranosemaStand stand(this);

			//Create host
			CHostPtr pHost = make_shared<CHost>(&stand);

			//Init host
			pHost->m_nbMinObjects = 100;
			pHost->m_nbMaxObjects = 2500;
			pHost->Initialize<CTranosema>(initialPopulation);
			//double nbAlive = pHost->GetNbSpecimenAlive();

			//Init stand
			stand.m_bApplyAttrition = m_bHaveAttrition;
			stand.m_generationAttrition = m_generationAttrition;
			stand.m_diapauseAge = m_diapauseAge;
			stand.m_criticalDaylength = m_criticalDaylength;
			stand.m_lethalTemp = m_lethalTemp;
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

		vector<CModelStatVector> TranosemaStat;
		ExecuteDailyAllGenerations(SBWStat, TranosemaStat);

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
						m_output[TRef][O_A_GROWTH_RATE] = diapauseEnd / diapauseBegin;
						m_output[TRef][O_A_MEAN_GENERATION] = meanG[SUM] / diapause[SUM];
					}
				}

				
			}
		}

			

		return msg;
	}
	//************************************************************************************************
	ERMsg CTranosemaModel::OnExecuteAtemporal()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		//Init spruce budworm data
		CModelStatVector SBWStat;

		vector<CModelStatVector> TranosemaStat;
		ExecuteDailyAllGenerations(SBWStat, TranosemaStat);
		

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p.size()*(NB_GENERATIONS - 1), CTRef(0,0,0,0,CTM(CTM::ATEMPORAL)), NB_GENERATION_OUTPUT,-999, GENERATION_HEADER);


		//now compute generation growth rates
		size_t maxG = min(NB_GENERATIONS, TranosemaStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				double diapauseBegin = 100;
				for (size_t g = 1; g < maxG; g++)
				{
					CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));

					CStatistic diapauseStat = TranosemaStat[g].GetStat(E_DIAPAUSE, season);
					if (diapauseStat.IsInit() && diapauseBegin>0)
					{
						size_t y = TRef - p.Begin();
						size_t gg = y*(NB_GENERATIONS-1) + (g - 1);
						m_output[gg][O_G_YEAR] = TRef.GetYear();
						m_output[gg][O_G_GENERATION] = g;
						m_output[gg][O_G_DIAPAUSE] = diapauseStat[SUM];
						m_output[gg][O_G_GROWTH_RATE] = diapauseStat[SUM] / diapauseBegin;

						diapauseBegin = diapauseStat[SUM];
					}
				}
			}
		}


		return msg;
	}
	//************************************************************************************************


}