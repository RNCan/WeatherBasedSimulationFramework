#pragma once


//#include "CountrySelection.h"
//#include "StateSelection.h"
//#include "FileStamp.h"
#include "LocationOptimisation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"

namespace WBSF
{

	class CGDALDatasetEx;

	//****************************************************************
	//GHCND extractor
	class CUIGHCND : public CTaskBase
	{
	public:

		enum TAttributes { WORKING_DIR, FIRST_YEAR, LAST_YEAR, COUNTRIES, STATES, SHOW_PROGRESS, NB_ATTRIBUTES };

		enum TD32x0
		{
			ASMM, ASSS, AWND, CLDG, DPNT, DPTP, DYSW, DYVC,
			F2MN, F5SC, FMTM, FRGB, FRGT, FRTH, FSIN, FSMI,
			FSMN, GAHT, HTDG, MNRH, MNTP, MXRH, PGTM, PKGS,
			PRCP, PRES, PSUN, RDIR, RWND, SAMM, SASS, SCMM,
			SCSS, SGMM, SGSS, SLVP, SMMM, SMSS, SNOW, SNWD,
			WESF, WESD,
			STMM, STSS, THIC, TMAX, TMIN, TMPW, TSUN, WTEQ,
			//TD3200 only
			EVAP, MNPN, MXPN, TOBS, WDMV,
			NB_ELEMENT
		};

		enum TSimpleGHCN{ V_TMIN, V_TMAX, V_PRCP, V_AWND, V_WESF, V_SNWD, V_WESD, NB_VARIABLES };
		static const int GHCN_VARIABLES[NB_VARIABLES];
		static size_t GetElementType(const char* type);
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIGHCND); }


		CUIGHCND(void);
		virtual ~CUIGHCND(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
virtual UINT GetDescriptionStringID()const{ return DESCRIPTION_TITLE_ID; }
		

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }

		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		std::string GetStationListFilePath()const;
		std::string GetMissingFilePath()const;

		ERMsg LoadStationList(CCallback& callback);
		ERMsg UpdateStationList(UtilWWW::CFtpConnectionPtr& pConnection, CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CTM TM, CWeatherYear& data, CCallback& callback)const;


		std::string GetStationFilePath(bool bLocal = true)const{ return (bLocal ? GetDir(WORKING_DIR) : std::string(SERVER_PATH)) + "ghcnd-stations.txt"; }
		ERMsg sevenZ(const std::string& filePathZip, const std::string& workingDir, CCallback& callback);
		ERMsg FTPDownload(const std::string& server, const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback);


		ERMsg UpdateStationHistory(CCallback& callback);
		ERMsg GetFileList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;

		ERMsg UpdateOptimisationStationFile(const std::string& workingDir, CCallback& callBack = DEFAULT_CALLBACK)const;

		std::string GetOptFilePath()const;

		ERMsg LoadOptimisation();
		bool StationExist(const std::string& fileTitle)const;
		void GetStationInformation(const std::string& fileTitle, CLocation& location)const;

		//Get stations list part
		ERMsg CleanList(CFileInfoVector& fileList, CCallback& callback = DEFAULT_CALLBACK)const;

		std::string GetOutputFilePath(const CFileInfo& info)const;
		std::string GetOutputFilePath(const std::string& stationName)const;

		ERMsg CleanList(StringVector& fileList, CCallback& callback)const;
		bool IsFileInclude(const std::string& fileTitle)const;
		bool IsStationInclude(const std::string& ID)const;



	/*	CCountrySelection m_countries;
		CStateSelection m_states;
		CGeoRect m_boundingBox;

		std::string m_DEMFilePath;
		bool m_bUseDEMStation;
		bool m_bShowFTPTransfer;
		bool m_bExtractWind;
		bool m_bExtractSnow;
*/

		ERMsg PreProcess(CCallback& callback);
		//optimisation for GetStations
		CGHCNStationOptimisation m_optFile;

		typedef std::string StationID;
		class SimpleDataDay : public std::array < float, NB_VARIABLES >
		{
		public:
			SimpleDataDay()
			{
				fill(-999);
			}
		};


		typedef std::array<SimpleDataDay, 366> SimpleDataYear;
		typedef std::map<short, SimpleDataYear> SimpleDataYears;
		typedef std::map<StationID, SimpleDataYears> SimpleDataMap;

		SimpleDataMap m_loadedData;

		std::set<StationID> m_included;
		std::set<StationID> m_rejected;
		ERMsg LoadData(const std::string& filePath, SimpleDataMap& data, CCallback& callback)const;


		static const char* ELEM_CODE[NB_ELEMENT];
		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
	};

}