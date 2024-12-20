//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     R�mi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <array>
#include <map>

#include <boost\serialization\access.hpp>
#include <unordered_map>
#include <utility>
#include <algorithm>

#include "Basic/WeatherDefine.h"
#include "Basic/UtilZen.h"
#include "Basic/GeoBasic.h"
#include "Basic/Callback.h"



namespace WBSF
{

	typedef std::unordered_map < std::string, std::pair<std::string, size_t> > SiteSpeceficInformationMapBase;
	class SiteSpeceficInformationMap : public SiteSpeceficInformationMapBase
	{
	public:


		bool operator ==(const SiteSpeceficInformationMap& in)const
		{
			bool bEqual = size() == in.size();
			for (const_iterator it = begin(); it != end() && bEqual; it++)
				bEqual = in.find(it->first) != in.end() && it->second == in.at(it->first);

			return bEqual;
		}
		bool operator !=(const SiteSpeceficInformationMap& in)const{ return !operator==(in); }

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<SiteSpeceficInformationMapBase>(*this);
		}
		friend boost::serialization::access;


		std::ostream& operator << (std::ostream& stream)const;
		std::istream& operator >> (std::istream& stream);
		friend std::ostream& operator << (std::ostream& stream, const SiteSpeceficInformationMap& data) { return data << stream; }
		friend std::istream& operator >> (std::istream& stream, SiteSpeceficInformationMap& data) { return data >> stream; }

	};


	

	//****************************************************************
	// CLocation
	class CLocation : public CGeoPoint3D
	{
	public:

		enum TMember{ ID, NAME, LAT, LON, ELEV, SITE_SPECIFIC_INFORMATION, SSI = SITE_SPECIFIC_INFORMATION, NB_MEMBER };
		static const char* GetMemberName(size_t i){ _ASSERTE(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetMemberTitle(size_t d);
		static const char* GetXMLFlag(){ return XML_FLAG; }
		enum TDefaultSSI{ OPTIONAL_ID, SLOPE, ASPECT, USED_IT, DATA_FILE_NAME, TIME_ZONE, VARIABLES, YEARS, SHORE_DIST, E_HORIZON, W_HORIZON, NB_DEFAULT_SSI };

		static const char* GetDefaultSSIName(size_t i){ ASSERT(i >= 0 && i < NB_DEFAULT_SSI); return DEFAULT_SSI_NAME[i]; }
		static const char* GetDefaultSSITitle(size_t i);
		static size_t GetMemberFromName(const std::string& headerIn);
		static std::vector<size_t> GetMembers(const StringVector& header);

		


		//public member
		std::string m_name;		//name of the simulation point
		std::string m_ID;		//ID of the simulation point


		SiteSpeceficInformationMap m_siteSpeceficInformation;//all other information must be place in m_siteSpeceficInformation

		CLocation(std::string name = "", std::string ID = "", double lat = -999, double lon = -999, double elev = -999);
		CLocation(const CLocation& in){ operator=(in); }
		virtual ~CLocation(){}
		virtual ERMsg SaveData(const std::string& filePath, CTM TM = CTM(), char separator = ',')const{ assert(false);  return ERMsg(); }

		void clear(){ Reset(); }
		void Reset();
		void Init(const std::string& name, const std::string& ID, double lat, double lon, double elev);

		bool operator ==(const CLocation& in)const;
		bool operator !=(const CLocation& in)const{ return !operator==(in); }
		CLocation& operator =(const CLocation& in);

		std::istream&  LoadV2(std::istream& io);
		double GetDouble(size_t i)const
		{
			double value = 0;
			switch (i)
			{
			case 0: value = m_lat; break;
			case 1: value = m_lon; break;
			case 2: value = m_elev; break;
			case 3: value = GetSlope(); break;
			case 4: value = GetAspect(); break;
			default: ASSERT(false);
			}
			return value;
		}

		double GetSlope()const;
		double GetSlopeInDegree()const;
		double GetAspect()const;
		double GetShoreDistance()const;
		void SetShoreDistance(double shore_distance);

		std::string GetDataFileName()const;
		void SetDataFileName(std::string in){ SetDefaultSSI(CLocation::DATA_FILE_NAME, in); }

		double GetTimeZone()const;
		void SetTimeZone(double in){ SetDefaultSSI(CLocation::TIME_ZONE, ToString(in)); }

		bool UseIt()const { std::string tmp = GetDefaultSSI(CLocation::USED_IT); return tmp.empty() || tmp != "0"; }
		void UseIt(bool in){ SetDefaultSSI(CLocation::USED_IT, in ? "1" : "0"); }

		double GetDayLength(size_t d)const;
		double GetDayLength(CTRef d)const;
		double GetPressure()const;//Default altitude pressure [hPa]
		double GetDistance(const CLocation& in, bool bTakeElevation, bool bTakeShoreDistance)const;
		double GetXTemp(const CLocation& station, bool bTakeElevation, bool bTakeShoreDistance)const;

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);

		// Definition of the template
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoPoint3D>(*this);
			ar & m_ID & m_name & m_siteSpeceficInformation;
		}

		friend boost::serialization::access;
		std::ostream& operator>>(std::ostream &s)const;
		std::istream& operator<<(std::istream &s);
		friend std::ostream& operator<<(std::ostream &s, const CLocation& pt) { pt >> s; return s; }
		friend std::istream& operator>>(std::istream &s, CLocation& pt) { pt << s;	return s; }


		//set site specific information (SSI)
		std::string GetDefaultSSI(size_t i)const;
		void SetDefaultSSI(size_t i, const std::string& ssi);
		std::string GetSSI(const std::string& name)const;
		void SetSSI(const std::string& name, const std::string& ssi);
		StringVector GetSSIHeader()const;

		ERMsg IsValid(bool bExludeUnknownElev = true)const;
		bool IsInit()const;

		void AppendMergeID(const std::string& ID);
		void AppendMergeID(const CLocation& ID);

		double GetGeocentricCoord(size_t i)const;

	protected:

		
		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];
		static StringVector MEMBER_TITLE;
		static const char* DEFAULT_SSI_NAME[NB_DEFAULT_SSI];
		static StringVector DEFAULT_SSI_TITLE;

	};

	class FindLocationByName
	{
	public:

		FindLocationByName(const std::string& name, bool bCase = false) :m_name(name), m_bCase(bCase)
		{}

		bool operator ()(const CLocation& in)const{ return WBSF::IsEqual(in.m_name, m_name, m_bCase); }

	protected:
		std::string m_name;
		bool m_bCase;
	};

	class FindLocationByID
	{
	public:

		FindLocationByID(const std::string& ID, bool bCase=false) :m_ID(ID), m_bCase(bCase)
		{}

		bool operator ()(const CLocation& in)const{ return WBSF::IsEqual(in.m_ID,m_ID, m_bCase); }

	protected:
		std::string m_ID;
		bool m_bCase;
	};

	class FindLocationByDataFileName
	{
	public:

		FindLocationByDataFileName(const std::string& dataFileName, bool bCase = false) :m_dataFileName(dataFileName), m_bCase(bCase)
		{}

		bool operator ()(const CLocation& in)const{ return WBSF::IsEqual(in.GetDataFileName(), m_dataFileName, m_bCase); }

	protected:
		std::string m_dataFileName;
		bool m_bCase;
	};

	class FindLocationBySSI
	{
	public:

		FindLocationBySSI(const std::string& SSI, const std::string& value, bool bCase = true) :m_SSI(SSI), m_value(value), m_bCase(bCase)
		{}

		bool operator ()(const CLocation& in)const{ return WBSF::IsEqual(in.GetSSI(m_SSI), m_value, m_bCase); }

	protected:

		bool m_bCase;
		std::string m_SSI;
		std::string m_value;
	};
	

	typedef std::vector<CLocation> CLocationVectorBase;
	class CLocationVector : public CLocationVectorBase
	{
	public:

		static ERMsg ExtractNominatimName(CLocationVector& locations, bool bReplaceAll, bool bName, bool bState, bool bCountry, CCallback& callback);
		static ERMsg ExtractOpenTopoDataElevation(CLocationVector& locations, bool bReplaceAll, size_t eProduct, size_t eINterpol, CCallback& callback);
		static ERMsg ExtractShoreDistance(CLocationVector& locations, bool bReplaceAll, CCallback& callback);



		CLocationVector(size_t size = 0) :CLocationVectorBase(size)
		{}

		ERMsg Load(const std::string& filePath, const char* separator = ",;", CCallback& callback = CCallback::DEFAULT_CALLBACK);
		ERMsg Load(std::istream& filePath, const char* separator = ",;", CCallback& callback = CCallback::DEFAULT_CALLBACK);
		ERMsg Save(const std::string& filePath, char separator = ',', CCallback& callback = CCallback::DEFAULT_CALLBACK)const;
		ERMsg Save(std::ostream& filePath, char separator = ',', CCallback& callback = CCallback::DEFAULT_CALLBACK)const;
		ERMsg IsValid(bool bExludeUnknownElev=true)const;
		ERMsg ExtractNominatimName(bool bReplaceAll, bool bName, bool bState, bool bCountry, CCallback& callback) { return ExtractNominatimName(*this, bReplaceAll, bName, bState, bCountry, callback); }
		ERMsg ExtractOpenTopoDataElevation(bool bReplaceAll, size_t eProduct, size_t eINterpol, CCallback& callback) { return ExtractOpenTopoDataElevation(*this, bReplaceAll, eProduct, eINterpol, callback); }
		ERMsg ExtractShoreDistance(bool bReplaceAll, CCallback& callback) { return ExtractShoreDistance(*this, bReplaceAll, callback); }




		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CLocationVectorBase>(*this);
		}
		friend boost::serialization::access;

		std::ostream& operator << (std::ostream& stream)const;
		std::istream& operator >> (std::istream& stream);
		friend std::ostream& operator << (std::ostream& stream, const CLocationVector& data) { return data << stream; }
		friend std::istream& operator >> (std::istream& stream, CLocationVector& data) { return data >> stream; }

		const std::string& GetFilePath()const{ return m_filePath; }

		StringVector GetHeader()const
		{
			StringVector header(CLocation::SITE_SPECIFIC_INFORMATION);
			for (size_t i = 0; i < CLocation::SITE_SPECIFIC_INFORMATION; i++)
				header[i] = CLocation::GetMemberTitle(i);

			StringVector SSI = GetSSIHeader();

			header.insert(header.end(), SSI.begin(), SSI.end());

			return header;
		}

		StringVector GetSSIHeader()const{ return empty() ? StringVector() : front().GetSSIHeader(); }

		size_t FindPosByID(const std::string& ID, bool bCase=false)const;
		CLocationVector::const_iterator FindByID(const std::string& ID, bool bCase = false)const;
		CLocationVector::iterator FindByID(const std::string& ID, bool bCase = false);
		CLocationVector::const_iterator FindByName(const std::string& ID, bool bCase = false)const;
		CLocationVector::iterator FindByName(const std::string& ID, bool bCase = false);
		CLocationVector::const_iterator FindBySSI(const std::string& SSI, const std::string& value, bool bCase = false)const;
		CLocationVector::iterator FindBySSI(const std::string& SSI, const std::string& value, bool bCase = false);
		

		StringVector GetHeaderFromData()const;

		void TrimData(char separator);
	protected:

		std::string m_filePath;
	};


	typedef std::shared_ptr < CLocationVector > CLocationVectorPtr;


	class CLocationMap : public std::map < std::string, CLocation >
	{
	public:

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);
		ERMsg LoadFromCSV(const std::string& filePath);
		ERMsg SaveToCSV(const std::string& filePath);

	};

}//namespace WBSF

namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CLocation& loc, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CLocation::GetMemberName(WBSF::CLocation::ID)](loc.m_ID);
		out[WBSF::CLocation::GetMemberName(WBSF::CLocation::NAME)](loc.m_name);
		out[WBSF::CLocation::GetMemberName(WBSF::CLocation::LAT)](loc.m_lat);
		out[WBSF::CLocation::GetMemberName(WBSF::CLocation::LON)](loc.m_lon);
		out[WBSF::CLocation::GetMemberName(WBSF::CLocation::ELEV)](loc.m_elev);

		WBSF::StringVector header = loc.GetSSIHeader();
		assert(header.size() == loc.m_siteSpeceficInformation.size());
		//for(SiteSpeceficInformationMap::const_iterator it=loc.m_siteSpeceficInformation.begin(); it!=loc.m_siteSpeceficInformation.end(); it++)
		for (size_t i = 0; i != loc.m_siteSpeceficInformation.size(); i++)
		{
			assert(loc.m_siteSpeceficInformation.find(header[i]) != loc.m_siteSpeceficInformation.end());

			XmlElement& newChild = output.addChild(header[i]);
			newChild.setValue(loc.m_siteSpeceficInformation.at(header[i]).first);
		}
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CLocation& loc)
	{
		XmlIn in(input);


		std::pair<XmlElement::ChildIterConst, XmlElement::ChildIterConst> iterPair = input.getChildren();
		for (XmlElement::ChildIterConst iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			std::string header = iter->getNameAs<std::string>();

			if (WBSF::IsEqualNoCase(header, "ID"))
				iter->getValue(loc.m_ID);
			else if (WBSF::IsEqualNoCase(header, "Name"))
				iter->getValue(loc.m_name);
			else if (WBSF::IsEqualNoCase(header, "Latitude"))
				iter->getValue(loc.m_lat);
			else if (WBSF::IsEqualNoCase(header, "Longitude"))
				iter->getValue(loc.m_lon);
			else if (WBSF::IsEqualNoCase(header, "Elevation"))
				iter->getValue(loc.m_elev);
			else
			{
				std::string SSI;
				iter->getValue(SSI);
				loc.m_siteSpeceficInformation[header] = std::make_pair(SSI, loc.m_siteSpeceficInformation.size());
			}
		}

		return true;
	}

	
	template <> inline
		void writeStruc(const WBSF::CLocationMap& in, XmlElement& output)
	{
		for (WBSF::CLocationMap::const_iterator it = in.begin(); it != in.end(); it++)
			writeStruc(it->second, output.addChild("Location"));
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CLocationMap& out)
	{
		auto iterators = input.getChildren("Location");

		for (XmlElement::ChildIterConst2 it = iterators.first; it != iterators.second; it++)
		{
			WBSF::CLocation loc;
			readStruc(*it, loc);
			out[loc.m_ID] = loc;
		}

		return true;
	}

	template <> inline
		void writeStruc(const WBSF::CLocationVector& in, XmlElement& output)
	{
		writeStruc3(in, output, WBSF::CLocation::GetXMLFlag());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CLocationVector& out)
	{
		readStruc3(input, out, WBSF::CLocation::GetXMLFlag());
		return true;
	}

}
