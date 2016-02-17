//*****************************************************************************
// Individual-based model of Spruce Bark Beetle (SBB)
// 
// Rémi Saint-Amant
// Canadian Forest Service
// 
//*****************************************************************************
//*****************************************************************************
// File: WSBModel.cpp
//
// Class: CSBBModel
//
// Description: CSBBModel is a individual base BioSIM model that computes European Spruce Bark Beetle 
//              seasonal biology. 
//
//*****************************************************************************
// 22/01/2016	1.1.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 30/09/2013	1.0.0	Rémi Saint-Amant    Creation
//*****************************************************************************

#include "basic/timeStep.h"
#include "basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include "ModelBase/SnowMelt.h"
#include "SBBEquations.h"
#include "SBBModel.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

//uncomment this line to activate version for simulated annealing
static const bool ACTIVATE_PARAMETRIZATION = false;
static const bool P1_ONLY = false;
static CCriticalSection CS;




//**************************
//this line link this model with the EntryPoint of the DLL
static const bool bRegistred = 
	CModelFactory::RegisterModel( CSBBModel::CreateObject );



enum TOuput{	O_ADULT_0, O_DEAD_ADULT_0, O_EMERGENCE_0, O_SWARMING_0_P1, O_SWARMING_0_P2, O_SWARMING_0_P3, O_TOTAL_FEMALE_0, O_BROOD_0_P1, O_BROOD_0_P2, O_BROOD_0_P3, O_TOTAL_BROOD_0, O_DIAPAUSE_0,
				O_EGG_1, O_L1_1, O_L2_1, O_L3_1, O_PUPAE_1, O_TENERAL_ADULT_1, O_ADULT_1, O_DEAD_ADULT_1, O_SWARMING_1_F1_i, O_SWARMING_1_F1_ii, O_SWARMING_1_F1_iii, O_SWARMING_1_F2_i, O_SWARMING_1_F2_ii, O_SWARMING_1_F2_iii, O_SWARMING_1_F3_i, O_SWARMING_1_F3_ii, O_SWARMING_1_F3_iii, O_TOTAL_FEMALE_1, O_BROOD_1, O_TOTAL_BROOD_1, O_DIAPAUSE_1,
				//O_EGG_2, O_L1_2, O_L2_2, O_L3_2, O_PUPAE2, O_TENERAL_ADULT_2, O_ADULT_2, O_DEAD_ADULT_2, O_SWARMING_2, O_TOTAL_FEMALE_2, O_BROOD_2, O_TOTAL_BROOD_2, O_DIAPAUSE_2,
				O_EGG_2, O_L1_2, O_L2_2, O_L3_2, O_PUPAE_2, O_TENERAL_ADULT_2, O_ADULT_2, 
				O_DEAD_ATTRITION, O_DEAD_FROZEN, O_DEAD, 
				O_W_STAT, O_DT_STAT, O_DDL_STAT, O_DAY_LENGTH, O_DI50, O_T_MEAN, O_TI50, 
				O_SWARMING_0_P2_MALE, O_SWARMING_0_P2_FEMALE,
				NB_OUTPUT
			};

typedef CModelStatVectorTemplate<NB_OUTPUT> CDailyOutputVector;

enum TOuputA{ O_A_SWARMING_OBS, O_A_SWARMING_SIM, O_A_SWARMING, O_A_DIAPAUSE, O_A_W_STAT, O_A_DT_STAT, O_A_DDL_STAT, O_A_DAY_LENGTH,  O_A_DI50, O_A_T_MEAN, O_A_TI50, NB_OUTPUT_A};
typedef CModelStatVectorTemplate<NB_OUTPUT_A> CAnnualOutputVector;


enum TData{ OBS_ID, LONG_DAY_L, SHORT_DAY_L, I_TNIGHT, I_TDAY, FIRST_STAGE_SHORT_DAY, LAST_STAGE_SHORT_DAY, NB_SHORT_DAY, DIAPAUSE, NB_DIAPAUSE, NB_BUGS=DIAPAUSE, NB_DEAD, SWARMING_DATE, SWARMING_SD, NB_SWARMING};
static std::vector<CWeatherStation> WEATHER_ARRAY;
static const double OBSERVATION_SWARMING_W[20][NB_SWARMING] = 
//Long day length	Short day length	T day	T night Number of insects	% dead		Swarming 
//																				time in days
{
	{ 1, 17, 17, 25.0, 25.0,   EGG, ADULT,  0,  64, 40, 41.7, 0},
	{ 2, 12, 12, 25.0, 25.0,   EGG, ADULT,  0,  92, 29, 42.0, 0},
	{ 3, 13, 13, 13.0, 26.0,   EGG, ADULT,  0, 456, 23, 49.1, 0},
	{ 4, 18, 18, 20.0, 20.0,   EGG, ADULT,  0,  94, 28, 50.9, 0},
	{ 5, 12, 12, 23.0, 23.0,   EGG, ADULT,  0, 265, 24, 52.3, 0},
	{ 6, 18, 12, 20.0, 20.0,   EGG,    L2, 16, 326, 16, 55.9, 0},
	{ 7, 18, 12, 20.0, 20.0,   2.5,   3.5, 10, 220, 16, 56.8, 0},
	{ 8, 18, 12, 20.0, 20.0,    L3,    L3, 10, 280, 16, 64.3, 0},
	{ 9, 18, 12, 20.0, 20.0, PUPAE, PUPAE,  6, 335, 22, 69.7, 0},
	{10, 16, 16,  6.0, 26.0,   EGG, ADULT,  0, 182, 10, 70.1, 0},
	{11, 18, 12, 20.0, 20.0,   EGG,    L3, 25, 335, 20, 72.7, 0},
	{12, 17, 17,  6.0, 20.0,   EGG, ADULT,  0, 336, 20,120.5, 0},
	{13, 18, 12, 20.0, 20.0,    L3, PUPAE, 11, 169, 23,365.0, 0},
	{14, 18, 12, 20.0, 20.0, PUPAE, ADULT, 15, 214, 33,365.0, 0},
	{15, 18, 12, 20.0, 20.0,    L3, ADULT, 21, 158, 21,365.0, 0},
	{16, 12, 12, 20.0, 20.0,   EGG, ADULT,  0,  0,   0,365.0, 0},
	{17, 12, 12, 22.0, 22.0,   EGG, ADULT,  0,  82, 26,365.0, 0},
	{18, 13, 13, 20.0, 20.0,   EGG, ADULT,  0,  0,   0,365.0, 0},
	{19, 14, 14, 20.0, 20.0,   EGG, ADULT,  0,  0,   0,365.0, 0},
	{20, 14, 14,  6.0, 26.0,   EGG, ADULT,  0, 117, 64,365.0, 0},
};



static const double OBSERVATION_DIAPAUSE[24][NB_DIAPAUSE] = 
	//Exposure period			 L:D		T	mean T	Number of insects	Diapause  Swarming 
//														(% dead)*					time in days
{
	{ 1, 15.0, 15.0, 20.0, 20.0,   EGG, ADULT,  0,  -1},
	{ 2, 15.5, 15.5, 20.0, 20.0,   EGG, ADULT,  0,  -1},
	{ 3, 16.0, 16.0, 20.0, 20.0,   EGG, ADULT,  0,  -1},
	{ 4, 17.0, 17.0, 25.0, 25.0,   EGG, ADULT,  0,  -1},
	{ 5, 12.0, 12.0, 25.0, 25.0,   EGG, ADULT,  0,  -1},
	{ 6, 13.0, 13.0, 13.0, 26.0,   EGG, ADULT,  0,  -1},
	{ 7, 18.0, 18.0, 20.0, 20.0,   EGG, ADULT,  0,  -1},
	{ 8, 12.0, 12.0, 23.0, 23.0,   EGG, ADULT,  0,  -1},
	{ 9, 18.0, 12.0, 20.0, 20.0,   EGG,    L2, 16,  -1},
	{10, 18.0, 12.0, 20.0, 20.0,   2.5,   3.5, 10,  -1},
	{11, 18.0, 12.0, 20.0, 20.0,    L3,    L3, 10,  -1},
	{12, 18.0, 12.0, 20.0, 20.0, PUPAE, PUPAE,  6,  -1},
	{13, 16.0, 16.0,  6.0, 26.0,   EGG, ADULT,  0,  -1},
	{14, 18.0, 12.0, 20.0, 20.0,   EGG,    L3, 25,  -1},
	{15, 17.0, 17.0,  6.0, 20.0,   EGG, ADULT,  0,  -1},
	{16, 18.0, 12.0, 20.0, 20.0,    L3, PUPAE, 11,   1},
	{17, 18.0, 12.0, 20.0, 20.0, PUPAE, ADULT, 15,   1},
	{18, 18.0, 12.0, 20.0, 20.0,    L3, ADULT, 21,   1},
	{19, 12.0, 12.0, 20.0, 20.0,   EGG, ADULT,  0,   1},
	{20, 12.0, 12.0, 22.0, 22.0,   EGG, ADULT,  0,   1},
	{21, 13.0, 13.0, 20.0, 20.0,   EGG, ADULT,  0,   1},
	{22, 14.0, 14.0, 20.0, 20.0,   EGG, ADULT,  0,   1},
	{23, 14.0, 14.0,  6.0, 26.0,   EGG, ADULT,  0,   1},
	{24, 14.5, 14.5, 20.0, 20.0,   EGG, ADULT,  0,   1}	
};

void CSBBModel::ExecuteDailyTest(CModelStatVector& output)
{
	//gTimeStep.Set(1);

	//output.resize(365);
	//output.SetFirstTRef(CTRef(0,0,0));
	//
	//
	//int locNo = ToInt(m_info.m_loc.m_ID)-1;
	//const double* OBSERVATION = m_info.m_locCounter.GetTotal()==24?&(OBSERVATION_DIAPAUSE[locNo][OBS_ID]):&(OBSERVATION_SWARMING_W[locNo][OBS_ID]);
	//	
	//CS.Enter();
	//if( WEATHER_ARRAY.empty())
	//	WEATHER_ARRAY.resize(m_info.m_locCounter.GetTotal());
	//
	//if( WEATHER_ARRAY[locNo].GetNbYears() == 0)
	//{
	//	CWeatherDay wDay;
	//	wDay(DAILY_DATA::TMIN) = OBSERVATION[T_NIGHT];
	//	wDay(DAILY_DATA::TMAX) = OBSERVATION[T_DAY];
	//
	//	WEATHER_ARRAY[locNo].AddYear(0);
	//	for(CTRef d=CTRef(0, FIRST_MONTH, FIRST_DAY); d<=CTRef(0, LAST_MONTH, LAST_DAY); d++)
	//	{
	//		WEATHER_ARRAY[locNo].SetData(d, wDay);
	//	}
	//}
	//CS.Leave();

	//CSpruceBarkBeetleStatVector simStat;

	//
	//Simulate( locNo, OBSERVATION[LONG_DAY_L], OBSERVATION[SHORT_DAY_L], OBSERVATION[T_NIGHT], OBSERVATION[T_DAY], OBSERVATION[FIRST_STAGE_SHORT_DAY], OBSERVATION[LAST_STAGE_SHORT_DAY], (int)OBSERVATION[NB_SHORT_DAY], simStat);
	//ComputeRegularStat(simStat, output);
}



CSBBModel::CSBBModel()
{

	//**************************
	//NB_INPUT_PARAMETER is used to determine if the dll
	//uses the same number of parameters than the model interface

	NB_INPUT_PARAMETER = ACTIVATE_PARAMETRIZATION?4+NB_STAGES+5+5+1:3;

	VERSION = "1.1.0 (2016)";

	m_bApplyMortality = true;
	m_bFertilEgg=false;	//If female is fertile, eggs will be added to the development
	m_bAutoBalanceObject = true;
	m_survivalRate=100;

	//developer parameters
	m_initialPopulation=100;	
	m_bApplyAttrition = true;
	m_bApplyWinterMortality = true;
	

	m_nbObjects = 400;       //Number of females in the initial attack 
	m_nbMinObjects = 100;
	m_nbMaxObjects = 1000;
	m_bCumulatif=false;

	//Simulated Annealing data
	m_dataType = DATA_UNKNOWN;
	m_bInit = false;
	
	for(size_t i=0; i<m_p.size(); i++)
		m_p[i] = 0;
		
	for(size_t i=0; i<m_k.size(); i++)
		m_k[i] = 0;
		
	for(int i=0; i<NB_STAGES; i++)
		m_s[i] = 0;

	m_swarmingNo=-1;
	m_nbReemergeMax=3;

	m_firstJday.fill(999);
	m_lastJday.fill(-999);
}

CSBBModel::~CSBBModel()
{
}


ERMsg CSBBModel::OnExecuteDaily()
{
	ERMsg msg;

	//Actual model execution
	//CSnowMelt snow;
	//snow.SetLon(-120);
	//snow.Compute(m_weather);
	
	//CTPeriod p = m_weather.GetEntireTPeriod()
	//for(CTRef d=p.Begin(); d<=p.End(); d++)
	//{
	//	CWeatherDay day = m_weather[d];
	//	day(SNOW) = snow.GetResult()[d].m_hs;//mm
	//	day(SNDH) = snow.GetResult()[d].m_hs;//cm
	//	m_weather.SetData(d,day);
	//}


	CSpruceBarkBeetleStatVector stat;
	GetDailyStat(stat);

	//CDailyOutputVector output;
	ComputeRegularStat(stat, m_output);
	//SetOutput(output);

	
	

	//CDailyOutputVector output;
	//ExecuteDailyTest(output);
	//SetOutput(output);
 

	return msg;
}


ERMsg CSBBModel::OnExecuteAnnual()
{
	ERMsg msg;

	//CAnnualOutputVector output(1,CTRef(0,0,0));


	//CDailyOutputVector stat;
	//ExecuteDailyTest(stat);


	//CTRef May30(0,MAY,LAST_DAY);
	//CTPeriod p(CTRef(0,FIRST_MONTH,FIRST_DAY), May30);

	//
	//double sumDiapause = stat.GetStat(S_DIAPAUSE_1)[SUM];
	//double sumSwarming = stat.GetStat(S_SWARMING_1)[SUM];
	//	
	//int locNo = m_info.m_loc.GetID().ToInt()-1;
	//if( m_info.m_locCounter.GetTotal()==24 )
	//{
	//	output[0][O_A_SWARMING_OBS] = OBSERVATION_DIAPAUSE[locNo][DIAPAUSE];
	//	output[0][O_A_SWARMING_SIM] = sumDiapause/50-1;
	//}
	//else
	//{
	//	double swarmingJday = 365;
	//	if( sumSwarming>50 )
	//	{
	//		CStatistic swarmingStat;
	//		for(CTRef d=p.Begin(); d<=p.End(); d++)
	//			swarmingStat += d.GetJDay()*stat[d][S_SWARMING_1];

	//		swarmingJday = swarmingStat[SUM]/sumSwarming;
	//		ASSERT( swarmingJday>=0 && swarmingJday<=360);
	//	}

	//	output[0][O_A_SWARMING_OBS] = OBSERVATION_SWARMING_W[locNo][SWARMING_DATE];
	//	output[0][O_A_SWARMING_SIM] = swarmingJday;
	//}
	//
	//
	//	
	//output[0][O_A_SWARMING] = sumSwarming;
	//output[0][O_A_DIAPAUSE] = sumDiapause;
	//output[0][O_A_W_STAT] = stat[May30][O_W_STAT];
	//output[0][O_A_DT_STAT] = stat[May30][O_DT_STAT];
	//output[0][O_A_DDL_STAT] = stat[May30][O_DDL_STAT];
	//output[0][O_A_DAY_LENGTH] = stat[May30][O_DAY_LENGTH];
	//output[0][O_A_DI50] = stat[May30][O_DI50];
	//output[0][O_A_T_MEAN] = stat[May30][O_T_MEAN];
	//output[0][O_A_TI50] = stat[May30][O_TI50];
	//

	//SetOutput(output);

	//Actual model execution
	CSpruceBarkBeetleStatVector stat;
	GetDailyStat(stat);

	CAnnualOutputVector output(m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL)));

	for(size_t y=0; y<m_weather.size(); y++)
	{
		CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
		//int year = m_weather[y].GetTRef().GetYear();
		//CTPeriod p(CTRef(year, FIRST_MONTH, FIRST_DAY), CTRef(year, LAST_MONTH, LAST_DAY));

		double sumDiapause = stat.GetStat(S_DIAPAUSE_1, p)[SUM];
		double sumSwarming = stat.GetStat(S_SWARMING_1_F1_i+m_swarmingNo, p)[SUM];
		
		double swarmingJday = 365;
		if( sumSwarming>0 )
		{
			CStatistic swarmingStat;
			for(CTRef d=p.Begin(); d<=p.End(); d++)
				swarmingStat += d.GetJDay()*stat[d][S_SWARMING_1_F1_i+m_swarmingNo];

			swarmingJday = swarmingStat[SUM]/sumSwarming;
			ASSERT( swarmingJday>=0 && swarmingJday<=360);
		}

		output[y][O_A_SWARMING_OBS] = swarmingJday;
		output[y][O_A_SWARMING_SIM] = swarmingJday;
		
		output[0][O_A_SWARMING] = sumSwarming;
		output[0][O_A_DIAPAUSE] = sumDiapause;
		output[0][O_A_W_STAT] = stat[p.End()][O_W_STAT];
		output[0][O_A_DT_STAT] = stat[p.End()][O_DT_STAT];
		output[0][O_A_DDL_STAT] = stat[p.End()][O_DDL_STAT];
		output[0][O_A_DAY_LENGTH] = stat[p.End()][O_DAY_LENGTH];
		output[0][O_A_DI50] = stat[p.End()][O_DI50];
		output[0][O_A_T_MEAN] = stat[p.End()][O_T_MEAN];
		output[0][O_A_TI50] = stat[p.End()][O_TI50];
	}
	

	SetOutput(output);


	//if( m_weather.GetNbYear() > 1)
	//{
	//	//fill ouptut matrix
	//	CAnnualOutputVector output(m_weather.GetNbYear()-1, CTRef(m_weather[1].GetYear()) );
	//	
	//	for(int y=0; y<m_weather.GetNbYear()-1; y++)
	//	{
	//		CStatistic s = stat.GetStat( E_L2o2, m_weather[y+1].GetTPeriod() );
	//		output[y][O_GROWING_RATE] = s[SUM]/100; 
	//	}

	//	SetOutput(output);
	//}

	//
	
 

	return msg;
}

void CSBBModel::GetDailyStat(CModelStatVector& stat)
{
	if (m_weather.IsDaily())
		m_weather.ComputeHourlyVariables();

	//This is where the model is actually executed
	stat.Init(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)), NB_STAT);

	
	//we simulate 2 years at a time. 
	//we also manager the possibility to have only one year
	for(size_t y1=0; y1<m_weather.size(); y1++)
	{
		CTPeriod p = m_weather[y1].GetEntireTPeriod(CTM(CTM::DAILY));
		//Create stand
		CSpruceBarkBeetleStand stand(this);
		stand.m_bFertilEgg = m_bFertilEgg;
		stand.m_survivalRate = m_survivalRate;

		stand.m_p = m_p;
		stand.m_k = m_k;
		stand.m_s = m_s;

		stand.m_bApplyAttrition = m_bApplyMortality&&m_bApplyAttrition;
		stand.m_bApplyWinterMortality = m_bApplyMortality&&m_bApplyWinterMortality;
		stand.m_nbReemergeMax = m_nbReemergeMax;
		
		//Create the initial attack
		CHostPtr pTree = make_shared<CSpruceBarkBeetleTree>(&stand);
		pTree->m_nbMinObjects = m_nbMinObjects;
		pTree->m_nbMaxObjects = m_nbMaxObjects;
		pTree->Initialize<CSpruceBarkBeetle>(CInitialPopulation(p.Begin(), m_nbObjects, m_initialPopulation, ADULT, NOT_INIT, true));
	
		stand.m_host.push_back(pTree);

		int nbYear = m_bFertilEgg?2:1;
		for(size_t y=0; y<nbYear && y1+y<m_weather.size(); y++)
		{
			size_t yy=y1+y; 

			CTPeriod pp = m_weather[yy].GetEntireTPeriod(CTM(CTM::DAILY));
			for(CTRef d=pp.Begin(); d<=pp.End(); d++)
			{
				stand.Live(m_weather.GetDay(d));
				stand.GetStat(d, stat[d]);

				
				if(P1_ONLY && stat[d][S_DEAD_ADULT_0]==100)//temporaire
					d=pp.End();

				//if( m_bAutoBalanceObject )
				stand.AdjustPopulation();

				HxGridTestConnection();
			}
			stand.HappyNewYear();
		}
	}
}

void CSBBModel::ComputeRegularStat(CModelStatVector& stat, CModelStatVector& output)
{
	ASSERT( O_DEAD == S_DEAD );
	if( output.empty() )
	{
		output.resize(stat.size());
		output.SetFirstTRef(stat.GetFirstTRef());
	}

	for (CTRef d=stat.GetFirstTRef(); d<=stat.GetLastTRef(); d++)
	{
		for(int i=0; i<NB_OUTPUT; i++)
			output[d][i] = stat[d][i];
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


//**************************
//this method is called to load parameters in variables
ERMsg CSBBModel::ProcessParameter(const CParameterVector& parameters)
{
	ASSERT( m_weather.GetNbYear() > 0);

    ERMsg msg;

	int c = 0;

	m_bApplyMortality = parameters[c++].GetBool();
	m_bFertilEgg = parameters[c++].GetBool();
	m_survivalRate = parameters[c++].GetReal()/100;
	
	if(ACTIVATE_PARAMETRIZATION)
	{
		for(size_t i=0; i<m_p.size(); i++)
			m_p[i] = parameters[c++].GetReal();
		
		for(size_t i=0; i<m_k.size(); i++)
			m_k[i] = parameters[c++].GetReal();
		
		for(int i=0; i<NB_STAGES; i++)
			m_s[i] = parameters[c++].GetReal();
	
		m_swarmingNo = parameters[c++].GetInt()-1;
		m_nbReemergeMax=parameters[c++].GetInt();

		//for(int s=0; s<NB_STAGES; s++)
			//m_rho25Factor[s]=parameters[c++].GetReal();
	}

	return msg;
}



//simulated annaling 
void CSBBModel::AddDailyResult(const StringVector& header, const StringVector& data)
{
	if( data.size() == 8||data.size() == 9)
	{
		std::vector<double> obs(1);
	
		CTRef ref(ToInt(data[2]), ToInt(data[3]) - 1, ToInt(data[4]) - 1);
		obs[0] = ToDouble(data[6]);

		m_SAResult.push_back( CSAResult(ref, obs) );
	}
	else if ( data.size() == 13)
	{
		std::vector<double> obs(4);
	
		CTRef ref(ToInt(data[2]), ToInt(data[3]) - 1, ToInt(data[4]) - 1);
		obs[0] = ToDouble(data[6]);
		obs[1] = ToDouble(data[8]);
		obs[2] = ToDouble(data[10]);

		m_SAResult.push_back( CSAResult(ref, obs) );
	}
	else if(data.size() == 20)
	{
		//Sweden
		std::vector<double> obs(3);
	
		CTRef ref(-999, ToInt(data[2]) - 1, ToInt(data[3]) - 1);

		CStatistic stat[3];
		bool bAdd=false;
		for(int i=0; i<3; i++)//F1, F2, F3
		{
			obs[i] = ToDouble(data[i + 7]);
			bAdd = bAdd || obs[i]>0;//at least one value is not zero
		}

		if( bAdd )
			m_SAResult.push_back( CSAResult(ref, obs) );

	}
	
}


double GetRate(double alpha, double beta, double gamma, double Tmax, double T)
{
	double Rt=0;
	//if( T>7.9 && T<33.7)
	//{
		double a1= exp(alpha*T);
		double a2= exp(alpha*Tmax - (Tmax-T)/beta); 
		Rt = a1-a2-gamma;
	//}

	return max(0.0, min(1.0, Rt));
}

double GetF(double alpha, double beta, double gamma, double Tmax, double T)
{
	double F=0;
	//if( T>7.9 && T<33.7)
	//{
		double a1= exp(alpha*T);
		double a2= exp(alpha*Tmax - (Tmax-T)/beta); 
		F = a1-a2-gamma;
	//}

	return max(0.0, F);
}

void CSBBModel::Simulate( int locNo, double longtDayLength, double shortDayLength, double TNight, double TDay, double firstStageShortDay, double lastStageShortDay, int nbShortDay, CSpruceBarkBeetleStatVector& stat)
{
//	
//	m_timeStep.Set(1);
//
//	CTRef firsTRef(0, 0, 0);
//
//
//	
//	CDailyWaveVector dailyWave;
//	dailyWave.resize(24);
//	dailyWave.m_timeStep=1;
//	for(int i=0; i<24; i++)
//		dailyWave[i] = i<longtDayLength?TDay:TNight;
//
//		//This is where the model is actually executed
//	stat.resize(365);
//	stat.SetFirstTRef(firsTRef);
//
//
////Create stand
//	CSpruceBarkBeetleStand stand(this);
//	//stand.m_rates.Save("D:/Rates.csv");
//	stand.m_bFertilEgg = m_bFertilEgg;
//	stand.m_survivalRate = m_survivalRate;
//	stand.m_longDayLength = longtDayLength;
//	stand.m_shortDayLength = shortDayLength;
//	stand.m_firstStageShortDay = firstStageShortDay;
//	stand.m_lastStageShortDay = lastStageShortDay;
//	stand.m_nbShortDay = nbShortDay;
//	
//	
//	stand.m_p = m_p;
//	stand.m_k = m_k;
//	stand.m_s = m_s;
//
//	
//	//stand.m_swarmingP = m_p;
//	//stand.m_defoliation=m_defoliation;
//		
//
//	stand.m_bApplyAttrition = m_bApplyMortality&&m_bApplyAttrition;
//	stand.m_bApplyWinterMortality = m_bApplyMortality&&m_bApplyWinterMortality;
//	stand.m_nbReemergeMax = m_nbReemergeMax;
//		
//	//Create the initial attack
//	CSpruceBarkBeetleTree* pTree = new CSpruceBarkBeetleTree(&stand);
//	pTree->m_nbMinObjects = m_nbMinObjects;
//	pTree->m_nbMaxObjects = m_nbMaxObjects;
//	pTree->Initialize(m_nbObjects, m_initialPopulation, firsTRef, EGG, false, 1);
//	//pTree->Initialize(m_nbObjects, m_initialPopulation, weather.GetFirstTRef(), ADULT, false, 0);
//	
//	stand.SetTree(pTree);
//
////if Simulated Annealing, set 
//	if(ACTIVATE_PARAMETRIZATION)
//	{
//		//stand.m_rates.SetRho25(m_rho25Factor);
//		//stand.m_rates.Save("D:\\Rates.csv");
//	}
//
//	for(CTRef d=WEATHER_ARRAY[locNo].GetFirstTRef(); d<=CTRef(0, MAY, LAST_DAY); d++)
//	{
//		dailyWave.m_firstTRef = d;
//		
//		//stand.Live(d, weather[d]);
//		stand.Live(dailyWave, WEATHER_ARRAY[locNo].GetDay(d) );
//		stand.GetStat(d, stat[d]);
//
//		if( m_bAutoBalanceObject )
//			stand.AdjustPopulation();
//
//		HxGridTestConnection();
//	}
}

double CSBBModel::GetDi50(double T)
{
	double Th1 = 12.4;
	double Th2 = 22.8;
	//double MU  = 15.6588; 

	//double Th1 = m_p[3];
	//double Th2 = m_p[4];
	//double MU =  m_k[0];

	//shifting labda using day light
	double PHI = m_p[0];
	double MU =  m_p[1];
	//double LIMIT1 =  24;//m_p[1];
	//double LIMIT2 =  0;//m_p[2];
	

	//double x1 = PHI*(dayLength-MU); 
	//double TI50 = Th1 + (Th2-Th1)*(1-1/(1+exp(-x1)));




	double p = (T-Th1)/(Th2-Th1);
	//double logit = p<=0?LIMIT:p>=1?-LIMIT:-log((p/(1-p)));
	//double DI50 = MU + PHI*logit;
	double DI50 = MU - PHI*log(p/(1-p));

	return DI50;
}

double CSBBModel::GetTi50(double dayLength)
{
	double Th1 = 12.4;
	double Th2 = 22.8;
	double PHI =  1.0417;
	double MU  = 15.6588; 

	//double Th1 = m_p[3];
	//double Th2 = m_p[4];
	//double MU =  m_k[0];

	//double Th1_MU_T = 12.4;
	//double Th2_MU_T = 22.8;
	//double PHI_MU_T =  1.0417;
	//double MU_MU_T  = 15.6588; 



	double delta_T = dayLength-MU;
	double MU_T = Th1 + (Th2-Th1)*(1-1/(1+exp(-PHI*delta_T)));  //temperature insidance causing 50% diapause at dayLength

	return MU_T;
}	

double GetLoganLactin( double T, double alpha,double beta,double gamma,double Tmax,double Tl, double Tu)
{
	double Rt = 0;

	if(T>=Tl && T<=Tu)
	{
		double a1= exp(alpha*T);
		double a2= exp(alpha*Tmax - (Tmax-T)/beta); 
		Rt = (a1-a2-gamma);
	}

	return Rt;
}
void CSBBModel::GetFValueDaily(CStatisticXY& stat)
{
	ERMsg msg;

	//GetFValueDailyEmergence(stat);
	//return ;
	
	/*
	double LIMIT1 = GetDi50(12.4001);
	double LIMIT2 =  GetDi50(22.7999);

	for(int h=0; h<48; h++)
	{
		double dayLength = h/2.0;
		double TI50 = GetTi50(dayLength);
		double DI50 = GetDi50(TI50);
		stat.Add( DI50, dayLength);
	}

	return ;
	*/

	/*
	
	stat.Add( GetDi50(m_s[0], 17), 17);
	stat.Add( GetDi50(20, 14.7), 14.7);
	stat.Add( GetDi50(m_s[1], 12), 12);
	

	
//	stat.Add( GetDi50(m_s[0], 12), m_s[0]);
	//stat.Add( GetDi50(20, 14.7), 20);
	//stat.Add( GetDi50(m_s[1], 17), m_s[1]);

	

	return;
*/	
	
	/*
	static const double OBS[9][2] = 
{
	{12.0,100},
	{13.0, 99},
	{14.0, 97},
	{14.5, 73},
	{14.7, 50},
	{15.0, 21},
	{15.5, 20},
	{16.0,  0},
	{18.0,  0}
};

	for(int h=0; h<9; h++)
	{
		//double x = m_p[0] + m_p[1]*(OBS[h][0]-15);
		//double diapause = 0.5-erf(x)/2;

		//double x =  (OBS[h][0]-10)/(20-10);
		//double x = OBS[h][0]/24;
		//double x =  (OBS[h][0]-12)/(18-12);
		//double x =  (OBS[h][0]-10)/(20-10);
		//double x =  (OBS[h][0]-8)/(22-8);
		//double diapause = m_p[0] + m_p[1]*exp(-pow(x/m_p[3],m_p[2]));
	
		double x1 = m_p[0]*(OBS[h][0]-14.7); //GetStand()->m_diapauseP[2]*(0.5-x0);
		double diapause = 1 - 1/(1+exp(-x1));

		stat.Add(diapause*100, OBS[h][1]);
	}
	
	return;
	
	*/
	

	/*
	for(int i=0; i<(int)m_SAResult.size(); i++)
	{
		
	}
	*/

	/*if( 
		(GetDi50(15.7) - 17 > 0) ||
		(fabs(GetDi50(20) - 14.7) > 0.5) ||
		(fabs(GetDi50(22.5) - 12) > 0.5) ||
		(fabs(GetTi50(12) - 22.5) > 0.5) ||
		(fabs(GetTi50(14.7) - 20) > 0.5) ||
		(GetTi50(17) - 15.7 > 0)
		)
	{
		stat.Add( -999, 999);
		stat.Add( 999, -999);
		stat.Add( -999, -999);
		stat.Add( 999, 999);

		stat.Add( GetDi50(15.7), 17);
		stat.Add( GetDi50(20), 14.7);
		stat.Add( GetDi50(22.5), 12);
		stat.Add( GetTi50(12), 22.5);
		stat.Add( GetTi50(14.7), 20);
		stat.Add( GetTi50(17), 15.7);

		return;
	}*/


/*
	CTRef May30(0,MAY,LAST_DAY);
	if( m_info.m_locCounter.GetTotal() == 24 )
	{
		int locNo = m_info.m_loc.GetID().ToInt()-1; 
	
		CSpruceBarkBeetleStatVector simStat;
		Simulate( locNo, m_SAResult[0].m_obs[LONG_DAY_L], m_SAResult[0].m_obs[SHORT_DAY_L], m_SAResult[0].m_obs[T_NIGHT], m_SAResult[0].m_obs[T_DAY], (int)m_SAResult[0].m_obs[FIRST_STAGE_SHORT_DAY], (int)m_SAResult[0].m_obs[LAST_STAGE_SHORT_DAY], (int)m_SAResult[0].m_obs[NB_SHORT_DAY], simStat);
		//double sumDiapause = simStat.GetStat(O_DIAPAUSE_1)[SUM];

		
		//double diapause = stat[May30][O_W_STAT];
		//double diapause = simStat[May30][O_DIAPAUSE];
		//output[0][O_A_DAY_LENGTH] = stat[May30][O_DAY_LENGTH];
		//output[0][O_A_T_MEAN] = stat[May30][O_T_MEAN];
		//output[0][O_A_TI50] = stat[May30][O_TI50];
		
		double sumDiapause = simStat.GetStat(S_DIAPAUSE_1)[SUM];

		stat.Add(sumDiapause/50-1, m_SAResult[0].m_obs[DIAPAUSE]); 
	}
	else
	{
		int locNo = m_info.m_loc.GetID().ToInt()-1; 
	
		CSpruceBarkBeetleStatVector simStat;
		Simulate( locNo, m_SAResult[0].m_obs[LONG_DAY_L], m_SAResult[0].m_obs[SHORT_DAY_L], m_SAResult[0].m_obs[T_NIGHT], m_SAResult[0].m_obs[T_DAY], m_SAResult[0].m_obs[FIRST_STAGE_SHORT_DAY], m_SAResult[0].m_obs[LAST_STAGE_SHORT_DAY], (int)m_SAResult[0].m_obs[NB_SHORT_DAY], simStat);
		//double swarmingJday = simStat[May30][O_SWARMING_JDAY];
		double sumSwarming = simStat.GetStat(O_SWARMING_1)[SUM];
		
		//double w = simStat[May30][O_W_STAT];
		
		
		double swarmingJday = 365;
		if( sumSwarming>50 )
		{
			CStatistic swarmingStat;
			for(CTRef d=simStat.GetFirstTRef(); d<simStat.GetLastTRef(); d++)
			{
				swarmingStat += d.GetJDay()*simStat[d][O_SWARMING_1];
			}

			swarmingJday = swarmingStat[SUM]/sumSwarming;
			ASSERT( swarmingJday>=0 && swarmingJday<=360);
		}

		//if(sumSwarming<50)
			//swarmingJday=365;

		//if( swarmingJday!=365 && m_SAResult[0].m_obs[SWARMING_DATE]!=365)
		{
			stat.Add(swarmingJday, m_SAResult[0].m_obs[SWARMING_DATE]); 
		}
		//else
		//{
		//	if( (swarmingJday==365 && m_SAResult[0].m_obs[SWARMING_DATE]!=365) ||
		//		(swarmingJday!=365 && m_SAResult[0].m_obs[SWARMING_DATE]==365) )
		//	{
		//		stat.Add(Rand(-365,365), m_SAResult[0].m_obs[SWARMING_DATE]); 
		//		//stat.Add(-999, 365); 
		//		//stat.Add(-100, -999); 
		//		//stat.Add(365, -100); 
		//	}

		//}
	}
	*/

	
	if( !m_SAResult.empty() )
	{
		if( m_SAResult.front().m_obs.size() == 3 )
		{
			if( m_firstJday.front() == 999)
			{
				/*CSnowMelt snow;
				snow.SetLon(-120);
				snow.Compute(m_weather);*/
	

				//for (CTRef d = p.Begin(); d <= p.End(); d++)
				//{
				//	CWeatherDay day = m_weather[d];
				//	day(SNOW) = snow.GetResult()[d].m_hs;//mm
				//	day(SNDH) = snow.GetResult()[d].m_hs;//cm
				//	m_weather.SetData(d,day);
				//}

				//m_firstJday = m_SAResult.front().m_ref.GetJDay(); 
				//m_lastJday = m_SAResult.back().m_ref.GetJDay();
				for(size_t i=0; i<m_SAResult.size(); i++)
				{
					for(size_t j=0; j<3; j++)
					{
						if( m_SAResult[i].m_obs[j] > 0)
						{
							m_firstJday[j] = min(m_firstJday[j], m_SAResult[i].m_ref.GetJDay()); 
							m_lastJday[j] = max(m_lastJday[j], m_SAResult[i].m_ref.GetJDay());
						}
					}
				}


				//if( GetInfo().m_loc.m_ID == "49" )
				//{
				//	for(int y=0; y<m_weather.GetNbYears(); y++)
				//		if( m_weather[y].GetYear() == 2010)
				//			m_weather.RemoveYear(y);//remove 2010 for Gallarmstrop
				//}

				//put input as % of total swarm
				CStatistic stat[3];
				for(size_t i=0; i<m_SAResult.size(); i++)
					for(int j=0; j<3; j++)//F1, F2, F3
						stat[j] += m_SAResult[i].m_obs[j];

				double totalSwarm = stat[0][HIGHEST] + stat[1][HIGHEST] + stat[2][HIGHEST];
				for(size_t i=0; i<m_SAResult.size(); i++)
					for(int j=0; j<3; j++)//F1, F2, F3
						m_SAResult[i].m_obs[j] = 100*m_SAResult[i].m_obs[j]/totalSwarm;
			}


			CSpruceBarkBeetleStatVector simStat;
			GetDailyStat(simStat);

			//merge years
			CSpruceBarkBeetleStatVector merge(365, CTRef(-999,0,0) );
			
			for(int i=0; i<simStat.front().size(); i++)
			{
				
				CStatistic stat[365];
				for(CTRef d=simStat.GetFirstTRef(); d<=simStat.GetLastTRef(); d++)
				{
					if( simStat[d][i] >-999 )
					{
						size_t jd = min(364ull, d.GetJDay());
						stat[jd]+=simStat[d][i];
					}
				}

				for(int jd=0; jd<365; jd++)
					merge[jd][i] = stat[jd][MEAN];//mean of 3 years
			}

			CStatistic eggs;
	
			//compute F1, F2 and F3 in function of the total population (nb eggs)
			for(int jd=0; jd<365; jd++)
				for(int i=0; i<3; i++)
					eggs += merge[jd][O_BROOD_0_P1+i];
			
			
			for(int jd=1; jd<365; jd++)
				for(int i=0; i<9; i++)
					merge[jd][O_SWARMING_1_F1_i+i] = merge[jd-1][O_SWARMING_1_F1_i+i] + 100*merge[jd][O_SWARMING_1_F1_i+i]/eggs[SUM];

			static const int VARIABLES[3][3] = 
			{
				{  O_SWARMING_1_F1_i,  O_SWARMING_1_F2_i,    O_SWARMING_1_F3_i},
				{				  -1, 				  -1,					-1},
				{				  -1,				  -1,					-1}
			};

			//static const CTRef beginTRef[3] =  {CTRef(-999,JANUARY, FIRST_DAY), CTRef(-999,JUNE,FIRST_DAY), CTRef(-999,JULY,FIRST_DAY)};
			//static const CTRef endTRef[3] = {CTRef(-999,MAY, LAST_DAY), CTRef(-999,JUNE,LAST_DAY), CTRef(-999,DECEMBER,LAST_DAY)};

			//double broods = merge.GetStat(O_BROOD_0_P1)[SUM] + merge.GetStat(O_BROOD_0_P2)[SUM] + merge.GetStat(O_BROOD_0_P3)[SUM];
			double totalSwarms = 0;
			for(int x=0; x<3; x++)
				for(int y=0; y<3; y++)
					if(VARIABLES[x][y]>=0)
						totalSwarms += merge.GetStat(VARIABLES[x][y])[HIGHEST];

			//to avoid the elimination of all swarm
			if( totalSwarms < 5)
				return;

			for(int i=0; i<9; i++)//F1, F2, F3
				merge[0][O_SWARMING_1_F1_i+i] = 0;

			for(int jd=0; jd<365; jd++)
				for(int i=0; i<9; i++)//F1, F2, F3
					merge[jd][O_SWARMING_1_F1_i+i] = 100*merge[jd][O_SWARMING_1_F1_i+i]/totalSwarms;


			for(int y=0; y<2; y++)
			{
				for(size_t i=0; i<m_SAResult.size(); i++)
				{

					double threshold = m_SAResult[i].m_obs[y];
					ASSERT(threshold>=0 && threshold<=100);

					
					double swarmingJday = -999;
					if( totalSwarms>0 && m_SAResult[i].m_obs[y]>0 )
					{
						ASSERT( m_SAResult[i].m_obs[0] >= 0 && m_SAResult[i].m_obs[0] <= 100);
				
						//CStatistic swarmingStatI;
				
						for(int d=0; d<365&&swarmingJday==-999; d++)
						{
							double value = merge[d][VARIABLES[0][y]];
							//if(VARIABLES[1][y]>=0)
							//	value += merge[d][VARIABLES[1][y]];
							//if(VARIABLES[2][y]>=0)
							//	value += merge[d][VARIABLES[2][y]];

							if( value>threshold)
							{ 
								//double next = swarmingStatI.IsInit()?swarmingStatI[SUM]+value/totalSwarms:value/totalSwarms;
								//if( next>=threshold ) 
								//{
								double y1 = merge[d-1][VARIABLES[0][y]];//swarmingStatI[SUM];
								double x = (threshold-y1)/(value-y1);
								ASSERT(x>=0 && x<=1);
							
								//to avoid integer value (more difficult for SA)
								swarmingJday = d - 1 + x;
								//}

								//swarmingStatI += value/totalSwarms;
							}
						}

						if( swarmingJday==-999)
							swarmingJday=m_lastJday[y];
			
						size_t jDay1 = m_firstJday[y];
						size_t jDay2 = m_lastJday[y];
								
						double obsJday = ((double)m_SAResult[i].m_ref.GetJDay()-jDay1)/(jDay2-jDay1)*100;
						double simJday = (swarmingJday-jDay1)/(jDay2-jDay1)*100;
						stat.Add(simJday, obsJday);
					}


					//double sim = 0;
					//if(totalSwarms>0)
					//{
					//int jd = min(365, m_SAResult[i].m_ref.GetJDay());
						//CStatistic swarmingStatI;
				
						//for(int d=0; d<=m_SAResult[i].m_ref.GetJDay(); d++)
						//{
						//			
							//swarmingStatI += merge[jd][VARIABLES[0][y]];
							//if(VARIABLES[1][y]>=0)
							//	swarmingStatI += merge[jd][VARIABLES[1][y]];
							//if(VARIABLES[2][y]>=0)
							//	swarmingStatI += merge[jd][VARIABLES[2][y]];
						//}

					
					//}
							
					size_t jd = min(365ull, m_SAResult[i].m_ref.GetJDay());
					double sim = merge[jd][VARIABLES[0][y]];///totalSwarms;		
					double obs = m_SAResult[i].m_obs[y];
					stat.Add(sim, obs);
							
				}
			}
		}	
		else if( m_SAResult.front().m_obs.size() == 4 )
		{

			if( m_firstJday[0] == 999)
			{
				//CSnowMelt snow;
				//snow.SetLon(-120);
				//snow.Compute(m_weather);
	
				//for(CTRef d=m_weather.GetFirstTRef(); d<=m_weather.GetLastTRef(); d++)
				//{
				//	CWeatherDay day = m_weather[d];
				//	day(SNOW) = snow.GetResult()[d].m_hs;//mm
				//	day(SNDH) = snow.GetResult()[d].m_hs;//cm
				//	m_weather.SetData(d,day);
				//}

				m_firstJday[0] = 364;
				m_lastJday[0] = 0;
				const CSimulatedAnnealingVector& SA = GetSimulatedAnnealingVector();
				for(size_t i=0; i<SA.size(); i++)
				{
					const CSAResultVector& resul = SA[i]->GetSAResult();
					for(size_t j=0; j<resul.size(); j++)
					{
						m_firstJday[0] = min( m_firstJday[0], resul[j].m_ref.GetJDay() );
						m_lastJday[0] = max( m_lastJday[0], resul[j].m_ref.GetJDay() );
					}
				}

				CStatistic years;
				for(CSAResultVector::const_iterator p= m_SAResult.begin(); p!=m_SAResult.end(); p++)
					years += p->m_ref.GetYear();
 
				int firstYear = (int)years[LOWEST];
				int lastYear = (int)years[HIGHEST];
				//while( m_weather.GetNbYears() > 1 && m_weather.GetFirstYear() < firstYear )
					//m_weather.RemoveYear(0);
	
				//while( m_weather.GetNbYear() > 1 && m_weather.GetLastYear() > lastYear )
					//m_weather.RemoveYear(m_weather.GetNbYear()-1);

				ASSERT( m_weather.GetFirstYear() == firstYear );
				ASSERT( m_weather.GetLastYear() == lastYear );
				ASSERT( m_firstJday<m_lastJday );
			}


		

			CSpruceBarkBeetleStatVector simStat;
			GetDailyStat(simStat);
		
			for(size_t i=0; i<m_SAResult.size(); i++)
			{
				if( simStat.IsInside( m_SAResult[i].m_ref) )
				{
					CTPeriod p( CTRef(m_SAResult[i].m_ref.GetYear(),FIRST_MONTH, FIRST_DAY), CTRef(m_SAResult[i].m_ref.GetYear(), LAST_MONTH, LAST_DAY) );

			
					//CTRef firstTRef = simStat.GetFirstTRef(O_SWARMING_0_P1+m_swarmingNo,1.0,1, p);
					//CTRef lastTRef = simStat.GetLastTRef(O_SWARMING_0_P1+m_swarmingNo,99.0,1, p);

					enum TTypes{ NB_TYPES=3};
					int TYPE[NB_TYPES] = {S_SWARMING_0_P2_MALE, S_SWARMING_0_P2_FEMALE, S_SWARMING_0_P2};
					for(int k=0; k<NB_TYPES; k++)
					{
						double sumSwarming = simStat.GetStat(TYPE[k], p)[SUM];
						double threshold = m_SAResult[i].m_obs[k]/100;
						ASSERT(threshold>=0 && threshold<=1);

						double swarmingJday = -999;
				
						if(sumSwarming>0)
						{
							ASSERT( m_SAResult[i].m_obs[k] >= 0 && m_SAResult[i].m_obs[k] <= 100);
				
							CStatistic swarmingStatI;
				
							for(CTRef d=p.Begin(); d<=p.End()&&swarmingJday==-999; d++)
							{
								if( simStat[d][TYPE[k]] > 0)
								{
									//swarmingStatI += d.GetJDay();
						
									double next = swarmingStatI.IsInit()?swarmingStatI[SUM]+simStat[d][TYPE[k]]/sumSwarming:simStat[d][TYPE[k]]/sumSwarming;
									if( next>threshold || (fabs(next-1)<0.0001 && fabs(threshold-1)<0.0001) ) 
									{
										double y1 = swarmingStatI[SUM];
										double x = (threshold-y1)/(next-y1);
										ASSERT(threshold>=0 && threshold<=1);
							
										//to avoid integer value (more difficult for SA)
										swarmingJday = d.GetJDay() - 1 + x;
									}

									swarmingStatI += simStat[d][TYPE[k]]/sumSwarming;
								}
							}

							ASSERT( swarmingJday>-999);
						
				
			
							size_t jDay1 = m_firstJday[0];
							size_t jDay2 = m_lastJday[0];
							double obsJday = ((double)m_SAResult[i].m_ref.GetJDay()-jDay1)/(jDay2-jDay1)*100;
							double simJday = (swarmingJday-jDay1)/(jDay2-jDay1)*100;
							stat.Add(simJday, obsJday);

						}
						//stat.Add(simJday, obsJday);
				
			
						double sim = -999;
						if(sumSwarming>0)
						{
							CStatistic swarmingStatI;
				
							for(CTRef d=p.Begin(); d<=m_SAResult[i].m_ref; d++)
							{
								swarmingStatI += simStat[d][TYPE[k]];
							}

							sim = swarmingStatI[SUM]/sumSwarming*100;
						}

						double obs = m_SAResult[i].m_obs[k];
						stat.Add(sim, obs);
					}//male and female
				}//if this result are in the simulation
			}//for all result
		}//
		else if( m_SAResult.front().m_obs.size() == 1 )
		{

			if( m_firstJday[0] == 999)
			{
				//CSnowMelt snow;
				//snow.SetLon(-120);
				//snow.Compute(m_weather);
	
				//for(CTRef d=m_weather.GetFirstTRef(); d<=m_weather.GetLastTRef(); d++)
				//{
				//	CWeatherDay day = m_weather[d];
				//	day(SNOW) = snow.GetResult()[d].m_hs;//mm
				//	day(SNDH) = snow.GetResult()[d].m_hs;//cm
				//	m_weather.SetData(d,day);
				//}

				m_firstJday[0] = 364;
				m_lastJday[0] = 0;
				const CSimulatedAnnealingVector& SA = GetSimulatedAnnealingVector();
				for(size_t i=0; i<SA.size(); i++)
				{
					const CSAResultVector& resul = SA[i]->GetSAResult();
					for(size_t j=0; j<resul.size(); j++)
					{
						m_firstJday[0] = min( m_firstJday[0], resul[j].m_ref.GetJDay() );
						m_lastJday[0] = max( m_lastJday[0], resul[j].m_ref.GetJDay() );
					}
				}

				CStatistic years;
				for(CSAResultVector::const_iterator p= m_SAResult.begin(); p!=m_SAResult.end(); p++)
					years += p->m_ref.GetYear();
 
				int firstYear = (int)years[LOWEST];
				int lastYear = (int)years[HIGHEST];
				//while( m_weather.GetNbYears() > 1 && m_weather.GetFirstYear() < firstYear )
					//m_weather.RemoveYear(0);
	
				//while( m_weather.GetNbYear() > 1 && m_weather.GetLastYear() > lastYear )
					//m_weather.RemoveYear(m_weather.GetNbYear()-1);

				ASSERT( m_weather.GetFirstYear() == firstYear );
				ASSERT( m_weather.GetLastYear() == lastYear );
				ASSERT( m_firstJday<m_lastJday );
			}


		

			CSpruceBarkBeetleStatVector simStat;
			GetDailyStat(simStat);
		
			for(size_t i=0; i<m_SAResult.size(); i++)
			{
				if( simStat.IsInside( m_SAResult[i].m_ref) )
				{
					CTPeriod p( CTRef(m_SAResult[i].m_ref.GetYear(),FIRST_MONTH, FIRST_DAY), CTRef(m_SAResult[i].m_ref.GetYear(), LAST_MONTH, LAST_DAY) );

			
					//CTRef firstTRef = simStat.GetFirstTRef(O_SWARMING_0_P1+m_swarmingNo,1.0,1, p);
					//CTRef lastTRef = simStat.GetLastTRef(O_SWARMING_0_P1+m_swarmingNo,99.0,1, p);

					double sumSwarming = simStat.GetStat(O_SWARMING_0_P1+m_swarmingNo, p)[SUM];
					double threshold = m_SAResult[i].m_obs[0]/100;
					ASSERT(threshold>=0 && threshold<=1);

					double swarmingJday = -999;
					if( sumSwarming>0  )
					{
						ASSERT( m_SAResult[i].m_obs[0] >= 0 && m_SAResult[i].m_obs[0] <= 100);

				
				
						CStatistic swarmingStatI;
				
						for(CTRef d=p.Begin(); d<=p.End()&&swarmingJday==-999; d++)
						{
							if( simStat[d][O_SWARMING_0_P1+m_swarmingNo] > 0)
							{
								//swarmingStatI += d.GetJDay();
						
								double next = swarmingStatI.IsInit()?swarmingStatI[SUM]+simStat[d][O_SWARMING_0_P1+m_swarmingNo]/sumSwarming:simStat[d][O_SWARMING_0_P1+m_swarmingNo]/sumSwarming;
								if( next>threshold || (fabs(next-1)<0.0001 && fabs(threshold-1)<0.0001) ) 
								{
									double y1 = swarmingStatI[SUM];
									double x = (threshold-y1)/(next-y1);
									ASSERT(threshold>=0 && threshold<=1);
									//to avoid integer value (more difficult for SA)
									swarmingJday = d.GetJDay() - 1 + x;
								}

								swarmingStatI += simStat[d][O_SWARMING_0_P1+m_swarmingNo]/sumSwarming;
							}
						}

						ASSERT( swarmingJday>-999);
					}
			
					size_t jDay1 = m_firstJday[0];
					size_t jDay2 = m_lastJday[0];
					double obsJday = ((double)m_SAResult[i].m_ref.GetJDay()-jDay1)/(jDay2-jDay1)*100;
					double simJday = (swarmingJday-jDay1)/(jDay2-jDay1)*100;
					stat.Add(simJday, obsJday);
					//stat.Add(simJday, obsJday);
				
			
					double sim = -999;
					if(sumSwarming>0)
					{
						CStatistic swarmingStatI;
				
						for(CTRef d=p.Begin(); d<=m_SAResult[i].m_ref; d++)
						{
							swarmingStatI += simStat[d][O_SWARMING_0_P1+m_swarmingNo];
						}

						sim = swarmingStatI[SUM]/sumSwarming*100;
					}

					double obs = m_SAResult[i].m_obs[0];
					stat.Add(sim, obs);
				}//if the result are in the simulation
			}//for all result
		}//result have 1 value
	}//empty result	
}

void CSBBModel::GetFValueDailyEmergence(CStatisticXY& stat)
{
	//CSBBTableLookup table;
	CRandomGenerator random;

	static const double T[6] = 
	{
		12, 15, 20, 25, 30, 33
	};
	static const double N[10][6] = 
	{
		{70, 129, 214, 458, 147, 95},//Egg
		{ 0,  13,  60,  52,  27,  4},//L1
		{ 0,  13,  60,  52,  27,  4},//L2
		{ 0,  13,  60,  52,  27,  4},//L3
		{ 0,  24,  63,  38,  33,  3},//Pupae
		{ 0,  0 ,  45,  33,  29,  0},//Maturation
		{13,  22,  18,  21,   8, 14},//pre-ovip + ovip
		{13,  22,  18,  21,   8, 14},
		{13,  22,  18,  21,   8, 14},
		{13,  22,  18,  21,   8, 14},		
	};

	static const double DT[3][6] = 
	{
		{10.1, 10.7, 6.6, 6.7, 5.5, 3.4},//pre-ovip + ovip
		{2.5, 2.1, 1.0, 1.3, 1.1, 0.7},//pre-ovip
		{7.6, 8.6, 5.6, 5.4, 4.4, 2.7},//ovip
		
	};

	static const double SD[10][6] = 
	{
		{0.004, 0.0100, 0.0220, 0.0460, 0.0790, 0.0640},//Egg
		{0.000, 0.0050, 0.0090, 0.0140, 0.0140, 0.3200},//L1
		{0.000, 0.0050, 0.0090, 0.0140, 0.0140, 0.3200},//L2
		{0.000, 0.0050, 0.0090, 0.0140, 0.0140, 0.3200},//L3
		{0.000, 0.0130, 0.0140, 0.0540, 0.0840, 0.0510},//Pupae
		{0.000, 0.0000, 0.0330, 0.0230, 0.0230, 0.0000},//TENERAL_ADULT
		{1.630, 1.4700, 0.6900, 0.5500, 0.7600, 0.40},//pre ovip + ovip
		{0.430, 0.4800, 0.2000, 0.1700, 0.2300, 0.13},//pre-ovip //Ça semble être la déviation standard même si c'est marquer erreurs standard
		{1.200, 0.9900, 0.4900, 0.3800, 0.5300, 0.27},//ovip
		{0.070, 0.1300, 0.2500, 0.4300, 0.8200, 0.25}//ovip rate
		//{1.63, 1.47, 0.69, 0.55, 0.76, 0.4}
	};
	
	//static const double OR[6] = {0.45, 1.17, 2.64, 4.34, 5.33, 2.28};
	//static const double F[6] = {4.1, 9.5, 14.5, 23.3, 23.6, 6.4};
	//static const double TT[8] = {7.9, 12, 15, 20, 25, 30, 33 };
	static const double OR[6] = {0.09900990, 0.09345794, 0.15151515, 0.14925373, 0.18181818, 0.29411765};
	
	for(int t=0; t<6; t++)
	{
		//double r = m_s[0] + m_s[1]*Max(0, Min(1, exp(m_s[2]*(T[t]-33.7))));
		//stat.Add( r, OR[t]);
		stat.Add( max(0.0, GetLoganLactin( T[t], m_s[0],m_s[1],m_s[2],18.2, 7.9, 33.7)), OR[t]);
	}
	
	
	//static const double DEFAULT_P[7]={0.07254,	13.11231,	0.25539,	15.0,	27.4,	33.0,	40.8};//Maturation Feeding: get from paper and simulated Annealing

	
	
	//double TA[6] = {m_p[0],m_p[1],0.070, 0.091, 0.094, m_p[2]};
	//for(int t=0; t<6; t++)
	//{
	//	stat.Add( GetLoganLactin( T[t], m_s[0],m_s[1],m_s[2],m_s[3], m_s[4], m_s[5]), TA[t]);
	//}


	//CRandomGenerator RG;
	//CSBBTableLookup table;

	//int s = TENERAL_ADULT;
	//for(int t=0; t<6; t++)
	//{
	//	if( N[s][t] > 0)
	//	{
	//		double R = table.ComputeRate(s, T[t]);
	//		CStatistic S;
	//		for(int i=0; i<1000; i++)
	//		{
	//			double r = RG.RandLogNormal(0, m_k[0]);
	//			while(r<0.4||r>2.5)
	//				r = RG.RandLogNormal(0, m_k[0]);

	//			S+=r*R;
	//		}
	//	
	//		for(int i=0; i< N[s][t]; i++)
	//			stat.Add( S[STD_DEV], SD[s][t] );
	//	}
	//}



	////evaluate L1, L2 and L3 stage rates
	//static const double DEFAULT_P[7]={0.07919,	12.40338,	0.09250,	 8.7,	29.5,	39.1,	42.0};//Larvae (for all stage)
	////ERRIKA (1969) p.179
	//static const double RATIO[3] = {0.203, 0.322, 0.475};
	//for(int t=0; t<121; t++)
	//{
	//	double T = DEFAULT_P[3] + (DEFAULT_P[5]-DEFAULT_P[3])*t/120;
	//
	//	double R1 = Max(0, GetLoganLactin( T, m_p[0],m_p[1],m_p[2],DEFAULT_P[6], m_p[3], m_p[4]));
	//	double R2 = Max(0, GetLoganLactin( T, m_k[0],m_k[1],m_k[2],DEFAULT_P[6], m_k[3], m_k[4]));
	//	double R3 = Max(0, GetLoganLactin( T, m_s[0],m_s[1],m_s[2],DEFAULT_P[6], m_s[3], m_s[4]));
	//	double R = Max(0, GetLoganLactin( T, DEFAULT_P[0], DEFAULT_P[1], DEFAULT_P[2], DEFAULT_P[6], DEFAULT_P[3], DEFAULT_P[5]));

	//	if( R1!=0 && R2!=0 && R3!=0 && R!=0)
	//	{
	//		if( 1/R1<122 && 1/R2<122 && 1/R3<122 && 1/R<366)
	//		{
	//			stat.Add( 1/(R1*RATIO[0]), 1/R);
	//			stat.Add( 1/(R2*RATIO[1]), 1/R);
	//			stat.Add( 1/(R3*RATIO[2]), 1/R);
	//			stat.Add( 1/R1+1/R2+1/R3, 1/R);
	//		}
	//	}
	//}

	return ;
	
}



}