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
//******************************************************************************
#pragma once

#include <vector>
#include "basic/ERMsg.h"
#include "Basic/Location.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "ModelBase/WGInput.h"
#include "Simulation/WeatherGradient.h"
#include "Simulation/WeatherGeneratorKernel.h"
//#include "Simulation/ATM.h"

namespace WBSF
{

	typedef CWeatherStation CSimulationPoint;
	typedef CWeatherStationVector CSimulationPointVector;

	//class CWeather;
	class CWeatherYears;



	class CWeatherGenerator
	{
	public:


		enum TWarning{ W_DATA_FILLED_WITH_NORMAL, W_UNEEDED_REPLICATION, NB_WARNING };
		static void OutputWarning(const std::bitset<NB_WARNING>& bits, CCallback& callback);
		static void ExposureIndices(double exposure_index[12], double latit, double elev, float slope, float orientation, short albedoType);
		static int Sol9(double inlatit, double inelev, double inslope, double inazimuth, double *expin);
		static void CompleteSimpleVariables(CSimulationPoint& simulationPoint, CWVariables variables);
		static ERMsg ComputeHumidityRadiation(CSimulationPoint&  simulationPoint, CWVariables variables);
		static ERMsg ComputeSnow(CSimulationPoint&  simulationPoint, CWVariables variables);
		static ERMsg ComputeWindDirection(CSimulationPoint& simulationPoint);
		static ERMsg ComputePressure(CSimulationPoint& simulationPoint);
		static bool VerifyData(const CSimulationPointVector& simulationPoints, CWVariables variables1);


		CWeatherGenerator();
		~CWeatherGenerator();

		void Reset();
		ERMsg Initialize(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg Generate(CCallback& callback = DEFAULT_CALLBACK);
		const CSimulationPoint& GetWeather(size_t r)const{ assert(r < m_simulationPoints.size());  return m_simulationPoints[r]; }


		//initialization member
		std::string GetNormalDBFilePath()const{ return m_pNormalDB ? m_pNormalDB->GetFilePath() : ""; }
		std::string GetDailyDBFilePath()const{ return m_pDailyDB ? m_pDailyDB->GetFilePath() : ""; }
		std::string GetHourlyDBFilePath()const{ return m_pHourlyDB ? m_pHourlyDB->GetFilePath() : ""; }

		CNormalsDatabasePtr& GetNormalDB(){ return m_pNormalDB; }
		void SetNormalDB(CNormalsDatabasePtr& pNormalDB){ if (m_pNormalDB.get() != pNormalDB.get())m_pNormalDB = pNormalDB; }
		CDailyDatabasePtr& GetDailyDB(){ return m_pDailyDB; }
		void SetDailyDB(CDailyDatabasePtr& pDailyDB){ if (m_pDailyDB.get() != pDailyDB.get())m_pDailyDB = pDailyDB; }
		CHourlyDatabasePtr& GetHourlyDB(){ return m_pHourlyDB; }
		void SetHourlyDB(CHourlyDatabasePtr& pHourlyDB){ if (m_pHourlyDB.get() != pHourlyDB.get())m_pHourlyDB = pHourlyDB; }
		CSfcGribDatabasePtr& GetGribsDB(){ return m_pGribDB; }
		void SetGribsDB(CSfcGribDatabasePtr& pGribDB){ if (m_pGribDB.get() != pGribDB.get())m_pGribDB = pGribDB; }
		const CLocation& GetTarget()const{ return m_target; }
		void SetTarget(const CLocation& target){ ((CLocation&)m_target) = target; }
		size_t GetNbReplications() const { return m_nbReplications; }
		void SetNbReplications(size_t nbReplications){ m_nbReplications = nbReplications; }
		size_t GetSeed() const { return m_seed; }
		void SetSeed(size_t seed){ m_seed = seed; }
		const CWGInput& GetWGInput()const{ return m_tgi; }
		void SetWGInput(const CWGInput& TGInput){ m_tgi = TGInput; }

		

		ERMsg GetNormals(CNormalsStation& normals, CCallback& callback);
		ERMsg GetDaily(CSimulationPoint& simulationPoint, CCallback& callback);
		ERMsg GetHourly(CSimulationPoint& simulationPoint, CCallback& callback);
		//ERMsg GetGribs(CSimulationPoint& simulationPoint, CCallback& callback);
		ERMsg GenerateNormals(CSimulationPointVector& simulationPointVector, CCallback& callback);
		void RemoveForecast(CSimulationPoint& simulationPoint);

		const std::bitset<NB_WARNING>& GetWarningBits()const { return m_warning; }
		
	protected:

		void GenerateSeed();
		

		bool UseExpo()const{ return m_target.GetSlope() > 0 && m_tgi.m_albedo != CWGInput::NONE; }


		//members
		CWGInput m_tgi;				//WG Input
		CLocation m_target;			//Target location
		size_t m_nbReplications;	//number of replication
		size_t m_seed;				//seed generator

		CNormalsDatabasePtr m_pNormalDB;	//Normal database
		CDailyDatabasePtr m_pDailyDB;		//daily database
		CHourlyDatabasePtr m_pHourlyDB;		//hourly database
		CSfcGribDatabasePtr	m_pGribDB;

		//result
		CSeedMatrix m_seedMatrix;	//seed for random generator for each iteration
		CWeatherGradient m_gradients;	//gradient at the target location
		std::bitset<NB_WARNING> m_warning;

		//new result
		CSimulationPointVector m_simulationPoints;

	};

	typedef std::vector<CWeatherGenerator> CWeatherGeneratorVector;
}