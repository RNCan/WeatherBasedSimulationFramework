//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//  Class:	CLocation: hold geographic location and attributes
//
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 12-11-2013   Rémi Saint-Amant    Creation from existing code.
//*********************************************************************
#include "stdafx.h"
#include <algorithm>  
#include <boost/algorithm/string.hpp>

#include "Basic/Location.h"
#include "Basic/CSV.h"

#include "WeatherBasedSimulationString.h"

 
using namespace std;

namespace WBSF
{
	//************************************************************************
	//CLocation Class

	const char* CLocation::XML_FLAG = "Location";
	const char* CLocation::MEMBER_NAME[NB_MEMBER] = { "ID", "Name", "Latitude", "Longitude", "Elevation", "SiteSpecificInformation" };
	const char* CLocation::DEFAULT_SSI_NAME[NB_DEFAULT_SSI] =
	{
		"MergedStationIDs", "Slope", "Aspect", "UseIt", "DataFileName", "Categories", "Years", "LitoralDistance1", "LitoralWeight1", "LitoralDistance2", "LitoralWeight2", "Horizon",//WaterHoldingCapacity...
	};

	StringVector CLocation::MEMBER_TITLE;
	StringVector CLocation::DEFAULT_SSI_TITLE;

	const char* CLocation::GetMemberTitle(size_t i)
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		if (MEMBER_TITLE.empty())
			MEMBER_TITLE = Tokenize(GetString(IDS_STR_LOC_TITLE), ";|");

		return MEMBER_TITLE.empty() ? GetMemberName(i) : MEMBER_TITLE[i].c_str();
	}

	const char* CLocation::GetDefaultSSITitle(size_t i)
	{
		ASSERT(i >= 0 && i < NB_DEFAULT_SSI);

		if (DEFAULT_SSI_TITLE.empty())
			DEFAULT_SSI_TITLE = Tokenize(GetString(IDS_STR_LOCATION_SSI_TITLE), ";|");

		return DEFAULT_SSI_TITLE.empty() ? GetDefaultSSITitle(i) : DEFAULT_SSI_TITLE[i].c_str();
	}



	CLocation::CLocation(string name, string ID, double lat, double lon, double alt) :
		CGeoPoint3D(lon, lat, alt, PRJ_WGS_84),
		m_name(name),
		m_ID(ID)
	{}

	void CLocation::Reset()
	{
		m_name.clear();
		m_ID.clear();
		m_prjID = PRJ_WGS_84;
		m_lat = -999;
		m_lon = -999;
		m_elev = -999;
		m_siteSpeceficInformation.clear();
	}

	CLocation& CLocation::operator=(const CLocation& in)
	{
		if (&in != this)
		{
			CGeoPoint3D::operator=(in);
			m_name = in.m_name;
			m_ID = in.m_ID;
			//m_prjID=in.m_prjID;
			//m_elev = in.m_elev;
			m_siteSpeceficInformation.clear();

			if (!in.m_siteSpeceficInformation.empty())
				m_siteSpeceficInformation.insert(in.m_siteSpeceficInformation.begin(), in.m_siteSpeceficInformation.end());
		}

		return *this;
	}

	bool CLocation::operator ==(const CLocation& in)const
	{
		bool bEqual = true;

		if (CGeoPoint3D::operator!=(in))bEqual = false;
		if (m_name != in.m_name)bEqual = false;
		if (m_ID != in.m_ID)bEqual = false;
		if (m_siteSpeceficInformation != in.m_siteSpeceficInformation)bEqual = false;

		return bEqual;
	}

	void CLocation::Init(const std::string& name, const std::string& ID, double lat, double lon, double alt)
	{
		m_prjID = PRJ_WGS_84;
		m_name = name;
		m_ID = ID;
		m_lat = lat;
		m_lon = lon;
		m_elev = alt;
		m_siteSpeceficInformation.clear();
	}


	std::istream& CLocation::LoadV2(std::istream& io)
	{
		if (io)
		{
			char tmp[256] = { 0 };
			io.ignore(1);
			io.get(tmp, 255, '"');
			io.ignore(1);

			m_name = tmp;
			m_prjID = PRJ_WGS_84;
			io >> m_lat;
			io >> m_lon;
			io >> m_elev;
			float slope = 0;
			float aspect = 0;
			io >> slope;
			io >> aspect;
			SetDefaultSSI(SLOPE, ToString(slope));
			SetDefaultSSI(ASPECT, ToString(aspect));
			//take the line feed
			io.getline(tmp, _MAX_PATH);

		}

		return io;
	}

	//string CLocation::GetMember(int i, LPXNode& pNode)const
	//{
	//	_ASSERTE(i < NB_MEMBER);

	//	string str;
	//	switch (i)
	//	{
	//	case ID:   str = m_ID; break;
	//	case NAME: str = m_name; break;
	//	case LAT:  str = ToString(m_lat); break;
	//	case LON:  str = ToString(m_lon); break;
	//	case ELEV: str = ToString(m_elev); break;
	//	case SITE_SPECIFIC_INFORMATION:
	//	{
	//		for (SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.begin(); it != m_siteSpeceficInformation.end(); it++)
	//			str += "{" + it->first + ":" + it->second.first + "}";
	//		break;
	//	}
	//	default: _ASSERTE(false);
	//	}

	//	return str;
	//}


	//void CLocation::SetMember(int i, const string& str, LPXNode pNode)
	//{
	//	_ASSERTE(i < NB_MEMBER);

	//	switch (i)
	//	{
	//	case ID:  m_ID = str; break;
	//	case NAME: m_name = str; break;
	//	case LAT: m_lat = ToDouble(str); break;
	//	case LON: m_lon = ToDouble(str); break;
	//	case ELEV: m_elev = ToDouble(str); break;
	//	case SSI:
	//	{
	//		StringVector SSIs(str.c_str(), "{}");
	//		for (StringVector::const_iterator it = SSIs.begin(); it != SSIs.end(); it++)
	//		{
	//			string::size_type pos = it->find(':', 0);
	//			if (pos < string::npos)
	//				SetSSI(it->substr(0, pos), it->substr(pos + 1));
	//		}

	//		break;
	//	}

	//	default: _ASSERTE(false);
	//	}
	//}

	string CLocation::GetMember(size_t i)const
	{
		_ASSERTE(i < NB_MEMBER);

		string str;
		switch (i)
		{
		case ID:   str = m_ID; break;
		case NAME: str = m_name; break;
		case LAT:  str = ToString(m_lat); break;
		case LON:  str = ToString(m_lon); break;
		case ELEV: str = ToString(m_elev); break;
		case SITE_SPECIFIC_INFORMATION:
		{
			for (SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.begin(); it != m_siteSpeceficInformation.end(); it++)
				str += "{" + it->first + ":" + it->second.first + "}";
			break;
		}
		default: _ASSERTE(false);
		}

		return str;
	}


	void CLocation::SetMember(size_t i, const string& str)
	{
		_ASSERTE(i < NB_MEMBER);

		switch (i)
		{
		case ID:  m_ID = str; break;
		case NAME: m_name = str; break;
		case LAT: m_lat = ToDouble(str); break;
		case LON: m_lon = ToDouble(str); break;
		case ELEV: m_elev = ToDouble(str); break;
		case SSI:
		{
			StringVector SSIs(str.c_str(), "{}");
			for (StringVector::const_iterator it = SSIs.begin(); it != SSIs.end(); it++)
			{
				string::size_type pos = it->find(':', 0);
				if (pos < string::npos)
					SetSSI(it->substr(0, pos), it->substr(pos + 1));
			}

			break;
		}

		default: _ASSERTE(false);
		}
	}

	double CLocation::GetSlope()const
	{
		double slope = -999;
		SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.find(GetDefaultSSIName(SLOPE));
		if (it != m_siteSpeceficInformation.end())
			slope = ToDouble(it->second.first);

		return slope;
	}

	double CLocation::GetAspect()const
	{
		double aspect = -999;
		SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.find(GetDefaultSSIName(ASPECT));
		if (it != m_siteSpeceficInformation.end())
			aspect = ToDouble(it->second.first);

		_ASSERTE(aspect==-999 || (aspect >= 0 && aspect <= 360) );
		return aspect;
	}

	double CLocation::GetDayLength(short d)const
	{
		return WBSF::GetDayLength(m_lat, d);
	}

	double CLocation::GetDayLength(CTRef d)const
	{
		return WBSF::GetDayLength(m_lat, d);
	}

	double CLocation::attPressure()const
	{
		assert(false);//normalement le standard serait en hPa et non kPa
		return 101.3* pow((293 - 0.0065*m_elev) / 293, 5.26);//in kPa
	}

	std::string CLocation::GetDefaultSSI(size_t i)const
	{
		return GetSSI(GetDefaultSSIName(i));
	}

	void CLocation::SetDefaultSSI(size_t i, const std::string& SSI)
	{
		SetSSI(GetDefaultSSIName(i), SSI);
	}

	std::string CLocation::GetSSI(const std::string& name)const
	{
		SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.find(name);
		if (it != m_siteSpeceficInformation.end())
			return it->second.first;

		return "";
	}

	void CLocation::SetSSI(const std::string& name, const std::string& SSI)
	{
		if (SSI.empty())
		{
			size_t pos = UNKNOWN_POS;
			SiteSpeceficInformationMap::iterator it = m_siteSpeceficInformation.find(name);
			if (it != m_siteSpeceficInformation.end())
				pos = it->second.second;

			m_siteSpeceficInformation.erase(name);
			//update all position after pos

			for (SiteSpeceficInformationMap::iterator it = m_siteSpeceficInformation.begin(); it != m_siteSpeceficInformation.end(); it++)
				if (it->second.second > pos)
					it->second.second--;
		}
		else
		{
			m_siteSpeceficInformation[name] = make_pair(SSI, m_siteSpeceficInformation.size());
		}

	}

	std::string CLocation::GetDataFileName()const
	{
		std::string tmp = GetDefaultSSI(CLocation::DATA_FILE_NAME);
		if (tmp.empty())
			tmp = PurgeFileName(m_name + " [" + m_ID + "]") + ".csv";

		return tmp;
	}



	ERMsg CLocation::IsValid()const
	{
		ERMsg msg;

		if (m_ID.empty())
		{
			//...
		}


		if (m_lat < -90 || m_lat > 90)
		{
			msg.ajoute(GetString(IDS_BSC_INVALID_LAT));
		}

		if (m_lon < -360 || m_lon > 360)
		{
			msg.ajoute(GetString(IDS_BSC_INVALID_LON));
		}

		//some stations have under sea level atitude
		if (m_elev < -450 || m_elev > 9000)
		{
			msg.ajoute(GetString(IDS_BSC_INVALID_ELEV));
		}

		SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.find(GetDefaultSSIName(SLOPE));
		if (it != m_siteSpeceficInformation.end())
		{
			double slope = ToDouble(it->second.first);
			if (slope < 0)
			{

				msg.ajoute(GetString(IDS_BSC_INVALID_SLOPE));
			}
		}


		it = m_siteSpeceficInformation.find(GetDefaultSSIName(ASPECT));
		if (it != m_siteSpeceficInformation.end())
		{
			double aspect = ToDouble(it->second.first);

			if (aspect < 0 || aspect > 360)
			{
				msg.ajoute(GetString(IDS_BSC_INVALID_ASPECT));
			}
		}

		if (!msg)
		{
			msg.ajoute(FormatMsg(IDS_BSC_INVALID_LOCATION, m_name + " [" + m_ID + "]"));
		}

		return msg;

	}

	void CLocation::AppendMergeID(const std::string& mergeID2)
	{
		string mergeID1 = GetDefaultSSI(OPTIONAL_ID);
		StringVector IDs1 = Tokenize(mergeID1, "+");

		StringVector IDs2 = Tokenize(mergeID2, "+");

		for (StringVector::const_iterator it = IDs2.begin(); it != IDs2.end(); it++)
		{
			if ( /**it!=m_ID && */find(IDs1.begin(), IDs1.end(), *it) == IDs1.end())
				IDs1.push_back(*it);
		}

		mergeID1.clear();
		for (StringVector::const_iterator it = IDs1.begin(); it != IDs1.end(); it++)
		{
			mergeID1 += mergeID1.empty() ? "" : "+";
			mergeID1 += *it;
		}

		SetDefaultSSI(OPTIONAL_ID, mergeID1);
	}

	void CLocation::AppendMergeID(const CLocation& ID)
	{
		AppendMergeID(ID.m_ID);
		string mergeID2 = ID.GetDefaultSSI(OPTIONAL_ID);
		AppendMergeID(mergeID2);
	}

	double CLocation::GetDistance(const CLocation& in, bool bTakeElevation)const
	{
		double d = WBSF::GetDistance(in.m_lat, in.m_lon, m_lat, m_lon);

		if (bTakeElevation)
		{
			double e = (in.m_elev - m_elev) * 100;
			d = sqrt(d*d + e*e);
		}

		return d;
	}

	double CLocation::GetXTemp(const CLocation& station, bool bTakeElevation)const
	{
		double d = std::max(GetDistance(station, bTakeElevation), 0.0001);
		return pow(d, -2);
	}

	bool CLocation::IsInit()const
	{
		return (!m_name.empty() || !m_ID.empty()) && m_lat != -999 && m_lon != -999 && m_elev != -999;
	}

	size_t CLocation::GetMemberFromName(const string& headerIn)
	{
		size_t member = SSI;

		string header(headerIn);

		//header.Replace("(meters)", "");
		//header.Replace("(meter)", "");
		//header.Replace("(m)", "");
		//header.Replace("(%)", "");
		//header.Replace("(°)", "");
		string::size_type pos = header.find('(');
		if (pos != string::npos)
			header = Trim(header.substr(0, pos));


		if (boost::iequals(header, "KeyID") || boost::iequals(header, "ID") || boost::iequals(header, "UniqueID") || boost::iequals(header, "CleNo"))
			member = ID;
		else if (boost::iequals(header, "Name") || boost::iequals(header, "Nom") || boost::iequals(header, "StationName") || boost::iequals(header, "NomStation") || boost::iequals(header, "LOC_FILE 4") || boost::iequals(header, "LOC_FILE 4 1") || boost::iequals(header, "LOC_FILE 4 2") || boost::iequals(header, "LOC_FILE 3"))
			member = NAME;
		else if (boost::iequals(header, "Latitude") || boost::iequals(header, "Lat") || boost::iequals(header, "Y"))
			member = LAT;
		else if (boost::iequals(header, "Longitude") || boost::iequals(header, "Lon") || boost::iequals(header, "Long") || boost::iequals(header, "X"))
			member = LON;
		else if (boost::iequals(header, "Elevation") || boost::iequals(header, L"Élévation") || boost::iequals(header, "Elev") || boost::iequals(header, L"Élév") || boost::iequals(header, "Alt") || boost::iequals(header, "Altitude") || boost::iequals(header, "Z"))
			member = ELEV;

		return member;
	}

	vector<size_t> CLocation::GetMembers(const StringVector& header)
	{
		vector<size_t> members(header.size());

		for (size_t i = 0; i < header.size(); i++)
			members[i] = GetMemberFromName(header[i]);

		return members;
	}

	//return the eader in the same position as they was added
	StringVector CLocation::GetSSIHeader()const
	{
		vector<pair<size_t, string>> orderPair;
		for (SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.begin(); it != m_siteSpeceficInformation.end(); it++)
			orderPair.push_back(make_pair(it->second.second, it->first));

		std::sort(orderPair.begin(), orderPair.end());

		StringVector header;
		for (vector<pair<size_t, string>>::const_iterator it = orderPair.begin(); it != orderPair.end(); it++)
			header.push_back(it->second);

		return header;

		//return GetSSIOrder();

		//StringVector header;
		//for( SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.begin(); it!=m_siteSpeceficInformation.end(); it++)
		//for (StringVector::const_iterator it = order.begin(); it != order.end(); it++)
		//header.push_back(m_siteSpeceficInformation[*it]. );

		//return header;
	}

#include <math.h>       /* fmod */
	double CLocation::GetGeocentricCoord(size_t i)const
	{
		_ASSERTE(i < 5);
		_ASSERTE(m_lat >= -90 && m_lat <= 90);

		double y = 360.0;
		double lon = m_lon;
		if (lon > 180)
			lon -= 360;
		if (lon < -180)
			lon += 360;

		//modf(m_lon+720,&y)-180;
		_ASSERTE(lon >= -180 && lon <= 180);

		double xx = Deg2Rad(lon + 180);
		double yy = Deg2Rad(90 - m_lat);

		double v = 0;
		switch (i)
		{
		case 0:v = 6371 * 1000 * cos(xx)*sin(yy); break;
		case 1:v = 6371 * 1000 * sin(xx)*sin(yy); break;
		case 2:v = 6371 * 1000 * cos(yy); break;
		case 3:v = m_elev; break;
		case 4:v = GetExposition(m_lat, GetSlope(), GetAspect()); break;
		default: _ASSERTE(false);
		}
		return v;
	}


	//StringVector CLocation::GetSSIOrder()const
	//{
	//	vector<pair<size_t, string>> orderPair;
	//	for (SiteSpeceficInformationMap::const_iterator it = m_siteSpeceficInformation.begin(); it != m_siteSpeceficInformation.end(); it++)
	//		orderPair.push_back(make_pair(it->second.second, it->second.first));
	//	
	//	std::sort(orderPair.begin(), orderPair.end());
	//
	//	StringVector order;
	//	for (vector<pair<size_t, string>>::const_iterator it = orderPair.begin(); it != orderPair.end(); it++)
	//		order.push_back(it->second);
	//		
	//	return order;
	//}


	//******************************************************************************************************
	StringVector CLocationVector::GetHeaderFromData()const
	{
		StringVector header;
		for (CLocationVector::const_iterator it = begin(); it != end(); it++)
		{
			StringVector SSIHeader = it->GetSSIHeader();
			for (StringVector::const_iterator it2 = SSIHeader.begin(); it2 != SSIHeader.end(); it2++)
			{
				StringVector::const_iterator it3 = find(header.begin(), header.end(), *it2);
				if (it3 == header.end())
					header.push_back(*it2);
			}
		}

		return header;
	}


	ERMsg CLocationVector::Load(const std::string& filePath, const char* separator, CCallback& callback)
	{
		ERMsg msg;

		m_filePath.clear();


		ifStream file;
		msg = file.open(filePath);

		if (msg)
		{
			if (msg)
			{
				msg = Load(file, separator);
				if (msg)
					m_filePath = filePath;
			}
		}

		return msg;
	}

	ERMsg CLocationVector::Load(std::istream& file, const char* separator, CCallback& callback)
	{
		ERMsg msg;

		//estimate file size to reserve memory
		std::streampos ffirst = file.tellg();
		file.seekg(0, std::ios::end);
		std::streampos size = file.tellg() - ffirst;
		reserve(size_t(size / 40));

		callback.SetCurrentDescription("Load locations");
		callback.SetNbStep(size);

		//begin to read
		file.seekg(0);
		vector<size_t> members;

		size_t i = 0;
		for (CSVIterator loop(file, separator); loop != CSVIterator() && msg; ++loop, i++)
		{
			if (members.empty())
				members = CLocation::GetMembers((StringVector&)loop.Header());

			resize(this->size() + 1);

			for (size_t j = 0; j < members.size() && j < (*loop).size(); j++)
			{
				switch (members[j])
				{
				case CLocation::ID: at(i).m_ID = (*loop)[j]; break;
				case CLocation::NAME: at(i).m_name = (*loop)[j]; break;
				case CLocation::LAT: at(i).m_lat = ToDouble((*loop)[j]); break;
				case CLocation::LON: at(i).m_lon = ToDouble((*loop)[j]); break;
				case CLocation::ELEV: at(i).m_elev = ToDouble((*loop)[j]); break;
				case CLocation::SSI: at(i).SetSSI(loop.Header()[j], (*loop)[j]); break;
				default: ASSERT(false);
				}
			}

			msg += callback.SetCurrentStepPos((double)file.tellg());
		}


		return msg;
	}

	ERMsg CLocationVector::Save(const std::string& filePath, char separator, CCallback& callback)const
	{
		ERMsg msg;

		CLocationVector& me = const_cast<CLocationVector&>(*this);
		me.m_filePath.clear();

		ofStream file;
		msg = file.open(filePath);

		if (msg)
		{
			msg = Save(file, separator);
			if (msg)
				me.m_filePath = filePath;

			file.close();
		}

		return msg;
	}


	ERMsg CLocationVector::Save(std::ostream& file, char separator, CCallback& callback)const
	{
		ERMsg msg;

		callback.SetCurrentDescription("Save locations");
		callback.SetNbStep(size());

		string header = "KeyID"; //Excel have problem with file begginning by ID
		for (size_t i = 1; i < CLocation::SSI; i++)
		{
			header += separator;
			header += CLocation::GetMemberName(i);
		}

		StringVector SSIHeader = GetHeaderFromData();
		for (StringVector::const_iterator it = SSIHeader.begin(); it != SSIHeader.end(); it++)
		{
			header += separator;
			header += *it;
		}

		header += "\n";
		file << header;
		//file.write(header.c_str(), header.length());



		for (CLocationVector::const_iterator it = begin(); it != end() && msg; it++)
		{
			string str;
			str.reserve(75);

			for (size_t j = 0; j < 5; j++)
			{
				switch (j)
				{
				case CLocation::ID: str += it->m_ID + separator; break;
				case CLocation::NAME: str += it->m_name + separator; break;
				case CLocation::LAT: str += ToString(it->m_lat, 10) + separator; break;
				case CLocation::LON: str += ToString(it->m_lon, 10) + separator; break;
				case CLocation::ELEV: str += ToString(it->m_elev, 1); break;
				default: ASSERT(false);
				}
			}

			for (size_t j = 0; j < SSIHeader.size(); j++)
			{
				str += separator + it->GetSSI(SSIHeader[j]);
			}

			str += "\n";
			file.write(str.c_str(), str.length());

			msg += callback.StepIt();
		}


		return msg;
	}

	ERMsg CLocationVector::IsValid()const
	{
		//ASSERT(false);
		return ERMsg();
	}

	size_t CLocationVector::FindByID(const std::string& ID)const
	{
		CLocationVector::const_iterator it = std::find_if(begin(), end(), FindLocationByID(ID));
		return (it == end()) ? UNKNOWN_POS : it - begin();
	}

	void CLocationVector::TrimData(char separator)
	{
		for (iterator it = begin(); it != end(); it++)
			it->m_name = PurgeFileName(it->m_name);

		//trim SSI???
	}


	ERMsg CLocationMap::Load(const std::string& filePath)
	{
		return zen::LoadXML(filePath, "Locations", "1", *this);
	}

	ERMsg CLocationMap::Save(const std::string& filePath)
	{
		return zen::SaveXML(filePath, "Locations", "1", *this);
	}

}//namespace WBSF