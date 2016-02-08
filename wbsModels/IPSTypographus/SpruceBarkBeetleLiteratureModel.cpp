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
// Class: CSpruceBarkBeetleLiteratureModel
//
// Description: CSpruceBarkBeetleLiteratureModel simulate European Spruce Bark Beetle 
//              seasonal biology from literature models. 
//
//*****************************************************************************
// 29/01/2014	1.0		Rémi Saint-Amant    Creation
//*****************************************************************************

#define NO_MINMAX
#include "SpruceBarkBeetleLiteratureModel.h"
#include "timeStep.h"
#include "EntryPoint.h"
//#include "SimulatedAnnealingVector.h"
#include "UtilMath.h"
#include "SnowMelt.h"
//#include "SBBDevelopment.h"

using namespace std;
using namespace CFL;
using namespace DAILY_DATA;
using namespace HOURLY_DATA;

//const double CSpruceBarkBeetleLiteratureModel::PARENTAL[5] = {};
//const double CSpruceBarkBeetleLiteratureModel::FILIAL[4] = {};
//
//
//
//ERMsg CCanolaModel::OnExecuteDaily()
//{
//    ERMsg msg;
//
//
//	CModelStatVector stat;
//	m_CR.Execute(m_weather, stat);
//
//	
//
//	SetOutput(output);
//
//	return msg;
//}

//uncomment this line to activate version for simulated annealing
//static const bool ACTIVATE_PARAMETRIZATION = true;
//static const bool P1_ONLY = true;
static CFL::CCriticalSection CS;




//**************************
//this line link this model with the EntryPoint of the DLL
static const bool bRegistred = 
	CModelFactory::RegisterModel( CSpruceBarkBeetleLiteratureModel::CreateObject );



enum TOuput{	O_OVERWINTER, O_FIRST_FLIGHT, O_INFESTATION, O_RE_EMERGENCE1, O_RE_EMERGENCE2, O_ADULT_ALIVE, O_ADULT_DEAD, 
				O_FILIAL_EMERGENCE1, O_FILIAL_EMERGENCE2, O_FILIAL_ALIVE, O_FILIAL_DEAD, 
				NB_OUTPUT
			};

typedef CModelStatVectorTemplate<NB_OUTPUT> CDailyOutputVector;

enum TOuputA{ O_A_SWARMING_OBS, O_A_SWARMING_SIM, O_A_SWARMING, O_A_DIAPAUSE, O_A_W_STAT, O_A_DT_STAT, O_A_DDL_STAT, O_A_DAY_LENGTH,  O_A_DI50, O_A_T_MEAN, O_A_TI50, NB_OUTPUT_A};
typedef CModelStatVectorTemplate<NB_OUTPUT_A> CAnnualOutputVector;

enum TOuputH{ O_T_AIR_ALLEN_WAVE, O_SRAD, O_SNOW, O_T_PHLOEM_OPEN_TOP, O_T_PHLOEM_OPEN_BOTTOM, O_T_PHLOEM_CLOSE, O_T_SOIL, NB_OUTPUT_H };

typedef CModelStatVectorTemplate<NB_OUTPUT_H> CHourlyOutputVector;


CSpruceBarkBeetleLiteratureModel::CSpruceBarkBeetleLiteratureModel()
{

	//**************************
	//NB_INPUT_PARAMETER is used to determine if the dll
	//uses the same number of parameters than the model interface

	NB_INPUT_PARAMETER = 0;

	VERSION = "1.0 (2014)";

	m_model = OHRN;
	m_bApplyMortality = true;
}

CSpruceBarkBeetleLiteratureModel::~CSpruceBarkBeetleLiteratureModel()
{
}




class CTOpenTopPhloem : public COverheat
{
public:
	
	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		
		double Tmin   = weaDay.GetTMin();
		//double Tmax   = weaDay.GetTMean()*(1+weaDay[SRAD]/21)+4;
		double Tmax   = weaDay.GetTMax()*(1+weaDay[SRAD]/38)+2;
		double Trange = Tmax-Tmin;//weaDay.GetTRange();
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*weaDay.GetTRange() + 0.00000*Tmin*Sin + 0.005741*Tmin*weaDay.GetTRange() + 0.02370*Sin*weaDay.GetTRange();
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};

class CTOpenBottomPhloem: public COverheat
{
public:
	
	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		double Tmin   = weaDay.GetTMin();
		//double Tmax   = weaDay.GetTMean()*(1+weaDay[SRAD]/33.7);
		double Tmax   = weaDay.GetTMax()*(1+weaDay[SRAD]/139)+1;
		double Trange = Tmax-Tmin;//weaDay.GetTRange();
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*weaDay.GetTRange() + 0.00000*Tmin*Sin + 0.005741*Tmin*weaDay.GetTRange() + 0.02370*Sin*weaDay.GetTRange();
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};

class CTClosePhloem : public COverheat
{
public:
	

	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		double Tmin   = weaDay.GetTMin()-0.5;
		double Tmax   = weaDay.GetTMean()+2;
		double Trange = Tmax-Tmin;//weaDay.GetTRange();
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};

class CTSoil : public COverheat
{
public:
	
	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		//after Annila 1969: coarse approximation
		if( weaDay[SNDH] <= 2)
		{
			double Tmin   = weaDay.GetTMin()-0.5;
			double Tmax   = max(Tmin, weaDay.GetTMean()+1.2);
			double Trange = Tmax-Tmin;
			double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

			//convert air temperature to bark temperature
			weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
			weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;
		}
		else
		{
			weaDay(DAILY_DATA::TMIN)= -1;
			weaDay(DAILY_DATA::TMAX)= 1;
		}


		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};
	
/*class CTopPhloemOverheat : public COverheat
{
public:
	
	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		double Tmin   = weaDay.GetTMin();
		double Tmax   = weaDay.GetTMax()*max(1.0, weaDay[SRAD]/24);
		double Trange = Tmax-Tmin;//weaDay.GetTRange();
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};
*/
typedef auto_ptr<COverheat> COverheatPtr;
ERMsg CSpruceBarkBeetleLiteratureModel::OnExecuteHourly()
{
	ERMsg msg;

	CHourlyOutputVector output(m_weather.GetNbDay()*24, m_weather.GetFirstTRef().Transform(CTM(CTM::HOURLY)));

	//compute snow
	CSnowMelt snow;
	snow.SetLon(-120);
	snow.Compute(m_weather);
	
	for(CTRef d=m_weather.GetFirstTRef(); d<=m_weather.GetLastTRef(); d++)
	{
		CWeatherDay day = m_weather[d];
		day(SNOW) = snow.GetResult()[d].m_hs;//mm
		day(SNDH) = snow.GetResult()[d].m_hs;//cm
		m_weather.SetData(d,day);
	}

	
	//CPOverheat barkOverheat;
	//CBottomPhloemOverheat bottomPhloemOverheat;
	//CPhloemClose closePhloem;
	//CTopOpenPhloem topPhloem;

	COverheatPtr overheat[4] = {COverheatPtr (new CTOpenTopPhloem), COverheatPtr(new CTOpenBottomPhloem), COverheatPtr (new CTClosePhloem), COverheatPtr(new CTSoil)};
	
	//CDailyWaveVector TtopOpen;// hourly temperature array 
	//CDailyWaveVector TbottomOpen;// hourly temperature array 
	//CDailyWaveVector Tclose;// hourly temperature array 

	
	

	CWeather adjustWeather(m_weather);
	adjustWeather.AjusteMeanForHourlyRequest(m_info.m_loc);
	CYearsVector weather;
	adjustWeather.GetHourlyVar(weather, m_info.m_loc);
	


	

	//CTRef TRef = m_weather.GetFirstTRef();
	for(int y=0; y<m_weather.GetNbYear(); y++)
	{
		int year = m_weather[y].GetYear();
			
		for(int m=0; m<12; m++)
		{
			for(int d=0; d<m_weather[y][m].GetNbDay(); d++)
			{
				//Regniere Bark Temperature
				CDailyWaveVector Tallen;// hourly temperature array 
				m_weather[y][m][d].GetAllenWave(Tallen, 16, 1 );//16 is a factor of 4. Number usually used

				CDailyWaveVector T[4];// hourly temperature array 
				for(int i=0; i<4; i++)
						m_weather[y][m][d].GetAllenWave(T[i], 16, 1, *overheat[i] );//16 is a factor of 4. Number usually used

				for(int h=0; h<24; h++)
				{
					CTRef TRef(year,m,d,h);
					output[TRef][O_T_AIR_ALLEN_WAVE]=Tallen[h];
					output[TRef][O_SRAD] = weather[TRef][H_SRAD];
					output[TRef][O_SNOW] = m_weather[y][m][d][SNDH];
					
					for(int i=0; i<4; i++)
						output[TRef][O_T_PHLOEM_OPEN_TOP+i] = T[i][h]; 
				}
			}
		}
	}

	
	
	
	SetOutput(output);

	return msg;
}


ERMsg CSpruceBarkBeetleLiteratureModel::OnExecuteDaily()
{
	ERMsg msg;

	//Actual model execution
	CSpruceBarkBeetleStatVector output;
	GetDailyStatOhrn(output);

	//CDailyOutputVector output;
	//ComputeRegularStat(stat, output);
	SetOutput(output);

	return msg;
}


ERMsg CSpruceBarkBeetleLiteratureModel::OnExecuteAnnual()
{
	ERMsg msg;

	
	//Actual model execution
	//CSpruceBarkBeetleStatVector stat;
	//GetDailyStatOhrn(stat);

	//CAnnualOutputVector output(m_weather.GetNbYear(), CTRef(m_weather[0].GetYear()));

	//for(int y=0; y<m_weather.GetNbYear(); y++)
	//{
	//	int year = m_weather[y].GetYear();
	//	CTPeriod p(CTRef(year, FIRST_MONTH, FIRST_DAY), CTRef(year, LAST_MONTH, LAST_DAY));

	//	double sumDiapause = stat.GetStat(S_DIAPAUSE_1, p)[SUM];
	//	double sumSwarming = stat.GetStat(S_SWARMING_1_F1_i+m_swarmingNo, p)[SUM];
	//	
	//	double swarmingJday = 365;
	//	if( sumSwarming>0 )
	//	{
	//		CStatistic swarmingStat;
	//		for(CTRef d=p.Begin(); d<=p.End(); d++)
	//			swarmingStat += d.GetJDay()*stat[d][S_SWARMING_1_F1_i+m_swarmingNo];

	//		swarmingJday = swarmingStat[SUM]/sumSwarming;
	//		ASSERT( swarmingJday>=0 && swarmingJday<=360);
	//	}

	//	//output[y][O_A_SWARMING_OBS] = swarmingJday;
	//	//output[y][O_A_SWARMING_SIM] = swarmingJday;
	//	//
	//	//output[0][O_A_SWARMING] = sumSwarming;
	//	//output[0][O_A_DIAPAUSE] = sumDiapause;
	//	//output[0][O_A_W_STAT] = stat[p.End()][O_W_STAT];
	//	//output[0][O_A_DT_STAT] = stat[p.End()][O_DT_STAT];
	//	//output[0][O_A_DDL_STAT] = stat[p.End()][O_DDL_STAT];
	//	//output[0][O_A_DAY_LENGTH] = stat[p.End()][O_DAY_LENGTH];
	//	//output[0][O_A_DI50] = stat[p.End()][O_DI50];
	//	//output[0][O_A_T_MEAN] = stat[p.End()][O_T_MEAN];
	//	//output[0][O_A_TI50] = stat[p.End()][O_TI50];
	//}
	//

	//SetOutput(output);


	return msg;
}


//enum TFilialEvent{FILIAL_EMERGENCE_1, FILIAL_RE_EMERGENCE_1, DIAPAUSE, NB_EVENTS};

int GetEvent(double DD, const CWeatherDay& day)
{
	static const double DD_REQUIRED[NB_EVENTS] = {-1, 46.5, 81.8, 195.5, 334.3, 527.5, 1097.3};
	static const double TMAX_REQUIRED[NB_EVENTS] = {-999, 16.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	
	int ev = OVERWINTER;
	for(int e=0; e<NB_EVENTS; e++)
	{
		if( DD>DD_REQUIRED[e] && day[TMAX]>TMAX_REQUIRED[e] )
			ev = e;
	}

	return ev;
}

void CSpruceBarkBeetleLiteratureModel::GetDailyStatOhrn(CModelStatVector& stat)
{

	//This is where the model is actually executed
	stat.resize(m_weather.GetNbDay());
	stat.SetFirstTRef(m_weather.GetFirstTRef());
	
	//we simulate 2 years at a time. 
	//we also manager the possibility to have only one year
	for(int y1=0; y1<m_weather.GetNbYear(); y1++)
	{

		int nbYear = 1;//m_bFertilEgg?2:1;
		for(int y=0; y<nbYear && y1+y<m_weather.GetNbYear(); y++)
		{
			static const double DD_MEAN[NB_EVENTS] =		{0, 4.336, 3.370, 4.662, 4.679, 6.081, 6.224};
			static const double DD_SD[NB_EVENTS] =			{0, 0.400, 0.537, 0.451, 0.573, 0.249, 0.521};
			static const double SWARMING_MEAN[NB_EVENTS] =	{0, 94.8,  75.3,  57.9,  46.4,  20.1,  14.7};
			static const double SWARMING_SD[NB_EVENTS] =	{0,  9.7,  26.8,  33.3,  27.9,  21.5,  11.5};
			static const double MORTALITY[NB_EVENTS] =		{0,  2.0,   8.0,   1.1,   6.0,   1.3,   4.0};//mortality relive to non-swarming
			//static const double ALIVE_MEAN[NB_EVENTS] =		{  2.7,  21.6,   3.8,  44.7,  20.4,  63.9};
			//static const double ALIVE_SD[NB_EVENTS] =		{  8.8,   2.9,  12.1,  29.0,  21.2,  31.9};
			//static const double MORTALITY_MEAN[NB_EVENTS] = {  2.6,   3.1,  38.3,   8.9,  59.6,  21.5};
			//static const double MORTALITY_SD[NB_EVENTS] =	{  3.9,   4.2,  31.5,   8.5,  22.6,  28.0};


			double DDRequired[NB_EVENTS] = {0};
			double swarming[NB_EVENTS] = {0};
			//double mortality[NB_EVENTS] = {0};
			//double alive[NB_EVENTS] = {0};
			
			for(int i=FIRST_FLIGHT; i<NB_EVENTS; i++)
			{
				DDRequired[i] = RandomGenerator().RandLogNormal(DD_MEAN[i], DD_SD[i]);

				while( i>0 && DDRequired[i] <= DDRequired[i-1])//avoid non sense
				{
					while( log(DDRequired[i])<DD_MEAN[i]/2 || log(DDRequired[i])>DD_MEAN[i]*2)
						DDRequired[i] = RandomGenerator().RandLogNormal(DD_MEAN[i], DD_SD[i]);
				
					switch(i)
					{
						case FIRST_FLIGHT: break;
						case ONSET_INFESTATION:		DDRequired[i] += DDRequired[FIRST_FLIGHT]; break;
						case RE_EMERGENCE_1_START:
						case FILIAL_EMERGENCE_1_START: DDRequired[i] += DDRequired[ONSET_INFESTATION]; break;
						case RE_EMERGENCE_1:		DDRequired[i] += DDRequired[RE_EMERGENCE_1_START]; break;
						case FILIAL_EMERGENCE_1:	DDRequired[i] += DDRequired[FILIAL_EMERGENCE_1_START]; break;
						default: ASSERT(false);
					}
				}

				swarming[i] = RandomGenerator().RandNormal(SWARMING_MEAN[i], SWARMING_SD[i]);
				while( swarming[i]<0 || swarming[i]>100)
					swarming[i] = RandomGenerator().RandNormal(SWARMING_MEAN[i], SWARMING_SD[i]);
			}
			

			int yy=y1+y; 

			enum TState {EVENT, ACTIF, ALIVE, DEAD, NB_STATES};
			double parent[NB_STATES] = {OVERWINTER, 100, 0, 0};
			double filial[NB_STATES] = {-1, 100, 0, 0};

			//int e = OVERWINTER;
			double DD = 0;
			for(CTRef d=m_weather[yy].GetFirstTRef(); d<=m_weather[yy].GetLastTRef(); d++)
			{
				const CWeatherDay& day = m_weather[d];
				
				//spring emergence
				DD += day.GetDD(5);
				
				int ee = GetEvent(DD, day);
			
				if( ee!=int(parent[EVENT]))
				{
					int e = (int)parent[EVENT];
					//apply mortality
					//if( e==ONSET_INFESTATION || e==RE_EMERGENCE_START || e==RE_EMERGENCE_50)
					//{
					//	double actif = parent[ACTIF]*swarming[e]/100;
					//	double dead = (parent[ACTIF]-actif)*MORTALITY[e]/100;
					//	double alive = parent[ACTIF]-actif-dead;

					//	parent[ACTIF] = actif;
					//	parent[DEAD] = dead;
					//	parent[ALIVE] = alive;
					//}

					parent[EVENT]=e;
				}
				
				if( ee!=int(filial[EVENT]) )
				{
					int e = (int)filial[EVENT];
					//if( e==ONSET_INFESTATION || e==FILIAL_EMERGENCE_START || e==FILIAL_EMERGENCE_50)
					//{
					//	double actif = filial[ACTIF]*swarming[e]/100;
					//	double dead = (filial[ACTIF]-actif)*MORTALITY[e]/100;
					//	double alive = filial[ACTIF]-actif-dead;

					//	filial[ACTIF] = actif;
					//	filial[DEAD] = dead;
					//	filial[ALIVE] = alive;
					//}
					
					filial[EVENT]=e;
				}

				//stat[d][e] = 100;


				//compute attrition

				HxGridTestConnection();
			}
		}
	}
}


//**************************
//this method is called to load parameters in variables
ERMsg CSpruceBarkBeetleLiteratureModel::ProcessParameter(const CParameterVector& parameters)
{
	ASSERT( m_weather.GetNbYear() > 0);

    ERMsg msg;

	int c = 0;

	//m_model = parameters[c++].GetInt();
	//m_bApplyMortality = parameters[c++].GetBool();
	

	return msg;
}



//Simulated Annealing 
void CSpruceBarkBeetleLiteratureModel::AddDailyResult(const CStringVector& header, const CStringVector& data)
{
	if( data.size() == 8)
	{
		std::vector<double> obs(1);
	
		CTRef ref(data[2].ToInt(), data[3].ToInt()-1, data[4].ToInt()-1);
		obs[0] = data[6].ToDouble();//=="BEGIN"?0:data[6]=="PEAK"?1:data[6]=="END"?2:-1;

		m_SAResult.push_back( CSAResult(ref, obs) );
	}
}


using namespace DAILY_DATA;
void CSpruceBarkBeetleLiteratureModel::GetFValueDaily(CStatisticXY& stat)
{
	ERMsg msg;

	if( m_SAResult.size() > 0)
	{
		//look to see if all ai are in growing order
	/*	bool bValid=true;
		for(int s=1; s<NB_PARAM&&bValid; s++)
		{
			if( m_CR.m_a[s] < m_CR.m_a[s-1] )
				bValid = false;
		}
		
		CFL::CStatistic statDD[NB_PARAM];
		CFL::CStatistic::SetVMiss(-999);
		if( bValid )
		{
			CModelStatVector statSim;
			m_CR.Execute(m_weather, statSim);
			for(int d=0; d<statSim.size(); d++)
			{
				for(int s=0; s<NB_PARAM; s++)
				{
					if( statSim[d][CCanolaContinuingRatio::O_FIRST_STAGE+s+1] > 1 &&
						statSim[d][CCanolaContinuingRatio::O_FIRST_STAGE+s+1] < 99 )
					{
						double DD =statSim[d][CCanolaContinuingRatio::O_DD]; 
						statDD[s]+=DD;
					}
				}
			}
		}

		
		for(int s=0; s<NB_PARAM; s++)
		{
			for(int t=0; t<2; t++)
			{
				double obs= m_SAResult[0].m_obs[s*2+t];
				double sim= statDD[s][t==0?LOWEST:HIGHEST];
				stat.Add(sim, obs); 
			}
		}*/
	}
}








//
//void CCanolaModel::AddDailyResult(const CStringVector& header, const CStringVector& data)
//{
//	ASSERT( header.size() == NB_PARAM*2+1 );
//	ASSERT( header[0] == "ID");
//	
//
//	std::vector<double> obs(16);
//	for(int i=0; i<16; i++)
//		obs[i] = data[i+1].ToDouble();
//	
//	m_SAResult.push_back( CSAResult(CTRef(), obs ) );
//}
//
//void CCanolaModel::GetFValueDaily(CFL::CStatisticXY& stat)
//{

//}
