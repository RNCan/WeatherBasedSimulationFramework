//*********************************************************************
// File: SafranyikModel.cpp
//
// Class: CSLR
//
//************** MODIFICATIONS  LOG ********************
// 23/01/2018	Rémi Saint-Amant    Bug correction in computing logan stability
// 04/05/2017	Rémi Saint-Amant    New compile
// 11/05/2016   Rémi Saint-Amant    Compile with WBSF
// 25/09/2012   Rémi Saint-Amant    Compile with new developement table
// 31/10/2007	Rémi Saint-Amant    Remove NEWMAT, use AllenWave and tableLookup
// 29/08/2007   Rémi Saint-Amant    Add Cold tolerance 
// 01/09/2003   Rémi Saint-Amant    Creation
//*********************************************************************

#include "SLR.h"
#include "Generations.h"
#include "../MPBColdTolerance.h"
#include "../MPBDevRates.h"
#include "../DevelopementVector.h"

#include "Basic/GrowingSeason.h"
#include "Basic/DegreeDays.h"
#include "Basic/weatherStation.h"
#include "Basic/TimeStep.h"
#include "Basic/Evapotranspiration.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//*******************************************************
	//CSafranyikLoganRegniere (CSLR)
	CSLR::CSLR(const CRandomGenerator& RG) :
		m_RG(RG)
	{
		// initialise your variables here (optionnal)
		m_nbGeneration = 50;
		m_overheat = 0;
		m_dayStart = 199;
		m_minOvipDate = 179;
		m_maxOvipDate = 242;
		m_runLength = -1;
	}

	CSLR::~CSLR()
	{
	}

	//**************************
	//This method is called to compute the solution
	ERMsg CSLR::Execute(const CWeatherStation& weather)
	{
		ERMsg msg;


		//compute Cold Tolerance
		CMPBColdTolerance coldTolerance;

		coldTolerance.ComputeAnnual(weather);
		const CMPBCTResultVector& CT = coldTolerance.GetResult();

		//***********************************
		// fill accumulator


		CAccumulator accumulator(m_n);

		for (size_t y = 0; y < weather.GetNbYears(); y++)
		{
			const CWeatherYear& weatherYear = weather[y];

			CGrowingSeason GS(CGSInfo::TT_TMIN, 3, 0, CGSInfo::TT_TMIN, 3, 0);
			CTPeriod p = GS.GetGrowingSeason(weatherYear);
			p.Transform(CTM::DAILY);

			//est-ce que c'est la même chose qu'avant???
			//			CTPeriod p = weatherYear.GetGrowingSeason();

			CAccumulatorData data;
			//Th = 42 F, Aug 1 to end of effective growing season
			if (p.End().GetMonth() >= AUGUST)
			{
				p.Begin().SetJDay(FIRST_DAY);
				p.Begin().m_month = AUGUST;

				CDegreeDays DD(CDegreeDays::DAILY_AVERAGE, 5.56);
				data.m_DDHatch = DD.GetDD(weatherYear, p);
				//data.m_DDHatch = weatherYear.GetDD(5.56, p);
				//Th = 42 F, whole year
				//data.m_DDGen = weatherYear.GetDD(5.56);
				data.m_DDGen = DD.GetDD(weatherYear);
			}

			data.m_lowestMinimum = weatherYear[H_TMIN][LOWEST];//weatherYear.GetStat(STAT_TMIN, LOWEST);
			data.m_meanMaxAugust = weatherYear[AUGUST][H_TMAX][MEAN];//.GetStat(STAT_TMAX, MEAN);
			data.m_totalPrecip = weatherYear[H_PRCP][SUM];//GetStat(STAT_PRCP, SUM);
			//wather deficit was in mm

			//CThornthwaiteET TPET(CThornthwaiteET::POTENTIEL_STANDARD);
			//CModelStatVector PET;
			//TPET.GetET(.Execute(PET);
			//(weatherYear, 0, CThornthwaiteET::POTENTIEL_STANDARD);
			//data.m_waterDeficit = TPET.GetWaterDeficit(weatherYear) / 25.4; //Water deficit, in inches
			data.m_waterDeficit = GetWaterDeficit(weatherYear) / 25.4; //Water deficit, in inches

			data.m_precAMJ = 0;
			data.m_precAMJ += weatherYear[APRIL][H_PRCP][SUM];//GetStat(STAT_PRCP, SUM);
			data.m_precAMJ += weatherYear[MAY][H_PRCP][SUM];	 //GetStat(STAT_PRCP, SUM);
			data.m_precAMJ += weatherYear[JUNE][H_PRCP][SUM]; //GetStat(STAT_PRCP, SUM);
			data.m_stabilityFlag = GetStabilityFlag(weatherYear);

			_ASSERTE(CT[y].m_year == weatherYear.GetTRef().GetYear());
			//Skip the first year: No Cold Resistance
			data.m_S = (y > 0) ? CT[y].m_Psurv : 1;

			accumulator.push_back(data);
		}

		accumulator.ComputeMeanP_Y1_Y2();


		//***********************************

		if (m_runLength == 0 || m_runLength > weather.GetNbYears())
			m_runLength = weather.GetNbYears();


		//skip the first year if m_runLength == weather.GetNbYears()-1
		size_t  s0 = max((size_t)1, m_runLength - 1);
		m_firstYear = weather.GetFirstYear() + int(s0);//keep in memory the first year

		for (size_t i = 0; i < NB_OUTPUT; i++)
		{
			//on doit changer le code si on veux utiliser la notion de runlength
			for (size_t y = s0; y < weather.GetNbYears(); y++)
			{
				m_F[i].push_back(GetProbability(accumulator, i, y, m_runLength));
			}
		}


		return msg;
	}


	double CSLR::GetWaterDeficit(const CWeatherYear& weather)const
	{
		CThornthwaiteET TPET(CThornthwaiteET::POTENTIEL_STANDARD);

		double I = TPET.GetI(weather);
		//TPET.GetET(.Execute(PET);
		//(weatherYear, 0, CThornthwaiteET::POTENTIEL_STANDARD);
		//data.m_waterDeficit = TPET.GetWaterDeficit(weatherYear) / 25.4; //Water deficit, in inches

		double A = 0;
		for (size_t m = 0; m < 12; m++)
		{
			if (weather[m][H_TNTX][MEAN] > 0)
			{
				double ET = TPET.GetET(weather[m], I);
				//precipitation in mm
				double A_tmp = (ET - weather[m][H_PRCP][SUM]);
				if (A_tmp > 0)
					A += A_tmp;
			}
		}

		return A;
	}

	//********************************************************************
	// This is the main module of Logan's MPB seasonality model It is passed 
	//
	// Input:
	//  t : 1 years of hourly temperatures 
	//  nbGen: number of generations for the stability test. nbGen > 0.
	//  dayStart: initial oviposition date. minOvipDate <= dayStart <= maxOvipDate.
	//  minOvipDate and maxOvipDate: biologically feasible min and max ovip date (model parameters)
	//
	// Output :
	//  false: unstable seasonality; true: stable seasonality
	//********************************************************************
	bool CSLR::GetStabilityFlag(const CWeatherYear& weatherYear)
	{
		bool bStabilityFlag = false;

		//**********************************
		// first part : compute developpement rates

		//init the CMPBDevelopmentVector
		/*CDailyWaveVector t;
		weatherYear.GetHourlyGeneration(t, HG_ALLEN_WAVE, 1, 3, m_overheat);

		CMPBDevelopmentVector devRates(m_RG);
		devRates.Init(t);*/


		CMPBDevelopmentVector devRates(m_RG);
		devRates.resize(weatherYear.GetNbDays());


		for (size_t d = 0; d < weatherYear.GetNbDays(); d++)
		{
			CDailyWaveVector t;
			weatherYear.GetDay(d).GetHourlyGeneration(t, HG_ALLEN_WAVE, 1, 3, m_overheat);
			ASSERT(t.size() == 24);

			for (int s = 0; s < NB_STAGES; s++)
			{
				devRates[d][s] = 0; 
				
				for (size_t h = 0; h < 24; h++)
					devRates[d][s] += devRates.MPB_RATES_TABLE.GetRate(s, t[h]) / 24.0;
			}
		}

		//RATE_TABLE.Save("d:\\RateTable.csv");

		//**********************************
		// second part : Evaluate stabilty over years

		CGenerationVector generations;
		if (generations.SimulateGeneration(m_nbGeneration, m_dayStart, devRates))
		{
			//if the generation is succesfull
			//test criterias to know the stablility
			bStabilityFlag = generations.GetStabilityFlag(m_minOvipDate, m_maxOvipDate);
		}

		//generations.Save("d:\\newgen.csv");

		return bStabilityFlag;
	}

	double CSLR::GetProbability(CAccumulator& acc, size_t model, size_t y0, size_t runLength)
	{
		_ASSERTE(acc.size() > 1);

		//Summarize the overall result:

		double p = 0;

		switch (model)
		{
		case LOGAN:			p = acc.GetProbability(CAccumulator::P_LOGAN, y0, runLength); break;
		case LOGAN2b:		p = acc.GetProbability(CAccumulator::P_LOGAN2b, y0, runLength); break;//2b call a new way to compute value
		case SAFRANYIK:		p = acc.GetProbability(CAccumulator::P_SAFRANYIK, y0, runLength); break;
		case SAFRANYIK_P3P4:	p = acc.GetProbability(CAccumulator::P_SAFRANYIK_P3P4, y0, runLength); break;
		case COLD_TOLERANCE:p = acc.GetPsurv(y0, runLength); break;
		case HYBRID_CT:		p = acc.GetProbability(CAccumulator::P_HYBRID_CT, y0, runLength); break;
		default: _ASSERTE(false);
		}


		return p;
	}


	//**************************
	//this method is called to load parameters in your variables
	ERMsg CSLR::ProcessParameter(const CParameterVector& parameters)
	{
		_ASSERTE(parameters.size() == 7);

		ERMsg msg;

		//transfer your parameter here
		m_nbGeneration = parameters[0].GetInt();
		m_overheat = parameters[1].GetReal();
		m_dayStart = parameters[2].GetInt() - 1;
		m_minOvipDate = parameters[3].GetInt() - 1;
		m_maxOvipDate = parameters[4].GetInt() - 1;
		m_runLength = parameters[5].GetInt();
		m_n = parameters[6].GetInt();


		_ASSERT(m_nbGeneration > 0);
		_ASSERT(m_dayStart >= 0 && m_dayStart < 365);
		_ASSERT(m_minOvipDate >= 0 && m_minOvipDate < 365);
		_ASSERT(m_maxOvipDate >= 0 && m_maxOvipDate < 365);

		return msg;
	}

}