//*********************************************************************
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

	enum TOutput { O_FIRST_DAY, O_LAST_DAY, O_GS_LENGTH, NB_OUTPUT };

	//typedef CModelStatVectorTemplate<NB_OUTPUT> CStatVector;


	CGrowingSeasonModel::CGrowingSeasonModel()
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER = 6;
		VERSION = "1.1.1 (2016)";


		m_Ttype[BEGIN] = CGSInfo::TT_TMIN;
		m_threshold[BEGIN] = 0;
		m_nbDays[BEGIN] = 3;
		m_Ttype[END] = CGSInfo::TT_TMIN;
		m_threshold[END] = 0;
		m_nbDays[END] = 3;

	}

	CGrowingSeasonModel::~CGrowingSeasonModel()
	{}

	//this method is call to load your parameter in your variable
	ERMsg CGrowingSeasonModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;


		//transfer your parameter here
		short c = 0;

		m_nbDays[BEGIN] = parameters[c++].GetInt();
		m_Ttype[BEGIN] = parameters[c++].GetInt();
		m_threshold[BEGIN] = parameters[c++].GetReal();
		m_nbDays[END] = parameters[c++].GetInt();
		m_Ttype[END] = parameters[c++].GetInt();
		m_threshold[END] = parameters[c++].GetReal();
		ASSERT(c == NB_INPUT_PARAMETER);

		return msg;
	}

	//This method is call to compute solution
	ERMsg CGrowingSeasonModel::OnExecuteAnnual()
	{
		ERMsg msg;

		

		//Init class member
		CGrowingSeason GS(m_Ttype[BEGIN], m_nbDays[BEGIN], m_threshold[BEGIN], m_Ttype[END], m_nbDays[END], m_threshold[END]);
		
		//Create output from result
		//CModelStatVector output;// (m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL)));
//		GS.Execute(m_weather, output);


		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), NB_OUTPUT);
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			CTPeriod p = GS.GetGrowingSeason(m_weather[y]); 
			p.Transform(CTM(CTM::DAILY));

			m_output[y][O_FIRST_DAY] = p.Begin().GetRef();//p.Begin().Get__int32();
			m_output[y][O_LAST_DAY] = p.End().GetRef();//p.End().Get__int32(); 
			m_output[y][O_GS_LENGTH] = p.GetLength();
		}

		//set output
		//SetOutput(output);


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