#pragma once

#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/CSV.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{

	//**************************************************************

	class CSnoTelFile
	{
	public:

		enum TSnoTelVariable
		{
			V_SITE_ID, V_DATE, V_TIME,
			V_TAVG, V_TMAX, V_TMIN, V_TOBS, V_PRES, V_BATT, V_BATV, V_BATX, V_BATN, V_ETIB, V_COND, V_DPTP, V_DIAG, V_SRDOX, V_DISO, V_DISP, V_DIVD,
			V_DIV, V_HFTV, V_EVAP, V_FUEL, V_FMTMP, V_VOLT, V_TGSV, V_TGSX, V_TGSN, V_TGSI, V_JDAY, V_MXPN, V_MNPN, V_NTRDV, V_NTRDX, V_NTRDN, V_NTRDC,
			V_H2OPH, V_PARV, V_PART, V_PREC, V_PRCP, V_PRCPSA, V_ETIL, V_RDC, V_RHUM, V_RHUMV, V_RHENC, V_RHUMX, V_RHUMN, V_REST, V_RESC, V_SRDOO,
			V_RVST, V_SAL, V_SNWD, V_SNWDV, V_SNWDX, V_SNWDN, V_SNOW, V_WTEQ, V_WTEQV, V_WTEQX, V_WTEQN, V_SMOV, V_SMOC, V_SMOX, V_SMON, V_SMS, V_SMV,
			V_SMX, V_SMN, V_STV, V_STX, V_STN, V_STO, V_SRAD, V_SRADV, V_SRADX, V_SRADN, V_SRADT, V_LRAD, V_LRADX, V_LRADT, V_SRMV, V_SRMX, V_SRMN,
			V_SRMO, V_SRVO, V_SRVOX, V_SRVOO, V_OI, V_CDD, V_GDD, V_HDD, V_TURB, V_RESA, V_PVPV, V_SVPV, V_WLEVV, V_WLEVX, V_WLEVN, V_WLEV, V_WTEMP,
			V_WTAVG, V_WTMAX, V_WTMIN, V_WELL, V_WDIRV, V_WDIR, V_WDIRZ, V_WDMVV, V_WDMVX, V_WDMVN, V_WDMV, V_WDMVT, V_WSPDV, V_WSPDX, V_WSPDN, V_WSPD,
			NB_SNOTEL_VARIABLES
		};

		static const char* SNOTEL_VAR_NAME[NB_SNOTEL_VARIABLES];

		static size_t GetVariable(const std::string& strType);
		static std::vector<size_t> GetMembers(StringVector header);
		static std::map<size_t, size_t> GetVariablesPos(std::vector<size_t> members);

		CSnoTelFile()
		{

		}

		ERMsg open(const std::string& filePath);



		void operator++()
		{
			++(*m_loop);
			while ((*m_loop) != CSVIterator() && (*m_loop)->empty())//skip empty line
				++(*m_loop);
		}// Pre Increment

		void operator++(int)
		{
			(*m_loop)++;
			while ((*m_loop) != CSVIterator() && (*m_loop)->empty())//skip empty line
				(*m_loop)++;
		}// Post increment

		bool IsMissing(size_t v)const
		{
			return m_pos.find(v) != m_pos.end();
		}

		std::string at(size_t v)const
		{
			std::string str;
			std::map<size_t, size_t> ::const_iterator it = m_pos.find(v);
			if (it != m_pos.end())
			{
				assert(it->second < (*(*m_loop)).size());
				str = (*(*m_loop))[it->second];
			}

			return str;
		}

		CTRef GetTRef()const;


		double operator[](size_t v)const
		{
			double value = WEATHER::MISSING;
			std::string str = at(v);
			if (!str.empty() && str != "-99.9")
				value = ToDouble(str);

			return value;
		}


		void close()
		{
			m_file.close();
		}

		bool end()const{ return (*m_loop) == CSVIterator(); }

		bool IsHourly()const{ return m_bHourlyData; }

	protected:

		std::unique_ptr<CSVIterator> m_loop;
		ifStream m_file;
		std::vector<size_t> m_members;
		std::map<size_t, size_t> m_pos;
		bool m_bHourlyData;

	};


	//**************************************************************
	class CUISnoTel : public CTaskBase
	{
	public:

		enum TTemporalType { T_HOURLY, T_DAILY, T_MONTHLY, NB_TEMPORAL_TYPE };
		enum TSnoTelVariable
		{
			V_SITE_ID, V_DATE, V_TIME,
			V_TAVG, V_TMAX, V_TMIN, V_TOBS, V_PRES, V_BATT, V_BATV, V_BATX, V_BATN, V_ETIB, V_COND, V_DPTP, V_DIAG, V_SRDOX, V_DISO, V_DISP, V_DIVD,
			V_DIV, V_HFTV, V_EVAP, V_FUEL, V_FMTMP, V_VOLT, V_TGSV, V_TGSX, V_TGSN, V_TGSI, V_JDAY, V_MXPN, V_MNPN, V_NTRDV, V_NTRDX, V_NTRDN, V_NTRDC,
			V_H2OPH, V_PARV, V_PART, V_PREC, V_PRCP, V_PRCPSA, V_ETIL, V_RDC, V_RHUM, V_RHUMV, V_RHENC, V_RHUMX, V_RHUMN, V_REST, V_RESC, V_SRDOO,
			V_RVST, V_SAL, V_SNWD, V_SNWDV, V_SNWDX, V_SNWDN, V_SNOW, V_WTEQ, V_WTEQV, V_WTEQX, V_WTEQN, V_SMOV, V_SMOC, V_SMOX, V_SMON, V_SMS, V_SMV,
			V_SMX, V_SMN, V_STV, V_STX, V_STN, V_STO, V_SRAD, V_SRADV, V_SRADX, V_SRADN, V_SRADT, V_LRAD, V_LRADX, V_LRADT, V_SRMV, V_SRMX, V_SRMN,
			V_SRMO, V_SRVO, V_SRVOX, V_SRVOO, V_OI, V_CDD, V_GDD, V_HDD, V_TURB, V_RESA, V_PVPV, V_SVPV, V_WLEVV, V_WLEVX, V_WLEVN, V_WLEV, V_WTEMP,
			V_WTAVG, V_WTMAX, V_WTMIN, V_WELL, V_WDIRV, V_WDIR, V_WDIRZ, V_WDMVV, V_WDMVX, V_WDMVN, V_WDMV, V_WDMVT, V_WSPDV, V_WSPDX, V_WSPDN, V_WSPD,
			NB_SNOTEL_VARIABLES
		};

		enum TAttributes { WORKING_DIR, TEMPORAL_TYPE, STATES, FIRST_YEAR, LAST_YEAR, UPDATE_STATIONLIST, NB_ATTRIBUTES };
		static const char* SNOTEL_VAR_NAME[NB_SNOTEL_VARIABLES];
		static const char* TEMPORAL_TYPE_NAME[NB_TEMPORAL_TYPE];
		static size_t GetVariable(const std::string& strType);
		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CUISnoTel); }

		CUISnoTel(void);
		virtual ~CUISnoTel(void);

		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const; 
		virtual UINT GetTitleStringID()const{return ATTRIBUTE_TITLE_ID;}

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetStationList(StringVector& stationList, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual std::string Option(size_t i)const;
		virtual std::string Default(size_t i)const;

	protected:

		std::string GetOutputFilePath(int year, size_t type, const std::string& ID)const;

		std::string GetStationListFilePath()const{ return GetDir(WORKING_DIR) + "StationsList.csv"; }
		ERMsg UpdateStationInformation(CCallback& callback)const;
		ERMsg ReadData(const std::string& filePath, CYear& dailyData, CCallback& callback)const;

		//CStateSelection m_states;
		//bool m_bUpdateStationList;
		//size_t m_type;

		CLocationVector m_stations;

		static CTPeriod GetStationPeriod(const CLocation& location);
		static std::vector<size_t> CUISnoTel::GetMembers(StringVector header);
		static std::map<size_t, size_t> GetVariablesPos(std::vector<size_t> members);

		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const UINT ATTRIBUTE_TITLE_ID;
		static const char* SERVER_NAME;

	};

}