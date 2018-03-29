//****************************************************************************
// File:	WGInput.cpp
// Class:	CWGInput
//****************************************************************************
// 15/09/2008	Rémi Saint-Amant	Created from old file
//****************************************************************************

#include "stdafx.h"
#include "Basic/UtilMath.h"
#include "Basic/utilzen.h"
#include "ModelBase/WGInput.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	const short CWGInput::NB_STATION_SEARCH_MAX = 1000;
	const char * CWGInput::FILE_EXT = ".wgs";

	const char* CWGInput::XML_FLAG = "WGInput";
	const char* CWGInput::MEMBERS_NAME[NB_MEMBERS] = { "Variables", "SourceType", "GenerationType", "NbNormalsYears", "FirstYear", "LastYear", "UseForecast", "UseRadarPrcp", "NormalDBName", "NbNormalsStations",
		"DailyDBName", "NbDailyStations", "HourlyDBName", "NbHourlyStations", "GribsDBName", "UseGribs", "AtSurfaceOnly", "Albedo", "Seed", "AllowedDerivedVariables", "Xvalidation", "SkipVerify", "SearchRadius", "NoFillMissing", "UseShore" };

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	CWGInput::CWGInput()
	{
		Reset();
	}

	CWGInput::CWGInput(const CWGInput& TGInput)
	{
		operator = (TGInput);
	}


	void CWGInput::clear()
	{
		m_variables = "TN T TX P";
		m_sourceType = FROM_DISAGGREGATIONS;
		m_generationType = GENERATE_DAILY;
		m_nbNormalsYears = 1;
		m_firstYear = CTRef::GetCurrentTRef().GetYear();
		m_lastYear = CTRef::GetCurrentTRef().GetYear();
		m_bUseForecast = true;
		m_bUseRadarPrcp = false;

		m_normalsDBName.clear();
		m_nbNormalsStations = 4;
		m_dailyDBName.clear();
		m_nbDailyStations = 4;
		m_hourlyDBName.clear();
		m_nbHourlyStations = 4;
		m_gribsDBName.clear();
		m_bUseGribs = false;
		m_bAtSurfaceOnly = true;
		m_albedo = CANOPY;
		m_seed = CRandomGenerator::RANDOM_SEED;
		m_allowedDerivedVariables = "T WD R Z S SD SWE WS2 A1 A2";
		m_bXValidation = false;
		m_bSkipVerify = false;
		m_bNoFillMissing = false;
		m_bUseShore = true;
		m_searchRadius.reset();

		m_filePath.clear();
	}

	CWGInput& CWGInput::operator=(const CWGInput& in)
	{
		if (&in != this)
		{
			m_variables = in.m_variables;
			m_sourceType = in.m_sourceType;
			m_generationType = in.m_generationType;
			m_nbNormalsYears = in.m_nbNormalsYears;
			m_firstYear = in.m_firstYear;
			m_lastYear = in.m_lastYear;
			m_bUseForecast = in.m_bUseForecast;
			m_bUseRadarPrcp = in.m_bUseRadarPrcp;

			m_normalsDBName = in.m_normalsDBName;
			m_nbNormalsStations = in.m_nbNormalsStations;
			m_dailyDBName = in.m_dailyDBName;
			m_nbDailyStations = in.m_nbDailyStations;
			m_hourlyDBName = in.m_hourlyDBName;
			m_nbHourlyStations = in.m_nbHourlyStations;
			m_gribsDBName = in.m_gribsDBName;
			m_bUseGribs = in.m_bUseGribs;
			m_bAtSurfaceOnly = in.m_bAtSurfaceOnly;
			m_albedo = in.m_albedo;
			m_seed = in.m_seed;
			m_allowedDerivedVariables = in.m_allowedDerivedVariables;
			m_bXValidation = in.m_bXValidation;
			m_filePath = in.m_filePath;
			m_bSkipVerify = in.m_bSkipVerify;
			m_bNoFillMissing = in.m_bNoFillMissing;
			m_bUseShore = in.m_bUseShore;
			m_searchRadius = in.m_searchRadius;
		}

		ASSERT(operator==(in));

		return *this;
	}

	string CWGInput::GetMember(size_t i)const
	{
		ASSERT(i < NB_MEMBERS);

		string str;
		switch (i)
		{
		case VARIABLES:			str = m_variables.to_string(); break;
		case SOURCE_TYPE:		str = ToString(m_sourceType); break;
		case GENERATION_TYPE:	str = ToString(m_generationType); break;
		case NB_NORMALS_YEARS:	str = ToString(m_nbNormalsYears); break;
		case FIRST_YEAR:		str = ToString(m_firstYear); break;
		case LAST_YEAR:			str = ToString(m_lastYear); break;
		case USE_FORECAST:		str = ToString(m_bUseForecast); break;
		case USE_RADAR_PRCP:	str = ToString(m_bUseRadarPrcp); break;
		case NORMAL_DB_NAME:	str = m_normalsDBName; break;
		case NB_NORMAL_STATION:	str = ToString(m_nbNormalsStations); break;
		case DAILY_DB_NAME:		str = m_dailyDBName; break;
		case NB_DAILY_STATION:	str = ToString(m_nbDailyStations); break;
		case HOURLY_DB_NAME:	str = m_hourlyDBName; break;
		case NB_HOURLY_STATION:	str = ToString(m_nbHourlyStations); break;
		case GRIBS_DB_NAME:		str = m_gribsDBName; break;
		case USE_GRIBS:			str = ToString(m_bUseGribs); break;
		case ATSURFACE_ONLY:	str = ToString(m_bAtSurfaceOnly); break;
		case ALBEDO:			str = ToString(m_albedo); break;
		case SEED:				str = ToString(m_seed); break;
		case ALLOWED_DERIVED_VARIABLES: str = m_allowedDerivedVariables.to_string(); break;
		case XVALIDATION:		str = ToString(m_bXValidation); break;
		case SKIP_VERIFY:		str = ToString(m_bSkipVerify); break;
		case NO_FILL_MISSING:	str = ToString(m_bNoFillMissing); break;
		case USE_SHORE:			str = ToString(m_bUseShore); break;
		case SEARCH_RADIUS:		str = to_string(m_searchRadius, "," ); break;
		default: ASSERT(false);
		}

		return str;
	}

	bool CWGInput::operator==(const CWGInput& in)const
	{
		bool bEqual = true;

		if (m_variables != in.m_variables)bEqual = false;
		if (m_sourceType != in.m_sourceType)bEqual = false;
		if (m_generationType != in.m_generationType)bEqual = false;
		if (m_nbNormalsYears != in.m_nbNormalsYears)bEqual = false;
		if (m_firstYear != in.m_firstYear)bEqual = false;
		if (m_lastYear != in.m_lastYear)bEqual = false;
		if (m_bUseForecast != in.m_bUseForecast)bEqual = false;
		if (m_bUseRadarPrcp != in.m_bUseRadarPrcp)bEqual = false;

		if (m_normalsDBName != in.m_normalsDBName)bEqual = false;
		if (m_nbNormalsStations != in.m_nbNormalsStations)bEqual = false;
		if (m_dailyDBName != in.m_dailyDBName)bEqual = false;
		if (m_nbDailyStations != in.m_nbDailyStations)bEqual = false;
		if (m_hourlyDBName != in.m_hourlyDBName)bEqual = false;
		if (m_nbHourlyStations != in.m_nbHourlyStations)bEqual = false;
		if (m_gribsDBName != in.m_gribsDBName)bEqual = false;
		if (m_bUseGribs != in.m_bUseGribs)bEqual = false;
		if (m_bAtSurfaceOnly != in.m_bAtSurfaceOnly)bEqual = false;
		if (m_albedo != in.m_albedo)bEqual = false;
		if (m_seed != in.m_seed)bEqual = false;
		if (m_allowedDerivedVariables != in.m_allowedDerivedVariables)bEqual = false;
		if (m_bXValidation != in.m_bXValidation)bEqual = false;
		if (m_bSkipVerify != in.m_bSkipVerify)bEqual = false;
		if (m_bNoFillMissing != in.m_bNoFillMissing)bEqual = false;
		if (m_bUseShore != in.m_bUseShore)bEqual = false;
		if (m_searchRadius != in.m_searchRadius)bEqual = false;

		return bEqual;
	}

	bool CWGInput::operator!=(const CWGInput& TGInput)const
	{
		return !operator==(TGInput);
	}

	void CWGInput::LoadDefaultParameter()
	{
		Reset();


		string filePath = GetUserDataPath() + "Default\\DefaultWG.tgs";
		if (FileExists(filePath))
		{
			Load(filePath);
		}
		else
		{
			m_variables = "TN T TX P";
			m_normalsDBName = "Canada-USA 1981-2010";
			m_allowedDerivedVariables = "T R Z S SD SWE ES EA VPD WS2";
		}

		m_filePath = STRDEFAULT;
	}

	void CWGInput::SetAsDefaultParameter()
	{
		string path = GetUserDataPath() + "Default\\";
		if (CreateMultipleDir(path))
			Save(path + "DefaultWG.tgs");
	}

	ERMsg CWGInput::Load(const string& filePath)
	{
		clear();
		return zen::LoadXML(filePath, GetXMLFlag(), "1", *this);
	}

	ERMsg CWGInput::Save(const string& filePath)
	{
		return zen::SaveXML(filePath, GetXMLFlag(), "1", *this);
	}

	ERMsg CWGInput::IsValid()const
	{
		ERMsg msg;

		if (m_normalsDBName.empty())
			msg.ajoute(GetString(IDS_BSC_NO_NORMALS_DATABASE));


		if (m_sourceType == FROM_OBSERVATIONS)
		{
			if (m_generationType == GENERATE_DAILY)
			{
				if (m_dailyDBName.empty())
					msg.ajoute(GetString(IDS_BSC_NO_DAILY_DATABASE));
			}
			else if (m_generationType == GENERATE_HOURLY)
			{
				if (m_hourlyDBName.empty())
					msg.ajoute(GetString(IDS_BSC_NO_HOURLY_DATABASE));
			}
		}

		return msg;
	}


	CModelOutputVariableDefVector CWGInput::GetOutputDefenition()const
	{
		CModelOutputVariableDefVector variables;

		for (size_t v = 0; v < m_variables.size(); v++)
		{
			if (m_variables[v])
			{
				string name = HOURLY_DATA::GetVariableName(v);
				string title = HOURLY_DATA::GetVariableTitle(v);
				string units = HOURLY_DATA::GetVariableUnits(v);
				variables.push_back(CModelOutputVariableDef(name, title, units, "", CTM(), 1, "", v));
			}
		}

		return variables;
	}

	CTPeriod CWGInput::GetTPeriod()const
	{
		return CTPeriod(CTRef(GetFirstYear(), FIRST_MONTH, FIRST_DAY, FIRST_HOUR, GetTM()), CTRef(GetLastYear(), LAST_MONTH, LAST_DAY, LAST_HOUR, GetTM()));
	}

	//todo ; a éclaircir tout cela....
	CWVariables CWGInput::GetMissingInputVariables()const
	{
		//DerivedVariable
		CWVariables dVariables;
		//if (m_variables[H_TRNG] && m_allowedDerivedVariables[H_TRNG] && !m_variables[H_TAIR])
		//dVariables.set(H_TAIR);

		if ((m_variables[H_TMIN2] && !m_variables[H_TAIR2]) || (m_variables[H_TMAX2] && !m_variables[H_TAIR2]))
			dVariables.set(H_TAIR2);
		if (m_variables[H_TDEW] && m_allowedDerivedVariables[H_TDEW] && !m_variables[H_TAIR2])
			dVariables.set(H_TAIR2);
		if (m_variables[H_TDEW] && m_allowedDerivedVariables[H_TDEW] && !m_variables[H_PRCP])
			dVariables.set(H_PRCP);
		if (m_variables[H_RELH] && m_allowedDerivedVariables[H_RELH] && !m_variables[H_TAIR2])
			dVariables.set(H_TAIR2);
		if (m_variables[H_RELH] && m_allowedDerivedVariables[H_RELH] && !m_variables[H_PRCP])
			dVariables.set(H_PRCP);
		if (m_variables[H_SRAD2] && m_allowedDerivedVariables[H_SRAD2] && !m_variables[H_TAIR2])
			dVariables.set(H_TAIR2);
		if (m_variables[H_SRAD2] && m_allowedDerivedVariables[H_SRAD2] && !m_variables[H_PRCP])
			dVariables.set(H_PRCP);
		if (m_variables[H_SNOW] && m_allowedDerivedVariables[H_SNOW] && !m_variables[H_TAIR2])
			dVariables.set(H_TAIR2);
		if (m_variables[H_SNOW] && m_allowedDerivedVariables[H_SNOW] && !m_variables[H_PRCP])
			dVariables.set(H_PRCP);
		if (m_variables[H_SNDH] && m_allowedDerivedVariables[H_SNDH] && !m_variables[H_TAIR2])
			dVariables.set(H_TAIR2);
		if (m_variables[H_SNDH] && m_allowedDerivedVariables[H_SNDH] && !m_variables[H_PRCP])
			dVariables.set(H_PRCP);
		if (m_variables[H_SWE] && m_allowedDerivedVariables[H_SWE] && !m_variables[H_TAIR2])
			dVariables.set(H_TAIR2);
		if (m_variables[H_SWE] && m_allowedDerivedVariables[H_SWE] && !m_variables[H_PRCP])
			dVariables.set(H_PRCP);
		/*if (m_variables[H_EA] && m_allowedDerivedVariables[H_EA] && !m_variables[H_TDEW])
			dVariables.set(H_TDEW);
		if (m_variables[H_ES] && m_allowedDerivedVariables[H_ES] && !m_variables[H_TAIR])
			dVariables.set(H_TAIR);
		if (m_variables[H_VPD] && m_allowedDerivedVariables[H_VPD] && !m_variables[H_TAIR])
			dVariables.set(H_TAIR);*/
		//if (m_variables[H_VPD] && m_allowedDerivedVariables[H_VPD] && !m_variables[H_TDEW])
			//dVariables.set(H_TDEW);
		if (m_variables[H_WND2] && m_allowedDerivedVariables[H_WND2] && !m_variables[H_WNDS])
			dVariables.set(H_WNDS);


		

		/*if (m_variables[H_TAIR2] && (!m_variables[H_TMIN2] || !m_variables[H_TMAX2]))
		{
			dVariables.set(H_TMIN2);
			dVariables.set(H_TMAX2);
		}*/

		if (dVariables[H_TAIR2])
		{
			dVariables.set(H_TMIN2);
			dVariables.set(H_TMAX2);
		}


		return dVariables;
	}



	CWVariables CWGInput::GetMandatoryVariables()const
	{
		CWVariables vars(m_variables);
		vars |= GetMissingInputVariables();

		return vars;
	}


	CWVariables CWGInput::GetNormalMandatoryVariables()const
	{
		CWVariables mVariables(m_variables);
		mVariables |= GetMissingInputVariables();

		/*if (mVariables[H_TAIR2] && !mVariables[H_TMIN2])
			mVariables.set(H_TMIN2);

		if (mVariables[H_TAIR2] && !mVariables[H_TMAX2])
			mVariables.set(H_TMAX2);*/

		if (mVariables[H_PRCP] && ! mVariables[H_TAIR2] )
			mVariables.set(H_TAIR2);

		if (mVariables[H_TDEW] && !mVariables[H_RELH])
			mVariables.set(H_RELH);

		if (mVariables[H_TDEW] && mVariables[H_TAIR2] )
			mVariables.set(H_TAIR2);

		//if (mVariables[H_TDEW] && !mVariables[H_TMAX2])
			//mVariables.set(H_TMAX2);

		if (mVariables[H_WND2] && !mVariables[H_WNDS])
			mVariables.set(H_WNDS);

		if (mVariables[H_WNDS] && !mVariables[H_PRCP])
			mVariables.set(H_PRCP);

		if (mVariables[H_TAIR2])
		{
			mVariables.set(H_TMIN2);
			mVariables.set(H_TMAX2);
		}

		return mVariables;
	}

}