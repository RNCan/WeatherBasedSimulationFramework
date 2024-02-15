//**********************************************************************
// 14/02/2024	1.0		Rémi Saint-Amant	Creation
// 
// convert in C++ from Robert Hijmans
// November 2009
// License GPL3
//
// based on :
// MkBCvars.AML
// Author Robert Hijmans
// January 2006
// Museum of Vertebrate Zoology, UC Berkeley
//
// Version 2.3
// These summary Bioclimatic variables are after :
//   Nix, 1986. A biogeographic analysis of Australian elapid snakes.In: R.Longmore(ed.).
//      Atlas of elapid snakes of Australia.Australian Flora and Fauna Series 7.
//      Australian Government Publishing Service, Canberra.
//
// and Expanded following the ANUCLIM manual
// 
//**********************************************************************

#include "WorldClimVars.h"
#include <valarray>
//#include "Basic/Evapotranspiration.h"
//#include "Basic/DegreeDays.h"
//#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CWorldClimVarsModel::CreateObject);

	//Constructor
	CWorldClimVarsModel::CWorldClimVarsModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2024)";
	}

	CWorldClimVarsModel::~CWorldClimVarsModel()
	{}



	// function to create19 BIOCLIM variables from
	// monthly T - min, T - max, and Precipitation data
	//
	// BIO_01 = Annual Mean Temperature
	// BIO_02 = Mean Diurnal Range(Mean of monthly(max temp - min temp))
	// BIO_03 = Isothermality(P2 / P7) (*100)
	// BIO_04 = Temperature Seasonality(standard deviation * 100)
	// BIO_05 = Max Temperature of Warmest Month
	// BIO_06 = Min Temperature of Coldest Month
	// BIO_07 = Temperature Annual Range(P5 - P6)
	// BIO_08 = Mean Temperature of Wettest Quarter
	// BIO_09 = Mean Temperature of Driest Quarter
	// BIO_10 = Mean Temperature of Warmest Quarter
	// BIO_11 = Mean Temperature of Coldest Quarter
	// BIO_12 = Annual Precipitation
	// BIO_13 = Precipitation of Wettest Month
	// BIO_14 = Precipitation of Driest Month
	// BIO_15 = Precipitation Seasonality(Coefficient of Variation)
	// BIO_16 = Precipitation of Wettest Quarter
	// BIO_17 = Precipitation of Driest Quarter
	// BIO_18 = Precipitation of Warmest Quarter
	// BIO_19 = Precipitation of Coldest Quarter
	// 
	
	enum TAnnualStat {
		O_BIO_01, O_BIO_02, O_BIO_03, O_BIO_04, O_BIO_05, O_BIO_06, O_BIO_07, O_BIO_08, O_BIO_09, O_BIO_10,
		O_BIO_11, O_BIO_12, O_BIO_13, O_BIO_14, O_BIO_15, O_BIO_16, O_BIO_17, O_BIO_18, O_BIO_19, NB_ANNUAL_STATS
	};


	//this method is call to load your parameter in your variable
	ERMsg CWorldClimVarsModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		int c = 0;


		return msg;
	}


	ERMsg CWorldClimVarsModel::OnExecuteAnnual()
	{
		ERMsg msg;

		
		m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_ANNUAL_STATS, -999);


		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			//int year = m_weather.GetFirstYear() + int(y);

			//std::valarray<float> Tmin = { 10,12,14,16,18,20,22,21,19,17,15,12 };
			//std::valarray<float> Tmax = Tmin+5;
			//std::valarray<float> Tavg = (Tmin + Tmax) / 2.0;
			//std::valarray<float> Prcp = { 0, 2, 10, 30, 80, 160, 80, 20, 40, 60, 20, 0 };

			std::valarray<float> Tmin(12);
			std::valarray<float> Tmax(12);
			std::valarray<float> Tavg(12);
			std::valarray<float> Prcp(12);
			
			for (size_t m = 0; m < 12; m++)
			{
				Tmin[m] = m_weather[y][m].GetStat(H_TMIN)[MEAN];
				Tmax[m] = m_weather[y][m].GetStat(H_TMAX)[MEAN];
				Tavg[m] = (Tmin[m] + Tmax[m]) / 2.0;
				Prcp[m] = m_weather[y][m].GetStat(H_PRCP)[SUM];
			}

			std::valarray<float> wet(12);
			std::valarray<float> tmp(12);

			for (size_t m = 0; m < 12; m++)
			{
				tmp[m] = (Tavg[m] + Tavg[(m + 1) % 12] + Tavg[(m + 2) % 12]) / 3.0;
				wet[m] = Prcp[m] + Prcp[(m + 1) % 12] + Prcp[(m + 2) % 12];
			}
		
			// P1.Annual Mean Temperature
			m_output[y][O_BIO_01] = Tavg.sum() / Tavg.size();
			// P2.Mean Diurnal Range(Mean(period max - min))
			m_output[y][O_BIO_02] = (Tmax - Tmin).sum() / Tmin.size();
			// P4.Temperature standard deviation
			float mean_T = Tavg.sum() / Tavg.size();
			float ss_T = ((Tavg - mean_T) * (Tavg - mean_T)).sum();		//sum of squares
			m_output[y][O_BIO_04] = 100 * sqrt(ss_T /(Tavg.size()-1));

			// P5.Max Temperature of Warmest Period
			m_output[y][O_BIO_05] = Tmax.max();
			// P6.Min Temperature of Coldest Period
			m_output[y][O_BIO_06] = Tmin.min();
			// P7.Temperature Annual Range(P5 - P6)
			m_output[y][O_BIO_07] = m_output[y][O_BIO_05] - m_output[y][O_BIO_06];
			// P3.Isothermality(P2 / P7)
			m_output[y][O_BIO_03] = 100 * m_output[y][O_BIO_02] / m_output[y][O_BIO_07];
			// P12.Annual Precipitation
			m_output[y][O_BIO_12] = Prcp.sum();
			// P13.Precipitation of Wettest Period
			m_output[y][O_BIO_13] = Prcp.max();
			// P14.Precipitation of Driest Period
			m_output[y][O_BIO_14] = Prcp.min();
			// P15.Precipitation Seasonality(Coefficient of Variation)
			// the "1 +" is to avoid strange CVs for areas where mean rainfall is < 1)
			float mean_P = (Prcp + 1).sum() / Prcp.size();
			float ss_P = (((Prcp + 1) - mean_P) * ((Prcp + 1) - mean_P)).sum();		//sum of squares
			m_output[y][O_BIO_15] = 100*sqrt(ss_P / (Tavg.size()-1)) / mean_P;


			// precipitation by quarter(3 months)
			// P16.Precipitation of Wettest Quarter
			m_output[y][O_BIO_16] = wet.max();
			// P17.Precipitation of Driest Quarter
			m_output[y][O_BIO_17] = wet.min();
			// P8.Mean Temperature of Wettest Quarter
			size_t wetqrt = std::distance(begin(wet), std::max_element(begin(wet), end(wet)));
			m_output[y][O_BIO_08] = tmp[wetqrt];
			// P9.Mean Temperature of Driest Quarter
			size_t dryqrt = std::distance(begin(wet), std::min_element(begin(wet), end(wet)));
			m_output[y][O_BIO_09] = tmp[dryqrt];
			// P10 Mean Temperature of Warmest Quarter
			m_output[y][O_BIO_10] = tmp.max();
			// P11 Mean Temperature of Coldest Quarter
			m_output[y][O_BIO_11] = tmp.min();


			// P18.Precipitation of Warmest Quarter
			size_t hot = std::distance(begin(tmp), std::max_element(begin(tmp), end(tmp)));
			m_output[y][O_BIO_18] = wet[hot];
			// P19.Precipitation of Coldest Quarter
			size_t cold = std::distance(begin(tmp), std::min_element(begin(tmp), end(tmp)));
			m_output[y][O_BIO_19] = wet[cold];

		}

		return msg;
	}


}


