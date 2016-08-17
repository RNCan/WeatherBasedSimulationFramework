//*********************************************************************
// File: SafranyikModel.cpp
//
// Class: CLoganModel
//
//************** MODIFICATIONS  LOG ********************
// 18/06/2013   Rémi Saint-Amant    Creation
//*********************************************************************
#include "MPB-LoganModel.h"
#include "EntryPoint.h"


//this line link this model with the EntryPoint of the DLL
static const bool bRegistred = 
	CModelFactory::RegisterModel( CLoganModel::CreateObject );




//**************************
//set if needs MTClim computation( Radiation, Vapor Pressure Defficit)
CLoganModel::CLoganModel()
{
	//**************************
	//put the number of input parameters
	//NB_INPUT_PARAMETER is used to determine if the dll
	//uses the same number of parameters than the model interface
	NB_INPUT_PARAMETER = 7;
	VERSION = "2.2 (2013)";

}

CLoganModel::~CLoganModel()
{
}

enum TAnnualStat{O_LOGAN, O_LOGAN2b, NB_OUTPUT_STATS };

typedef CModelStatVectorTemplate<NB_OUTPUT_STATS> CAnnualStatVector;


//**************************
//This method is called to compute the solution
ERMsg CLoganModel::OnExecuteAnnual()
{
	_ASSERTE( m_weather.GetNbYear() >= 2);

	ERMsg msg;
	

	CAnnualStatVector stat(model.GetNbYear(), CTRef((short)model.GetYear(0)) );

	// save result to disk
	for( int y=0; y<(int)model.GetNbYear(); y++)
	{
		//compute Cold Tolerance
		CMPBColdTolerance coldTolerance;

		coldTolerance.ComputeAnnual(weather);
		const CMPBCTResultVector& CT=coldTolerance.GetResult();

	//***********************************
	// fill accumulator

		CAccumulator accumulator(m_n);

		for(int y=0; y<weather.GetNbYear(); y++)
		{
			const CWeatherYear& weatherYear = weather[y];
			CTPeriod p = weatherYear.GetGrowingSeason();

			CAccumulatorData data;
			//Th = 42 F, Aug 1 to end of effective growing season
			if( p.End().GetMonth() >= AUGUST )
			{
				p.Begin().SetJDay(FIRST_DAY); 
				p.Begin().m_month = AUGUST;
				data.m_DDHatch = weatherYear.GetDD(5.56, p); 
				//Th = 42 F, whole year
				data.m_DDGen   = weatherYear.GetDD(5.56); 
			}

			data.m_lowestMinimum = weatherYear.GetStat(STAT_TMIN, LOWEST);
			data.m_meanMaxAugust = weatherYear[AUGUST].GetStat(STAT_TMAX, MEAN);
			data.m_totalPrecip = weatherYear.GetStat(STAT_PRCP, SUM); 
			//wather deficit was in mm

			CThornthwaitePET TPET(weatherYear, 0, CThornthwaitePET::POTENTIEL_STANDARD);
			data.m_waterDeficit = TPET.GetWaterDeficit(weatherYear)/25.4; //Water deficit, in inches
		
			data.m_precAMJ = 0;
			data.m_precAMJ += weatherYear[APRIL].GetStat(STAT_PRCP, SUM);
			data.m_precAMJ += weatherYear[MAY].GetStat(STAT_PRCP, SUM);
			data.m_precAMJ += weatherYear[JUNE].GetStat(STAT_PRCP, SUM);
			data.m_stabilityFlag = GetStabilityFlag(weatherYear);

			_ASSERTE( CT[y].m_year == weatherYear.GetYear());
			//Skip the first year: No Cold Resistance
			data.m_S = (y>0)?CT[y].m_Psurv:1;

			accumulator.push_back(data);
		}

		accumulator.ComputeMeanP_Y1_Y2();


	//***********************************
	
		if( m_runLength<=0 || m_runLength>weather.GetNbYear())
			m_runLength = weather.GetNbYear();


		//skip the first year if m_runLength == weather.GetNbYear()-1
		int s0 = max(1, m_runLength-1);
		m_firstYear = weather.GetFirstYear() + s0;//keep in memory the first year

		for(int i=0; i<NB_OUTPUT; i++)
		{
			//on doit changer le code si on veux utiliser la notion de runlength
			for(int y=s0; y<weather.GetNbYear(); y++)
			{
				m_F[i].push_back(GetProbability(accumulator, i, y, m_runLength) );
			}
		}
	}

	SetOutput(stat);

    return msg;
}




//**************************
//this method is called to load parameters in your variables
ERMsg CLoganModel::ProcessParameter(const CParameterVector& parameters)
{
    ERMsg msg;

    //transfer your parameter here
	m_parameters = parameters;
    
    return msg;
}


