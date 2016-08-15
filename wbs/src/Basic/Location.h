//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <array>
#include <map>
//#include <math.h>
#include <boost\serialization\access.hpp>
#include <unordered_map>
#include <utility>
#include <algorithm>

#include "Basic/WeatherDefine.h"
#include "Basic/UtilZen.h"
#include "Basic/GeoBasic.h"
//#include "Basic/XMLite.h"
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


	};


	

	//****************************************************************
	// CLocation
	class CLocation : public CGeoPoint3D
	{
	public:

		//SLOPE, ASPECT
		enum TMember{ ID, NAME, LAT, LON, ELEV, SITE_SPECIFIC_INFORMATION, SSI = SITE_SPECIFIC_INFORMATION, NB_MEMBER };
		static const char* GetMemberName(size_t i){ _ASSERTE(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetMemberTitle(size_t d);
		static const char* GetXMLFlag(){ return XML_FLAG; }
		enum TDefaultSSI{ OPTIONAL_ID, SLOPE, ASPECT, USED_IT, DATA_FILE_NAME, VARIABLES, YEARS, LITORAL_DIST1, LITORAL_WEIGHT1, LITORAL_DIST2, LITORAL_WEIGHT2, HORIZON, NB_DEFAULT_SSI };

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
		double GetSlopeInDegree()const
		{
			double slope = GetSlope();
			if (slope == -999)
				return -999;

			ASSERT(slope >= 0);
			ASSERT(atan(GetSlope() / 100) * RAD2DEG >= 0);
			ASSERT(atan(GetSlope() / 100) * RAD2DEG <= 90);
			return float(atan(GetSlope() / 100) * RAD2DEG);
		}
		double GetAspect()const;


		std::string GetDataFileName()const;
		void SetDataFileName(std::string in){ SetDefaultSSI(CLocation::DATA_FILE_NAME, in); }

		bool UseIt()const { std::string tmp = GetDefaultSSI(CLocation::USED_IT); return tmp.empty() || tmp != "0"; }
		void UseIt(bool in){ SetDefaultSSI(CLocation::USED_IT, in ? "1" : "0"); }

		//void SetAspect(float aspect){_ASSERTE( aspect >= 0 && aspect <= 360);m_aspect = aspect ;}

		double GetDayLength(short d)const;
		double GetDayLength(CTRef d)const;
		double attPressure()const;//in kPa
		double GetDistance(const CLocation& in, bool bTakeElevation)const;
		double GetXTemp(const CLocation& station, bool bTakeElevation)const;

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);

		//std::string GetMember(int i, LPXNode& pNode = NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode = NULL_ROOT);
		//void GetXML(LPXNode& pRoot)const{ XGetXML(*this, pRoot); }
		//void SetXML(const LPXNode pRoot){ XSetXML(*this, pRoot); }

		// Definition of the template
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CGeoPoint3D>(*this);
			ar & m_ID & m_name & m_siteSpeceficInformation;
		}
		friend boost::serialization::access;

		//set site specific information (SSI)
		std::string GetDefaultSSI(size_t i)const;
		void SetDefaultSSI(size_t i, const std::string& ssi);
		std::string GetSSI(const std::string& name)const;
		void SetSSI(const std::string& name, const std::string& ssi);
		StringVector GetSSIHeader()const;

		ERMsg IsValid()const;
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

		FindLocationByName(const std::string& name) :m_name(name)
		{}

		bool operator ()(const CLocation& in)const{ return in.m_name == m_name; }

	protected:
		std::string m_name;
	};

	class FindLocationByID
	{
	public:

		FindLocationByID(const std::string& ID) :m_ID(ID)
		{}

		bool operator ()(const CLocation& in)const{ return in.m_ID == m_ID; }

	protected:
		std::string m_ID;
	};

	class FindLocationByDataFileName
	{
	public:

		FindLocationByDataFileName(const std::string& dataFileName) :m_dataFileName(dataFileName)
		{}

		bool operator ()(const CLocation& in)const{ return in.GetDataFileName() == m_dataFileName; }

	protected:
		std::string m_dataFileName;
	};

	

	typedef std::vector<CLocation> CLocationVectorBase;
	class CLocationVector : public CLocationVectorBase
	{
	public:

		CLocationVector(size_t size = 0) :CLocationVectorBase(size)
		{}

		ERMsg Load(const std::string& filePath, const char* separator = ",;", CCallback& callback = CCallback::DEFAULT_CALLBACK);
		ERMsg Load(std::istream& filePath, const char* separator = ",;", CCallback& callback = CCallback::DEFAULT_CALLBACK);
		ERMsg Save(const std::string& filePath, char separator = ',', CCallback& callback = CCallback::DEFAULT_CALLBACK)const;
		ERMsg Save(std::ostream& filePath, char separator = ',', CCallback& callback = CCallback::DEFAULT_CALLBACK)const;
		ERMsg IsValid()const;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CLocationVectorBase>(*this);
		}
		friend boost::serialization::access;

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

		size_t FindByID(const std::string& ID)const;

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
