#pragma once

#include "ProvinceSelection.h"
#include "Basic/WeatherStation.h"
//#include "EnvCanLocationMap.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"
#include "basic/csv.h"

//namespace cctz{ class time_zone; }


namespace WBSF
{

	
	//**************************************************************
	class CUIEnvCanHourly : public CTaskBase
	{
	public:


		class CParnerNetwork : public std::map<std::string, std::string>
		{
		public:
			
			ERMsg load(const std::string& filepath)
			{
				ERMsg msg;
				
				ifStream ifile;
				if (ifile.open(filepath))
				{
					for (CSVIterator loop(ifile, ",", true); loop != CSVIterator(); ++loop)
					{
						if (loop->size() == 2)
							insert( make_pair( (*loop)[0], (*loop)[1]));
					}

					ifile.close();
				}
				
				return msg;
			}

			ERMsg save(const std::string& filepath)
			{
				ERMsg msg;
				ofStream oFile;
				msg = oFile.open(filepath);
				if (msg)
				{
					oFile << "KeyID,Network" << std::endl;
					for (auto it = begin(); it != end(); it++)
					{
						oFile << it->first << "," << it->second << std::endl;
					}

					oFile.close();
				}

				return msg;
			};

		};

		enum TSWOBVar{
			SWOB_STN_PRES, SWOB_AIR_TEMP, SWOB_REL_HUM, SWOB_MAX_WND_SPD_10M_PST1HR, SWOB_SNW_DPTH_WTR_EQUI,
			SWOB_WND_DIR_10M_PST1HR_MAX_SPD, SWOB_AVG_WND_SPD_10M_PST1HR, SWOB_AVG_WND_DIR_10M_PST1HR, SWOB_AVG_AIR_TEMP_PST1HR,
			SWOB_MAX_AIR_TEMP_PST1HR, SWOB_MAX_REL_HUM_PST1HR, SWOB_MIN_AIR_TEMP_PST1HR, SWOB_MIN_REL_HUM_PST1HR, SWOB_PCPN_AMT_PST1HR,
			SWOB_SNW_DPTH, SWOB_RNFL_AMT_PST1HR, SWOB_MAX_VIS_PST1HR, SWOB_DWPT_TEMP, SWOB_TOT_GLOBL_SOLR_RADN_PST1HR,
			SWOB_MIN_AIR_TEMP_PST24HRS, SWOB_MAX_AIR_TEMP_PST24HRS, SWOB_PCPN_AMT_PST24HRS, SWOB_MIN_REL_HUM_PST24HRS, SWOB_MAX_REL_HUM_PST24HRS,
			AVG_WND_SPD_PST2MTS, AVG_WND_DIR_PST2MTS, RNFL_AMT_PST24HRS,
			NB_SWOB_VARIABLES /*= 27*/ };
		enum TNetwork{ N_HISTORICAL, N_SWOB, N_SWOB_PARTNERS, NB_NETWORKS };
		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, PROVINCE, NETWORK, PARTNERS_NETWORK, MAX_SWOB_DAYS, HOURLY_PRCP_MAX, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanHourly); }

		CUIEnvCanHourly(void);
		virtual ~CUIEnvCanHourly(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
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
		virtual std::string Default(size_t i)const;
		virtual std::string Option(size_t i)const;

	protected:

		typedef std::array<std::string, NB_SWOB_VARIABLES * 2 + 4> SWOBDataHour;
		typedef std::array< std::array< SWOBDataHour, 24>, 31> SWOBData;


		ERMsg ExecuteHistorical(CCallback& callback);
		std::string GetOutputFilePath(size_t n, const std::string& prov, int year, size_t m, const std::string& stationName)const;
		//std::bitset<CUIEnvCanHourly::NB_NETWORKS> GetStationInformation(const std::string& ID, CLocation& station)const;
		CLocation GetStationInformation(std::string network, const std::string& ID)const;
		ERMsg ReadSwobData(size_t network, CTM TM, CWeatherStation& station, CCallback& callback);

		//Update station list part
		ERMsg DownloadStationList(CLocationVector& stationList, CCallback& callback = DEFAULT_CALLBACK)const;
		ERMsg GetNbStation(const std::string& URL, size_t& nbStation)const;
		ERMsg UpdateCoordinate(__int64 ID, int year, size_t m, size_t d, CLocation& station)const;
		std::string GetStationListFilePath()const{ return GetDir(WORKING_DIR) + "HourlyStationsList.csv"; }

		ERMsg GetStationListPage(const std::string& URL, CLocationVector& stationList)const;
		ERMsg ParseStationListPage(const std::string& source, CLocationVector& stationList)const;
		ERMsg UpdateStationList(CLocationVector& stationList, CLocationVector & stations, CCallback& callback)const;
		std::bitset<CUIEnvCanHourly::NB_NETWORKS> GetNetWork()const;

		//Update data part
		ERMsg DownloadStation(const CLocation& info, CCallback& callback);
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherYear& data, CCallback& callback = DEFAULT_CALLBACK)const;

		ERMsg ExecuteSWOB(size_t network, CCallback& callback);
		//ERMsg ExecuteSWOBPartners(CCallback& callback);
		std::string GetSWOBStationsListFilePath(size_t network)const;
		//std::string GetSWOBPartnersStationsListFilePath()const;
		ERMsg UpdateSWOBLocations(size_t network, CCallback& callback);
		ERMsg GetSWOBLocation(const std::string& filePath, CLocation& location);
		ERMsg GetSWOBList(size_t network, CLocationVector& locations, std::map<std::string, CFileInfoVector>& fileList, CCallback& callback);
		CLocation GetMissingLocation(std::string filepath);
		//std::set<std::string>& missingID, 
//		ERMsg UpdateMissingLocation(size_t network, CLocationVector& locations, const std::map<std::string, CFileInfoVector>& fileList, std::set<std::string>& missingID, CCallback& callback);
		ERMsg DownloadSWOB(size_t network, const CLocationVector& locations, const std::map<std::string, CFileInfoVector>& fileList, CCallback& callback);
		ERMsg ReadSWOBData(const std::string& filePath, CTM TM, CWeatherStation& data, CCallback& callback);
		ERMsg ParseSWOB(CTRef TRef, const std::string& source, SWOBDataHour& data, CCallback& callback);
		ERMsg UpdateLastUpdate(size_t network, const std::map<std::string, CTRef>& lastUpdate);

		ERMsg ReadSWOB(const std::string& filePath, SWOBData& data);
		ERMsg SaveSWOB(const std::string& filePath, const SWOBData& data);
		
		bool NeedDownload(const std::string& filePath, const CLocation& info, int year, size_t m)const;
		//ERMsg CopyStationDataPage(__int64 ID, int year, size_t m, const std::string& page, CCallback& callback);

		
		CLocationVector m_stations;
		CLocationVector m_SWOBstations;
		CLocationVector m_SWOB_partners_stations;

		static CTPeriod String2Period(std::string period);
		static long GetNbDay(const CTime& t);
		static long GetNbDay(int y, size_t m, size_t d);
		static CTRef GetSWOBTRef(const std::string & fileName, bool bLighthouse);
		static std::string GetProvinceFormID(const std::string& ID);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME[NB_NETWORKS];
		static const char* SWOB_VARIABLE_NAME[NB_SWOB_VARIABLES];
		static const char* DEFAULT_UNIT[NB_SWOB_VARIABLES];
		static const HOURLY_DATA::TVarH VARIABLE_TYPE[NB_SWOB_VARIABLES];
	};

}