#pragma once

#include "Basic/UtilTime.h"
#include "LocationOptimisation.h"
//#include "UIGHCN.h"


namespace WBSF
{

	class CIDSLiteStation : public CLocation
	{
	public:

		std::string m_USAF;
		std::string m_WBAN;
		std::string m_country;
		std::string m_state;
		CTPeriod m_period;

		void SetToSSI()
		{
			CLocation::SetSSI("USAF_ID", m_USAF);
			CLocation::SetSSI("WBAN_ID", m_WBAN);
			CLocation::SetSSI("Country", m_country);
			CLocation::SetSSI("State", m_state);
			CLocation::SetSSI("Period", m_period.GetFormatedString("%1 %2"));
		}

		void GetFromSSI()
		{
			m_USAF = CLocation::GetSSI("USAF_ID");
			m_WBAN = CLocation::GetSSI("WBAN_ID");
			m_country = CLocation::GetSSI("Country");
			m_state = CLocation::GetSSI("State");
			m_period.FromFormatedString("%1 %2", CLocation::GetSSI("Period"));
		}

		bool ReadStation(const StringVector& line);

		bool operator>(const CIDSLiteStation& in)const;
		bool operator<(const CIDSLiteStation& in)const{ return !operator>(in); }


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoPoint>(*this);
			ar & m_USAF & m_WBAN & m_country & m_state& m_period
		}



		enum TColumn { C_USAF, C_WBAN, C_STATION_NAME, C_CTRY, C_STATE, C_ICAO, C_LAT, C_LON, C_ELEV, C_BEGIN, C_END, NB_COLUMNS };
	};

	//template <> void AFXAPI SerializeElements<CIDSLiteStation> (CArchive& ar, CIDSLiteStation* pElements, INT_PTR nCount);

	//typedef CMap < std::string, const std::string&, long, long > CMapLongToLong;
	//****************************************************
	//CIDSLiteStationOptimisation

	class CIDSLiteStationOptimisation : public CLocationOptimisation
	{
	public:

		enum TType{ USAF, WBAN, NB_TYPE };
		//	static const char* GetTypeName(int i){ ASSERT(i>=0&&i<NB_TYPE);return TYPE_NAME[i]; }
		//const char* GetTypeName()const{return GetTypeName(m_type);}

		CIDSLiteStationOptimisation()
		{
			//ASSERT(type>=0&&type<NB_TYPE);
			//m_type = type;
		}
		virtual ERMsg Update(const std::string& referencedFilePath, CCallback& callback = DEFAULT_CALLBACK);


		std::string GetID(short product, const std::string& line);



		//static const char* TYPE_NAME[NB_TYPE];
	};


	typedef std::vector<CIDSLiteStationOptimisation> CIDSLiteStationOptimisationVector;

}