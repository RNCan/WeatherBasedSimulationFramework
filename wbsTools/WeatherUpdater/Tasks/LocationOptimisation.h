#pragma once

//#include "CountrySelection.h"
//#include "StateSelection.h"
//#include "FileStamp.h"
//#include "Basic/Location.h"
#include "Basic/WeatherStation.h"

//#include "TaskBase.h"

namespace WBSF
{

	class CGDALDatasetEx;


	class CLocationOptimisation : public std::map < std::string, CLocation >
	{
	public:

		static const __int32 VERSION = 1;
		static CFileInfo GetLastUpdate(const std::string& filePath);
		static bool NeedUpdate(const std::string& refFilePath, const std::string& optFilePath);

		bool KeyExists(const std::string& key)const{ return find(key) != end(); }

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);

		//friend boost::serialization::access;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<std::map<std::string, CLocation>>(*this);
			ar & m_referenceFileStamp;
		}

	protected:

		CFileInfo m_referenceFileStamp;
	};


	class CGHCNStationOptimisation : public CLocationOptimisation
	{
	public:

		ERMsg Update(const std::string& refFilePath, CCallback& callback = DEFAULT_CALLBACK);

		std::string m_DEMFilePath;

	protected:

		static int GetPrecision(const std::string& line);
		static double GetStationElevation(const CGDALDatasetEx& dataset, const CLocation& station, int nbNearest, double deltatElevMax);
		static void GetZoneElevation(const CGDALDatasetEx& dataset, const CGeoPoint& pt0, const CGeoPoint& pt1, const CGeoPoint& pt2, double& evelLow, double& elev, double& elevHigh);
		static bool LocationFromLine(std::string line, CLocation& station);

	};

	//****************************************************************
	//typedef std::map<std::string, CWeatherStation> CWeatherStationMap;

	
}