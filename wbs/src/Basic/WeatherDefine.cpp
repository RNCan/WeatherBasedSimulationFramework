//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 28-11-2018	Rémi Saint-Amant	Add of Normals Categories
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include "stdafx.h"

#include "Basic/WeatherDefine.h"
#include "Basic/UtilStd.h"

#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{
	static const char* TEMPORAL_NAME[NB_TEMPORAL] = { "Year", "Month", "Day", "Hour", "Jday", "No" };
	static const char* TEMPORAL_NAME_UE[NB_TEMPORAL] = { "YEAR", "MONTH", "DAY", "HOUR", "JDAY", "NO" };
	static const char* TEMPORAL_NAME_UF[NB_TEMPORAL] = { "ANNEE", "MOIS", "JOUR", "HEURE", "JJ", "NO" };


	const char* GetTemporalName(size_t t, bool bUpperCase)
	{
		return bUpperCase ? TEMPORAL_NAME_UE[t] : TEMPORAL_NAME[t];
	}

	size_t GetTemporalFromName(const string& name)
	{

		size_t t = UNKNOWN_POS;

		string varName(name);
		MakeUpper(Trim(varName));
		if (varName == "DOY")
			return T_JDAY;

		for (size_t v = 0; v < NB_TEMPORAL &&t == UNKNOWN_POS; v++)
			if (varName == TEMPORAL_NAME_UE[v] || varName == TEMPORAL_NAME_UF[v])
				t = v;

		return t;
	}

	void CTemporal::SetTM(CTM TM)
	{
		CTemporal& me = *this;
		me[T_HOUR] = TM.Type() >= CTM::HOURLY;
		me[T_DAY] = TM.Type() >= CTM::DAILY;
		me[T_MONTH] = TM.Type() >= CTM::MONTHLY;
		me[T_YEAR] = TM.Type() >= CTM::ANNUAL;
	}

	CTM CTemporal::GetTM()const
	{
		const CTemporal& me = *this;
		short type = CTM::UNKNOWN;
		short mode = me[T_YEAR] ? CTM::FOR_EACH_YEAR : CTM::OVERALL_YEARS;

		if (me[T_HOUR] && me[T_DAY] && me[T_MONTH])
			type = CTM::HOURLY;
		else if (me[T_HOUR] && me[T_JDAY])
			type = CTM::HOURLY;
		else if (me[T_DAY] && me[T_MONTH])
			type = CTM::DAILY;
		else if (me[T_JDAY])
			type = CTM::DAILY;
		else if (me[T_MONTH])
			type = CTM::MONTHLY;
		else if (me[T_YEAR])
			type = CTM::ANNUAL;
		else if (me[T_REFERENCE])
			type = CTM::ATEMPORAL;

		return CTM(type, mode);
	}

	void CTemporal::Set(const string& header, const char* separator)
	{
		StringVector columnsHeader = Tokenize(header, separator, true);
		return Set(columnsHeader);
	}

	void CTemporal::Set(const StringVector& columnsHeader)
	{
		for (size_t i = 0; i < columnsHeader.size(); i++)
		{
			size_t t = GetTemporalFromName(columnsHeader[i]);
			if (t != UNKNOWN_POS)
				set(t);
		}
	}

	std::string CTemporal::GetHeader(const char* separator)const
	{
		const CTemporal& me = *this;

		std::string format;

		//begin with time reference
		for (size_t t = T_YEAR; t < NB_TEMPORAL; t++)
		{
			if (me[t])
			{
				if (!format.empty())
					format += separator;

				format += GetTemporalName(t);
			}
		}

		return format;
	}


	namespace WEATHER
	{
		static const double ELEV_FACTOR = 100.0;
		static const double SHORE_DISTANCE_FACTOR = 1.0;
	}

	namespace NORMALS_DATA
	{

		static const char* FIELDS_TITLES[NB_FIELDS] = { "Tmin Mean", "Tmax Mean", "Tmin/Tmax Corr", "Tmin Std", "Tmax Std", "A1", "A2", "B1", "B2", "Prcp Tot", "Prcp Std", "TDew Mean", "Rel Hum Mean", "Rel Hum Std", "Wnd Spd Mean", "Wnd Spd Std" };
		static const char* FIELDS_HEADERS[NB_FIELDS] = { "TMIN_MN", "TMAX_MN", "TMNMX_R", "DEL_STD", "EPS_STD", "TACF_A1", "TACF_A2", "TACF_B1", "TACF_B2", "PRCP_TT", "PRCP_SD", "TDEW_MN", "RELH_MN", "RELH_SD", "WNDS_MN", "WNDS_SD" };

		//Wind speed in the normal is the log of the wind speed
		static const float LIMIT_N[NB_FIELDS][2] = { { -99, 99 }, { -99, 99 }, { -99, 99 }, { 0, 99 }, { 0, 99 }, { -99, 99 }, { -99, 99 }, { -99, 99 }, { -99, 99 }, { 0, 3500 }, { 0, 99 }, { -99, 99 }, { 0, 100 }, { 0, 999 }, { -30, 6 }, { 0, 99 } };
		float GetLimitN(size_t v, short kind)
		{
			_ASSERTE(NB_FIELDS == 16);
			_ASSERTE(v >= 0 && v < NB_FIELDS);
			_ASSERTE(kind >= 0 && kind <= 1);//0=min, 1=max

			return LIMIT_N[v][kind];
		}


		std::string GetLineFormat()
		{
			string str;
			str.resize(100);
			_snprintf(&(str[0]), 100, "%%-%d.%ds\n", LINE_LENGTH1, LINE_LENGTH1);
			str.resize(strlen(str.c_str()));
			return str;
		}

		std::string GetRecordFormat(size_t i)
		{
			string str;
			str.resize(100);
			_snprintf(&(str[0]), 100, " %%%d.%df", RECORD_LENGTH - 1, GetNormalDataPrecision(i));
			str.resize(strlen(str.c_str()));
			return str;
		}

		std::string GetEmptyRecord()
		{
			string tmp;
			tmp.insert(tmp.begin(), RECORD_LENGTH, ' ');
			//for(int i=0; i<NORMAL_DATA::RECORD_LENGTH; i++) 

			return tmp;
		}


		const char* GetFieldTitle(size_t f)
		{
			_ASSERTE(f < NB_FIELDS);
			return FIELDS_TITLES[f];
		}

		const char* GetFieldHeader(size_t f)
		{
			_ASSERTE(f < NB_FIELDS);
			return FIELDS_HEADERS[f];
		}


		size_t F2V(size_t f)
		{
			ASSERT(f < NB_FIELDS);

			size_t v = 0;
			if (f == TMIN_MN )
				v = HOURLY_DATA::H_TMIN;
			else if (f == TMAX_MN)
				v = HOURLY_DATA::H_TMAX;
			else if (f >= TMNMX_R && f <= TACF_B2 )
				v = HOURLY_DATA::H_TAIR;
			else if (f >= PRCP_TT && f <= PRCP_SD)
				v = HOURLY_DATA::H_PRCP;
			else if (f == TDEW_MN)
				v = HOURLY_DATA::H_TDEW;
			else if (f >= RELH_MN && f <= RELH_SD)
				v = HOURLY_DATA::H_RELH;
			else if (f >= WNDS_MN && f <= WNDS_SD)
				v = HOURLY_DATA::H_WNDS;

			return v;
		}


		size_t V2F(size_t v)
		{
			ASSERT(v < HOURLY_DATA::NB_VAR_H);

			size_t f = UNKNOWN_POS;
			switch (v)
			{
			case HOURLY_DATA::H_TMIN: f = TMIN_MN; break;
			case HOURLY_DATA::H_TMAX: f = TMAX_MN; break;
			case HOURLY_DATA::H_PRCP:
			case HOURLY_DATA::H_SNOW:
			case HOURLY_DATA::H_SNDH:
			case HOURLY_DATA::H_SWE:  f = PRCP_TT; break;
			case HOURLY_DATA::H_TDEW: f = TDEW_MN; break;
			case HOURLY_DATA::H_RELH: f = RELH_MN; break;
			case HOURLY_DATA::H_WNDS:
			case HOURLY_DATA::H_WND2: f = WNDS_MN; break;
			case HOURLY_DATA::H_WNDD:
			case HOURLY_DATA::H_TAIR:
			case HOURLY_DATA::H_PRES:
			case HOURLY_DATA::H_SRAD: break;
			default: break;
			}

			return f;
		}


		

		size_t GetCategoryV(size_t v)
		{
			size_t c = UNKNOWN_POS;
			switch (v)
			{
			case HOURLY_DATA::H_TMIN: 
			case HOURLY_DATA::H_TAIR:
			case HOURLY_DATA::H_TMAX: c = C_TEMPERATURE; break;
			case HOURLY_DATA::H_PRCP:
			case HOURLY_DATA::H_SNOW:
			case HOURLY_DATA::H_SNDH:
			case HOURLY_DATA::H_SWE:  c = C_PRECIPITATION; break;
			case HOURLY_DATA::H_TDEW: 
			case HOURLY_DATA::H_RELH: c = C_HUMIDITY; break;
			case HOURLY_DATA::H_WNDS:
			case HOURLY_DATA::H_WND2: c = C_WIND; break;
			case HOURLY_DATA::H_WNDD:
			case HOURLY_DATA::H_PRES:
			case HOURLY_DATA::H_SRAD: break;
			default: ASSERT(false);
			}

			return c;
		}

		size_t GetCategoryN(size_t f)
		{
			size_t c = UNKNOWN_POS;
			if (f >= TMIN_MN && f <= TACF_B2)
				c = C_TEMPERATURE;
			else if (f >= PRCP_TT && f <= PRCP_SD)
				c = C_PRECIPITATION;
			else if (f >= TDEW_MN && f <= RELH_SD)
				c = C_HUMIDITY;
			else if (f >= WNDS_MN && f <= WNDS_SD)
				c = C_WIND;

			return c;
		}
		std::bitset<NB_CATEGORIES> GetCategories(CWVariables variables)
		{
			std::bitset<NB_CATEGORIES> categories;
			for (size_t v = 0; v < HOURLY_DATA::NB_VAR_H; v++)
			{
				//select this category
				if (variables[v])
				{
					size_t c = GetCategoryV(v);
					if (c < categories.size())
						categories.set(c);
				}
			}

			return categories;
		}

		CWVariables GetCategoryVariables(size_t c)
		{
			CWVariables variables;
			switch (c)
			{
			case 0: variables = "TN T TX"; break;
			case 1: variables = "P"; break;
			case 2: variables = "TD H"; break;
			case 3: variables = "WS"; break;
			default: ASSERT(false);
			}

			return variables;
		}
		CWVariables GetCategoryVariables(const std::bitset<NB_CATEGORIES>& categories)
		{
			CWVariables variables;
			for (size_t c = 0; c < categories.size(); c++)
				if (categories[c])
					variables |= GetCategoryVariables(c);

			return variables;
		}

		HOURLY_DATA::TVarH GetCategoryLeadVariable(size_t c)
		{
			HOURLY_DATA::TVarH variable;
			switch (c)
			{
			case C_TEMPERATURE: variable = HOURLY_DATA::H_TAIR; break;
			case C_PRECIPITATION: variable = HOURLY_DATA::H_PRCP; break;
			case C_HUMIDITY: variable = HOURLY_DATA::H_TDEW; break;
			case C_WIND: variable = HOURLY_DATA::H_WNDS; break;
			default: ASSERT(false);
			}

			return variable;
		}
		CWVariables GetCategoryLeadVariables(const std::bitset<NB_CATEGORIES>& categories)
		{
			CWVariables variables;
			for (size_t c = 0; c < categories.size(); c++)
				if (categories[c])
					variables.set( GetCategoryLeadVariable(c) );

			return variables;
		}
		
		
	}

	namespace HOURLY_DATA
	{

		static const char* VARIABLES_UNITS[NB_VAR_H] = { "°C", "°C", "°C", "mm", "°C", "%", "km/h", "°", "W/m²", "hPa", "mm", "cm", "mm", "km/h", "", "" };
		static const char* VARIABLES_NAMES[NB_VAR_H] = { "Tmin", "Tair", "Tmax", "Prcp", "Tdew", "RelH", "WndS", "WndD", "SRad", "Pres", "Snow", "SnDh", "SWE", "Wnd2", "Add1", "Add2" };
		static const char* VARIABLES_NAMES_U[NB_VAR_H] = { "TMIN", "TAIR", "TMAX", "PRCP", "TDEW", "RELH", "WNDS", "WNDD", "SRAD", "PRES", "SNOW", "SNDH", "SWE", "WND2", "ADD1", "ADD2" };
		static const char* VARIABLES_ABVR[NB_VAR_H] = { "TN", "T", "TX", "P", "TD", "H", "WS", "WD", "R", "Z", "S", "SD", "SWE", "WS2", "A1", "A2" };

		//												Tmin          Tair		   Tmax		   prcp	    	Tdew		 Hr			 Ws			 Wd			Rad			pressure	      snow		snow depth		SWE			W2           	add1				add2
		static const double LIMIT_H[NB_VAR_H][2] = { { -60, 60 }, { -60, 60 }, { -60, 60 }, { 0, 3500 }, { -60, 60 }, { 0, 100 }, { 0, 200 }, { 0, 360 }, { 0, 3500 }, { 200, 1090 }, { 0, 3500 }, { 0, 3500 }, { 0, 3500 }, { 0, 200 }, { -99999, 99999 }, { -99999, 99999 } };
		double GetLimitH(size_t v, short kind)
		{
			_ASSERTE(NB_VAR_H == 16);
			_ASSERTE(v >= 0 && v < NB_VAR_H);
			_ASSERTE(kind >= 0 && kind <= 1);//0=min, 1=max

			return LIMIT_H[v][kind];
		}

		const char* GetVariableName(size_t v, bool bUpperCase)
		{
			_ASSERTE(v < NB_VAR_H);
			return bUpperCase ? VARIABLES_NAMES_U[v] : VARIABLES_NAMES[v];
		}
		const char* GetVariableAbvr(size_t v)
		{
			assert(v < NB_VAR_H);
			return VARIABLES_ABVR[v];
		}

		static StringVector VARIABLES_TITLE;
		const char* GetVariableTitle(size_t v)
		{
			assert(v < NB_VAR_H);
			if (VARIABLES_TITLE.empty())
				VARIABLES_TITLE.LoadString(IDS_STR_WEATHER_VARIABLES_TITLE, "|;");

			assert(VARIABLES_TITLE.size() == NB_VAR_H);

			return VARIABLES_TITLE[v].c_str();
		}

		static StringVector VARIABLES_UNITS_STR;
		const char* GetVariableUnits(size_t v)
		{
			assert(v < NB_VAR_H);

			VARIABLES_UNITS_STR.LoadString(IDS_STR_WEATHER_VARIABLES_UNITS, "|;");
			assert(VARIABLES_UNITS_STR.size() == NB_VAR_H);

			return VARIABLES_UNITS_STR[v].c_str();
		}

		void ReloadString(size_t v)
		{
			VARIABLES_TITLE.clear();
			VARIABLES_UNITS_STR.clear();
		}

		
		TVarH GetVariableFromName(const std::string& name, bool bLookAbrv)
		{

			TVarH var = (TVarH)H_SKIP;

			string varName(name);
			//Remove all after '(' 
			string::size_type pos = varName.find('(');
			if (pos != string::npos)
				varName = varName.substr(0, pos);

			MakeUpper(Trim(varName));

			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H&&var == H_SKIP; v++)
				if (varName == VARIABLES_NAMES_U[v] ||
					(bLookAbrv && varName == VARIABLES_ABVR[v]))
					var = v;

			if (var == H_SKIP)
			{
				if (varName == "TMIN" || varName == "TMINIM" )
					var = H_TMIN;
				if( varName == "TMAX" || varName == "TMAXIM")
					var = H_TMAX;
				else if (varName == "PRECIP")
					var = H_PRCP;
				else if (varName == "TDEWPT" || varName == "TDEWMIN" || varName == "TDEWMAX")
					var = H_TDEW;
				else if (varName == "RELHUM" || varName == "RELHMIN" || varName == "RELHMAX") 
					var = H_RELH;
				else if (varName == "WNDSPD")
					var = H_WNDS;
			}

			return var;
		}




		CWVariables GetVariables(const std::string& header, const char* separator, bool bLookAbrv)
		{
			CWVariables variables;
			StringVector fields = Tokenize(header, separator);


			for (size_t i = 0; i < fields.size(); i++)
			{
				//find by variables full names (ie Tair, Prcp ...) or abvr (ie. T P ...)
				//size_t v = GetTemporal(name);
				//if (v==H_SKIP)
				TVarH v = GetVariableFromName(fields[i].c_str(), bLookAbrv);

				if (v != H_SKIP)
					variables[v] = true;
			}

			return variables;
		}






	}

	namespace GRADIENT
	{
		size_t G2F(size_t g)
		{
			_ASSERTE(g < NB_GRADIENT);

			size_t f = 0;
			switch (g)
			{
			case TMIN_GR: f = NORMALS_DATA::TMIN_MN; break;
			//case TAIR_GR: ASSERT(false); break;
			case TMAX_GR: f = NORMALS_DATA::TMAX_MN; break;
			case PRCP_GR: f = NORMALS_DATA::PRCP_TT; break;
			case TDEW_GR: f = NORMALS_DATA::TDEW_MN; break;
			default: _ASSERTE(false);
			}

			return f;
		}
		size_t F2G(size_t f)
		{
			//ASSERT(f == NORMALS_DATA::TMIN_MN || f == NORMALS_DATA::TMAX_MN || f == NORMALS_DATA::PRCP_TT || f == NORMALS_DATA::TDEW_MN);

			size_t g = NOT_INIT;
			switch (f)
			{
			case NORMALS_DATA::TMIN_MN:	g = TMIN_GR;
			case NORMALS_DATA::TMAX_MN:	g = TMAX_GR;
			case NORMALS_DATA::PRCP_TT:	g = PRCP_GR;
			case NORMALS_DATA::TDEW_MN:	g = TDEW_GR;
			default: ASSERT(false);
			}

			return g;
		}

		size_t G2V(size_t g)
		{
			ASSERT(g < NB_GRADIENT);

			short v = 0;
			switch (g)
			{
			case TMIN_GR: v = HOURLY_DATA::H_TMIN; break;
			//case TAIR_GR: v = HOURLY_DATA::H_TAIR; break;
			case TMAX_GR: v = HOURLY_DATA::H_TMAX; break;
			case PRCP_GR: v = HOURLY_DATA::H_PRCP; break;
			case TDEW_GR: v = HOURLY_DATA::H_TDEW; break;
			default: _ASSERTE(false);
			}

			return v;
		}

		size_t V2G(size_t v)
		{
			size_t g = NOT_INIT;
			if (v == HOURLY_DATA::H_TMIN)
				g = TMIN_GR;
			//else if (v == HOURLY_DATA::H_TAIR)
				//g = TAIR_GR;
			else if (v == HOURLY_DATA::H_TMAX)
				g = TMAX_GR;
			else if (v == HOURLY_DATA::H_PRCP)
				g = PRCP_GR;
			else if (v == HOURLY_DATA::H_TDEW)
				g = TDEW_GR;

			return g;
		}
	}

	using namespace WEATHER;
	using namespace NORMALS_DATA;
	using namespace HOURLY_DATA;
	using namespace GRADIENT;


	
	std::string CWVariables::GetHeader(CTM TM, const char* separator)const
	{
		const CWVariables& me = *this;
		std::string format;

		CTemporal temporalRef(TM);
		format = temporalRef.GetHeader(separator);

		//add variables
		for (size_t v = 0; v != NB_VAR_H; v++)
		{
			if (me[v])
			{
				if (!format.empty())
					format += separator;

				//if (TM.Type() == CTM::DAILY && v == H_TAIR && at(H_TRNG))
				//{
				//	//exception for daily minimum and maximum temperature
				//	format += "Tmin";
				//	//format += separator;

				//}
				//else if (TM.Type() == CTM::DAILY && v == H_TRNG && at(H_TAIR))
				//{
				//	//don't add Trng in daily database; use Tmin and Tmax instead
				//	format += "Tmax";
				//}
				//else
				//{
					format += GetVariableName(v);
				//}
			}
		}

		return format;

	}

	std::string CWVariables::to_string(bool bAbvr, const char sep)const
	{
		const CWVariables& me = *this;
		string str;
		for (size_t v = 0; v < size(); v++)
		{
			if (me[v])
			{
				if (!str.empty())
					str += sep;

				str += (bAbvr ? VARIABLES_ABVR[v] : VARIABLES_NAMES[v]);
			}
		}

		return str;
	}

	std::string CWVariables::GetVariablesName(char sep)
	{
		string filterName;
		for (size_t i = 0; i < size(); i++)
		{
			if (test(i))
			{
				if (!filterName.empty())
					filterName += sep;
				filterName += GetVariableName(i);
			}
		}

		if (filterName.empty())
		{
			filterName = "----";
		}
			

		return filterName;
	}

	//*******************************************************
	CWVariables CWVariablesCounter::GetVariables()const
	{
		CWVariables WVars;
		for (size_t i = 0; i < size(); i++)
			if (at(i).first > 0)
				WVars.set(i);

		return WVars;
	}

	CWVariables::CWVariables(HOURLY_DATA::TVarH v)
	{
		set(v);
	}

	CWVariables::CWVariables(const char* str)
	{
		operator=(str);
	}

	CWVariables::CWVariables(const std::string& str)
	{
		operator=(str.c_str());
	}

	CWVariables& CWVariables::operator=(HOURLY_DATA::TVarH v)
	{
		reset();
		set(v);

		return *this;
	}

	CWVariables& CWVariables::operator=(const char* str)
	{
		reset();

		if (str != NULL)
			operator=(GetVariables(str, " ,;\t|", true));

		return *this;
	}



	//**********************************************************
	//CWeatherFormat
	void CWeatherFormat::clear()
	{
		CWeatherFormatBase::clear();
		m_header.clear();
		m_unknownFields.clear();
		//find time reference (hourly or daily)
		m_yearPos = H_SKIP;
		m_monthPos = H_SKIP;
		m_dayPos = H_SKIP;
		m_jdPos = H_SKIP;
		m_hourPos = H_SKIP;
	}


	size_t CWeatherFormat::GetStatFromName(const string& name)
	{
		size_t stat = MEAN;
		if (IsEqualNoCase(name, "Tmin") || IsEqualNoCase(name, "Tmax"))//exception for Tmin and Tmax
			stat = MEAN;
		else if (Find(name, "Minin") || Find(name, "Min") || Find(name, "Lo"))
			stat = LOWEST;
		else if (Find(name, "Maxim") || Find(name, "Max") || Find(name, "Hi"))
			stat = HIGHEST;
		else if (Find(name, "Sum"))
			stat = SUM;
		else if (IsEqualNoCase(name, "Prcp") || IsEqualNoCase(name, "Precip"))
			stat = SUM;
		else if (IsEqualNoCase(name, "SRad") || IsEqualNoCase(name, "SolRad"))
			stat = MEAN;
		else if (IsEqualNoCase(name, "Snow"))
			stat = SUM;


		return stat;
	}


	ERMsg CWeatherFormat::Set(const char* header, const char* separator, double nodata)
	{
		StringVector fields = WBSF::Tokenize(header, separator, true);
		return Set(fields, nodata);
	}

	ERMsg CWeatherFormat::Set(const StringVector& columnsHeader, double nodata)
	{
		ERMsg msg;

		clear();
		m_nodata = nodata;
		if (!columnsHeader.empty())
		{
			int nbUnknownField = 0;
			resize(columnsHeader.size());

			for (size_t i = 0; i < columnsHeader.size(); i++)
			{
				//temporal reference are the same for hourly and daily
				size_t t = GetTemporalFromName(columnsHeader[i]);

				if (t == T_YEAR)
					m_yearPos = i;
				else if (t == T_MONTH)
					m_monthPos = i;
				else if (t == T_DAY)
					m_dayPos = i;
				else if (t == T_HOUR)
					m_hourPos = i;
				else if (t == T_JDAY)
					m_jdPos = i;
				else
				{
					at(i).m_var = GetVariableFromName(columnsHeader[i]);
					at(i).m_stat = GetStatFromName(columnsHeader[i]);

					if (at(i).m_var == H_SKIP)
					{
						if (!m_unknownFields.empty())
							m_unknownFields += ',';

						m_unknownFields += columnsHeader[i];

						if (nbUnknownField < NB_ADD)
						{
							//replace unknown field by add
							at(i).m_var = TVarH(H_ADD1 + nbUnknownField);
						}

						nbUnknownField++;
					}
				}
			}

			if (m_yearPos != UNKNOWN_POS && ((m_monthPos != UNKNOWN_POS && m_dayPos != UNKNOWN_POS) || m_jdPos != UNKNOWN_POS) && m_hourPos != UNKNOWN_POS)
				m_TM = CTM(CTM::HOURLY, CTM::FOR_EACH_YEAR);
			else if (m_yearPos != UNKNOWN_POS && ((m_monthPos != UNKNOWN_POS && m_dayPos != UNKNOWN_POS) || m_jdPos != UNKNOWN_POS))
				m_TM = CTM(CTM::DAILY, CTM::FOR_EACH_YEAR);
			else if (m_yearPos != UNKNOWN_POS && m_monthPos != UNKNOWN_POS)
				m_TM = CTM(CTM::MONTHLY, CTM::FOR_EACH_YEAR);
			else if (m_yearPos != UNKNOWN_POS)
				m_TM = CTM(CTM::ANNUAL, CTM::FOR_EACH_YEAR);
			else if (m_monthPos != UNKNOWN_POS)
				m_TM = CTM(CTM::MONTHLY, CTM::OVERALL_YEARS);
			else msg.ajoute("Error reading header: Missing valid temporal definition");

			if (msg)
			{
				m_header = ToString(columnsHeader, "", ",", "");
			}
		}

		return msg;
	}



	void CWeatherFormat::Set(CTM TM, CWVariables variables)
	{
		clear();

		//set variables
		string tmp = CTemporal(TM).GetHeader(",");
		Set(tmp.c_str());//set temporal SKIP variables
		
		//add real variables
		for (TVarH v = H_FIRST_VAR; v != variables.size(); v++)
		{
			if (variables[v])
			{
				/*if (TM.Type() == CTM::DAILY && v == H_TAIR && variables[H_TRNG])
				{
					//exception for daily minimum and maximum temperature
					push_back(CWVarDef(H_TAIR, WBSF::LOWEST));
				}
				else if (TM.Type() == CTM::DAILY && v == H_TRNG && variables[H_TAIR])
				{
					//don't add Trng in daily database; use Tmin and Tmax instead
					push_back(CWVarDef(H_TAIR, WBSF::HIGHEST));
				}
				else */
				if (v == H_PRCP || v == H_SNOW )//|| v == H_SRAD
				{
					push_back(CWVarDef(v, WBSF::SUM));
				}
				else
				{
					push_back(CWVarDef(v, WBSF::MEAN));
				}

			}
		}

		//overide header
		m_header = variables.GetHeader(TM);

	}

	CWVariables CWeatherFormat::GetVariables()const
	{
		CWVariables variables;

		for (size_t i = 0; i < size(); i++)
			if (at(i).m_var >= 0)
				variables[at(i).m_var] = true;

		return variables;
	}


	CTRef CWeatherFormat::GetTRef(const StringVector& data)const
	{
		ASSERT(m_TM.Type() == CTM::HOURLY || m_TM.Type() == CTM::DAILY);
		ASSERT(data.size() == size());

		CTRef TRef;

		if (m_yearPos >= 0 && m_yearPos < data.size() &&
			m_monthPos >= 0 && m_monthPos < data.size() &&
			m_dayPos >= 0 && m_dayPos < data.size())
		{
			int year = ToInt(data[m_yearPos]);
			size_t m = ToSizeT(data[m_monthPos]) - 1;
			size_t d = ToSizeT(data[m_dayPos]) - 1;
			size_t h = 0;
			if (m_hourPos >= 0 && m_hourPos < data.size())
				h = ToSizeT(data[m_hourPos]);

			if (m < 12 && d < WBSF::GetNbDayPerMonth(year, m) && h <= 24)
				TRef = CTRef(year, m, d, h, m_TM);
		}
		else if (m_yearPos >= 0 && m_yearPos < size() &&
			m_jdPos >= 0 && m_jdPos < size())
		{
			int year = ToInt(data[m_yearPos]);
			size_t jd = std::stoull(data[m_jdPos]) - 1;
			TRef = CJDayRef(year, jd);
		}

		return TRef;
	}
}//namespace WBSF
