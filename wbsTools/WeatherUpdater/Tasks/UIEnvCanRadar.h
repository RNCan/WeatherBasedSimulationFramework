#pragma once


#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{


	enum TRadar{ R_ATL, R_ONT, R_PNR, R_PYR, R_QUE, R_WBI, R_WGJ, R_WHK, R_WHN, R_WKR, R_WMB, R_WMN, R_WSO, R_WTP, R_WUJ, R_WVY, R_WWW, R_XAM, R_XBE, R_XBU, R_XDR, R_XFT, R_XFW, R_XGO, R_XLA, R_XMB, R_XME, R_XNC, R_XNI, R_XPG, R_XRA, R_XSI, R_XSM, R_XSS, R_XTI, R_XWL, NB_RADAR };
	class CCanadianRadar : public std::bitset<NB_RADAR>
	{
	public:
		static const char* RADARS[NB_RADAR][3];
	};


	class CUIEnvCanRadar : public CTaskBase
	{
	public:

		enum TTypeIII{ CURRENT_RADAR, HISTORICAL_RADAR };
		enum TTypeI{ TYPE_06HOURS, TYPE_24HOURS };
		enum TTypeII{ T_SNOW, T_RAIN, NB_TYPE };

		enum TAttributes { WORKING_DIR, TYPE, PRCP_TYPE, RADAR, FIRST_DATE, LAST_DATE, NB_ATTRIBUTES };
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUIEnvCanRadar); }

		CUIEnvCanRadar(void);
		virtual ~CUIEnvCanRadar(void);

		//proptree param
		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}
		//virtual void UpdateLanguage();


		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		//virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		//virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		//virtual const std::string& Title(size_t i)const{ ASSERT(i < NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;


	protected:


//		ERMsg DownloadForecast(CCallback& callback);
	//	std::string GetOutputFilePath(const std::string& fileName)const;
		ERMsg ExecuteCurrent(CCallback& callback);
		ERMsg ExecuteHistorical(CCallback& callback);
		bool NeedDownload(const CFileInfo& info, const std::string& filePath)const;
		ERMsg GetRadarList(StringVector& stationList, CCallback& callback)const;
		ERMsg CleanRadarList(StringVector& stationList, CCallback& callback)const;
		std::string GetOutputFilePath(size_t t, const std::string& fileName)const;
		CTPeriod GetPeriod()const;

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const char* SERVER_NAME[2];
		static const char* SERVER_PATH;
		static const char* TYPE_NAME[NB_TYPE];
	};

}