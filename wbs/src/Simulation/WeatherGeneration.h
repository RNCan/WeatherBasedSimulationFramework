//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once


//#include "Basic/NormalsDatabase.h"
//#include "Basic/DailyDatabase.h"
//#include "Basic/HourlyDatabase.h"
//#include "Geomatic/SfcGribsDatabase.h"
#include "Basic/Timer.h"
#include "ModelBase/ModelInput.h"
#include "Simulation/Executable.h"
#include "Simulation/WeatherGenerator.h"

namespace WBSF
{
	class CFileManager;
	class CWGInput;
	//class CGribsMap;
	//class CSfvGribsDatabase;

	class CWeatherGeneration : public CExecutable
	{
	public:

		enum TMember{
			WG_INPUT_NAME = CExecutable::NB_MEMBERS, LOCATIONS_NAME, PVARIATIONS_NAME, NB_REPLICATIONS, /*LOCATIONS_REPLACED_BY_WEATHER_STATIONS,
			X_VALIDATION, */USE_HXGRID, NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};


		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CWeatherGeneration()); }

		//*** public member ***
		std::string m_WGInputName;
		std::string m_locationsName;
		std::string m_parametersVariationsName;
		short	m_nbReplications;
		bool	m_bUseHxGrid;


		//CWGVariationsVector m_PVariations;

		//	bool m_bLocationsReplacedByWeatherStations;
		//	bool m_bXValidation;


		//*********************


		CWeatherGeneration();
		CWeatherGeneration(const CWeatherGeneration& in);
		virtual ~CWeatherGeneration();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CWeatherGeneration(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CWeatherGeneration&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CWeatherGeneration&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);


		void Reset();
		void clear(){ Reset(); }

		CWeatherGeneration& operator =(const CWeatherGeneration& in);
		bool operator == (const CWeatherGeneration& in)const;
		bool operator != (const CWeatherGeneration& in)const{ return !operator==(in); }

		virtual std::string GetPath(const CFileManager& fileManager)const;

		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);
		virtual int GetNbTask()const{ return 5; }//open daily, open hourly, update gribs, verify locations and execute

		ERMsg GenerateWeather(const CFileManager& fileManager, CNormalsDatabasePtr& normalDB, CDailyDatabasePtr& dailyDB, CHourlyDatabasePtr& hourlyDB,
			CSfcGribDatabasePtr& gribsDB, const CWGInput& TGInput, const CLocationVector& locArray, CCallback& callback);

		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const;
		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_FLOAT; }


		ERMsg GetWGInput(const CFileManager& fileManager, CWGInput& WGInput)const;
		ERMsg GetLocations(const CFileManager& fileManager, CLocationVector& locations)const;
		ERMsg GetParametersVariations(const CFileManager& fileManager, CParametersVariationsDefinition& variations)const;

		static void InitValidationOptimisation();

		static ERMsg GenerateLocationListFromWeatherStation(const CFileManager& fileManager, const CWGInput& TGInput, const CLocationVector& locations, CLocationVector& stations);
		static void GetLocationIndexGrid(const CLocationVector& locArray, std::vector<size_t>& locPos);

	protected:

		std::string GetWGFilePath(const std::string& filePath, int noLoc, int noPV, int noRep)const;
		std::string GetModelOutputFilePath(const std::string& filePath, int noLoc, int noParam, short noRep)const;
		ERMsg ReadOutputFile(const std::string& filePath, CModelStatVector& data, CTM TM, bool bOutputJulianDay, int m_nbOutputVar);

		ERMsg CheckLocationsInDatabase(CNormalsDatabasePtr& normalDB, CDailyDatabasePtr& dailyDB, CHourlyDatabasePtr& hourlyDB, const CLocationVector& locations, const CWGInput& TGInput, CCallback& callback);
		ERMsg RunHxGridSimulation(const CFileManager& fileManager, CNormalsDatabasePtr& normalDB, CDailyDatabasePtr& dailyDB, CModel& model,
			const CWGInput& TGInput, const CModelInputVector& modelInputArray,
			const CLocationVector& locArray, CTimer& timerTG, CResult& result, CCallback& callback);
		ERMsg RunStreamSimulation(const CFileManager& fileManager, CNormalsDatabasePtr& normalDB, CDailyDatabasePtr& dailyDB, CModel& model,
			const CWGInput& TGInput, const CModelInputVector& modelInputArray,
			const CLocationVector& locArray, CTimer& timerTG, CResult& result, CCallback& callback);
		ERMsg RunFileSimulation(const CFileManager& fileManager, CNormalsDatabasePtr& normalDB, CDailyDatabasePtr& dailyDB, CModel& model,
			const CWGInput& TGInput, const CModelInputVector& modelInputArray,
			const CLocationVector& locArray, CTimer& timerTG, CResult& result, CCallback& callback);


		
		//ca va dans model...
		//ERMsg CheckTGInput(  const CWGInput& TGInput, const CModel& model)const;


		//optimisation : est-ce ici que ca vas?
		//    static CTime m_lastWeatherTime;


		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;
		//static CTime GetLastModificationDate(const std::string& filePath);



	};


	//
	//namespace zen
	//{
	//	template <> inline
	//	void writeStruc(const CWeatherGeneration& in, XmlElement& output)
	//	{
	//		writeStruc( (const CExecutable&)in, output );
	//
	//		XmlOut out(output);
	//		
	//		out[CWeatherGeneration::GetMemberName(CWeatherGeneration::WG_INPUT_NAME)](in.m_WGInputName);
	//		out[CWeatherGeneration::GetMemberName(CWeatherGeneration::LOCATIONS_NAME)](in.m_locationsName);
	//		out[CWeatherGeneration::GetMemberName(CWeatherGeneration::PVARIATIONS_NAME)](in.m_parametersVariationsName);
	//		out[CWeatherGeneration::GetMemberName(CWeatherGeneration::NB_REPLICATIONS)](in.m_nbReplications);
	//		//out[CWeatherGeneration::GetMemberName(CWeatherGeneration::LOCATIONS_REPLACED_BY_WEATHER_STATIONS)](in.m_bLocationsReplacedByWeatherStations);
	//		//out[CWeatherGeneration::GetMemberName(CWeatherGeneration::X_VALIDATION)](in.m_bXValidation);
	//		out[CWeatherGeneration::GetMemberName(CWeatherGeneration::USE_HXGRID)](in.m_bUseHxGrid);
	//	}
	//
	//	template <> inline
	//	bool readStruc(const XmlElement& input, CWeatherGeneration& out)
	//	{
	//		XmlIn in(input);
	//		
	//		readStruc( input, (CExecutable&)out );
	//
	//		in[CWeatherGeneration::GetMemberName(CWeatherGeneration::WG_INPUT_NAME)](out.m_WGInputName);
	//		in[CWeatherGeneration::GetMemberName(CWeatherGeneration::LOCATIONS_NAME)](out.m_locationsName);
	//		in[CWeatherGeneration::GetMemberName(CWeatherGeneration::PVARIATIONS_NAME)](out.m_parametersVariationsName);
	//		in[CWeatherGeneration::GetMemberName(CWeatherGeneration::NB_REPLICATIONS)](out.m_nbReplications);
	//		//in[CWeatherGeneration::GetMemberName(CWeatherGeneration::LOCATIONS_REPLACED_BY_WEATHER_STATIONS)](out.m_bLocationsReplacedByWeatherStations);
	//		//in[CWeatherGeneration::GetMemberName(CWeatherGeneration::X_VALIDATION)](out.m_bXValidation);
	//		in[CWeatherGeneration::GetMemberName(CWeatherGeneration::USE_HXGRID)](out.m_bUseHxGrid);
	//
	//		return true;
	//	}
	//}
}