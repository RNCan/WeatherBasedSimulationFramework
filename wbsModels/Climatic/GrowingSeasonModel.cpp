//*********************************************************************
// 20/09/2016	1.2.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 18/07/2016	1.1.1	Rémi Saint-Amant	Using CTRef as output
// 21/01/2016	1.1.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 07/02/2012	1.0		Rémi Saint-Amant	Creation
//
//*********************************************************************

#include "Basic/WeatherDefine.h"
#include "Basic/GrowingSeason.h"
//#include "Evapotranspiration.h"
#include "ModelBase/EntryPoint.h"
#include "GrowingSeasonModel.h"


using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CGrowingSeasonModel::CreateObject);

	//enum TOutput { O_FIRST_DAY, O_LAST_DAY, O_GS_LENGTH, NB_OUTPUT };

	//typedef CModelStatVectorTemplate<NB_OUTPUT> CStatVector;


	CGrowingSeasonModel::CGrowingSeasonModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = 10;
		VERSION = "1.2.1 (2022)";


		//m_Ttype[BEGIN] = CGSInfo::TT_TMIN;
		//m_threshold[BEGIN] = 0;
		//m_nbDays[BEGIN] = 3;
		//m_Ttype[END] = CGSInfo::TT_TMIN;
		//m_threshold[END] = 0;
		//m_nbDays[END] = 3;

	}

	CGrowingSeasonModel::~CGrowingSeasonModel()
	{}

	//this method is call to load your parameter in your variable
	ERMsg CGrowingSeasonModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		enum TShift { INCLUDE_NB_DAYS, EXCLUDE_NB_DAYS, NB_SHIFT };

		//transfer your parameter here
		size_t c = 0;

		m_begin.m_d = (CGSInfo::TDirection)parameters[c++].GetInt();
		m_begin.m_TT = (CGSInfo::TTemperature)parameters[c++].GetInt();
		m_begin.m_op = (parameters[c++].GetInt() == 0) ? '<' : '>';
		m_begin.m_threshold = parameters[c++].GetReal();
		m_begin.m_nbDays = parameters[c++].GetInt();
		//m_begin.m_first_month = GetInfo().m_loc.m_lat > 0 ? JANUARY : JULY;
		//m_begin.m_last_month = GetInfo().m_loc.m_lat > 0 ? JUNE : DECEMBER;
		//size_t shiftType1 = parameters[c++].GetInt();
		//m_begin.m_shift = shiftType1 == INCLUDE_NB_DAYS ? (m_begin.m_d == CGSInfo::GET_FIRST ? -int(m_begin.m_nbDays) : int(m_begin.m_nbDays)) : 0;

		m_end.m_d = (CGSInfo::TDirection)parameters[c++].GetInt();
		m_end.m_TT = (CGSInfo::TTemperature)parameters[c++].GetInt();
		m_end.m_op = (parameters[c++].GetInt() == 0) ? '<' : '>';
		m_end.m_threshold = parameters[c++].GetReal();
		m_end.m_nbDays = parameters[c++].GetInt();
		//m_end.m_first_month = GetInfo().m_loc.m_lat > 0 ? JULY : JANUARY;
		//m_end.m_last_month = GetInfo().m_loc.m_lat > 0 ? DECEMBER : JUNE;

		//size_t shiftType2 = parameters[c++].GetInt();
		//m_end.m_shift = shiftType2 == INCLUDE_NB_DAYS ? (m_end.m_d == CGSInfo::GET_FIRST ? -int(m_end.m_nbDays) : int(m_end.m_nbDays)) : 0;


		ASSERT(c == NB_INPUT_PARAMETER);

		return msg;
	}

	//This method is call to compute solution
	ERMsg CGrowingSeasonModel::OnExecuteAnnual()
	{
		ERMsg msg;


		//size_t m_D[NB_INPUT];
		//size_t m_TT[NB_INPUT];
		//char   m_op[NB_INPUT];
		//double m_threshold[NB_INPUT];
		//size_t m_nbDays[NB_INPUT];
		//size_t m_first_month[NB_INPUT];//-1 = default (January/July for north and July/January for south hemisphere
		//size_t m_last_month[NB_INPUT];//-1 = default (June/December for north and December/June for south hemisphere
		//size_t m_shiftType;


		//CGSInfo begin(m_Ttype[BEGIN], m_nbDays[BEGIN], m_threshold[BEGIN], m_Ttype[END], m_nbDays[END], m_threshold[END]);
		//CGSInfo end(m_Ttype[BEGIN], m_nbDays[BEGIN], m_threshold[BEGIN], m_Ttype[END], m_nbDays[END], m_threshold[END])

		////Init class member
		CGrowingSeason GS(m_begin, m_end);


		//Create output from result
		//CModelStatVector output;// (m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL)));
		GS.Execute(m_weather, m_output);
		//always output as daily values????
		//m_output.Transform(CTM(CTM::DAILY), 0);
		//m_output.Transform(CTM(CTM::DAILY), 1);


		//m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), NB_OUTPUT);
		//for (size_t y = 0; y < m_weather.size(); y++)
		//{
		//	CTPeriod p = GS.GetGrowingSeason(m_weather[y]); 
		//	p.Transform(CTM(CTM::DAILY));
		//
		//	m_output[y][O_FIRST_DAY] = p.Begin().GetRef();//p.Begin().Get__int32();
		//	m_output[y][O_LAST_DAY] = p.End().GetRef();//p.End().Get__int32(); 
		//	m_output[y][O_GS_LENGTH] = p.GetLength();
		//}

		

		return msg;
	}
	/*
	double CGrowingSeasonModel::GetT(const CWeatherDay& Wday, int t)
	{
	ASSERT( t>=0 && t<NB_TEMP);

	double T=0;
	switch(t)
	{
	case IN_TMIN:	T=Wday.GetTMin(); break;
	case IN_TMEAN:	T=Wday.GetTMean(); break;
	case IN_TMAX:	T=Wday.GetTMax(); break;
	default: ASSERT(false);
	}

	return T;
	}

	CTPeriod CGrowingSeasonModel::GetGrowingSeason(const CWeatherYear& weather, bool bAlwaysFillPeriod)const
	{

	CTPeriod p = weather.GetTPeriod();

	int d = 200; //(Mid-July)
	bool bGetIt=false;

	//Beginning of the growing season
	//look backward for the first occurrence of 3 successive days with frost
	do
	{
	d--;
	bGetIt = true;

	for(int dd=0; dd<m_nbDays[BEGIN]&&bGetIt; dd++)
	{
	ASSERT( d-dd>=0 );

	const CWeatherDay& Wday = weather.GetDay(d-dd);
	bGetIt = bGetIt && GetT(Wday, m_Ttype[BEGIN]) < m_threashold[BEGIN];
	}

	} while(!bGetIt && d>=m_nbDays[BEGIN]);


	if(bGetIt)
	{
	p.Begin().SetJDay(weather.GetYear(), d+1);
	}


	//End of growing season
	d = 200;
	do
	{
	d++;
	//look for the first occurrence of 3 successive days with frost
	bGetIt = true;
	for(int dd=0; dd<m_nbDays[END]&&bGetIt; dd++)
	{
	ASSERT( d+dd<weather.GetNbDay() );

	const CWeatherDay& Wday = weather.GetDay(d+dd);
	bGetIt = bGetIt && GetT(Wday, m_Ttype[END]) < m_threashold[END];
	}
	} while(!bGetIt && d<weather.GetNbDay()-m_nbDays[END]);

	if(bGetIt)
	{
	p.End().SetJDay( weather.GetYear(), d-1);
	}

	if( p.End() <= p.Begin() )
	{
	if( bAlwaysFillPeriod )
	p.End() = p.Begin() = CTRef(weather.GetYear(), JULY, 14);
	else
	p.Reset();
	}

	return p;
	}
	*/


}