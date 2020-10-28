//*****************************************************************************
// Class: CMeteorusTrachynotusOBLModel
//
// Description: CMeteorusTrachynotus_OBL_SBW_Model is a BioSIM model of MeteorusTrachynotus and ObliqueBandedLeafroller model
//*****************************************************************************
// 28/10/2020	1.0.2	Rémi Saint-Amant	Add SBW rpoportion as input parameters
// 26/10/2020	1.0.1	Rémi Saint-Amant	Add 
// 27/02/2020	1.0.0	Rémi Saint-Amant	Creation
//*****************************************************************************

#include "Basic/UtilStd.h"
#include "Basic/SnowAnalysis.h"
#include "ModelBase/EntryPoint.h"
#include "MeteorusTrachynotus_OBL_SBW_Model.h"



using namespace std;
using namespace WBSF::MeteorusTrachynotus;

namespace WBSF
{
	static const bool ACTIVATE_PARAMETRIZATION = false;
	static const size_t NB_GENERATIONS = 5;



	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CMeteorusTrachynotus_OBL_SBW_Model::CreateObject);

	
	enum TDailyOutput { O_D_IMMATURE, O_D_PUPA, O_D_ADULT, O_D_DEAD_ADULT, O_D_OVIPOSITING_ADULT, O_D_BROOD_OBL, O_D_BROOD_SBW, O_D_ATTRITION, O_D_CUMUL_REATCH_ADULT, O_D_CUMUL_DIAPAUSE, O_D_TOTAL, NB_DAILY_OUTPUT, O_D_DAY_LENGTH = NB_DAILY_OUTPUT * NB_GENERATIONS, O_D_NB_OBL, O_D_NB_OBL_L3D, O_D_NB_SBW, O_D_DIAPAUSE_AGE, O_SBW_WITH_METEO, NB_DAILY_OUTPUT_EX };
	enum TGenerationOutput{ O_G_YEAR, O_G_GENERATION, O_G_ADULTS, O_G_BROODS_OBL, O_G_BROODS_SBW, O_G_DIAPAUSE, O_G_FECONDITY, O_G_GROWTH_RATE, O_G_DEAD_ADULT, O_G_HOST_DIE, O_G_FROZEN, NB_GENERATION_OUTPUT };
	enum TAnnualOutput{ O_A_NB_GENERATION, O_A_ADULTS, O_A_BROODS_OBL, O_A_BROODS_SBW, O_A_DIAPAUSE, O_A_FECONDITY, O_A_GROWTH_RATE, O_A_DEAD_ADULT, O_A_HOST_DIE, O_A_FROZEN, NB_ANNUAL_OUTPUT };

	CMeteorusTrachynotus_OBL_SBW_Model::CMeteorusTrachynotus_OBL_SBW_Model()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL 
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 7;
		VERSION = "1.0.2 (2020)";

		// initialize your variables here (optimal values obtained by sensitivity analysis)
		m_generationAttrition = 0.01;//Attrition survival (cull in the egg stage, before creation)
		m_lethalTemp = -5.0;
		m_criticalDaylength = 13.5;
		m_preOvip = 3.0 / 22.4;
		m_bOBLAttition = false;
		m_bSBWAttition = false;
		m_pSBW = 0.5; //proportion of SBW relatively to OBL
	}

	CMeteorusTrachynotus_OBL_SBW_Model::~CMeteorusTrachynotus_OBL_SBW_Model()
	{
	}

	//************************************************************************************************

	//this method is call to load your parameter in your variable
	ERMsg CMeteorusTrachynotus_OBL_SBW_Model::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_generationAttrition = parameters[c++].GetReal();
		m_lethalTemp = parameters[c++].GetReal();
		m_criticalDaylength = parameters[c++].GetReal();
		m_preOvip = parameters[c++].GetReal();
		m_bOBLAttition = parameters[c++].GetBool();
		m_bSBWAttition = parameters[c++].GetBool();
		m_pSBW = parameters[c++].GetReal();

		ASSERT(m_pSBW >= 0 && m_pSBW <= 1);

		return msg;
	}

	
	//************************************************************************************************
	// Daily method
	
	//This method is called to compute the solution
	ERMsg CMeteorusTrachynotus_OBL_SBW_Model::OnExecuteDaily()
	{
		ERMsg msg;
		


		//if daily data, compute sub-daily data
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();
		
		
		//one CModelStatVector by generation
		vector<CModelStatVector> MeteorusTrachynotusStat;
		ExecuteDailyAllGenerations(MeteorusTrachynotusStat);

		//merge generations vector into one output vector (max of 5 generations)
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		size_t maxG = min(NB_GENERATIONS, MeteorusTrachynotusStat.size()); 
		m_output.Init(p.size(), p.Begin(), NB_DAILY_OUTPUT_EX, 0);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			CStatistic diapauseAge;
			for (size_t g = 0; g < maxG; g++)
			{
				for (size_t s = 0; s < NB_DAILY_OUTPUT; s++)
					m_output[TRef][g*NB_DAILY_OUTPUT + s] = MeteorusTrachynotusStat[g][TRef][s];

				if (MeteorusTrachynotusStat[g][TRef][M_DIAPAUSE] > 0)
					diapauseAge += MeteorusTrachynotusStat[g][TRef][M_DIAPAUSE_AGE] / MeteorusTrachynotusStat[g][TRef][M_DIAPAUSE];

				m_output[TRef][O_SBW_WITH_METEO] += MeteorusTrachynotusStat[g][TRef][S_SBW_WITH_METEO];
			}


			m_output[TRef][O_D_DAY_LENGTH] = m_weather.GetDayLength(TRef) / 3600.;
			m_output[TRef][O_D_NB_OBL] = MeteorusTrachynotusStat[0][TRef][S_NB_OBL];
			m_output[TRef][O_D_NB_OBL_L3D] = MeteorusTrachynotusStat[0][TRef][S_NB_OBL_L3D];
			m_output[TRef][O_D_NB_SBW] = MeteorusTrachynotusStat[0][TRef][S_NB_SBW];
			m_output[TRef][O_D_DIAPAUSE_AGE] = diapauseAge;
			
			
			 
		}


		return msg;
	}

	void CMeteorusTrachynotus_OBL_SBW_Model::ExecuteDailyAllGenerations(vector<CModelStatVector>& stat)
	{
		//This is where the model is actually executed
		CTPeriod entirePeriod = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CSnowAnalysis snowA;

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//get the annual period 
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

			//Create stand
			CMeteorusTrachynotus_OBL_SBW_Stand stand(this);
			stand.m_pSBW = m_pSBW;

			//OBL init
			std::shared_ptr<CHost> pHostOBL = make_shared<CHost>(&stand.m_OBLStand);
			pHostOBL->Initialize<CObliqueBandedLeafroller>(CInitialPopulation(p.Begin(), 0, 250, 100, OBL::L3D, RANDOM_SEX, true, 0));
			stand.m_OBLStand.m_bApplyAttrition = m_bOBLAttition;
			stand.m_OBLStand.m_host.push_front(pHostOBL);

			//SBW init
			std::shared_ptr<CSBWTree> pHostSBW = make_shared<CSBWTree>(&stand.m_SBWStand);
			pHostSBW->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, 250, 100, SBW::L2o, RANDOM_SEX, false, 0));
			stand.m_SBWStand.m_bApplyAttrition = m_bSBWAttition;
			stand.m_SBWStand.m_host.push_front(pHostSBW);

			//init MeteorusTrachynotus
			std::shared_ptr<CMeteorusTrachynotus_OBL_SBW_Host> pHostMeteorusTrachynotus = make_shared<CMeteorusTrachynotus_OBL_SBW_Host>(&stand);

			//Init host
			pHostMeteorusTrachynotus->m_nbMinObjects = 100;
			pHostMeteorusTrachynotus->m_nbMaxObjects = 1250;
			pHostMeteorusTrachynotus->Initialize(CInitialPopulation(p.Begin(), 0, 500, 100, IMMATURE, FEMALE, true, 0));
			stand.m_host.push_front(pHostMeteorusTrachynotus);

			//Init stand
			stand.m_generationAttrition = m_generationAttrition;
			stand.m_criticalDaylength = m_criticalDaylength;
			stand.m_lethalTemp = m_lethalTemp;
			stand.m_preOvip = m_preOvip;
			
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
	ERMsg CMeteorusTrachynotus_OBL_SBW_Model::OnExecuteAnnual()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		
		vector<CModelStatVector> MeteorusTrachynotusStat;
		ExecuteDailyAllGenerations(MeteorusTrachynotusStat);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_OUTPUT, 0);
		

		//now compute annual growth rates
		//Get last complete generation
		size_t maxG = min(NB_GENERATIONS, MeteorusTrachynotusStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));
				m_output[TRef][O_A_NB_GENERATION] = maxG;
				
				for (size_t g = 0; g < maxG; g++)
				{
					static const size_t VAR_IN[7] = { M_ADULT,  S_BROODS_OBL,  S_BROODS_SBW, M_DIAPAUSE, M_DEAD_ADULT, M_HOST_DIE, M_FROZEN};
					static const size_t VAR_OUT[7] = { O_A_ADULTS, O_A_BROODS_OBL, O_A_BROODS_SBW, O_A_DIAPAUSE, O_A_DEAD_ADULT, O_A_HOST_DIE, O_A_FROZEN};
					for (size_t i = 0; i < 7; i++)
					{
						CStatistic stat = MeteorusTrachynotusStat[g].GetStat(VAR_IN[i], season);
						ASSERT(stat.IsInit());
						m_output[TRef][VAR_OUT[i]] += stat[SUM];
					}
				}
				
				if (m_output[TRef][O_A_ADULTS] > 0)
					m_output[TRef][O_A_FECONDITY] = (m_output[TRef][O_A_BROODS_OBL] + m_output[TRef][O_A_BROODS_SBW]) / m_output[TRef][O_A_ADULTS];

				m_output[TRef][O_A_GROWTH_RATE] = m_output[TRef][O_A_DIAPAUSE] / 100;
			}
		}

			

		return msg;
	}
	//************************************************************************************************
	ERMsg CMeteorusTrachynotus_OBL_SBW_Model::OnExecuteAtemporal()
	{
		ERMsg msg;

		if (m_weather.IsDaily())//if daily data, compute sub-daily data
			m_weather.ComputeHourlyVariables();

		
		vector<CModelStatVector> MeteorusTrachynotusStat;
		ExecuteDailyAllGenerations(MeteorusTrachynotusStat);
		

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p.size()*NB_GENERATIONS, CTRef(0,0,0,0,CTM(CTM::ATEMPORAL)), NB_GENERATION_OUTPUT, -999, "");


		//now compute generation growth rates
		size_t maxG = min(NB_GENERATIONS, MeteorusTrachynotusStat.size());
		if (maxG > 0)
		{
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				size_t y = TRef - p.Begin();
				CTPeriod season(CTRef(TRef.GetYear(), FIRST_MONTH, FIRST_DAY), CTRef(TRef.GetYear(), LAST_MONTH, LAST_DAY));

				for (size_t g = 0; g < maxG; g++)
				{
					size_t gg = y*NB_GENERATIONS + g;
					m_output[gg][O_G_YEAR] = TRef.GetYear();
					m_output[gg][O_G_GENERATION] = g;

					static const size_t VAR_IN[7] = { M_ADULT,  S_BROODS_OBL,  S_BROODS_SBW, M_DIAPAUSE, M_DEAD_ADULT, M_HOST_DIE, M_FROZEN};
					static const size_t VAR_OUT[7] = { O_G_ADULTS, O_G_BROODS_OBL, O_G_BROODS_SBW, O_G_DIAPAUSE, O_G_DEAD_ADULT, O_G_HOST_DIE, O_G_FROZEN };
					for (size_t i = 0; i < 7; i++)
					{
						CStatistic stat = MeteorusTrachynotusStat[g].GetStat(VAR_IN[i], season);
						ASSERT(stat.IsInit());
						m_output[gg][VAR_OUT[i]] = stat[SUM];
					}

					if (m_output[gg][O_G_ADULTS]>0)
						m_output[gg][O_G_FECONDITY] = (m_output[gg][O_G_BROODS_OBL]+ m_output[gg][O_G_BROODS_SBW]) / m_output[gg][O_G_ADULTS];

					m_output[gg][O_G_GROWTH_RATE] = m_output[gg][O_G_DIAPAUSE] / 100;
				}
			}
		}


		return msg;
	}
	

}