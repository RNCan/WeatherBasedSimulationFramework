//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Jacques Régnière, Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// Description: 
//				This module take Normal/Daily/Hourly database and generate
//				weather variable (TMIN, TMAX, PRCP, TDEW, RELH, WNDS) at
//				a target location. They generate all iteration at the same time
//
// Reference:
//		Weather-regime assembler
//		Régnière, J., St-Amant, R. 2007. 
//		Stochastic simulation of daily air temperature and precipitation from monthly normals 
//		in North America north of Mexico. International Journal of Biometeorology. 51:415-430.
//
//******************************************************************************
//13/09/2016	Rémi Saint-Amant	Change Tair and Trng by Tmin and Tmax
//24/02/2016	Rémi Saint-Amant	Add gribs database to the weather generator
//01/01/2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//05/03/2015	Rémi Saint-Amant	Update with BioSIM11
//07/12/2014	Rémi Saint-Amant	Change CWeather by CWeatherStation
//08/11/2013	Rémi Saint-Amant	Add hourly generation; remove MFC, 64 bits
//10/02/2012	Rémi Saint-Amant	new name, new random genrator, thead safe
//15/12/2011	Rémi Saint-Amant	Compute snow with new algorithm
//14/02/2011	Rémi Saint-Amant	Add forecast switch to avoid use of forecast data
//12/12/2007	Rémi Saint-Amant	Integration of radiation and MTClim43
//29/11/2007	Rémi Saint-Amant	Integration of many new variables
//16/01/2007	Rémi Saint-Amant	new normal and daily calculation
//15/01/2006	Rémi Saint-Amant	new gradient calculation
//25/10/2003	Rémi Saint-Amant	new normal
//05/10/2002	Rémi Saint-Amant	Add Radiation
//25/01/2001	Jacques Regniere	Month Adjustement to simulate month variation
//17/01/2001	Rémi Saint-Amant	Use A GIDS algorithme to find termal gradients
//12/01/2001	Rémi Saint-Amant	Take charge of Iteration January 2001 by R.Saint-Amant
//18/09/1998	Rémi Saint-Amant	Use of fileManager
//04/04/1998	Rémi Saint-Amant	add in tempgen.cpp
//11/08/1998	Jacques Regniere	Translation to C
//******************************************************************************
#include "stdafx.h"   


#include "Basic/WeatherCorrection.h"
#include "Basic/Callback.h"
#include "Basic/CSV.h"
#include "Basic/openMP.h"
#include "ModelBase/MTClim43.h"
#include "ModelBase/SnowMelt.h"
#include "Simulation/WeatherGenerator.h"
#include "Geomatic/TimeZones.h"
#include "WeatherBasedSimulationString.h"





using namespace std;

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;
namespace WBSF
{

	//mbar	m
	//1013	0
	//1000	111
	//975	326
	//950	547
	//925	772
	//900	1002
	//875	1238
	//850	1479
	//825	1725
	//800	1978
	enum TGeoHeight{ GH_SURFACE, GH_1000, GH_975, GH_950, GH_925, GH_900, GH_875, GH_850, GH_825, GH_800, NB_GEO_HEIGHT = 20 };



	static const int NB_STATION_REGRESSION_LOCAL = 24;
	static const int NB_STATION_REGRESSION_REGIONAL = 100;
	static const CWVariables DERIVABLE_VARIABLES[NB_VAR_H] =
	{
		CWVariables("TDEW RELH SRAD SNOW SNDH SWE"), //Tmin
		CWVariables("TDEW RELH SRAD SNOW SNDH SWE"), //Tair
		CWVariables("TDEW RELH SRAD SNOW SNDH SWE"), //Tmax
		CWVariables("TDEW RELH SRAD SNOW SNDH SWE"), //Prcp
		CWVariables(""),//Tdew
		CWVariables(""),
		CWVariables("WND2"), //WndS
		CWVariables(""),	//WndD
		CWVariables(""),	//Srad
		CWVariables(""),	//pres
		CWVariables(""),
		CWVariables(""),
		CWVariables(""),
		CWVariables("WNDS"),
		CWVariables(""),
		CWVariables("")
	};

	static const CWVariables DERIVABLE_VARIABLES_INPUT[NB_VAR_H] =
	{
		CWVariables(""), //Tmin
		CWVariables("Tmin Tmax"), //Tair
		CWVariables(""), //Tmax
		CWVariables("Tair"), //Prcp
		CWVariables("Tair Prcp"),//Tdew
		CWVariables("Tair Prcp"),		//Reltive Humidity
		CWVariables("Wnd2"), //WndS
		CWVariables(""),	 //WndD
		CWVariables("Tair Prcp"),//Radiation
		CWVariables(""),//pressure
		CWVariables("Tair Prcp"),	//snow
		CWVariables("Tair Prcp"),	//snow depth
		CWVariables("Tair Prcp"),	//snow water equivalent
		CWVariables("WndS"),//Wnd2
		CWVariables(""),
		CWVariables("")
	};

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	CWeatherGenerator::CWeatherGenerator()
	{
		Reset();
	}

	CWeatherGenerator::~CWeatherGenerator()
	{}

	void CWeatherGenerator::Reset()
	{
		m_tgi.clear();			//TG Input
		m_target.clear();		//Target location

		m_nbReplications = 1;

		m_pNormalDB.reset();	//Normal database
		m_pDailyDB.reset();		//daily database
		m_pHourlyDB.reset();	//daily database

		//result
		m_gradients.reset();	//gradient at the target location
	}
	//****************************************************************************
	// Summary:		Initialize
	//
	// Description: clear output simulation and initialize random seed and 
	//				generation of weather gradient for the target
	//
	// Input:		
	//
	// Output:
	//
	// Note:		
	//****************************************************************************
	ERMsg CWeatherGenerator::Initialize(CCallback& callback)
	{
		ASSERT(m_target.IsValid());
		ASSERT(m_tgi.m_firstYear <= 0 || (m_tgi.m_firstYear >= 1800 && m_tgi.m_firstYear <= 2100));
		ASSERT(m_tgi.m_lastYear == 0 || (m_tgi.m_lastYear >= 1800 && m_tgi.m_lastYear <= 2100));
		ASSERT(m_tgi.m_nbNormalsStations >= 1 && m_tgi.m_nbNormalsStations <= 200);
		ASSERT(m_tgi.m_nbDailyStations >= 0 && m_tgi.m_nbDailyStations <= 200);
		ASSERT(m_tgi.m_seed >= CRandomGenerator::RANDOM_SEED && m_tgi.m_seed <= CRandomGenerator::FIXE_SEED);
		ASSERT(m_tgi.m_albedo >= 0 && m_tgi.m_albedo <= 1);
		ASSERT(m_nbReplications > 0);

		ERMsg msg;

		m_simulationPoints.clear();//clear simulation

		GenerateSeed();

		m_gradients.SetNormalsDatabase(m_pNormalDB);

		m_gradients.m_variables = m_tgi.GetNormalMandatoryVariables();
		if (m_gradients.m_variables[H_TMIN2] || m_gradients.m_variables[H_TAIR2] || m_gradients.m_variables[H_TMAX2])
		{
			m_gradients.m_variables.set(H_TMIN2);
			m_gradients.m_variables.set(H_TAIR2);
			m_gradients.m_variables.set(H_TMAX2);
		}

		m_gradients.m_allowDerivedVariables = m_tgi.m_allowedDerivedVariables;
		m_gradients.m_bXVal = m_tgi.m_bXValidation;
		m_gradients.m_target = m_target;

		m_gradients.CreateGradient(callback);
		//m_gradients.Save("c:\\tmp\\" + m_target.m_name + ".csv");


		return msg;
	}

	bool CWeatherGenerator::VerifyData(const CSimulationPointVector& simulationPoints, CWVariables variables1)
	{
		bool bRep = true;
		for (size_t i = 0; i < simulationPoints.size() && bRep; i++)
		{
			CTPeriod p = simulationPoints[i].GetEntireTPeriod();
			for (CTRef TRef = p.Begin(); TRef <= p.End() && bRep; TRef++)
			{
				CWVariables variables2 = simulationPoints[i][TRef].GetVariables();
				ASSERT(variables1 == variables2);
				bRep = variables1 == variables2;

				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					if (variables1[v])
					{

						ASSERT(simulationPoints[i][TRef][v].IsInit());
						ASSERT(!_isnan(simulationPoints[i][TRef][v][MEAN]));
						ASSERT(!IsMissing(simulationPoints[i][TRef][v][MEAN]));

						if (!simulationPoints[i][TRef][v].IsInit() || _isnan(simulationPoints[i][TRef][v][MEAN]) || IsMissing(simulationPoints[i][TRef][v][MEAN]))
							bRep = false;
					}
				}
			}
		}

		return bRep;
	}


	void CWeatherGenerator::CompleteSimpleVariables(CSimulationPoint& simulationPoint, CWVariables variables)
	{
		if (simulationPoint.empty())
			return;

		double pa = GetPressure(simulationPoint.m_alt);

		//compute direct hourly value. For example RH from Tdew and TAir or Ea from Tdew or Es from Tair
		CTPeriod period = simulationPoint.GetEntireTPeriod();
		for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
		{
			CDataInterface& data = simulationPoint[TRef];

			if (simulationPoint.IsHourly())
			{
				//in hourly, Tmin and Tmax is equal to Tair
				if (variables[H_TMIN2] && !data[H_TMIN2].IsInit() && data[H_TAIR2].IsInit())
					data.SetStat(H_TMIN2, data[H_TAIR2][MEAN]);

				if (variables[H_TMAX2] && !data[H_TMAX2].IsInit() && data[H_TAIR2].IsInit())
					data.SetStat(H_TMAX2, data[H_TAIR2][MEAN]);
			}
			else
			{
				if (variables[H_TAIR2] && !data[H_TAIR2].IsInit() && data[H_TMIN2].IsInit() && data[H_TMAX2].IsInit())
					data.SetStat(H_TAIR2, (data[H_TMIN2][MEAN] + data[H_TMAX2][MEAN]) / 2);
			}

			if (variables[H_PRES] && !data[H_PRES].IsInit())
			{
				data.SetStat(H_PRES, pa / 100);		//pressure [hPa]
			}

			if (variables[H_TDEW] && !data[H_TDEW].IsInit() && data[H_RELH].IsInit() && data[H_TAIR2].IsInit())
			{
				double Td = Hr2Td(data[H_TAIR2][MEAN], data[H_RELH][MEAN]);
				data.SetStat(H_TDEW, Td);
			}

			if (variables[H_RELH] && !data[H_RELH].IsInit() && data[H_TDEW].IsInit() && data[H_TAIR2].IsInit())
			{
				double Hr = Td2Hr(data[H_TAIR2][MEAN], data[H_TDEW][MEAN]);
				data.SetStat(H_RELH, Hr);
			}

			//WndS from Wnd2
			if (variables[H_WNDS] && !data[H_WNDS].IsInit() && data[H_WND2].IsInit())
			{
				double U2 = data[H_WND2][MEAN];//wind speed at 10 meters
				//[33]: Wind speed varies with height above the ground surface. For the calculation of ETsz, wind speed at 2 meters above the surface is required, therefore, wind measured at other heights must be adjusted. To adjust wind speed data to the 2-m height, Eq. 33 should be used for measurements taken above a short grass (or similar) surface, based on the full logarithmic wind speed profile equation B.14 given in Appendix B:
				double U10 = U2*4.87 / log(67.8 * 2 - 5.42);//wind speed at 2 meters
				data.SetStat(H_WNDS, U10);
			}

			//if (variables[H_EA] && !data[H_EA].IsInit() && data[H_TDEW].IsInit())
			//{
			//	double Ea = e°(data[H_TDEW][MEAN]);
			//	data.SetStat(H_EA, Ea * 1000);//[Pa]
			//}

			//if (variables[H_ES] && !data[H_ES].IsInit() && data[H_TAIR])
			//{
			//	double Es = e°(data[H_TMIN][MEAN], data[H_TMAX][MEAN]);//this format work for hourly and daily values
			//	data.SetStat(H_ES, Es * 1000);//[Pa]
			//}

			//if (variables[H_VPD] && !data[H_VPD].IsInit() && (data[H_EA].IsInit() || data[H_TDEW].IsInit()) && (data[H_ES].IsInit() || data[H_TAIR]))
			//{
			//	double Ea = data[H_EA].IsInit() ? data[H_EA][MEAN] : e°(data[H_TDEW][MEAN]);
			//	double Es = data[H_ES].IsInit() ? data[H_ES][MEAN] : e°(data[H_TMIN][MEAN], data[H_TMAX][MEAN]);

			//	data.SetStat(H_VPD, (Es - Ea) * 1000);//[Pa]
			//}

			//Wnd2 from WndS 
			if (variables[H_WND2] && !data[H_WND2].IsInit() && data[H_WNDS].IsInit())
			{
				double U10 = data[H_WNDS][MEAN];//wind speed at 10 meters
				double U2 = U10*4.87 / log(67.8 * 10 - 5.42);//wind speed at 2 meters
				data.SetStat(H_WND2, U2);
			}

			//for (CWeatherStation::iterator it2 = simulationPoint.begin(); it2 != simulationPoint.end(); it2++)
			//for (CWeatherYear::iterator it3 = it2->second->begin(); it3 != it2->second->end(); it3++)
			//for (CWeatherMonth::iterator it4 = it3->begin(); it4 != it3->end(); it4++)
			//for (CWeatherDay::iterator it5 = it4->begin(); it5 != it4->end(); it5++)
			//it5->SetStat(H_TRNG, 0);
		}
	}
	//****************************************************************************
	// Summary:		Generate
	//
	// Description: Main method of the class CWeatherGenerator. This method generate weather
	//
	// Input:		CCallback: callback for progress windows
	//
	// Output:		ERMsg: error message when error append
	//
	// Note:		This object must be init before. These member must be call:
	//				SetNormalDB:			set normal database
	//				SetDailyDB:				set daily database (optionnal)
	//				SetHourlyDB:			set hourly database (optionnal)
	//				SetTarget:				set the simulation point
	//				SetNbReplications:		set the seeds array for random number (iterations x variables)
	//				SetWGInput:				set all other input
	//****************************************************************************
	ERMsg CWeatherGenerator::Generate(CCallback& callback)
	{
		ERMsg msg;
		msg = Initialize();// init gradient and seeds


		if (!msg)
			return msg;

		//******************************************************************
		// Get hourly data
		m_simulationPoints.resize(m_nbReplications);

		for (size_t i = 0; i < m_nbReplications; i++)
			((CLocation&)m_simulationPoints[i]) = m_target;

		//******************************************************************
		if (m_tgi.IsHourly())
		{
			msg = GetHourly(m_simulationPoints[0], callback);
		}
		else if (m_tgi.IsDaily()) //(/* msg && m_tgi.UseDaily() pour le moment*/ && m_tgi.IsDaily() && !m_simulationPoints[0].IsComplete(m_tgi.m_variables))
		{
			msg = GetDaily(m_simulationPoints[0], callback);
		}


		// Get observation
		if (m_tgi.UseGribs())
		{
			CSimulationPoint gribsData;
			msg = GetGribs(gribsData, callback);

			//replace daily our ourly observation with gribs data
			CTPeriod p = m_tgi.GetTPeriod();
			for (CTRef TRef = p.Begin(); TRef <= p.End() && msg; TRef++)
			{
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)
				{
					if (m_tgi.m_variables[v] && gribsData[TRef][v].IsInit())
						m_simulationPoints[0][TRef].SetStat(v, gribsData[TRef][v]);
				}

				msg += callback.StepIt();
			}
		}
		//******************************************************************
		//Get normals data

		bool bCopyReplication = true;
		if (msg)
		{

			if (!m_simulationPoints[0].IsComplete(m_tgi.m_variables, m_tgi.GetTPeriod()))
			{

				CompleteSimpleVariables(m_simulationPoints[0], m_tgi.m_variables);
				m_simulationPoints[0].FillGaps();//internal completion

				if (!m_simulationPoints[0].IsComplete(m_tgi.m_variables, m_tgi.GetTPeriod()))
				{

					CWVariables mVariables = m_tgi.GetMandatoryVariables();
					//if they are missing variables
					//1- complete the simple mandatory variables
					CompleteSimpleVariables(m_simulationPoints[0], mVariables);


					//2- if some mandatory variables is complete for some complex variables, complete theses variables
					bool bHR = mVariables[H_TDEW] || mVariables[H_RELH] || mVariables[H_SRAD2];
					bool bSN = mVariables[H_SNOW] || mVariables[H_SNDH] || mVariables[H_SWE];
					bool bWD = mVariables[H_WNDD];
					bool bTPcomplet = m_simulationPoints[0].IsComplete("Tair Trng Prcp", m_tgi.GetTPeriod());
					bool bHRcomplet = m_simulationPoints[0].IsComplete("Tdew", m_tgi.GetTPeriod());
					//bool bEAcomplete = m_simulationPoints[0].IsComplete("Ea", m_tgi.GetTPeriod());

					if (msg && bHR && bTPcomplet && (bHRcomplet/* || bEAcomplete*/))
						msg = ComputeHumidityRadiation(m_simulationPoints[0], m_tgi.m_variables);

					if (msg && bSN && bTPcomplet)//m_simulationPoints[0].IsComplete("Tair Trng Prcp", m_tgi.GetTPeriod()))
						msg = ComputeSnow(m_simulationPoints[0], m_tgi.m_variables);

					//fill wind direction because not integrated yet into the weather generator
					if (msg && bWD)
						msg = ComputeWindDirection(m_simulationPoints[0], m_tgi.m_variables);

					//3- if they are missing mendatory variables, complete with normals 
					if (!m_simulationPoints[0].IsComplete(m_tgi.GetMandatoryVariables(), m_tgi.GetTPeriod()))
					{
						bCopyReplication = false;
						for (size_t i = 1; i < m_simulationPoints.size(); i++)
							m_simulationPoints[i] = m_simulationPoints[0];


						if (!m_tgi.m_bNoFillMissing)
						{
							if (!m_tgi.IsNormals())
								m_warning.set(W_DATA_FILLED_WITH_NORMAL);

							msg = GenerateNormals(m_simulationPoints, callback);
						}

						if (msg)
						{
							for (size_t r = 0; r < m_simulationPoints.size() && msg; r++)
							{
								//4- now complete simple variables if missing
								CompleteSimpleVariables(m_simulationPoints[r], mVariables);//est-ce utile????

								//5- and complete complex variables if missing
								msg = ComputeHumidityRadiation(m_simulationPoints[r], m_tgi.m_variables);

								if (msg)
									msg = ComputeSnow(m_simulationPoints[r], m_tgi.m_variables);

								if (msg)
									msg = ComputeWindDirection(m_simulationPoints[r], m_tgi.m_variables);
							}
						}
						//do nothing for the moment, will be activated laters
						//if (msg && bWD)
						//msg = ComputeWindDirection(m_simulationPoints[0], m_tgi.m_variables);

					}
				}
			}
		}


		if (!msg)
			return msg;


		//round to 0.1
		for (size_t r = 0; r < m_simulationPoints.size() && msg; r++)
		{
			for (size_t y = 0; y < m_simulationPoints[r].size() && msg; y++)
			{
				for (size_t m = 0; m < m_simulationPoints[r][y].size(); m++)
				{
					for (size_t d = 0; d < m_simulationPoints[r][y][m].size(); d++)
					{
						for (TVarH v = H_TMIN2; v < H_SRAD2; v++)
						{
							if (m_tgi.m_variables[v])
							{
								if (m_tgi.m_generationType == CWGInput::GENERATE_HOURLY)
								{
									for (size_t h = 0; h < m_simulationPoints[r][y][m][d].size(); h++)
									{
										m_simulationPoints[r][y][m][d][h][v] = WBSF::Round(m_simulationPoints[r][y][m][d][h][v], 1);
									}
								}
								else
								{
									m_simulationPoints[r][y][m][d][v] = WBSF::Round(m_simulationPoints[r][y][m][d][v], 1);
								}
							}
						}
					}
				}
			}
		}

		if (bCopyReplication && m_nbReplications > 1)
		{
			//copy data for all replications
			for (size_t i = 1; i < m_simulationPoints.size(); i++)
				m_simulationPoints[i] = m_simulationPoints[0];

			m_warning.set(W_UNEEDED_REPLICATION);
		}

		//******************************************************************
		// compute exposition

		if ((m_tgi.m_variables[H_TMIN2] || m_tgi.m_variables[H_TMAX2]))
		{


			// Apply exposure overheating on Tmax
			if (UseExpo())
			{
				double exposureIndex[12] = { 0 };
				ExposureIndices(exposureIndex, m_target.m_lat, m_target.m_elev, m_target.GetSlopeInDegree(), m_target.GetAspect(), (short)m_tgi.m_albedo);

				for (CSimulationPointVector::iterator itR = m_simulationPoints.begin(); itR != m_simulationPoints.end() && msg; itR++)//for all replication
				{
					for (CWeatherYears::iterator itY = itR->begin(); itY != itR->end() && msg; itY++)//for all years
					{
						for (CWeatherYear::iterator itM = itY->second->begin(); itM != itY->second->end() && msg; itM++)//for all months
						{
							size_t m = itM->GetTRef().GetMonth();
							for (CMonth::iterator itD = itM->begin(); itD != itM->end() && msg; itD++)//for all days
							{
								if (itD->IsHourly())
								{
									//Todo
									ASSERT(false);
								}
								else
								{
									double Tmin = (*itD)[H_TMIN2][MEAN];
									double Tmax = (*itD)[H_TMAX2][MEAN];
									Tmax += float(exposureIndex[m] * (Tmax - Tmin));

									if (m_tgi.m_variables[H_TAIR2])
										itD->SetStat(H_TAIR2, (Tmin + Tmax) / 2);

									if (m_tgi.m_variables[H_TMAX2])
										itD->SetStat(H_TMAX2, Tmax);
								}
							}//all days
						}//all months
					}//all years
				}//all replications
			}//if apply exposition
		}

		//remove mandatory variables not requested
		if (m_simulationPoints[0].GetVariables() != m_tgi.m_variables)
			m_simulationPoints.CleanUnusedVariable(m_tgi.m_variables);

		ASSERT(!msg || VerifyData(m_simulationPoints, m_tgi.m_variables));

		return msg;

	}

	ERMsg CWeatherGenerator::ComputeHumidityRadiation(CSimulationPoint& simulationPoint, CWVariables variables)
	{
		assert(!simulationPoint.empty());

		ERMsg msg;

		bool bSRad = false;
		bool bTdew = false;

		if (variables[H_TDEW] && !simulationPoint.IsComplete(CWVariables(H_TDEW)))
			bTdew = true;
		else if (variables[H_SRAD2] && !simulationPoint.IsComplete(CWVariables(H_SRAD2)))
			bSRad = true;

		if (msg && (bTdew || bSRad))
		{
			MTClim43::CControl ctrl;
			MTClim43::CParameter p;

			p.site_lat = simulationPoint.m_lat;
			p.site_elev = simulationPoint.m_elev;
			p.site_slp = simulationPoint.GetSlopeInDegree();
			p.site_asp = simulationPoint.GetAspect();
			p.site_ehoriz = 0;
			p.site_whoriz = 0;

			MTClim43::CData data;
			data.data_alloc(366);

			MTClim43::CMTClim43 MTClim43;

			for (size_t y = 0; y != simulationPoint.size(); y++)//for all years
			{
				ctrl.ndays = (int)simulationPoint[y].GetNbDays();
				ASSERT(ctrl.ndays == 365 || ctrl.ndays == 366);

				for (size_t m = 0; m != simulationPoint[y].size(); m++)//for all months
				{
					for (size_t d = 0; d != simulationPoint[y][m].size(); d++)//for all days
					{
						const CDay& day = (const CDay&)simulationPoint[y][m][d];
						size_t jd = day.GetTRef().GetJDay();



						//double Ea = day[H_EA].IsInit() ? (241.88 * log(day[H_EA][MEAN] / 610.78)) / (17.558 - log(day[H_EA][MEAN] / 610.78)) : -999;
						double Tdew = day[H_TDEW].IsInit() ? day[H_TDEW][MEAN] : -999;

						//ASSERT(Ea > -999 || Tdew > -999);// at least one humidity
						//intput
						data.yday[jd] = int(jd + 1);
						data.s_tmin[jd] = day[H_TMIN2][MEAN];
						data.s_tmax[jd] = day[H_TMAX2][MEAN];
						data.s_tday[jd] = day.GetTdaylight();
						data.s_prcp[jd] = day[H_PRCP][SUM] / 10;	//ppt in cm
						data.s_swe[jd] = variables[H_SWE] ? day[H_SWE][MEAN] / 10 : 0;	//Snow water equivalent MTClim 4.3 in cm. If not available, will be computed later
						data.s_tdew[jd] = Tdew;// Ea > -999 ? Ea : Tdew;
						_ASSERTE(!_isnan(data.s_tdew[jd]));

						//output
						data.s_srad[jd] = 0;
						data.s_dayl[jd] = 0;

					}
				}


				if (!simulationPoint.IsComplete(CWVariables(H_SWE)))//if no snow wather equivalent, compute them.
					data.snowpack();

				//si on a le point de rosée alors on peut utiliser cet methode
				if (simulationPoint.IsComplete(CWVariables(H_TDEW)))// || simulationPoint.IsComplete(CWVariables(H_EA)
					MTClim43.calc_srad_humidity(&ctrl, &p, &data);
				else
					MTClim43.calc_srad_humidity_iterative(&ctrl, &p, &data);

				//int year = m_tgi.m_firstYear + int(y);
				//for (CWeatherYear::iterator itM = itY->second->begin(); itM != itY->second->end(); itM++)//for all months
				//{//
				//for (CMonth::iterator itD = itM->begin(); itD != itM->end(); itD++)//for all days
				//{
				int year = simulationPoint[y].GetTRef().GetYear();
				CWeatherStation copy;
				((CLocation&)copy) = simulationPoint;
				copy.CreateYear(year);

				for (size_t m = 0; m != simulationPoint[y].size(); m++)//for all months
				{
					for (size_t d = 0; d != simulationPoint[y][m].size(); d++)//for all days
					{
						CDay& wDay = copy[year][m][d];
						size_t jd = wDay.GetTRef().GetJDay();

						//need temperature to compure hourly Tdew and Hr
						wDay[H_TMIN2] = data.s_tmin[jd];
						wDay[H_TMAX2] = data.s_tmax[jd];

						if (variables[H_TDEW])// && !wDay[H_TDEW].IsInit())
							wDay[H_TDEW] = data.s_tdew[jd];

						if (variables[H_RELH])// && !wDay[H_RELH].IsInit())
						{
							//how to compute relative humidity like MTCLim or BioSIM?????
							double Ea = 610.7 * exp(17.38 * data.s_tdew[jd] / (239.0 + data.s_tdew[jd]));
							double Es = 610.7 * exp(17.38 * data.s_tday[jd] / (239.0 + data.s_tday[jd]));
							double Hr1 = Ea / Es * 100;

							double Tair = (data.s_tmin[jd] + data.s_tmax[jd]) / 2;
							double Hr2 = Td2Hr(Tair, data.s_tdew[jd]);
							_ASSERTE(!_isnan(Hr2));
							//how to compute relative humidity like MTCLim or BioSIM????? a vérifier
							wDay[H_RELH] = Hr2;
							//wDay[H_RELH] = (data.s_Ea[jd] / data.s_Es[jd] * 100);
						}


						if (variables[H_SRAD2])// && !wDay[H_SRAD].IsInit())
						{
							_ASSERTE(data.s_srad[jd] >= 0);
							_ASSERTE(data.s_dayl[jd] >= 0);
							_ASSERTE(!_isnan(data.s_srad[jd]));
							_ASSERTE(!_isnan(data.s_dayl[jd]));
							//wDay[H_SRAD2] = (data.s_srad[jd] * data.s_dayl[jd] / 1000000); // convert W/m² to MJ/m²·day
							wDay[H_SRAD2] = data.s_srad[jd] * data.s_dayl[jd] / (24 * 3600); // convert daylight radiation [W/m²] into daily radiation [W/m²]
						}

						_ASSERTE(!variables[H_SRAD2] || wDay[H_SRAD2].IsInit());
					}//for all days
				}//for all month

				if (simulationPoint.IsHourly())
				{
					//CWVariables test("TDEW RELH SRAD");
					//test &= variables;
					copy.ComputeHourlyVariables();
				}
				//copy data if missing
				CTPeriod period = simulationPoint.GetEntireTPeriod();
				for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
				{
					CDataInterface& data = simulationPoint[y][TRef];

					if (variables[H_TDEW] && !data[H_TDEW].IsInit())
						data.SetStat(H_TDEW, copy[TRef][H_TDEW]);

					if (variables[H_RELH] && !data[H_RELH].IsInit())
						data.SetStat(H_RELH, copy[TRef][H_RELH]);

					if (variables[H_SRAD2] && !data[H_SRAD2].IsInit())
						data.SetStat(H_SRAD2, copy[TRef][H_SRAD2]);
				}//for all TRef
			}//for all years

			data.data_free();
		}//if compile



		return msg;
	}


	//ERMsg CWeatherGenerator::ComputeAtmosphericPressure(CSimulationPointVector&  simulationPointVector)
	//{
	//	ERMsg msg;
	//
	//	bool bCompute = false;
	//
	//	if (m_tgi.m_variables[H_PRES] && !simulationPointVector.IsComplete(CWVariables(H_PRES)))
	//		bCompute = true;
	//
	//	if (msg && bCompute)
	//	{
	//		for (CSimulationPointVector::iterator itR = simulationPointVector.begin(); itR != simulationPointVector.end(); itR++)//for all replication
	//		{
	//			CTPeriod period = itR->GetEntireTPeriod();
	//			for (CTRef d = period.Begin(); d <= period.End(); d++)
	//			{
	//				CDataInterface& data = (*itR)[d];
	//				//output
	//				if (!data[H_PRES].IsInit())
	//				{
	//					// daily atmospheric pressure (Pa) as a function of elevation (m) 
	//					// From the discussion on atmospheric statics in:
	//					// Iribane, J.V., and W.L. Godson, 1981. Atmospheric Thermodynamics, 2nd
	//					// Edition. D. Reidel Publishing Company, Dordrecht, The Netherlands. (p. 168)
	//
	//					const double MA = 28.9644e-3;     // (kg mol-1) molecular weight of air 
	//					const double R = 8.3143;          // (m3 Pa mol-1 K-1) gas law constant 
	//					const double LR_STD = 0.0065;     // (-K m-1) standard temperature lapse rate 
	//					const double G_STD = 9.80665;     // (m s-2) standard gravitational accel.  
	//					const double P_STD = 101325.0;    // (Pa) standard pressure at 0.0 m elevation 
	//					const double T_STD = 288.15;      // (K) standard temp at 0.0 m elevation   
	//
	//					double alt = itR->m_alt;
	//					double t1 = 1.0 - (LR_STD * alt) / T_STD;
	//					double t2 = G_STD / (LR_STD * (R / MA));
	//					double pa = P_STD * pow(t1, t2);
	//					data.SetStat( H_PRES, pa / 100);		//pressure [hPa]
	//				}
	//			}
	//		}
	//	}
	//
	//	return msg;
	//}

	//ERMsg CWeatherGenerator::ComputeVaporPressure(CSimulationPointVector&  simulationPointVector)
	//{
	//	ERMsg msg;
	//
	//	bool bCompute = false;
	//	
	//	if (m_tgi.m_variables[H_EA] && !simulationPointVector.IsComplete(CWVariables(H_EA)))
	//		bCompute = true;
	//	else if(m_tgi.m_variables[H_ES] && !simulationPointVector.IsComplete(CWVariables(H_ES))) 
	//		bCompute = true;
	//	else if (m_tgi.m_variables[H_VPD] && !simulationPointVector.IsComplete(CWVariables(H_VPD)))
	//		bCompute = true;
	//
	//	if (msg && bCompute)
	//	{
	//		for (CSimulationPointVector::iterator itR = simulationPointVector.begin(); itR != simulationPointVector.end(); itR++)//for all replication
	//		{
	//			CTPeriod period = itR->GetEntireTPeriod();
	//			for (CTRef d = period.Begin(); d <= period.End(); d++)
	//			{
	//				CDataInterface& data = (*itR)[d];
	//				//output
	//				if (m_tgi.m_variables[H_EA] && !data[H_EA].IsInit())
	//				{
	//					double Ea = e°(data[H_TDEW][MEAN]);
	//					//data.SetStat(H_EA, 610.7 * exp(17.38 * data[H_TDEW][MEAN] / (239.0 + data[H_TDEW][MEAN])));
	//					data.SetStat(H_EA, Ea);
	//				}
	//					
	//					//data.SetStat(H_EA, 610.7 * exp(17.38 * data[H_TDEW][MEAN] / (239.0 + data[H_TDEW][MEAN])));
	//
	//				if (m_tgi.m_variables[H_ES] && !data[H_ES].IsInit())
	//				{
	//					double Es = e°(data[H_TMAX][MEAN], data[H_TMAX][MEAN]);
	//					data.SetStat(H_ES, Es);
	//				}
	//					
	//
	//				if (m_tgi.m_variables[H_VPD] && !data[H_VPD].IsInit())
	//				{
	//					//ASSERT(false); //a vérifier
	//
	//					double Ea = data[H_EA].IsInit() ? data[H_EA][MEAN] : e°(data[H_TDEW][MEAN]);
	//					double Es = data[H_ES].IsInit() ? data[H_ES][MEAN] : e°(data[H_TMIN][MEAN], data[H_TMAX][MEAN]);
	//
	//					data.SetStat(H_VPD, Es-Ea);
	//				}
	//			}
	//		}
	//	}
	//
	//	return msg;
	//}

	ERMsg CWeatherGenerator::ComputeSnow(CSimulationPoint&  simulationPoint, CWVariables variables)
	{
		ERMsg msg;

		bool bCompute = false;

		if (variables[H_SNOW] && !simulationPoint.IsComplete(CWVariables(H_SNOW)))
			bCompute = true;
		else if (variables[H_SNDH] && !simulationPoint.IsComplete(CWVariables(H_SNDH)))
			bCompute = true;
		else if (variables[H_SWE] && !simulationPoint.IsComplete(CWVariables(H_SWE)))
			bCompute = true;

		if (msg && bCompute)
		{

			//compute the new SnowPack value
			//if we are in North America, we used the longitude to 
			//make a correction on the model.
			//else, the default arbritary value of -100 is used.
			CSnowMelt snowMelt;
			if (simulationPoint.m_lat > 30 &&
				simulationPoint.m_lon > -180 &&
				simulationPoint.m_lon < -50)
			{
				snowMelt.SetLon(simulationPoint.m_lon);
			}


			snowMelt.Compute(simulationPoint);
			const CSnowMeltResultVector& snow = snowMelt.GetResult();

			size_t jd = 0;
			for (CWeatherYears::iterator itY = simulationPoint.begin(); itY != simulationPoint.end(); itY++)//for all years
			{
				for (CWeatherYear::iterator itM = itY->second->begin(); itM != itY->second->end(); itM++)//for all months
				{
					for (CMonth::iterator itD = itM->begin(); itD != itM->end(); itD++, jd++)//for all days
					{
						//size_t jd = itD->GetTRef().GetJDay();

						if (variables[H_SNOW] && !(*itD)[H_SNOW].IsInit())
							(*itD)[H_SNOW] = snow[jd].m_newSWE;

						if (variables[H_SNDH] && !(*itD)[H_SNDH].IsInit())
							(*itD)[H_SNDH] = snow[jd].m_hs;

						if (variables[H_SWE] && !(*itD)[H_SWE].IsInit())
							(*itD)[H_SWE] = snow[jd].m_SWE;

						if (simulationPoint.IsHourly())
						{
							//init hourly data
							for (size_t h = 0; h < 24; h++)
							{
								if (variables[H_SNOW] && WEATHER::IsMissing((*itD)[h][H_SNOW]))
									(*itD)[h][H_SNOW] = snow[jd].m_newSWE / 24;

								if (variables[H_SNDH] && WEATHER::IsMissing((*itD)[h][H_SNDH]))//a faire : interplation entre d-1 et d+1
									(*itD)[h][H_SNDH] = snow[jd].m_hs;

								if (variables[H_SWE] && WEATHER::IsMissing((*itD)[h][H_SWE]))//a faire : interplation entre d-1 et d+1
									(*itD)[h][H_SWE] = snow[jd].m_SWE;
							}
						}
					}
				}
			}
		}


		return msg;
	}


	ERMsg CWeatherGenerator::ComputeWindDirection(CSimulationPoint& simulationPoint, CWVariables variables)
	{
		ERMsg msg;

		bool bCompute = false;
		if (variables[H_WNDD] && !simulationPoint.IsComplete(CWVariables(H_WNDD)))
			bCompute = true;

		if (msg && bCompute)
		{
			//put random number
			//for (CSimulationPointVector::iterator itR = simulationPointVector.begin(); itR != simulationPointVector.end(); itR++)//for all replication
			//{
			double lastWindDir = 0;
			CTPeriod period = simulationPoint.GetEntireTPeriod();
			for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
			{
				CDataInterface& data = simulationPoint[TRef];
				//output
				if (data[H_WNDD].IsInit())
				{
					lastWindDir = data[H_WNDD][MEAN];
				}
				else
				{
					data.SetStat(H_WNDD, lastWindDir);
				}
			}

		}

		return msg;

	}


	//ERMsg CWeatherGenerator::Save(const std::stringArray& outputFilePathArray, short IOFileVersion)const
	//{
	//	ASSERT( m_results.size()==outputFilePathArray.size() );
	//
	//	ERMsg msg;
	//	for(int i=0; i<(int)m_results.size()&&msg; i++)
	//	{
	//		msg = CWeatherGenerator::SaveOutputToDisk(outputFilePathArray[i], m_results[i], IOFileVersion);
	//	}
	//
	//	return msg;
	//}



	//***************************************************************
	//      ******************* Hourly *******************           

	//****************************************************************************
	// Summary:		GetHourly
	//
	// Description: Get hourly data for the target from hourly database.
	//
	// Input:		
	//
	// Output:		CSimulationPoint& simulationPoint: the simulation point for all years
	//
	// Note:		
	//				
	//****************************************************************************


	ERMsg CWeatherGenerator::GetHourly(CSimulationPoint& simulationPoint, CCallback& callback)
	{
		assert(m_tgi.m_generationType == CWGInput::GENERATE_HOURLY);
		assert(m_tgi.m_firstYear > 0);
		assert(m_tgi.GetNbYears() > 0);
		assert(m_tgi.m_nbHourlyStations > 0);
		assert(m_tgi.m_variables.any());
		assert(m_tgi.XVal() == 0 || m_tgi.XVal() == 1);

		ERMsg msg;
		int currentYear = CTRef::GetCurrentTRef().GetYear();
		((CLocation&)simulationPoint) = m_target;

		for (size_t y = 0; y < m_tgi.GetNbYears() && msg; y++)
		{
			int year = m_tgi.GetFirstYear() + int(y);


			//first step: get direct observations variables
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)
			{
				if (m_tgi.m_variables[v] && (v != H_TMIN2 && v != H_TMAX2))
				{
					CSearchResultVector results;
					msg = m_pHourlyDB->Search(results, m_target, m_tgi.m_nbHourlyStations + m_tgi.XVal(), m_tgi.m_searchRadius[v], v, year);
					if (!results.empty() && m_tgi.XVal())
						results.erase(results.begin());

					//if future year, we continue anyway
					if (year > currentYear)
						msg.asgType(ERMsg::OK);

					//if (!msg && (v == H_TMIN2 || v == H_TMAX2))//Tmin and Tmax equal Tair in hourly data
					//msg.asgType(ERMsg::OK);

					if (!msg)
					{
						CWVariables missingVariables = DERIVABLE_VARIABLES_INPUT[v];
						if (m_tgi.m_allowedDerivedVariables[v] && (missingVariables.any() || v == H_PRES))//|| v == H_WNDD
							msg.asgType(ERMsg::OK);
					}

					if (msg && results.size() > 0)
					{
						CWeatherStationVector stations;
						msg = m_pHourlyDB->GetStations(stations, results, year);
						if (msg)
						{
							stations.FillGaps();//internal completion
							stations.ApplyCorrections(m_gradients);
							stations.GetInverseDistanceMean(v, m_target, simulationPoint);
						}
					}

					msg += callback.StepIt(0);
				}
			}//for all category


			//second step: get extra observation variables to compute derivable variables
			if (msg)
			{
				CWVariables observationVariables = simulationPoint[year].GetVariables();
				CWVariables neededVariables;
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)
				{
					if (m_tgi.m_variables[v] && !observationVariables[v] && m_tgi.m_allowedDerivedVariables[v])//!simulationPoint[y].IsComplete(v))
					{
						neededVariables |= DERIVABLE_VARIABLES_INPUT[v];
					}
				}
				neededVariables &= ~observationVariables;//remove observation variables from needed variables

				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)
				{
					if (neededVariables[v] && (v != H_TMIN2 && v != H_TMAX2))
					{
						CSearchResultVector results;
						msg = m_pHourlyDB->Search(results, m_target, m_tgi.m_nbHourlyStations + m_tgi.XVal(), m_tgi.m_searchRadius[v], v, year);
						if (!results.empty() && m_tgi.XVal())
							results.erase(results.begin());

						//if future year, we continue anyway
						if (year > currentYear)
							msg.asgType(ERMsg::OK);

						//if (!msg && v == H_TRNG)//Trange assume to be zero in hourly data
						//	msg.asgType(ERMsg::OK);

						if (!msg)
						{
							string vars;
							for (TVarH vv = H_FIRST_VAR; vv < NB_VAR_H; vv++)
							{
								if (m_tgi.m_variables[vv] && DERIVABLE_VARIABLES[v][vv])
								{
									if (!vars.empty())
										vars += ", ";

									vars += GetVariableTitle(vv);
								}
							}

							if (!vars.empty())
								msg.ajoute(FormatMsg(IDS_WG_MISS_COMPUTE_INPUT, GetVariableTitle(v), GetFileName(m_pHourlyDB->GetFilePath()), vars));
						}

						if (msg && results.size() > 0)
						{
							CWeatherStationVector stations;
							msg = m_pHourlyDB->GetStations(stations, results, year);
							if (msg)
							{
								stations.FillGaps();//internal completion
								stations.ApplyCorrections(m_gradients);//apply gradient to weather data
								stations.GetInverseDistanceMean(v, m_target, simulationPoint);
							}
						}
						/*else if (v == H_TRNG)
						{
						msg.asgType(ERMsg::OK);
						}*/

						msg += callback.StepIt(0);
					}//is it a needed variables to derivate variables
				}//for all variables
			}//if msg
		}//for all years

		if (!m_tgi.m_bUseForecast)
			RemoveForecast(simulationPoint);

		return msg;
	}


	//****************************************************************************
	//      ******************* Daily *******************           

	//****************************************************************************
	// Summary:		GetDaily
	//
	// Description: Get daily data for the target from daily database.
	//
	// Input:		
	//
	// Output:		CDailyDataVector& dailyData: the daily data for all years
	//
	// Note:		
	//				
	//****************************************************************************
	ERMsg CWeatherGenerator::GetDaily(CSimulationPoint& simulationPoint, CCallback& callback)
	{
		ASSERT(m_tgi.m_firstYear > 0);
		ASSERT(m_tgi.GetNbYears() > 0);
		ASSERT(m_tgi.m_nbDailyStations > 0);
		ASSERT(m_tgi.m_variables.any());
		ASSERT(m_tgi.XVal() == 0 || m_tgi.XVal() == 1);

		ERMsg msg;

		int currentYear = CTRef::GetCurrentTRef().GetYear();
		((CLocation&)simulationPoint) = m_target;
		//CWVariables mVariables = m_tgi.GetMandatoryVariables();

		for (size_t y = 0; y < m_tgi.GetNbYears() && msg; y++)
		{
			int year = m_tgi.GetFirstYear() + int(y);

			//first step, get observations from database
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)
			{
				if (m_tgi.m_variables[v] && v != H_TAIR2)
				{
					CSearchResultVector results;
					msg = m_pDailyDB->Search(results, m_target, m_tgi.m_nbDailyStations + m_tgi.XVal(), m_tgi.m_searchRadius[v], v, year);
					if (!results.empty() && m_tgi.XVal())
						results.erase(results.begin());

					//if int the future, we continue anyway
					if (year > currentYear)
						msg.asgType(ERMsg::OK);

					//if (!msg && v == H_TAIR2)//Tair = (Tmin + Tmax)/2 in daily data
					//msg.asgType(ERMsg::OK);

					if (!msg)
					{
						CWVariables missingVariables = DERIVABLE_VARIABLES_INPUT[v];
						if (m_tgi.m_allowedDerivedVariables[v] && (missingVariables.any() || v == H_PRES))
							msg.asgType(ERMsg::OK);
					}

					if (msg && results.size() > 0)
					{
						CDailyStationVector stationsVector;
						msg = m_pDailyDB->GetStations(stationsVector, results, year);

						if (msg)
						{
							stationsVector.FillGaps();//internal completion
							stationsVector.ApplyCorrections(m_gradients);
							stationsVector.GetInverseDistanceMean(v, m_target, simulationPoint);
						}
					}

					msg += callback.StepIt(0);
				}
			}//for all variables


			//second step: get needed missing variable to derivate variable (like SRAD)
			if (msg)
			{
				CWVariables observationVariables = simulationPoint[year].GetVariables();
				CWVariables neededVariables;
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)
				{
					if (m_tgi.m_variables[v] && !observationVariables[v] && m_tgi.m_allowedDerivedVariables[v])//!simulationPoint[y].IsComplete(v))
					{
						neededVariables |= DERIVABLE_VARIABLES_INPUT[v];
					}
				}

				neededVariables &= ~observationVariables;//remove variable already loaded
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)
				{
					if (neededVariables[v] && v != H_TAIR2)
					{
						CSearchResultVector results;
						msg = m_pDailyDB->Search(results, m_target, m_tgi.m_nbHourlyStations + m_tgi.XVal(), m_tgi.m_searchRadius[v], v, year);
						if (!results.empty() && m_tgi.XVal())
							results.erase(results.begin());

						//if future year, we continue anyway
						if (year > currentYear)
							msg.asgType(ERMsg::OK);

						if (!msg)
						{

							string vars;
							for (TVarH vv = H_FIRST_VAR; vv < NB_VAR_H; vv++)
							{
								if (m_tgi.m_variables[vv] && DERIVABLE_VARIABLES[v][vv])
								{
									if (!vars.empty())
										vars += ", ";

									vars += GetVariableTitle(vv);
								}
							}

							if (!vars.empty())
								msg.ajoute(FormatMsg(IDS_WG_MISS_COMPUTE_INPUT, GetVariableTitle(v), GetFileName(m_pDailyDB->GetFilePath()), vars));

						}

						if (msg && results.size() > 0)
						{
							CWeatherStationVector stations;
							msg = m_pDailyDB->GetStations(stations, results, year);
							if (msg)
							{
								stations.FillGaps();//internal completion
								stations.ApplyCorrections(m_gradients);
								stations.GetInverseDistanceMean(v, m_target, simulationPoint);
							}
						}

						msg += callback.StepIt(0);
					}//is it a missing varaibles
				}//for all variables
			}
		}


		if (!m_tgi.m_bUseForecast)
			RemoveForecast(simulationPoint);

		return msg;
	}

	//****************************************************************************
	// GenerateNormals 
	//
	//****************************************************************************
	// Summary:		GenerateNormals
	//
	// Description: Generate daily data for the target from normals with CWeatherGeneratorKernel. 
	//				Generate hourly from daily when hourly.
	//
	// Input:		
	//
	// Output:		CSimulationPointVector& simulationPointVector: the normal data
	//
	// Note:		
	//				
	//****************************************************************************
	ERMsg CWeatherGenerator::GenerateNormals(CSimulationPointVector& simulationPointVector, CCallback& callback)
	{
		ASSERT(m_tgi.XVal() == 0 || m_tgi.XVal() == 1);

		ERMsg msg;

		//******************************************************************
		//Load normals data from normals database
		CNormalsStation normals;
		msg = CWeatherGenerator::GetNormals(normals, callback);
		if (!msg)
			return msg;


		//******************************************************************
		//Create CWeatherGeneratorKernel: the generator of daily data from normals
		CWVariables mVariables = m_tgi.GetNormalMandatoryVariables();
		CWeatherGeneratorKernel kernel(mVariables);
		kernel.SetNormals(normals);


		//******************************************************************
		//	Final computation for all replications
		assert(m_seedMatrix.size() == m_nbReplications);
		assert(simulationPointVector.size() == m_nbReplications);

		//generate daily/hourly data 
		for (size_t r = 0; r < m_nbReplications&&msg; r++)
		{
			if (simulationPointVector[r].empty())
			{
				//creation from normals only
				simulationPointVector[r].CreateYears(m_tgi.GetTPeriod());

				for (size_t y = 0; y < m_tgi.GetNbYears() && msg; y++)
				{
					int year = m_tgi.GetFirstYear() + int(y);
					kernel.SetSeed(m_seedMatrix[r][y]);

					kernel.Generate(simulationPointVector[r][y]);
					msg += callback.StepIt(0);
				}

				if (m_tgi.m_generationType == CWGInput::GENERATE_HOURLY)
				{
					simulationPointVector[r].ComputeHourlyVariables();
				}
			}
			else
			{
				//creation from incomplete observations
				simulationPointVector[r].CreateYears(m_tgi.GetTPeriod());//comlete the creation of all years if missing

				for (size_t y = 0; y < m_tgi.GetNbYears() && msg; y++)
				{
					if (!simulationPointVector[r][y].IsComplete(mVariables/*m_tgi.m_variables*/))
					{
						int year = m_tgi.GetFirstYear() + int(y);
						kernel.SetSeed(m_seedMatrix[r][y]);

						//generate annual daily data
						CWeatherStation annualData;
						((CLocation&)annualData) = m_target;
						kernel.Generate(annualData[year]);

						//if hourly, compute hourly data
						if (m_tgi.m_generationType == CWGInput::GENERATE_HOURLY)
							annualData.ComputeHourlyVariables();

						//complete data
						for (size_t m = 0; m < simulationPointVector[r][y].size(); m++)
						{
							for (size_t d = 0; d < simulationPointVector[r][y][m].size(); d++)
							{
								for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
								{
									if (mVariables[v])
									{
										static const int DIGIT_RES[NB_VAR_H] = { 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 3, 3 };
										if (m_tgi.m_generationType == CWGInput::GENERATE_HOURLY)
										{
											//replace missing hourly value by normals generation
											for (size_t h = 0; h < simulationPointVector[r][y][m][d].size(); h++)
											{
												if (IsMissing(simulationPointVector[r][y][m][d][h][v]))
													simulationPointVector[r][y][m][d][h][v] = WBSF::Round(annualData[year][m][d][h][v], DIGIT_RES[v]);
													//simulationPointVector[r][y][m][d][h][v] = annualData[year][m][d][h][v];
												
											}
										}
										else
										{
											//replace missing daily value by normals generation
											if (!simulationPointVector[r][y][m][d][v].IsInit())
												simulationPointVector[r][y][m][d][v] = WBSF::Round(annualData[year][m][d][v], DIGIT_RES[v]);
												//simulationPointVector[r][y][m][d][v] = annualData[year][m][d][v];
										}
									}//if hourly/daily
								}//for all variable
							}//for all days
						}//for all months

						msg += callback.StepIt(0);
					}//is year complete
				}//for all years
			}//from normals or observation
		}//for all replication



		return msg;
	}

	ERMsg CWeatherGenerator::GetNormals(CNormalsStation& normals, CCallback& callback)
	{
		ERMsg msg;

		//load wanted variable
		CWVariables mVariables = m_tgi.GetNormalMandatoryVariables();
		for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
		{
			//if (m_tgi.m_variables[v] )
			if (mVariables[v])
			{
				CSearchResultVector results;
				// find stations with category
				msg = m_pNormalDB->Search(results, m_target, m_tgi.m_nbNormalsStations + m_tgi.XVal(), m_tgi.m_searchRadius[v], v);

				if (!results.empty() && m_tgi.XVal())
					results.erase(results.begin());

				if (!msg)
				{
					//if it can be derived fro other variable, we load these variables
					if (m_tgi.m_allowedDerivedVariables[v] && (DERIVABLE_VARIABLES_INPUT[v].any() || v == H_PRES || v == H_WNDD))// 
					{
						msg = ERMsg();//remove error
					}//if allow to derive
				}

				if (msg && results.size() > 0)
				{
					CNormalsStationVector stationsVector;
					m_pNormalDB->GetStations(stationsVector, results);
					stationsVector.ApplyCorrections(m_gradients);
					stationsVector.GetInverseDistanceMean(m_target, v, normals);
				}
			}
		}

		if (msg)
		{

			CWVariables mVariables = m_tgi.GetMissingInputVariables();
			if (mVariables[H_WND2] && !mVariables[H_WNDS])
				mVariables.set(H_WNDS);

			if (mVariables[H_WNDS] && !mVariables[H_PRCP])
				mVariables.set(H_PRCP);

			if (mVariables[H_PRCP] && !mVariables[H_TMIN2])
				mVariables.set(H_TMIN2);

			if (mVariables[H_PRCP] && !mVariables[H_TMAX2])
				mVariables.set(H_TMAX2);

			if (mVariables[H_TDEW] && !mVariables[H_RELH])
				mVariables.set(H_RELH);

			if (mVariables[H_TDEW] && !mVariables[H_TMIN2])
				mVariables.set(H_TMIN2);

			if (mVariables[H_TDEW] && !mVariables[H_TMAX2])
				mVariables.set(H_TMAX2);

			//load derived input missing variables
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				if (mVariables[v])
				{
					CSearchResultVector results;
					// find stations with category
					msg = m_pNormalDB->Search(results, m_target, m_tgi.m_nbNormalsStations + m_tgi.XVal(), m_tgi.m_searchRadius[v], v);
					if (!results.empty() && m_tgi.XVal())
						results.erase(results.begin());

					if (msg)
					{
						CNormalsStationVector stationsVector;
						m_pNormalDB->GetStations(stationsVector, results);
						stationsVector.ApplyCorrections(m_gradients);
						stationsVector.GetInverseDistanceMean(m_target, v, normals);
					}
					else
					{
						string vars;
						for (TVarH vv = H_FIRST_VAR; vv < NB_VAR_H; vv++)
						{
							if (m_tgi.m_variables[vv] && DERIVABLE_VARIABLES[v][vv])
							{
								if (!vars.empty())
									vars += ", ";

								vars += GetVariableTitle(vv);
							}
						}

						if (!vars.empty())
							msg.ajoute(FormatMsg(IDS_WG_MISS_COMPUTE_INPUT, GetVariableTitle(v), GetFileName(m_pNormalDB->GetFilePath()), vars));
					}
				}
			}
		}

		return msg;
	}

	void CWeatherGenerator::RemoveForecast(CSimulationPoint& simulationPoint)
	{

		CTRef today = CTRef::GetCurrentTRef(simulationPoint.GetTM());
		CTRef end = simulationPoint.GetEntireTPeriod().End();

		if (today < end)
		{
			for (CTRef Tref = today + 1; Tref <= end; Tref++)
			{
				simulationPoint[Tref].Reset();
			}
		}
	}


	//*******************************************************************************
	//exposition 

	void CWeatherGenerator::ExposureIndices(double exposure_index[12], double latit, double elev, float slope, float aspect, short albedoType)
	{
		_ASSERTE(albedoType >= CWGInput::NONE && albedoType < CWGInput::NB_ALBEDO);
		_ASSERTE(latit >= -90 && latit <= 90);
		_ASSERTE(elev >= -100 && elev < 10000);
		_ASSERTE(slope == -999 || (slope >= 0 && slope <= 90));
		_ASSERTE(aspect == -999 || (aspect >= 0 && aspect <= 360));

		//double expin[12];
		for (int m = 0; m < 12; m++)
			exposure_index[m] = 0;

		if (slope == -999 || aspect == -999)
			return;

		//todo: ajouter unn facteur albedo autre que 4
		const double maxt_elev = 4.0;
		const double range95 = 28.1;

		Sol9(float(latit), elev, slope, aspect, exposure_index);

		for (int m = 0; m < 12; m++)
		{
			//Equation from Paul Bolstad's nstemp8.c program. 
			exposure_index[m] *= (maxt_elev / range95);
		}

	}


	int CWeatherGenerator::Sol9(double inlatit, double inelev, double inslope, double inazimuth, double *expin)
	{

		//  Provided by Paul Bolstad. Modified (efficiency and
		//	index itself by Regniere 
		double elev = inelev;
		double slope = inslope / RAD2DEG;
		double azimuth = inazimuth / RAD2DEG;
		double latit = inlatit / RAD2DEG;

		//		All angles should be in radians for trig functions, all latitudes,
		//	longitudes in decimal degrees, and converted to radians for trig 

		//      Solar trajectory formulae taken mostly from Paltridge and Platt, "Radiative
		//	processes in meteorology and climatology", Elsevier.  Most of the transmittance
		//	taken from Hoyt, Solar Energy, 1976 

		double   ct1, ct2, st1, st2, sin_declin, cos_declin, sin_sol_zen,
			cos_sol_zen, sin_latit, cos_latit,
			sol_zen,               // solar zenith angle
			sol_alt,               // solar altitude angle
			sol_azim,              // solar azimuth angle
			lap_time,              // local apparent time, for hour angle calc.
			eq_time,               // equation of time, correction for earth orbit velocity non-linearities
			long_cor = 0.0,          // longitude time correction, 4 minutes for each degree away from t. zone cen. mer
			declin;                // maximum solar declination for given julian day

		double  flux_exo_atm = 1373.0,   // exo-atmospheric flux, watts/m2
			trans = 0,                 // transmittance through atmosphere
			beam,                  // beam irradiance in free air
			ground_beam,           // beam radiance at ground
			opt_path,              // optical path, zenith units = 1
			dif_irrad,             // diffuse irradiance
			temp1,                 // temporary variables
			time,
			time_start = 11.0,       // time values in hours, and are
			interim,
			time_stop = 15.0,        // converted to radians for some calcs.
			time_step = 0.2,         // timestep, in hours, for calculations
			cosi,                  // cosine of incidence angle      
			beam_en_flat,          // incident beam on level surface 
			beam_en;               // incident beam energy for each slope/aspect combination, in watt-hours 
		//midpnt[12]={ 16.5,  45.5,  75.0, 105.5, 136.0, 166.5,
		//			197.0, 228.0, 258.5, 289.0, 319.5, 350.};
		// mid-month day array 

		double   julian_day,        // current day (mid month)
			Rmax = 250;              // Standard max solar radiation difference between sloped and flat surfaces

		int     vis_fact = 0,        // visibility, 0 for high (clear), 1 for low (hazy)
			month;                 // current month




		// compute sin(latit) & cos(latit) 
		sin_latit = sin(latit);
		cos_latit = cos(latit);

		// loop over times of year: mid month

		for (month = 0; month < 12; ++month)
		{
			//julian_day = (float)midpnt[month];
			julian_day = (MidDayInMonth(month) + 1);//avec le code 0 base
			temp1 = julian_day*0.0172141f;
			ct1 = cos(temp1);
			st1 = sin(temp1);
			st2 = (2 * temp1);
			ct2 = cos(st2);
			st2 = sin(st2);
			declin = 0.006918 - 0.399912*ct1 + 0.070257*st1 -
				0.006758*ct2 + 0.000907*st2 -
				(0.002697*cos(3 * temp1) + 0.001480*sin(3 * temp1));

			// earth axis declination, varies by time of year, can be approximated
			//using the above formula, based on julian day 

			eq_time = (0.000075 + 0.001868*ct1 - 0.032077*st1 -
				0.014615*ct2 - 0.040849*st2);
			// eq_time is in radians, used for calculating solar altitude, azimuth

			// compute sin(declin) and cos(declin)
			sin_declin = sin(declin);
			cos_declin = cos(declin);

			beam_en_flat = 0.0;
			beam_en = 0.0;

			for (time = time_start; time <= time_stop + time_step / 2.; time += time_step)
			{
				lap_time = (time*0.261799388);
				// time by pi/12 = time in radians (2pi rads/24 hours)

				if (lap_time > 3.141592654)
					lap_time = (lap_time - 3.141592654);
				else
					lap_time = (lap_time + 3.141592654);

				lap_time = lap_time + long_cor + eq_time;
				sol_alt = (asin(sin_declin *sin_latit +
					cos_declin *cos_latit *cos(lap_time)));

				if (sol_alt > 0)
				{
					sol_zen = (1.570795 - sol_alt);
					sin_sol_zen = sin(sol_zen);
					cos_sol_zen = cos(sol_zen);
					interim = ((sin_declin *
						cos_latit -
						cos_declin *sin_latit
						*cos(lap_time))
						/ cos(sol_alt));
					if (interim > 1.) interim = 1.;
					if (interim < -1.) interim = -1.;
					sol_azim = (6.28318 - acos(interim));
					opt_path = (1 / (sin(sol_alt) +
						0.15*pow((sol_alt*RAD2DEG
						+ 3.885), -1.253)));
					//  optical path length, longer for lower horizon angles 

					if (vis_fact == 0)            // clear sky, 23 km viz 
						trans = 0.4237 - 0.00821*
						pow((6.0 - elev / 1000.), 2) +
						(0.5055 + 0.00595*
						pow((6.5 - elev / 1000.), 2))*
						exp(-(0.2711 + 0.01858*
						pow((2.5 - elev / 1000.), 2))
						/ cos_sol_zen);

					beam = flux_exo_atm*exp(-trans*opt_path);

					dif_irrad = beam*0.136*pow(cos(slope), 2);
					//				there are a number of ways to calculate diffuse/direct ratios,
					//				basically different empirical models under different sky
					//				conditions.  Note, this is only clear sky, and is a value
					//				for an average elevation of 1200 m, with clear skies.
					//				I checked three citations, there was not much difference in
					//				the predictions, and this was the simplest.  Will check two more
					//				for which I have citations, one of which looks at tropical vs
					//				temperate vs. boreal diffuse radiation 

					cosi = (cos_sol_zen*cos(slope) +
						sin_sol_zen*sin(slope)*
						cos(sol_azim - azimuth));

					//				i is the incidence angle between surface normal and incoming
					//				beam.  As it approaches 1, full beam energy, as it approaches
					//				90 deg, beam energy approaches 0, hence, cos function 

					if (cosi < 0.0)
						cosi = 0.0;

					ground_beam = beam * cosi;
					// above adjusts incident ground beam energy for surface normal angles

					beam_en = beam_en + ground_beam + dif_irrad;
					beam_en_flat = beam_en_flat + beam*(cos_sol_zen + 0.136);

				} // end if for solar altitude > 0 
			}  // end of time loop 

			expin[month] = ((beam_en - beam_en_flat) / (Rmax / time_step *(time_stop - time_start)));


		} // end of month loop 

		return 0;
	}

	//void CWeatherGenerator::GetWeather(short i, CWeather& weather)const
	//{
	//	ASSERT( i < (int)m_results.size() );
	//
	//	weather.SetYearInfo(m_tgi.GetFirstYear(), m_tgi.GetNbYear() );
	//
	//	for(int y=0; y<m_tgi.GetNbYear(); y++)
	//	{
	//		int year = m_tgi.GetFirstYear()+y;
	//		int jd = 0;
	//		for(int m=0; m<12; m++)
	//		{
	//			int nbDay = GetNbDayPerMonth(year, m);
	//			for(int d=0; d<nbDay; d++) 
	//			{
	//				CWeatherDay wDay;
	//				
	//				for(int v=0; v<DAILY_DATA::NB_VAR; v++)
	//					wDay(v) = m_results[i][y][jd][v];
	//				
	//				weather.SetData(y,m,d,wDay);
	//
	//				//for(int v=0; v<m_tgi.GetCategory().GetNbVarD(); v++)
	//				//{
	//				//	int var = m_tgi.GetCategory().GetVarD(v);
	//				//	tmp.Format(" %.3f", finalVector[y][jd][var]);
	//				//	line += tmp;
	//				//}
	//
	//				//file.WriteString(line + "\n");
	//
	//				jd++;
	//			}
	//		}
	//	}
	//}
	//
	//float CWeatherGeneratorKernel::SolSub(float LONG, float LAT, float AZ, float INS, float SC, int DAY)
	//{
	//	_ASSERTE( LONG >= -180 && LONG <= 180);
	//	_ASSERTE( LAT >= -90 && LAT <= 90 );
	//	_ASSERTE( AZ >= 0 && AZ < 360);
	//	_ASSERTE( INS >= 0 && INS <= 90);
	//	_ASSERTE( DAY >= 1 && DAY <= 366);
	//
	////++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//// C. D. WHITEMAN AND K. J. ALLWINE I C PACIFIC NORTHWEST LABORATORY, RICHLAND, WA 99352 I C---------------------------------------------------------------------
	//// USES GARNIER & OHMURA (1968) AND MCCULLOUGH (1968) SCHEMES TO 1
	//// CALCULATE EXTRATERRESTRIAL SOLAR RADIATION AT A REQUESTED TIME I
	//// OF DAY ON A SLOPE OF ANY AZIMUTH AND INCLINATION ANGLE GIVEN THE1
	//// LATITUDE AND LONGITUDE, DATE, AND SLOPE AZIMUTH AND INCLINATION I
	//// ANGLES. ALSO INTEGRATES INSTANTANEOUS VALUES TO DETERMINE I
	//// THE DAILY TOTAL IF REQUESTED. 
	////---------------------------------------------------------------------
	//// INPUTS: LONG = (-180. TO 180. DEG), LAT = (-90. TO 90. DEG),
	//// AZ = SMPE AZIMUTH ANGLE (0. TO 359.), INS = SLOPE INCLINATION
	//// ANGLE (0. TO 90. DEG), SC = SOLAR CONSTANT (E.G. 1353. W/M**2),
	//// DAY = DAY (1 TO 366)
	//
	////---------------------------------------------------------------
	//// OUTPUTS: OUT1 = TOTAL RADIATION (MJ/M**2),
	////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//
	//	
	//	double LONGCOR = 0;
	//	double LONGSUN = 0;
	//	//double INSLO = 0;
	//
	//	double IHR=0;
	//	double MM=0;
	//	double OUT1=0;
	//	
	//// *** CONSTANTS.
	//	const double ACOF[4] = {0.00839,-0.05391,-0.00154,-0.00222};
	//	const double BCOF[4] = {-0.12193,-0.15699,-0.00657,-0.00370};
	//
	//	const double DZERO = 80.;
	//	const double ECCENT = .0167;
	//	const double PI = 3.14159;
	//	const double CALINT = 1.;
	//	const double RTOD = PI/180.;
	//	const double DECMAX = (23.+26./60.)*RTOD;
	//	const double OMEGA = 2.*PI/365.;
	//	const double ONEHR = 15.*RTOD;
	//
	//// *** JULIAN DATE
	//	double D = DAY;
	//	
	//
	//// *** RATIO OF RADIUS VECTORS SQUARED,
	//	double OMD = OMEGA*D;
	//	double OMDZERO = OMEGA*DZERO;
	//	double RDVECSQ = pow( 1./(1.-ECCENT*cos(OMD)), 2);
	//
	//// *** DECLINATION OF SUN.
	//	LONGSUN=OMEGA* (D-DZERO)+2.*ECCENT*(sin(OMD)-sin(OMDZERO));
	//	double DECLIN=asin(sin(DECMAX)*sin(LONGSUN));
	//	double SDEC=sin(DECLIN);
	//	double CDEC=cos(DECLIN);
	//// *** CHECK FOR POLAR NIGHT OR DAY.
	//	double ARG=((PI/2.)-fabs(DECLIN))/RTOD;
	//	double SR = 0;
	//
	//	if(fabs(LAT) > ARG) 
	//	{
	//		if((LAT > 0. && DECLIN < 0.) || (LAT < 0. && DECLIN > 0.) ) 
	//		{
	//			return 0;
	//		}
	//		SR=-PI;
	//	}
	//	else
	//	{
	//// *** SUNRISE HOUR ANGLE.
	//		SR=-fabs(acos(-tan(LAT*RTOD)*tan(DECLIN)));
	//	}
	//// *** STANDARD TIME MERIDIAN FOR SITE.
	//	double STDMRDN=NINT(LONG/15.)*15.;
	//	LONGCOR=(LONG-STDMRDN)/15.;
	//// *** COMPUTE TIME CORRECTION FROM EQUATION OF TIME.
	//	double B=2.*PI* (D-.4)/365.;
	//	double EM=0.;
	//	for(int I=1; I<=4; I++)
	//	{
	//		EM=EM+(BCOF[I-1]*sin(I*B)+ACOF[I-1]*cos(I*B));
	//	}
	//// *** TIME OF SOLAR NOON.
	//	double TIMNOON=12.-EM-LONGCOR;
	//
	//	double AZSLO=AZ*RTOD;
	//	double INSLO = INS * RTOD;
	//	double SLAT=sin(LAT*RTOD);
	//	double CLAT=cos(LAT*RTOD);
	//	double CAZ=cos(AZSLO);
	//	double SAZ=sin(AZSLO);
	//	double SINC=sin(INSLO);
	//	double CINC=cos(INSLO);
	//
	//// *** COMPUTE DAILY TOTAL.
	//	double HINC=CALINT*ONEHR/60.;
	//	double IK=(2.*fabs(SR)/HINC)+2.;
	//
	//	for( I=1; I<=IK; I++)
	//	{
	//		double H=SR+HINC*double(I-1);
	//		double COSZ=SLAT*SDEC+CLAT*CDEC*cos(H);
	//		double COSBETA=CDEC* ((SLAT*cos(H))* (-CAZ*SINC)-
	//     		sin(H)* (SAZ*SINC)+(CLAT*cos(H))*CINC)+
	//      		SDEC* (CLAT* (CAZ*SINC)+SLAT*CINC);
	//		double EXTRA=SC*RDVECSQ*COSZ;
	//		if(EXTRA < 0.) EXTRA=0.;
	//
	//		double EXTSLO=SC*RDVECSQ*COSBETA;
	//		if(EXTRA <=0. || EXTSLO < 0.) EXTSLO=0.;
	//		
	//		OUT1=EXTSLO+OUT1;
	//	}
	//	
	//	OUT1=OUT1*CALINT*60./1000000.;
	//	
	//	
	//	return (float)OUT1;
	//}



	/*#ifdef _DEBUG

	double CWeatherGenerator::TotalWeight(const UtilWin::CDoubleArray& weight)
	{
	double sumWeight = 0;
	for(int i=0; i<weight.size(); i++)
	sumWeight += weight[i];

	if( sumWeight > 0.9999999999 &&  sumWeight < 1.000000001)
	sumWeight = 1;

	return sumWeight;
	}
	#endif //_DEBUG


	*/


	void CWeatherGenerator::GenerateSeed()
	{
		CRandomGenerator rand(m_seed);

		m_seedMatrix.resize(m_nbReplications);
		for (size_t i = 0; i < m_nbReplications; i++)
		{
			m_seedMatrix[i].resize(m_tgi.GetNbYears());
			for (size_t j = 0; j < m_tgi.GetNbYears(); j++)
			{
				for (size_t v = 0; v < NB_VAR_H; v++)
					m_seedMatrix[i][j][v] = rand.Rand();
			}
		}

		ASSERT(m_seedMatrix.size() == m_nbReplications);
	}

	void GetWarning()//todo
	{
		//callback.AddMessage("COMMENT: Complete normals data with more than one replications. Replications is not usefull in this case.");
	}

	//DPT:2 m
	//RH


	ERMsg CWeatherGenerator::GetGribs(CSimulationPoint& simulationPoint, CCallback& callback)
	{
		assert(m_tgi.m_firstYear > 0);
		assert(m_tgi.GetNbYears() > 0);
		assert(m_tgi.m_nbHourlyStations > 0);
		assert(m_tgi.m_variables.any());
		assert(m_tgi.XVal() == 0 || m_tgi.XVal() == 1);

		//if (omp_get_num_threads() == 0)
		//{
		CTPeriod p = m_tgi.GetTPeriod().Transform(CTM(CTM::HOURLY));
		size_t nbSteps = 0;
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			nbSteps += m_pGribsDB->get_image_filepath(TRef).empty() ? 0 : 2;//2 = load + unload

		callback.PushTask("Load gribs data weather for " + m_target.m_name, nbSteps);
		//callback.SetNbStep(nbSteps);
		//}

		ERMsg msg;
		//int currentYear = CTRef::GetCurrentTRef().GetYear();
		((CLocation&)simulationPoint) = m_target;
		simulationPoint.SetHourly(true);

		for (size_t y = 0; y < m_tgi.GetNbYears() && msg; y++)
		{
			int year = m_tgi.GetFirstYear() + int(y);

			for (size_t m = 0; m < 12 && msg; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(year, m) && msg; d++)
				{
					for (size_t h = 0; h < 24 && msg; h++)
					{
						CTRef UTCRef = CTimeZones::LocalTRef2UTCTRef(CTRef(year, m, d, h), m_target);
						if (!m_pGribsDB->get_image_filepath(UTCRef).empty())
						{
							msg += m_pGribsDB->LoadWeather(UTCRef, callback);
							if (msg)
							{
								for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)
								{
									if (m_tgi.m_variables[v])
									{
										double value = m_pGribsDB->GetWeather(m_target, UTCRef, v);
										if (value != -9999)
											simulationPoint[year][m][d][h][v] = value;
									}

								}//for all variables
								msg += callback.StepIt();
							}//if the gribs exist and it's correctly loaded
						}//not empty
						msg += callback.StepIt(0);
					}//for all hours
				}//for all days
			}//for all months
		}//for all years

		callback.PopTask();

		return msg;
	}

	//******************************************************************************

	string CGribsDatabase::get_image_filepath(CTRef TRef)const
	{
		string filePath;
		TRef.Transform(CTM(CTM::HOURLY));
		TRefFilePathMap::const_iterator it = m_filepath_map.find(TRef);
		if (it != m_filepath_map.end())
		{
			filePath = GetAbsolutePath(GetPath(m_filePathGribs), it->second);
		}

		return filePath;
	}

	ERMsg CGribsDatabase::Open(const std::string& filepath, CCallback& callback)
	{
		ERMsg msg;

		m_p_weather_DS.m_max_hour_load = 3;
		m_extents.Reset();

		ifStream file;
		msg = file.open(filepath);
		if (msg)
		{
			std::ios::pos_type length = file.length();
			callback.PushTask("Open Gribs", length);
			//callback.SetNbStep(length);

			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if ((*loop).size() == 2)
				{
					CTRef TRef;
					TRef.FromFormatedString((*loop)[0], "", "-", 1);
					assert(TRef.IsValid());

					m_filepath_map[TRef] = (*loop)[1];
				}

				callback.SetCurrentStepPos((double)file.tellg());
				msg += callback.StepIt(0);
			}

			if (msg)
				m_filePathGribs = filepath;


			callback.PopTask();
		}


		if (msg)
		{
			CTRef TRef = m_filepath_map.begin()->first;
			msg = m_p_weather_DS.load(TRef, get_image_filepath(TRef), callback);
			if (msg)
			{
				//reproject into DEM projection
				m_extents.ExtendBounds(m_p_weather_DS.Get(TRef)->GetExtents());
				m_GEO2WEA = GetReProjection(PRJ_WGS_84, m_extents.GetPrjID());
			}
		}

		return msg;

	}
	double CGribsDatabase::GetWeather(const CGeoPoint3D& ptIn, CTRef UTCRef, size_t vv)const
	{
		ASSERT(ptIn.m_z >= -100);
		ASSERT(ptIn.IsGeographic());

		ERMsg msg;

		if (!IsLoaded(UTCRef))
			return -999;

		//if (vv == H_TRNG)
		//return 0;

		size_t v = Hourly2ATM(vv);
		if (v == NOT_INIT)
			return -999;

		CGeoPoint3D pt(ptIn);
		pt.Reproject(m_GEO2WEA);

		const CGeoExtents& extents = m_p_weather_DS.GetExtents(UTCRef);

		if (!extents.IsInside(pt))
			return -9999;

		CGeoPointIndex xy = get_xy(pt, UTCRef);
		double mean_alt[2] = { 0, 0 };
		double weather[2] = { -999, -999 };

		size_t nb_z = m_bUseOnlySurface ? 1 : 2;
		for (size_t z = 0; z < nb_z; z++)
		{
			int L = m_bUseOnlySurface ? 0 : get_level(xy, pt.m_alt, UTCRef, z == 0);
			size_t bGph = m_p_weather_DS.get_band(UTCRef, ATM_HGT, L);
			mean_alt[z] = m_p_weather_DS.GetPixel(UTCRef, bGph, xy); //geopotential height [m]

			size_t b = m_p_weather_DS.get_band(UTCRef, v, L);

			if (b != UNKNOWN_POS)
			{
				float value = m_p_weather_DS.GetPixel(UTCRef, b, xy);

				if (v == ATM_PRES)
					value /= 100; //convert Pa into hPa

				if (v == ATM_PRCP)
				{
					value *= 3600; //convert mm/s into mm/h
					if (value < 0.05)
						value = 0;

					ASSERT(value >= 0 && value < 40);
				}


				if (v == ATM_WNDU)
				{
					size_t b = m_p_weather_DS.get_band(UTCRef, ATM_WNDV, L);
					double u = value;
					double v = m_p_weather_DS.GetPixel(UTCRef, b, xy);
					assert(false);//need to convert to thrueNorth
					value = sqrt(Square(u) + Square(v)) * 3600 / 1000;
					ASSERT(value >= 0 && value < 300);
				}

				if (v == ATM_WNDV)
				{
					size_t b = m_p_weather_DS.get_band(UTCRef, ATM_WNDU, L);
					double u = m_p_weather_DS.GetPixel(UTCRef, b, xy);
					double v = value;

					assert(false);//need to convert to thrueNorth
					value = (float)GetWindDirection(u, v);
					ASSERT(value >= 0 && value <= 360);
					/*GetWindUV(10, 0, u, v);
					GetWindUV(10, 45, u, v);
					GetWindUV(10, 90, u, v);
					GetWindUV(10, 135, u, v);
					GetWindUV(10, 180, u, v);
					GetWindUV(10, 225, u, v);
					GetWindUV(10, 270, u, v);
					GetWindUV(10, 315, u, v);*/


					//double alpha = PI / 2;
					//if (u != 0 || v != 0)
					//alpha = atan2(u,v);//y, x


					//double angle = int(360 + Rad2Deg(alpha)) % 360;
					//ASSERT(angle >= 0 && angle <= 360);
					//value = angle;

					//value = GetWindDirection(00, 10);
					//value = GetWindDirection(10, 10);
					//value = GetWinBdDirection(10, 00);
					//value = GetWindDirection(10, -10);
					//value = GetWindDirection(00, -10);
					//value = GetWindDirection(-10, -10);
					//value = GetWindDirection(-10, 00);
					//value = GetWindDirection(-10, 10);


				}

				weather[z] = value;
			}
		}//z



		CStatistic  w;
		CStatistic  d;

		for (size_t z = 0; z < nb_z; z++)
		{
			if (weather[z] > -999)
			{
				double p = fabs(pt.m_alt - mean_alt[z]) + 1;
				w += weather[z] / p;
				d += 1.0 / p;
			}
		}

		return w.IsInit() ? w[SUM] / d[SUM] : -999;
	}



	CGeoPointIndex CGribsDatabase::get_xy(const CGeoPoint& pt, CTRef UTCTRef)const
	{
		CGeoExtents extents = m_p_weather_DS.GetExtents(UTCTRef);
		return extents.CoordToXYPos(pt);//take lower-left corner
	}

	int CGribsDatabase::get_level(const CGeoPointIndex& xy, double alt, CTRef UTCTRef, bool bLow)const
	{
		vector<pair<double, int>> test;

		for (int l = 1; l < NB_LEVELS; l++)
		{
			size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, l);
			double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy); //geopotential height [m]
			test.push_back(make_pair(gph, l));

			if (alt < gph)
				break;
		}

		double grAlt = GetGroundAltitude(xy, UTCTRef);//get the first level over the ground
		test.push_back(make_pair(grAlt, 0));
		sort(test.begin(), test.end());

		int L = NB_LEVELS - 1;
		for (int l = 0; l < (int)test.size(); l++)
		{
			if (alt < test[l].first)
			{
				L = test[bLow ? max(0, l - 1) : l].second;
				break;
			}
		}

		ASSERT(L >= 0 && L < NB_LEVELS);
		return L;
	}

	int CGribsDatabase::get_level(const CGeoPointIndex& xy, double alt, CTRef UTCTRef)const
	{
		int L = NB_LEVELS - 1;//take the last level

		for (int l = 1; l < NB_LEVELS; l++)
		{
			size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, l);
			double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xy); //geopotential height [m]

			if (alt < gph)
			{
				L = l;
				break;
			}
		}

		return L;
	}

	double CGribsDatabase::GetGroundAltitude(const CGeoPointIndex& xy, CTRef UTCTRef)const
	{
		size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, 0);
		double alt = m_p_weather_DS.GetPixel(UTCTRef, b, xy); //geopotential height [m]

		return alt;
	}


	CGeoPoint3DIndex CGribsDatabase::get_xyz(const CGeoPoint3D& pt, CTRef UTCTRef)const
	{
		CGeoExtents extents = m_p_weather_DS.GetExtents(UTCTRef);
		CGeoPoint3DIndex xyz;
		((CGeoPointIndex&)xyz) = extents.CoordToXYPos(pt + CGeoDistance3D(extents.XRes() / 2, extents.YRes() / 2, 0, extents.GetPrjID()));


		xyz.m_z = NB_LEVELS - 1;//take the last on by default
		for (size_t l = 0; l < NB_LEVELS; l++)
		{
			size_t b = m_p_weather_DS.get_band(UTCTRef, ATM_HGT, l);
			double gph = m_p_weather_DS.GetPixel(UTCTRef, b, xyz); //geopotential height [m]

			if (gph > pt.m_alt)
			{
				xyz.m_z = int(b);
				break;
			}
		}

		return xyz;
	}




	ERMsg CGribsDatabase::LoadWeather(CTRef UTCTRef, CCallback& callback)
	{
		ERMsg msg;

		if (!m_p_weather_DS.IsLoaded(UTCTRef))
		{
			string filePath = get_image_filepath(UTCTRef);
			if (!filePath.empty())
			{
				msg = m_p_weather_DS.load(UTCTRef, get_image_filepath(UTCTRef), callback);
			}
			else
			{
				msg.ajoute("File " + filePath + " is Missing");
			}
		}


		return msg;
	}

	ERMsg CGribsDatabase::Close(CCallback& callback)
	{
		m_filePathGribs.clear();
		m_filepath_map.clear();
		m_extents.clear();

		return m_p_weather_DS.Discard(callback);
	}

	ERMsg CGribsDatabase::DiscardWeather(CCallback& callback)
	{
		return m_p_weather_DS.Discard(callback);
	}


}