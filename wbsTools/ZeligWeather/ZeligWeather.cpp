// ZeligWeather.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include "ZeligWeather.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDefine.h"
#include "Basic/DynamicRessource.h"
#include "Simulation/WeatherGradient.h"
#include "Simulation/WeatherGenerator.h"
#include "FileManager/FileManager.h"
#include "WeatherBasedSimulationString.h"
#include "Basic/Shore.h"


using namespace WBSF;
using namespace HOURLY_DATA;
using namespace NORMALS_DATA;
extern HMODULE g_hDLL;


extern "C"
{
	static CNormalsDatabasePtr m_pNormalDB;
	

	void print_msg(ERMsg msg)
	{
		for (UINT i = 0; i < msg.dimension(); i++)
			std::cout << msg[i] << std::endl;
	}

	DLL_API bool LoadZeligWeather(const char* path)
	{
		ERMsg msg;
		
		
		CDynamicResources::set(g_hDLL);

		m_pNormalDB.reset(new CNormalsDatabase);
		
	
		msg = m_pNormalDB->Open(path);
		if (msg)
			msg = m_pNormalDB->OpenSearchOptimization(CCallback());//open here to be thread safe

		if (!msg)
			print_msg(msg);

		return msg;
	}

	DLL_API bool GetZeligWeather(double latitude, double longitude, double elevation, ZeligWeatherOutput output)
	{
		ERMsg msg;

		CLocation location("", "", latitude, longitude, elevation);

		CWeatherGradient gradients;

		gradients.SetNormalsDatabase(m_pNormalDB);
		gradients.m_variables = CWVariables("TN T TX P");
		gradients.m_bUseShore = false;
		gradients.m_target = location;
		msg = gradients.CreateGradient();

		for (size_t c = 0; c < 2 && msg; c++)//for temperature and precipitation
		{
			TVarH v = GetCategoryLeadVariable(c);

			CSearchResultVector results;
			// find stations with category
			msg = m_pNormalDB->Search(results, location, 4, -999, v);
			if (msg && results.size() == 4)
			{
				CNormalsStationVector stationsVector;
				m_pNormalDB->GetStations(stationsVector, results);
				stationsVector.ApplyCorrections(gradients);
				
				CNormalsStation normals;
				stationsVector.GetInverseDistanceMean(location, v, normals, true, false);
				for (size_t m = 0; m < 12; m++)
				{
					if (c == 0)
					{
						output[m][OUT_T_MEAN] = (normals[m][TMIN_MN] + normals[m][TMAX_MN])/2.0;
						output[m][OUT_T_SD] = normals[m][TMIN_SD] * 0.30020 + normals[m][TMAX_SD] * 0.08758 + normals[m][TACF_A1] * 1.96203 + normals[m][TACF_A2] * 2.33985 + normals[m][TACF_B1] * 1.07012 + normals[m][TACF_B2] * 0.92898 - 1.65794;
					}
					else
					{
						ASSERT(c == 1);

						output[m][OUT_P_TOT] = normals[m][PRCP_TT];
						output[m][OUT_P_SD] = normals[m][PRCP_SD]* normals[m][PRCP_TT];//convert coeficient of variation to standard deviation
					}
				}
			}
		}

		if (!msg)
			print_msg(msg);

		return msg;
	}

	DLL_API bool GetZeligNormals(double latitude, double longitude, double elevation, float out_normals[12][11])
	{
		ERMsg msg;

		CLocation location("", "", latitude, longitude, elevation);
		
	
		CWeatherGradient gradients;

		gradients.SetNormalsDatabase(m_pNormalDB);
		gradients.m_variables = CWVariables("TN T TX P");
		gradients.m_bUseShore = false;
		gradients.m_target = location;
		msg = gradients.CreateGradient();
		
		for (size_t c = 0; c < 2 && msg; c++)//for temperature and precipitation
		{
			TVarH v = GetCategoryLeadVariable(c);

			CSearchResultVector results;
			// find stations with category
			msg = m_pNormalDB->Search(results, location, 4, -999, v);
			if (msg && results.size() == 4)
			{
				CNormalsStationVector stationsVector;
				m_pNormalDB->GetStations(stationsVector, results);
				stationsVector.ApplyCorrections(gradients);

				CNormalsStation normals;
				stationsVector.GetInverseDistanceMean(location, v, normals, true, false);
				for (size_t m = 0; m < 12; m++)
				{
					if (c == 0)
					{
						for(size_t vv=0; vv< PRCP_TT; vv++)
							out_normals[m][vv] = normals[m][vv];
					}
					else
					{
						ASSERT(c == 1);

						out_normals[m][9] = normals[m][PRCP_TT];
						out_normals[m][10] = normals[m][PRCP_SD];
					}
				}
			}
		}


		if (!msg)
			print_msg(msg);

		return msg;
	}



	DLL_API bool GetZeligWG(const char* path, double latitude, double longitude, double elevation, float out_normals[12][11])
	{
		ERMsg msg;


		CLocation location("", "", latitude, longitude, elevation);

		//CShore::SetShore("D:/Project/bin/Releasex64/Layers/Shore.ann");
		CWeatherGenerator WG;
		//const CFileManager& fileManager = CFileManager::GetInstance();

		
		//Load WGInput 
		CWGInput WGInput;
		
		WGInput.m_variables = "TN T TX P";
		//int m_sourceType;
		//int m_generationType;
		WGInput.m_nbNormalsYears=1;
		//int m_firstYear;
		//int m_lastYear;
		//bool m_bUseForecast;
		//bool m_bUseRadarPrcp;
		//bool m_bUseGribs;

		//std::string m_normalsDBName;
		WGInput.m_nbNormalsStations = 4;
		//std::string m_dailyDBName;
		//size_t m_nbDailyStations;
		//std::string m_hourlyDBName;
		//size_t m_nbHourlyStations;
		//std::string m_gribsDBName;
		//size_t m_nbGribPoints;
		//size_t m_albedo;
		//size_t m_seed;
		//CWVariables m_allowedDerivedVariables;
		//bool m_bXValidation;
		//bool m_bSkipVerify;
		//bool m_bNoFillMissing;
		WGInput.m_bUseShore =false;
		//CSearchRadius m_searchRadius;

		
		//open normal database
		CNormalsDatabasePtr pNormalDB;
		if (msg)
		{
			pNormalDB.reset(new CNormalsDatabase);
			msg = pNormalDB->Open(path, CNormalsDatabase::modeRead, CCallback(), WGInput.m_bSkipVerify);
			if (msg)
				msg = pNormalDB->OpenSearchOptimization(CCallback());//open here to be thread safe
		}

		if (msg)
		{
			WG.SetWGInput(WGInput);
			WG.SetNbReplications(30);//no replication in daily db
			WG.SetNormalDB(pNormalDB);

			WG.SetTarget(location);
			msg = WG.Initialize();//create gradient

			if (msg)
			{
				CNormalsStation normals;
				WG.GetNormals(normals, CCallback());

				for (size_t c = 0; c < 2 && msg; c++)//for temperature and precipitation
				{
					for (size_t m = 0; m < 12; m++)
					{
						if (c == 0)
						{
							for (size_t vv = 0; vv < PRCP_TT; vv++)
								out_normals[m][vv] = normals[m][vv];
						}
						else
						{
							ASSERT(c == 1);

							out_normals[m][9] = normals[m][PRCP_TT];
							out_normals[m][10] = normals[m][PRCP_SD];
						}
					}
				}
			}
		}

		if (!msg)
			print_msg(msg);

		return msg;
	}
}
