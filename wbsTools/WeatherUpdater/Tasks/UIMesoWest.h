#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "Basic/WeatherDefine.h"
#include "UI/Common/UtilWWW.h"
#include "Basic\json\json11.hpp"
//namespace cctz{ class time_zone; }

namespace WBSF
{

	class CShapeFileBase;
	
	enum TNetwork { NETWORK_ID, NETWORK_NAME, NETWORK_SHORT_NAME, NETWORK_TYPE, NETWORK_ACTIVE_STATIONS, NB_NETWORK_COLUMN };
	typedef std::map < std::string, std::array<std::string, NB_NETWORK_COLUMN>> NetworkMap;

	//**************************************************************
	class CUIMesoWest : public CTaskBase
	{
	public:

		enum TAPI { LATEST, HISTORICAL, NB_API};
		
		enum Tattributes { WORKING_DIR, API_TOKEN, API_TYPE, FIRST_YEAR, LAST_YEAR, NETWORKS, STATES, PROVINCES, OTHER_COUNTRIES, SUBSET_ID, FORCE_UPDATE_STATIONS_LIST, WITH_TEMP, USE_PRCP, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIMesoWest); }

		CUIMesoWest(void);
		virtual ~CUIMesoWest(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;
		virtual UINT GetTitleStringID()const{ return ATTRIBUTE_TITLE_ID; }
		virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		virtual bool IsHourly()const{ return true; }
		virtual bool IsDaily()const{ return true; }
		virtual bool IsDatabase()const{ return true; }

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i<NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		ERMsg ExecuteHistorical(CCallback& callback);
		ERMsg ExecuteLatest(CCallback& callback);
		ERMsg UpdateNetwork(NetworkMap& networks, CCallback& callback)const;
		ERMsg UpdateStationList(CCallback& callback)const;
		ERMsg convert_json_to_csv(const std::string& txtFilePath, const std::string& csvFilePath, CCallback& callback)const;

		std::string GetStationListFilePath()const;
		std::string GetOutputFilePath(std::string country, std::string states, const std::string& ID, int year);
		CLocationVector GetStationList(CCallback& callback);
		
		ERMsg ReadJSONData(const std::string& filePath, CTM TM, int year, CWeatherAccumulator& accumulator, CWeatherStation& data, CCallback& callback)const;

		
		ERMsg MergeJsonData(std::string request, const std::string& source, size_t& nbDownload, size_t& SUs, CCallback& callback);
		ERMsg MergeData(const std::string& country, const std::string& state, const std::string& ID, const std::string& source, size_t& SUs, CCallback& callback);
		
		CTPeriod GetActualState(const std::string& filePath/*, double& last_hms*/);
		StringVector GetSubsetIds();

		CLocationVector m_stations;
		NetworkMap m_networks;
		

		static HOURLY_DATA::TVarH GetVar(const std::string& str);
		static std::vector<HOURLY_DATA::TVarH > GetVariables(const StringVector& header);
		static bool IsUnknownVar(const std::string& str);
		static CTRef GetTRef(const std::string& str);
		static bool IsValid(HOURLY_DATA::TVarH v, double value);
	
		static ERMsg LoadNetwork(std::string file_path, NetworkMap& map);
		//static double GetCountryState(CShapeFileBase& shapefile, double lat, double lon, const std::string& country, const std::string& state, std::string& countryII, std::string& stateII);
		static double GetCountrySubDivision(CShapeFileBase& shapefile, double lat, double lon, std::string countryI, std::string subDivisionI, std::string& countryII, std::string& subDivisionII);
		CLocation LocationFromJson(const json11::Json::array::const_iterator& it)const;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
	};

}