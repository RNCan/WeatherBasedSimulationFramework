//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include <vector>
#include <array>
#include <crtdbg.h> 
#include <bitset>
//#include <boost\array.hpp>
#include <boost\serialization\array.hpp>
#include <boost\serialization\bitset.hpp>
#include "basic/ERMsg.h"
#include "Basic/zenXml.h"
#include "Basic/UtilTime.h"
#include "Basic/Statistic.h"


namespace WBSF
{

	class CWVariables;
	class CTemporal;

	//**********************************************************************
	//Temporal
	enum TTemporal{ T_YEAR, T_MONTH, T_DAY, T_HOUR, T_JDAY, T_REFERENCE, NB_TEMPORAL };//
	const char* GetTemporalName(size_t v, bool bUpperCase = false);
	size_t GetTemporalFromName(const std::string& name);
	CTM GetTemporal(const StringVector& columnsHeader, const char* separator = " ,;\t|");
	//**********************************************************************
	//WEATHER

	namespace WEATHER
	{
		enum TMissing{ MISSING = -999 };
		inline bool IsMissing(double v){ return v <= MISSING; }
		inline bool HaveValue(double v){ return v > MISSING; }

		enum TMerge { MERGE_FROM_DB1, MERGE_FROM_MEAN, MERGE_FROM_DB2, NB_MERGE_TYPE };
		enum TPriorityRule { LARGEST_DATA, GREATEST_YEARS, OLDEST_YEAR, NEWEST_YEAR, NB_PRIORITY_RULE };
		enum TSourcePeriod{ NO_MATTER = -1, MIDNIGHT_MIDNIGHT, NOON_NOON, P06_06, P18_18, P22_22 };

		extern const double ELEV_FACTOR;
		extern const double SHORE_DISTANCE_FACTOR;

	}

	enum { INVLID_YEAR = YEAR_NOT_INIT };

	//**********************************************************************
	//Hourly

	namespace HOURLY_DATA
	{
		static const size_t SKIP = size_t(-1);

		enum TVarH
		{
			H_SKIP = -1,
			H_FIRST_VAR = 0,
			H_TMIN2 = H_FIRST_VAR,	//minimum daily temperature [°C]
			H_TAIR2 ,				//air temperature [°C]
			H_TMAX2,				//maximum daily temperature [°C]
			H_PRCP,					//precipitation accumulation [mm] 
			H_TDEW,					//Dew point temperature [°C]
			H_SPEH = H_TDEW,		//specific humidity [kg/kg]
			H_RELH,					//relatice humidity [%]
			H_WNDS,					//wind speed at 10 meters [km/h]
			H_WNDD,					//wind direction [° north]
			H_SRAD2,				//mean solar radiation [watt/m²]
			H_PRES,					//atmospheric pressure [hPa] 
			H_SNOW,					//snowfall (equivalent in water) [mm]
			H_SNDH,					//snow depth [cm]
			H_SWE,					//snow water equivalent [mm]
			H_WND2,					//wind speed at 2 meters [km/h]
			H_ADD1,					//extra variables [unknown]
			H_ADD2,					//extra variables [unknown]
			NB_VAR_H, NB_ADD = NB_VAR_H - H_ADD1
		};

		enum TVarEx
		{
			H_FIRST_VAR_EX = NB_VAR_H,
			H_KELV = NB_VAR_H,	//temperature in kelvin [K]
			H_PSYC,				//Psychrometric Constant [kPa °C-1]
			H_SSVP,				//slope of the saturation vapor pressure-temperature curve [kPa °C-1]
			H_LHVW,				//latent heat of vaporization of water [MJ kg-1]
			H_FNCD,				//cloudines function [dimensionless]
			H_CSRA,				//clear-sky radiation [MJ m-2 d-1] or [MJ m-2 hr-1]
			H_EXRA,				//extraterrestrial radiation [MJ m-2 d-1] or [MJ m-2 hr-1]
			H_SWRA,				//net short-wave radiation [MJ m-2 d-1] or [MJ m-2 hr-1]
			H_ES2,				//saturated vapor pressure from Tdew [Pa]
			H_EA2,				//actual vapor pressure from Tair [Pa]
			H_VPD2,				//vapor pressure deficit [Pa]
			H_TNTX,				//daily mean temperature [°C] from daily Tmin and Tmax
			H_TRNG2,			//daily diurnal temperature range [°C] from daily Tmin and Tmax
			H_SRMJ,				//mean solar radiation  [MJ m-2 d-1] or [MJ m-2 hr-1]
			NB_VAR_ALL,
			NB_VAR_EX = NB_VAR_ALL - H_FIRST_VAR_EX
		};

		inline TVarH& operator++(TVarH& v){ return (v = static_cast<TVarH>(static_cast<int>(v)+1)); }
		inline TVarH& operator++(TVarH& v, int){ return (v = static_cast<TVarH>(static_cast<int>(v)+1)); }
		inline TVarEx& operator++(TVarEx& v){ return (v = static_cast<TVarEx>(static_cast<int>(v)+1)); }
		inline TVarEx& operator++(TVarEx& v, int){ return (v = static_cast<TVarEx>(static_cast<int>(v)+1)); }

		inline bool IsVariable(size_t v){ ASSERT(v == H_SKIP || v < NB_VAR_H); return v < NB_VAR_H; }
		inline bool IsUnknown(size_t v){ return v == H_SKIP; }

		double GetLimitH(size_t v, short kind);

		const char* GetVariableName(size_t v, bool bUpperCase = false);
		const char* GetVariableAbvr(size_t v);

		const char* GetVariableTitle(size_t v);
		const char* GetVariableUnits(size_t v);
		void ReloadString(size_t v);

		TVarH GetVariableFromName(const std::string& name, bool bLookAbrv = false);
		CWVariables GetVariables(const std::string& header, const char* separator = " ,;\t", bool bLookAbrv = false);
	}

	//**********************************************************************
	//NORMAL_DATA

	namespace NORMALS_DATA
	{
		//TDEW_MN, TDEW_SD, 
		enum TField{ TMIN_MN, TMAX_MN, TMNMX_R, DEL_STD, TMIN_SD = DEL_STD, EPS_STD, TMAX_SD = EPS_STD, TACF_A1, TACF_A2, TACF_B1, TACF_B2, PRCP_TT, PRCP_SD, TDEW_MN, SPEH_MN = TDEW_MN, RELH_MN, RELH_SD, WNDS_MN, WNDS_SD, NB_FIELDS };
		enum { RECORD_LENGTH = 8, LINE_LENGTH1 = NB_FIELDS*RECORD_LENGTH, LINE_LENGTH2 = LINE_LENGTH1 + 2 };
		inline int GetNormalDataPrecision(size_t f){ return (f >= TMNMX_R&& f <= TACF_B2) ? 4 : (f == PRCP_SD || f == RELH_SD || f == WNDS_MN || f == WNDS_SD) ? 3 : 1; }
		float GetLimitN(size_t v, short kind);
		std::string GetLineFormat();
		std::string GetRecordFormat(size_t f);
		std::string GetEmptyRecord();

		const char* GetFieldTitle(size_t f);
		const char* GetFieldHeader(size_t f);

		size_t F2V(size_t f);
		size_t V2F(size_t v);
	}


	//**********************************************************************
	//GRADIENT

	namespace GRADIENT
	{

		enum TGradient{ TMIN_GR, TMAX_GR, PRCP_GR, TDEW_GR, NB_GRADIENT };
		enum TSpace{ X_GR, Y_GR, Z_GR, NB_SPACE, D_SHORE = NB_SPACE, S_GR = D_SHORE, NB_SPACE_EX };


		size_t G2F(size_t g);
		size_t F2G(size_t f);
		size_t G2V(size_t g);
		size_t V2G(size_t v);
	}



	class CTemporal : public std::bitset < NB_TEMPORAL >
	{
	public:

		CTemporal(CTM TM)
		{
			SetTM(TM);
		}

		CTemporal(const std::string& header, const char* separator = " ,;|\t")
		{
			Set(header, separator);
		}

		void Set(const std::string& header, const char* separator = " ,;|\t");
		void Set(const StringVector& columnsHeader);

		void SetTM(CTM TM);
		CTM GetTM()const;

		std::string GetHeader(const char* separator)const;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<std::bitset < NB_TEMPORAL >>(*this);
		}
	};

	class CWVariables : public std::bitset < HOURLY_DATA::NB_VAR_H >
	{
	public:

		CWVariables(HOURLY_DATA::TVarH v);
		CWVariables(const char* str = NULL);
		CWVariables(const std::string& str);

		using std::bitset<HOURLY_DATA::NB_VAR_H>::operator=;
		CWVariables& operator=(HOURLY_DATA::TVarH v);
		CWVariables& operator=(const char* str);
		CWVariables& operator=(const std::string& str){ return operator=(str.c_str()); }
		std::string to_string(bool bAbvr = true, const char sep = ' ')const;
		std::string GetVariablesName(char sep);

		bool operator==(const CWVariables& in)const{ return std::bitset<HOURLY_DATA::NB_VAR_H>::operator==(in); }
		bool operator!=(const CWVariables& in)const{ return std::bitset<HOURLY_DATA::NB_VAR_H>::operator!=(in); }


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<std::bitset < HOURLY_DATA::NB_VAR_H >>(*this);
			ar & m_mode;
		}


		std::string GetHeader(CTM TM, const char* separator = ",")const;
		short m_mode;//or/and mode

		std::string GetSelection()const{ return std::bitset<HOURLY_DATA::NB_VAR_H>::to_string(); }
		void SetSelection(const std::string& in){ std::bitset<HOURLY_DATA::NB_VAR_H>::operator = (std::bitset<HOURLY_DATA::NB_VAR_H>(in)); }

	};


	
	class CWAllVariables : public CWVariables
	{
	public:
		CWAllVariables()
		{
			set();
		}

	};

	typedef CWVariables CWFilter;

	class CCountPeriod : public std::pair < size_t, CTPeriod >
	{
	public:
		friend class boost::serialization::access;

		CCountPeriod(size_t count = 0, const CTPeriod& p = CTPeriod())
		{
			first = count;
			second = p;
		}

		CCountPeriod& operator+=(const CCountPeriod& in)
		{
			first += in.first;
			second += in.second;
			return *this;
		}

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar &  boost::serialization::base_object<std::pair < size_t, CTPeriod >>(*this);
		}
	};

	typedef std::array<CCountPeriod, HOURLY_DATA::NB_VAR_H> CWVariablesCounterBase;
	class CWVariablesCounter : public CWVariablesCounterBase
	{
	public:

		friend class boost::serialization::access;

		CWVariablesCounter()
		{
			fill(CCountPeriod());
		}

		CWVariablesCounter(const CWVariablesCounter& in){ operator=(in); }
		CWVariablesCounter& operator=(const CWVariablesCounter& in)
		{
			if (&in != this)
				CWVariablesCounterBase::operator=(in);

			return *this;
		}

		CWVariablesCounter& operator+=(const CWVariablesCounter& in)
		{
			for (size_t i = 0; i < size(); i++)
				at(i) += in.at(i);

			return *this;
		}

		CWVariables GetVariables()const;

		size_t GetSum()const
		{
			size_t sum = 0;
			for (size_t i = 0; i < size(); i++)
				sum += at(i).first;

			return sum;
		}
		CTPeriod GetTPeriod()const
		{
			CTPeriod p;
			for (size_t i = 0; i < size(); i++)
				p += at(i).second;

			return p;
		}

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar &  boost::serialization::base_object<CWVariablesCounterBase>(*this);
		}

	};

	class CWVarDef
	{
	public:

		CWVarDef(HOURLY_DATA::TVarH v = HOURLY_DATA::H_SKIP, size_t s = WBSF::MEAN)
		{
			m_var = v;
			m_stat = s;
		}

		HOURLY_DATA::TVarH m_var;  //variable type
		size_t m_stat; //statistic type

		bool operator==(const CWVarDef& in)const{ return m_var == in.m_var && m_stat == in.m_stat; }
		bool operator!=(const CWVarDef& in)const{ return !operator ==(in); }


	};

	typedef std::vector<CWVarDef> CWeatherFormatBase;
	class CWeatherFormat : public CWeatherFormatBase
	{
	public:

		CWeatherFormat(const char* header = "", const char* separator = ",;\t|", double nodata = WEATHER::MISSING)
		{
			Set(header, separator, nodata);
		}

		CWeatherFormat(CTM TM, CWVariables variables)
		{
			Set(TM, variables);
		}

		void clear();
		ERMsg Set(const char* header, const char* separator = ",;\t|", double nodata = WEATHER::MISSING);
		ERMsg Set(const StringVector& fields, double nodata);
		void Set(CTM TM, CWVariables variables);

		CWVariables GetVariables()const;

		CTM GetTM()const{ return m_TM; }
		CTRef GetTRef(const StringVector& str)const;

		StringVector GetHeaderVector()const{ return StringVector(m_header, ","); }

		const std::string& GetHeader()const{ return m_header; }
		const std::string& GetUnknownFields()const{ return m_unknownFields; }

		double GetNoData()const{ return m_nodata; }
		void SetNoData(double in){ m_nodata = in; }


		size_t GetStatFromName(const std::string& name);

		std::string Format();

	protected:


		std::string m_header;
		std::string m_unknownFields;

		CTM m_TM;
		size_t m_yearPos;
		size_t m_monthPos;
		size_t m_dayPos;
		size_t m_hourPos;
		size_t m_jdPos;
		double m_nodata;
	};


	
	typedef std::array<std::array< std::array< double, 24>, 31>, 12> CAnnualWeight;
	typedef std::map < int, CAnnualWeight > MultiYearsMap;
	class CMultiStationMultiYearWeight : public MultiYearsMap
	{
	public:

		double & operator[](const CTRef& t){ return  MultiYearsMap::operator[](t.GetYear())[t.GetMonth()][t.GetDay()][t.GetHour()]; }
		const double & operator[](const CTRef& t)const{ return at(t.GetYear())[t.GetMonth()][t.GetDay()][t.GetHour()]; }
	};

	typedef std::vector<CMultiStationMultiYearWeight> CMultiStationWeight;
	typedef std::array< CMultiStationWeight, HOURLY_DATA::NB_VAR_H> CWeightVector;

}//namespace WBSF


namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CWVariables& in, XmlElement& output)
	{
		output.setValue(in.to_string());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CWVariables& out)
	{
		std::string str;
		if (input.getValue(str))
			out = str;

		return true;
	}
}
