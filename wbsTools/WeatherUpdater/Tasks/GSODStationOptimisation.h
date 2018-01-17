#pragma once

//#include "UIGHCN.h"
#include "Basic/UtilTime.h"
#include "LocationOptimisation.h"


namespace WBSF
{

	class CGSODStation : public CLocation
	{
	public:

		enum { ID_USAF, ID_WBAN };

		std::string m_USAF;	//Air Force Datsav3 station number
		std::string m_WBAN;	//NCDC WBAN number
		std::string m_CALL; //ICAO call sign
		std::string m_country; //WMO historical country ID, followed by FIPS country ID
		std::string m_state;	//State for US, canada stations
		CTPeriod m_period;

		void SetToSSI()
		{
			CLocation::SetSSI("USAF_ID", m_USAF);
			CLocation::SetSSI("WBAN_ID", m_WBAN);
			CLocation::SetSSI("ICAO_ID", m_CALL);
			CLocation::SetSSI("Country", m_country);
			CLocation::SetSSI("State", m_state);
			CLocation::SetSSI("Period", m_period.GetFormatedString("%1|%2", "%Y-%m-%d"));
		}

		void GetFromSSI()
		{
			m_USAF = CLocation::GetSSI("USAF_ID");
			m_WBAN = CLocation::GetSSI("WBAN_ID");
			m_CALL = CLocation::GetSSI("ICAO_ID");
			m_country = CLocation::GetSSI("Country");
			m_state = CLocation::GetSSI("State");
			m_period.FromFormatedString(CLocation::GetSSI("Period"), "%1|%2", "%Y-%m-%d");
		}
		bool operator>(const CGSODStation& in)const{ return true; }
		bool operator<(const CGSODStation& in)const{ return !operator>(in); }


		std::string GetStationID()const{ return m_USAF + "-" + m_WBAN; }
		
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoPoint>(*this);
			ar & m_USAF & m_WBAN & m_CTRY & m_state & m_CALL & m_period
		}

	};


	class CGSODStationOptimisation : public CLocationOptimisation
	{
	public:

		ERMsg Update(const std::string& referencedFilePath, CCallback& callback = DEFAULT_CALLBACK);

	private:

		int m_nbEmptyName;
		bool ReadStation(std::istream& file, CGSODStation& station);
		CTPeriod GetPeriod(const std::string& line)const;
	};

}