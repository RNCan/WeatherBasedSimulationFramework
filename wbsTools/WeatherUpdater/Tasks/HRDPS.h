#pragma once



#include "Basic/HourlyDatabase.h"
#include "Basic/UtilStd.h"
#include "Geomatic/gdalbasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"



namespace WBSF
{

	enum THRDPSVariables
	{
		LFTX_SFC, ALBDO_SFC, APCP_SFC, DLWRF_SFC, DSWRF_SFC, HGT_SFC, ICEC_SFC, LAND_SFC, LHTFL_SFC, NLWRS_SFC, NSWRS_SFC, PRATE_SFC,
		PRES_SFC, SHOWA_SFC, SHTFL_SFC, SNOD_SFC, SPFH_SFC, TCDC_SFC, TSOIL_SFC, WEAFR_SFC, WEAPE_SFC, WEARN_SFC, WEASN_SFC,
		WTMP_SFC, GUST_SFC, ICETK_SFC, RH_SFC, SOILVIC_SFC,
		DEN_TGL, DEPR_TGL, DPT_TGL, RH_TGL, SPFH_TGL, TMP_TGL, UGRD_TGL, VGRD_TGL, WDIR_TGL, WIND_TGL, ABSV_ISBL,
		DEPR_ISBL, HGT_ISBL, RH_ISBL, SPFH_ISBL, TMP_ISBL, UGRD_ISBL, VGRD_ISBL, VVEL_ISBL, WDIR_ISBL, WIND_ISBL, CAPE_ETAL,
		CWAT_EATM, DSWRF_NTAT, HGT_ISBY, HLCY_ETAL, PRMSL_MSL, SOILW_DBLY, TSOIL_DBLL, ULWRF_NTAT, USWRF_NTAT, NB_HRDPS_VARIABLES
	};


	class CHRDPSVariables : public std::bitset<NB_HRDPS_VARIABLES>
	{
	public:


		enum THRDPSCategory{ HRDPS_TGL, HRDPS_SFR, HRDPS_ISBL, HRDPS_ETAL, HRDPS_EATM, HRDPS_NTAT, HRDPS_ISBY, HRDPS_MSL, HRDPS_DBLY, HRDPS_DBLL, NB_HRDPS_CATEGORY };

		static const char* GetName(size_t i){ ASSERT(i<NB_HRDPS_VARIABLES); return NAME[i]; }
		static const char* GetCategory(size_t i){ ASSERT(i<NB_HRDPS_CATEGORY); return CATEGORY[i]; }

		static const std::string& GetDescription(size_t i){ ASSERT(i < NB_HRDPS_VARIABLES); LoadDescription();  return DESCRIPTION[i]; }
		CHRDPSVariables& operator = (std::string in);
		

		static size_t GetVariable(const std::string& name);
		static size_t GetCategory(const std::string& name);
		static size_t GetLevel(const std::string& name);
		
		static std::string GetHRDPSSelectionString();

	protected:

		static void LoadDescription();

		static const char* NAME[NB_HRDPS_VARIABLES];
		static const char* CATEGORY[NB_HRDPS_CATEGORY];
		
		static StringVector DESCRIPTION;
	};



	//**************************************************************
	class CHRDPS
	{
	public:

		CHRDPS(const std::string& workingDir);
		virtual ~CHRDPS(void);

		bool m_bCreateVRT;
		CHRDPSVariables m_variables;
		bool m_bForecast;

		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetStationList(CLocationVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetVirtuelStation(const CLocationVector& stations, CWVariables variables, CTPeriod p, CWeatherStation& station, CCallback& callback = DEFAULT_CALLBACK);

		ERMsg CreateVRT(CCallback& callback = DEFAULT_CALLBACK);
		std::string GetVRTFilePath(CTRef TRef);

		

	protected:

		std::string m_workingDir;


		std::string GetOutputFilePath(const std::string& filetitle)const;
		std::string GetRemoteFilePath(size_t HH, size_t hhh, const std::string& filetitle)const;

		size_t GetHH(const std::string& title)const;
		size_t Gethhh(const std::string& title)const;
		size_t GetLatestHH(UtilWWW::CHttpConnectionPtr& pConnection)const;
		CTRef GetTRef(const std::string& title)const;
		ERMsg OpenDatasets(CCallback& callback);

		static size_t GetVariable(const std::string& fileTitle);
		static size_t GetLevel(const std::string& fileName);


		static const char* SERVER_NAME;
		static const char* SERVER_PATH;
	};

}