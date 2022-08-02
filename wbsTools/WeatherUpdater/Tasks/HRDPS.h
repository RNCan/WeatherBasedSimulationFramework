#pragma once



#include "Basic/HourlyDatabase.h"
#include "Basic/UtilStd.h"
#include "Geomatic/gdalbasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "UI/Common/UtilWWW.h"
#include "TaskBase.h"
#include "HRDPA.h"


namespace WBSF
{

	enum THRDPSVariables
	{
		LFTX_SFC, ALBDO_SFC, APCP_SFC, DLWRF_SFC, DSWRF_SFC, HGT_SFC, ICEC_SFC, LAND_SFC, LHTFL_SFC, NLWRS_SFC, NSWRS_SFC, PRATE_SFC,
		PRES_SFC, SHOWA_SFC, SHTFL_SFC, SNOD_SFC, SPFH_SFC, TCDC_SFC, TSOIL_SFC, WEAFR_SFC, WEAPE_SFC, WEARN_SFC, WEASN_SFC,
		WTMP_SFC, GUST_SFC, ICETK_SFC, RH_SFC, SOILVIC_SFC, GUST_MAX_SFC, GUST_MIN_SFC, SDEN_SFC, SFCWRO_SFC, SDWE_SFC, HPBL_SFC, PTYPE_SFC, SKINT_SFC, UTCI_SFC, LAST_SFC,
		DEN_TGL = LAST_SFC, DEPR_TGL, DPT_TGL, RH_TGL, SPFH_TGL, TMP_TGL, UGRD_TGL, VGRD_TGL, WDIR_TGL, WIND_TGL, GUST_MAX_TGL, GUST_MIN_TGL, GUST_TGL, LAST_TGL,
		ABSV_ISBL = LAST_TGL, DEPR_ISBL, HGT_ISBL, RH_ISBL, SPFH_ISBL, TMP_ISBL, UGRD_ISBL, VGRD_ISBL, VVEL_ISBL, WDIR_ISBL, WIND_ISBL, MU_VT_LI_ISBL, SHWINX_ISBL, LAST_ISBL,
		HGT_ISBY = LAST_ISBL, LAST_ISBY,
		CAPE_ETAL = LAST_ISBY, HLCY_ETAL, LAST_ETAL,
		CWAT_EATM = LAST_ETAL, LAST_EATM,
		PRMSL_MSL = LAST_EATM, LAST_MSL,
		SOILW_DBLY = LAST_MSL, LAST_DBLY,
		TSOIL_DBLL = LAST_DBLY, SOILW_DBLL, LAST_DBLL,
		DSWRF_NTAT = LAST_DBLL, ULWRF_NTAT, USWRF_NTAT, LAST_NTAT,
		NB_HRDPS_VARIABLES = LAST_NTAT
	};

	enum THRDPSCategory { HRDPS_SFR, HRDPS_TGL, HRDPS_ISBL, HRDPS_ISBY, HRDPS_ETAL, HRDPS_EATM, HRDPS_MSL, HRDPS_DBLY, HRDPS_DBLL, HRDPS_NTAT, NB_HRDPS_CATEGORY };

	class CHRDPSVariables : public std::bitset<NB_HRDPS_VARIABLES>
	{
	public:

		static const char* GetName(size_t i) { ASSERT(i < NB_HRDPS_VARIABLES); return NAME[i]; }
		static const char* GetCategory(size_t i) { ASSERT(i < NB_HRDPS_CATEGORY); return CATEGORY[i]; }

		static const std::string& GetDescription(size_t var)
		{
			ASSERT(var < NB_HRDPS_VARIABLES); LoadDescription();
			size_t c = GetCat(var);
			size_t v = GetVarPos(var);
			return DESCRIPTION[c][v];
		}

		CHRDPSVariables(std::string in="")
		{
			operator = (in);
		}

		using std::bitset<NB_HRDPS_VARIABLES>::operator =;
		CHRDPSVariables& operator = (std::string in);


		static size_t GetVariable(const std::string& name);
		static size_t GetCategory(const std::string& name);
		static size_t GetLevel(const std::string& name);

		static std::string GetHRDPSSelectionString()
		{
			std::string str;
			for (size_t c = 0; c < NB_HRDPS_CATEGORY; c++)
				str += CHRDPSVariables::GetHRDPSSelectionString(c) + "|"; 
			
			return str;
		}
		
		static std::string GetHRDPSSelectionString(size_t c);
		static bool Is(size_t var, THRDPSCategory c);
		static size_t GetNbVar(size_t c) { return CAT_RANGE[c][1] - CAT_RANGE[c][0]; }
		static size_t GetCat(size_t var);
		static size_t GetVarPos(size_t var);

	protected:

		static void LoadDescription();
		static const char* NAME[NB_HRDPS_VARIABLES];
		static const char* CATEGORY[NB_HRDPS_CATEGORY];
		static StringVector DESCRIPTION[NB_HRDPS_CATEGORY];
		static const size_t CAT_RANGE[NB_HRDPS_CATEGORY][2];
	};

	class CHRDPSLevels : public std::set<size_t>
	{
	public:

		CHRDPSLevels(std::string str = "");
		void FromString(std::string str);
		std::string ToString()const;
	};

	typedef CHRDPSLevels CHRDPSHeight;

	//**************************************************************

	class CPrcpHourToUpdate
	{
	public:


		CPrcpHourToUpdate()
		{
			m_nb_hours = 0;
			m_last_hour = 0;
		}

		CTRef m_date;
		size_t m_nb_hours;
		size_t m_last_hour;

		std::string m_HRDPA_file_path;
		std::string m_HRDPS_file_path_last;
		std::string m_HRDPS_file_path1;
		std::string m_HRDPS_file_path2;
		std::string m_out_file_path;

	};

	class CHRDPS
	{
	public:

		
		CHRDPS(const std::string& workingDir="");
		virtual ~CHRDPS(void);

		bool m_bCreateDailyGeotiff;
		//bool m_createHistiricalGeotiff;
		size_t m_update_last_n_days;
		CHRDPSVariables m_variables;
		CHRDPSHeight m_heights;
		CHRDPSLevels m_levels;

		int m_max_hours;
		bool m_bForecast;
		bool m_bHRDPA6h;
		bool m_bRadiation;

		std::string GetWorkingDir()const { return m_workingDir; }
		void SetWorkingDir(const std::string& in) { m_workingDir = in; }


		ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetStationList(CLocationVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetVirtuelStation(const CLocationVector& stations, CWVariables variables, CTPeriod p, CWeatherStation& station, CCallback& callback = DEFAULT_CALLBACK);
		std::string GetGeotiffFilePath(std::string outputFilePath) { return GetGeotiffFilePath(m_workingDir, outputFilePath); }
		
		std::set<std::string> Getlast_n_days(size_t nb_days, CCallback& callback);
		ERMsg CreateHourlyGeotiff(std::set<std::string> outputPath, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg CreateDailyGeotiff(std::set<std::string> outputPath, CCallback& callback)const;
		ERMsg GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg GetPrcpHourToUpdate(std::set<std::string> date_to_update, std::vector < CPrcpHourToUpdate>& hour_to_update, CCallback& callback)const;
		ERMsg CreateHourlyPrcp(std::set<std::string> outputPath, CCallback& callback = DEFAULT_CALLBACK)const;
		ERMsg CreateHourlySRad(std::set<std::string> outputPath, CCallback& callback)const;
		
		static CTRef GetTRef(std::string title, bool bAddForecast);
		static ERMsg Clean(int delete_after, std::string workingDir, CCallback& callback);
		static std::string GetGeotiffFilePath(std::string workingDir, std::string outputFilePath);

		
	protected:

		std::string m_workingDir;

		bool NeedDownload(const std::string& filePath)const { return !GoodGrib(filePath); }
		std::string GetOutputFilePath(const std::string& filetitle)const;
		std::string GetOutputFilePath(CTRef TRef, size_t HH)const;
		std::string GetRemoteFilePath(size_t HH, size_t hhh, const std::string& filetitle)const;

		size_t GetHH(const std::string& title)const;
		size_t Gethhh(std::string title)const;
		ERMsg GetLatestHH(size_t& HH, CCallback& callback)const;

		ERMsg OpenDatasets(CCallback& callback);

		static size_t GetHRDPSVariable(std::string fileTitle);
		static size_t GetLevel(std::string fileName);


		static const char* SERVER_NAME;
		static const char* SERVER_PATH;

		
		
	};

}