//***************************************************************************
// File:        TGInput.h
//
// Class:       CWGInput
//
// Abstract:    Manage input parameters for TemGen
//
// Description: A container class with XML serialisation for TemGen Input
//
// Note:        
//***************************************************************************
#pragma once

#include "Basic/WeatherDefine.h"
#include "ModelBase/ModelOutputVariable.h"

namespace WBSF
{
	class CSearchRadius : public std::array< double, HOURLY_DATA::NB_VAR_H>
	{
	public:

		CSearchRadius()
		{
			reset();
		}

		void reset()
		{
			fill(-999);//radius in meters
			for (size_t v = HOURLY_DATA::H_SRAD; v < HOURLY_DATA::NB_VAR_H; v++)
				at(v) = 0; //no search by default for secondery variables
		}

		bool operator==(const CSearchRadius& in)const
		{
			bool bEqual = true;

			for (size_t i = 0; i < size()&& bEqual; i++)
				bEqual = fabs(at(i) -in[i])<0.1;//0.1 meters


			return bEqual;
		}

		bool operator!=(const CSearchRadius& in)const { return !operator==(in); }

	};
}

namespace zen
{
	template <> inline
	void writeStruc(const WBSF::CSearchRadius& in, zen::XmlElement& output)
	{
		output.setValue(to_string(in, ","));
	}

	template <> inline
	bool readStruc(const zen::XmlElement& input, WBSF::CSearchRadius& out )
	{
		std::string str;
		if (input.getValue(str))
		{
			std::vector<double> tmp = WBSF::to_object<double>(str, ",");
			for (size_t i = 0; i < tmp.size() && i < WBSF::HOURLY_DATA::NB_VAR_H; i++)
				out[i] = tmp[i];
		}


		return true;
	}
}


namespace WBSF
{

	class CWGInput
	{
	public:


		enum TWeatherSource { FROM_DISAGGREGATIONS, FROM_OBSERVATIONS, NB_SOURCES };
		enum TWeatherGeneration { GENERATE_HOURLY, GENERATE_DAILY, NB_GENERATIONS };

		enum TAlbedo { NONE, CANOPY, NB_ALBEDO };

		enum TMember {
			VARIABLES, SOURCE_TYPE, GENERATION_TYPE, NB_NORMALS_YEARS, FIRST_YEAR, LAST_YEAR, USE_FORECAST, USE_RADAR_PRCP, NORMAL_DB_NAME, NB_NORMAL_STATION,
			DAILY_DB_NAME, NB_DAILY_STATION, HOURLY_DB_NAME, NB_HOURLY_STATION, USE_GRIBS, GRIBS_DB_NAME, NB_GRIB_POINTS, ALBEDO, SEED, ALLOWED_DERIVED_VARIABLES, XVALIDATION,
			SKIP_VERIFY, SEARCH_RADIUS, NO_FILL_MISSING, USE_SHORE, NB_MEMBERS
		};

		static const char* GetMemberName(size_t i){ ASSERT(i < NB_MEMBERS); return MEMBERS_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static const short NB_STATION_SEARCH_MAX;
		static const char * FILE_EXT;

		//*** public member ***
		CWVariables m_variables;
		int m_sourceType;
		int m_generationType;
		size_t m_nbNormalsYears;
		int m_firstYear;
		int m_lastYear;
		bool m_bUseForecast;
		bool m_bUseRadarPrcp;
		bool m_bUseGribs;
		//bool m_bAtSurfaceOnly;

		std::string m_normalsDBName;
		size_t m_nbNormalsStations;
		std::string m_dailyDBName;
		size_t m_nbDailyStations;
		std::string m_hourlyDBName;
		size_t m_nbHourlyStations;
		std::string m_gribsDBName;
		size_t m_nbGribPoints;
		size_t m_albedo;
		size_t m_seed;
		CWVariables m_allowedDerivedVariables;
		bool m_bXValidation;
		bool m_bSkipVerify;
		bool m_bNoFillMissing;
		bool m_bUseShore;
		CSearchRadius m_searchRadius;


		bool IsNormals()const{ return (m_sourceType == FROM_DISAGGREGATIONS); }
		bool IsObservation()const{ return (m_sourceType == FROM_OBSERVATIONS); }
		bool UseGribs()const{ return IsObservation() && m_bUseGribs; }
		bool IsDaily()const{ return (m_sourceType == FROM_OBSERVATIONS) && m_generationType == GENERATE_DAILY; }
		bool IsHourly()const{ return (m_sourceType == FROM_OBSERVATIONS) && m_generationType == GENERATE_HOURLY; }
		bool UseDaily()const{ return IsDaily(); }
		bool UseHourly()const{ return IsHourly(); }
		size_t GetNbYears()const{ return m_sourceType == FROM_DISAGGREGATIONS ? m_nbNormalsYears : m_lastYear - m_firstYear + 1; }
		int GetFirstYear()const{ return m_sourceType == FROM_DISAGGREGATIONS ? 1 - int(m_nbNormalsYears) : m_firstYear; }
		int GetLastYear()const{ return m_sourceType == FROM_DISAGGREGATIONS ? 0 : m_lastYear; }
		CTM GetTM()const{ return CTM(m_generationType == GENERATE_DAILY ? CTM::DAILY : CTM::HOURLY); }
		CTPeriod GetTPeriod()const;

		size_t XVal()const{ return m_bXValidation ? 1 : 0; }
		size_t GetNbNormalsToSearch()const{ return  m_nbNormalsStations + XVal(); }
		size_t GetNbDailyToSearch()const{ return  m_nbDailyStations + XVal(); }
		size_t GetNbHourlyToSearch()const{ return  m_nbHourlyStations + XVal(); }
		size_t GetNbObservationToSearch()const{ return  IsHourly() ? GetNbHourlyToSearch() : GetNbDailyToSearch(); }
		

		CModelOutputVariableDefVector GetOutputDefenition()const;

		CWVariables GetMissingInputVariables()const;
		CWVariables GetMandatoryVariables()const;
		CWVariables GetNormalMandatoryVariables()const;
		//*********************



		//constructor/assignation
		CWGInput();
		CWGInput(const CWGInput& in);


		void Reset(){ clear(); }
		void clear();
		CWGInput& operator=(const CWGInput& in);
		bool operator==(const CWGInput& in)const;
		bool operator!=(const CWGInput& in)const;

		std::string operator[](size_t index)const{ return GetMember(index); }
		std::string GetMember(size_t i)const;

		//operation
		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);
		//ERMsg LoadV1(const std::string& filePath);

		void LoadDefaultParameter();
		void SetAsDefaultParameter();

		ERMsg IsValid()const;


		std::string GetFilePath()const{ return m_filePath; }
		std::string GetName()const{ return GetFileTitle(m_filePath); }



		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_variables&m_sourceType&m_generationType&m_nbNormalsYears&m_firstYear&m_lastYear&m_bUseForecast&m_bUseRadarPrcp&m_normalsDBName;
			ar & m_nbNormalsStations&m_dailyDBName&m_nbDailyStations&m_hourlyDBName&m_nbHourlyStations&m_bUseGribs&m_gribsDBName&m_nbGribPoints&/*m_bAtSurfaceOnly&*/m_albedo&m_seed&m_allowedDerivedVariables&m_bXValidation&m_bSkipVerify&m_bNoFillMissing&m_bUseShore;
			ar & m_searchRadius;
		}


		friend boost::serialization::access;

	protected:

		std::string m_filePath;



		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS];
	};
}
namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CWGInput& in, XmlElement& output)
	{

		XmlOut out(output);

		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::VARIABLES)](in.m_variables);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::SOURCE_TYPE)](in.m_sourceType);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::GENERATION_TYPE)](in.m_generationType);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_NORMALS_YEARS)](in.m_nbNormalsYears);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::FIRST_YEAR)](in.m_firstYear);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::LAST_YEAR)](in.m_lastYear);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::USE_FORECAST)](in.m_bUseForecast);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::USE_RADAR_PRCP)](in.m_bUseRadarPrcp);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_NORMAL_STATION)](in.m_nbNormalsStations);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NORMAL_DB_NAME)](in.m_normalsDBName);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_DAILY_STATION)](in.m_nbDailyStations);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::DAILY_DB_NAME)](in.m_dailyDBName);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_HOURLY_STATION)](in.m_nbHourlyStations);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::HOURLY_DB_NAME)](in.m_hourlyDBName);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_GRIB_POINTS)](in.m_nbGribPoints);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::GRIBS_DB_NAME)](in.m_gribsDBName);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::USE_GRIBS)](in.m_bUseGribs);
//		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::ATSURFACE_ONLY)](in.m_bAtSurfaceOnly);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::ALBEDO)](in.m_albedo);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::SEED)](in.m_seed);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::ALLOWED_DERIVED_VARIABLES)](in.m_allowedDerivedVariables);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::XVALIDATION)](in.m_bXValidation);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::SKIP_VERIFY)](in.m_bSkipVerify);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NO_FILL_MISSING)](in.m_bNoFillMissing);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::USE_SHORE)](in.m_bUseShore);
		out[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::SEARCH_RADIUS)](in.m_searchRadius);
		
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CWGInput& out)
	{
		XmlIn in(input);

		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::VARIABLES)](out.m_variables);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::SOURCE_TYPE)](out.m_sourceType);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::GENERATION_TYPE)](out.m_generationType);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_NORMALS_YEARS)](out.m_nbNormalsYears);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::FIRST_YEAR)](out.m_firstYear);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::LAST_YEAR)](out.m_lastYear);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::USE_FORECAST)](out.m_bUseForecast);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::USE_RADAR_PRCP)](out.m_bUseRadarPrcp);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NORMAL_DB_NAME)](out.m_normalsDBName);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_NORMAL_STATION)](out.m_nbNormalsStations);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::DAILY_DB_NAME)](out.m_dailyDBName);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_DAILY_STATION)](out.m_nbDailyStations);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::HOURLY_DB_NAME)](out.m_hourlyDBName);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_HOURLY_STATION)](out.m_nbHourlyStations);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::GRIBS_DB_NAME)](out.m_gribsDBName);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NB_GRIB_POINTS)](out.m_nbGribPoints);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::USE_GRIBS)](out.m_bUseGribs);
		//in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::ATSURFACE_ONLY)](out.m_bAtSurfaceOnly);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::ALBEDO)](out.m_albedo);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::SEED)](out.m_seed);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::ALLOWED_DERIVED_VARIABLES)](out.m_allowedDerivedVariables);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::XVALIDATION)](out.m_bXValidation);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::SKIP_VERIFY)](out.m_bSkipVerify);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::NO_FILL_MISSING)](out.m_bNoFillMissing);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::USE_SHORE)](out.m_bUseShore);
		in[WBSF::CWGInput::GetMemberName(WBSF::CWGInput::SEARCH_RADIUS)](out.m_searchRadius);

		return true;
	}
}

