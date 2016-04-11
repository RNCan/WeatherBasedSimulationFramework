//*********************************************************************
// File: InputParam.cpp
//
// Class:	CParameter: hold input and output parameters
//          CLocation: hold simulation point
//			CMTGInput: hold Temperature generator parameter
//			CCounter: hold information about stage process
//
// Notes: 
//************** MODIFICATIONS  LOG ********************
// 01/01/2002   Rémi Saint-Amant    Creation from existing code.
// 13/01/2004	Rémi Saint-Amant	Implement the new file transfer format
// 17/02/2004	Rémi Saint-Amant	Created this file from CBioSIMModelBase.cpp
// 22/03/2007	Rémi Saint-Amant	replace CCFLString by string
// 12/12/2007	Rémi Saint-Amant	Add ModelInput, ModelOutput and ModelDefinition
// 04/02/2009	Rémi Saint-Amant	Add operator == and GetXML/SetXML
// 26/07/2011   Rémi Saint-Amant    Correction of a bug in the varaible "Year".
//									BioSIM 9 send first year and BioSIM 10 send last year
//*********************************************************************
#include "stdafx.h"
#include <fstream>

#include "ModelBase/InputParam.h"
#include "ModelBase/BioSimModelBase.h"

 
using namespace std;


namespace WBSF
{
	//************************************************************************

	//CParameter
	CParameter::CParameter(const string& name, const string& value, bool bIsVariable)
	{
		m_name = name;
		m_value = value;
		m_bIsVariable = bIsVariable;
	}

	void CParameter::clear()
	{
		m_name.clear();
		m_value.clear();
		m_bIsVariable = false;
	}

	bool CParameter::operator ==(const CParameter& in)const
	{
		bool bEqual = true;
		if (&in != this)
		{
			if (m_name != in.m_name)bEqual = false;
			if (m_value != in.m_value)bEqual = false;
			if (m_bIsVariable != in.m_bIsVariable)bEqual = false;
		}

		return bEqual;
	}

	istream& operator >> (istream& io, CParameter& param)
	{
		if (io)
		{
			param << io;
		}

		return io;
	}

	std::istream& CParameter::operator << (std::istream& io)
	{

		if (io)
		{
			string tmp;
			std::getline(io, tmp);
			StringVector str(tmp.c_str(), "|");

			if (str.size() == 3)
			{
				m_value = str[0];
				m_name = str[1];
				m_bIsVariable = std::stoi(str[2]) != 0;
			}
		}

		return io;
	}

	CTRef CParameter::GetTRef()const
	{
		CTRef date;
		StringVector str(m_value, "/\\ ");

		if (str.size() == 2 || str.size() == 3)
		{
			size_t d = ToInt(str[0]) - 1;
			size_t m = ToInt(str[1]) - 1;
			int year = YEAR_NOT_INIT;
			if (str.size() == 3)
				year = ToInt(str[2]);

			if (m >= 0 && m < 12 &&
				d >= 0 && d < GetNbDayPerMonth(year, m))
			{
				date = CTRef(year, m, d);
			}
		}

		return date;
	}

	//const char* CParameterVector::XMLFlagIn="ModelInput";
	//const char* CParameterVector::XMLFlagOut="ModelOutput";
	//const char* CParameterVector::XMLFlag="Parameter";


	//
	//void CParameterVector::GetXML(LPXNode& pRoot, short type)const
	//{
	//	XNode* pNode = NULL;
	//	if( pRoot == NULL )
	//	{
	//		pNode = pRoot = XNode::CreateNode( GetXMLFlag(type) );
	//	}
	//	else
	//	{
	//		pNode = pRoot->AppendChild( GetXMLFlag(type) );
	//	}
	//
	//	_ASSERTE( pNode );
	//	
	//	for(int i=0; i<(int)size(); i++)
	//	{
	//		LPXNode pVar = pNode->AppendChild(XMLFlag, at(i).GetString().c_str());
	//		pVar->AppendAttr("Name", at(i).GetName() );
	//		pVar->AppendAttr("Type", at(i).GetType() );
	//	}
	//}
	//
	//
	//void CParameterVector::SetXML(const LPXNode pRoot, short type)
	//{
	//	_ASSERTE( type>=0 && type<NB_TYPE);
	//
	//	clear();
	//
	//	LPXNode pNode = pRoot->Select(GetXMLFlag(type));
	//	if( pNode)
	//	{
	//		XNodes nodes = pNode->GetChilds(XMLFlag);
	//		if( nodes.size()==0)
	//			nodes = pNode->GetChilds("Parameters");//to be compatible with old BioSIM
	//		
	//		for(int i=0; i<(int)nodes.size(); i++)
	//		{
	//			CParameter param;
	//			param.SetName( nodes[i]->GetAttrValue("Name") );
	//			param.SetType( nodes[i]->GetAttrValue("Type") );
	//			param.SetString(nodes[i]->value);
	//			
	//			push_back(param);
	//		}
	//	}
	//}

	//************************************************************************
	//CLocation Class


	/*
	template<typename T>
	std::string toString(const T & val)
	{
	std::ostringstream ostr;
	ostr << val;
	return ostr.str();
	}

	template <class First, class Second>
	std::ostream & operator << (std::ostream & in, const std::pair<First,Second> & p)
	{
	return in<<p.first<<p.second;
	}

	std::ostream & operator<<(std::ostream & io, const std::map<std::string, std::string> & theMap)
	{
	//io << toString(theMap);
	io << theMap.size();
	for(std::map<std::string, std::string>::const_iterator it = theMap.begin(); it != theMap.end(); it++)
	{
	//const std::pair<string,string>& s = *it;
	io << *it;
	}

	return io;
	}

	//std::istream & operator>>(const std::map<std::string, std::string> & some_map, std::istream & stream)
	//{
	//    return stream;
	//}


	template <class First, class Second>
	std::istream & operator >> (std::istream & in, std::pair<First,Second> & p)
	{
	return in>>p.first>>p.second;
	}

	std::istream & operator>>(std::istream & io, std::map<std::string, std::string> & theMap)
	{
	size_t size=0;
	io >> size;
	for(size_t i=0; i<size; i++)
	{
	std::pair<string,string> s;
	io>>s;
	theMap.insert( s );
	}

	return io;
	}


	//"Slope", "Aspect",
	const char* CLocation::MEMBER_NAME[NB_MEMBER]={"Name", "ID", "Latitude", "Longitude", "Elevation", "SiteSpecificInformation" };
	const char* CLocation::DEFAULT_SSI_NAME[NB_DEFAULT_SSI]=
	{
	"OptionalID", "Slope", "Aspect", "Litoral1Distance","Litoral1Weight","Litoral2Distance", "Litoral2Weight",
	"Horizon1", "Horizon2", "Horizon3", "Horizon4", "Horizon5", "Horizon6", "Horizon7", "Horizon8"
	//WaterHoldingCapacity...
	};




	CLocation::CLocation (string name, string ID, double lat, double lon, double elev)
	{
	Reset();
	m_name = name;
	m_ID = ID;
	m_lat=lat;
	m_lon=lon;
	m_elev=elev;


	}

	void CLocation::Reset()
	{
	m_name.clear();
	m_ID.clear();
	m_lat=-999;
	m_lon=-999;
	m_elev=-999;
	m_siteSpeceficInformation.clear();
	//m_slope=0;
	//m_aspect=0;
	}

	bool CLocation::operator ==(const CLocation& in)const
	{
	bool bEqual=true;
	if( &in != this)
	{
	if( m_name != in.m_name)bEqual=false;
	if( m_ID != in.m_ID)bEqual=false;
	if( fabs(m_lat-in.m_lat)>0.00001 )bEqual=false;
	if( fabs(m_lon-in.m_lon)>0.00001)bEqual=false;
	if( fabs(m_elev-in.m_elev)>0.01)bEqual=false;
	//if( fabs(m_slope-in.m_slope)>0.01)bEqual=false;
	//if( fabs(m_aspect-in.m_aspect)>0.01)bEqual=false;
	if( !map_compare(m_siteSpeceficInformation, in.m_siteSpeceficInformation) )bEqual=false;
	}

	return bEqual;
	}

	istream& operator >> (istream& io, CLocation& station)
	{
	if( io )
	{
	station << io;
	}

	return io;
	}

	std::istream& CLocation::LoadV2(std::istream& io)
	{
	if( io )
	{
	char tmp[256]={0};
	io.ignore(1);
	io.get( tmp, 255, '"');
	io.ignore(1);

	m_name = tmp;
	io >> m_lat;
	io >> m_lon;
	io >> m_elev;
	float slope=0;
	float aspect=0;
	io >> slope;
	io >> aspect;
	SetDefaultSSI(SLOPE, ToString(slope) );
	SetDefaultSSI(ASPECT, ToString(aspect) );
	//take the line feed
	io.getline(tmp,_MAX_PATH);

	}

	return io;
	}


	//template<class Archive>
	// void save(Archive & ar, const unsigned int version) const
	// {
	//  ar & d1_ & d2_;
	// }
	// template<class Archive>
	// void load(Archive & ar, const unsigned int version)
	// {
	//   ar & d1_ & d2_;

	// }



	const char* CLocation::XML_FLAG = "Location";

	string CLocation::GetMember(int i, LPXNode& pNode)const
	{
	_ASSERTE(i>=0 && i<NB_MEMBER);

	string str;
	switch(i)
	{
	case NAME: str=m_name; break;
	case ID:   str=m_ID; break;
	case LAT:  str=ToString(m_lat); break;
	case LON:  str=ToString(m_lon); break;
	case ELEV: str=ToString(m_elev); break;
	case SITE_SPECIFIC_INFORMATION:
	{
	for(StringStringMap::const_iterator it=m_siteSpeceficInformation.begin(); it!=m_siteSpeceficInformation.end(); it++)
	str+="{" + it->first + "," + it->second + "}";
	break;
	}
	default: _ASSERTE(false);
	}

	return str;
	}


	void CLocation::SetMember(int i, const string& str, const LPXNode pNode)
	{
	_ASSERTE(i>=0 && i<NB_MEMBER);

	switch(i)
	{
	case NAME: m_name=str; break;
	case ID:  m_ID=str; break;
	case LAT: m_lat = ToDouble(str); break;
	case LON: m_lon = ToDouble(str); break;
	case ELEV: m_elev = ToDouble(str); break;
	case SITE_SPECIFIC_INFORMATION:
	{
	int pos=0;
	string tmp = str.Tokenize("}", pos); tmp.Trim();
	while( pos>=0)
	{
	if( !tmp.IsEmpty() )
	{
	tmp.Remove('{'); tmp.Remove('}');
	int pos2=tmp.Find(',',0);
	SetSSI( tmp.Mid(0,pos2), tmp.Mid(pos2+1));
	}

	tmp = str.Tokenize("}", pos);tmp.Trim();
	}
	break;
	}

	default: _ASSERTE(false);
	}
	}


	double CLocation::GetSlope()const
	{


	double slope=0;
	StringStringMap::const_iterator it = m_siteSpeceficInformation.find(GetDefaultSSIName(SLOPE));
	if( it != m_siteSpeceficInformation.end() )
	slope= ToDouble(it->second);

	return slope;
	}

	double CLocation::GetAspect()const
	{
	double aspect=0;
	StringStringMap::const_iterator it = m_siteSpeceficInformation.find(GetDefaultSSIName(ASPECT));
	if( it != m_siteSpeceficInformation.end() )
	aspect = ToDouble(it->second);

	_ASSERTE( aspect >= 0 && aspect <= 360);
	return aspect;
	}

	double CLocation::GetDayLength(short d)const
	{
	return GetDayLength(m_lat, d);
	}

	double CLocation::GetDayLength(CTRef d)const
	{
	return GetDayLength(m_lat, d);
	}

	double CLocation::GetAttPressure()const
	{
	return 101.3* pow( (293-0.0065*m_elev)/293, 5.26);//in kPa
	}

	void CLocation::SetDefaultSSI(int i, const std::string& SSI)
	{
	SetSSI(GetDefaultSSIName(i), SSI );
	}

	void CLocation::SetSSI(const std::string& name, const std::string& SSI)
	{
	m_siteSpeceficInformation[ name ] = SSI;
	}

	*/
	//************************************************************************
	//CMTGInput Class
	//
	//const char* CMTGInput::XML_FLAG = "TGInput";
	//const char* CMTGInput::MEMBER_NAME[NB_MEMBER] = {"Year", "NbYear", "NbNormalsStations", "NormalDBName", 
	//"NbDailyStations", "DailyDBName", "GradientType", "SeedType", "Category", "Xvalidation"};
	//
	//
	//
	//CMTGInput::CMTGInput()
	//{
	//	Reset();
	//}
	//void CMTGInput::Reset()
	//{
	//	m_lastYear=0;
	//	m_nbYear=0;
	//	m_nbNormalStation=0;
	//	m_nbDailyStation=0;
	//	m_category="T";
	//	m_bXValidation=false;
	//
	//	m_albedoType=CONIFER_CANOPY;
	//	m_seedType=RANDOM_SEED;
	//}
	//
	//bool CMTGInput::operator ==(const CMTGInput& in)const
	//{
	//	bool bEqual=true;
	//	if( &in != this)
	//	{
	//		if( m_lastYear!=in.m_lastYear)bEqual=false;
	//		if( m_nbYear!=in.m_nbYear)bEqual=false;
	//		if( m_nbNormalStation!=in.m_nbNormalStation)bEqual=false;
	//		if( m_nbDailyStation!=in.m_nbDailyStation)bEqual=false;
	//		if( m_category!=in.m_category)bEqual=false;
	//		if( m_bXValidation!=in.m_bXValidation)bEqual=false;
	//		if( m_albedoType!=in.m_albedoType)bEqual=false;
	//		if( m_seedType!=in.m_seedType)bEqual=false;
	//	}
	//
	//	return bEqual;
	//}
	//
	//istream& operator >> (istream& io, CMTGInput& TG)
	//{
	//    if( io )
	//    {
	//        TG << io;
	//    }
	//
	//    return io;
	//}
	//
	//std::istream& CMTGInput::operator << (std::istream& io)
	//{
	//    if( io )
	//    {
	//		short firstYear=0;
	//		io >> firstYear;
	//		io >> m_nbYear;
	//		//because if seem to have a confusion with the "year". In the old BioSIN it was the first year,
	//		// but with BioSIM 10 it's the last year. Then the lasYear was taken
	//		m_lastYear = firstYear + m_nbYear-1;
	//
	//		io >> m_nbNormalStation;
	//		io >> m_nbDailyStation;
	//		short elevationTolerance;
	//		io >> elevationTolerance;//for compatibility
	//		io >> m_albedoType;
	//		io >> m_seedType;
	//		
	//		char tmp[256]={0};
	//		io.get( tmp, 255, '"');
	//        io.ignore(1);
	//        io.get( tmp, 255, '"');
	//		io.clear();//if the name of the database is empty the flag needs to be cleared
	//        io.ignore(1);
	//		m_normalDBName = tmp;
	//	
	//		io.get( tmp, 255, '"');
	//		io.ignore(1);
	//		io.get( tmp, 255, '"');
	//		io.clear();//if the name of the database is empty the flag needs to be cleared
	//        io.ignore(1);
	//		m_dailyDBName = tmp;
	//
	//		int bSimPpt=0;
	//		io >> bSimPpt; 
	//		if( bSimPpt )
	//			m_category = "T P";
	//		//io >> test; m_bSimRad = test!=0;
	//
	//		//take the line feed
	//		io.getline(tmp,_MAX_PATH);
	//
	//    }
	//
	//    return io;
	//}
	//
	//
	//
	//
	////string CMTGInput::GetString(int i)const
	//string CMTGInput::GetMember(int i, LPXNode& pNode)const
	//{
	//	ASSERT(i>=0 && i<NB_MEMBER);
	//
	//	string str;
	//	switch(i)
	//	{
	//	case LAST_YEAR:			str = ToString(m_lastYear); break;
	//	case NB_YEAR:			str = ToString(m_nbYear); break;
	//	case NB_NORMAL_STATION:	str = ToString(m_nbNormalStation); break;
	//	case NORMAL_DB_NAME:	str = m_normalDBName; break;
	//	case NB_DAILY_STATION:	str = ToString(m_nbDailyStation); break;
	//	case DAILY_DB_NAME:		str = m_dailyDBName; break;
	//	case ALBEDO_TYPE:		str = ToString(m_albedoType); break;
	//	case SEED_TYPE:			str = ToString(m_seedType); break;
	//	case SIMULATED_CATEGORY:str = m_category.GetString(); break;
	//	case XVAL:				str = ToString(m_bXValidation); break;
	//
	//	default: ASSERT(false);
	//	}
	//
	//	return str;
	//}
	//
	////void CMTGInput::SetString(int i, const string& str)
	//void CMTGInput::SetMember(int i, const string& str, const LPXNode pNode)
	//{
	//	ASSERT(i>=0 && i<NB_MEMBER);
	//
	//	switch(i)
	//	{
	//	case LAST_YEAR:			m_lastYear = ToInt(str); break;
	//	case NB_YEAR:			m_nbYear= ToInt(str); break;
	//	case NB_NORMAL_STATION:	m_nbNormalStation= ToInt(str); break;
	//	case NORMAL_DB_NAME:	m_normalDBName=str; break;
	//	case NB_DAILY_STATION:	m_nbDailyStation= ToInt(str); break;
	//	case DAILY_DB_NAME:		m_dailyDBName=str; break;
	//	case ALBEDO_TYPE:		m_albedoType= ToInt(str); break;
	//	case SEED_TYPE:			m_seedType= ToInt(str); break;
	//	case SIMULATED_CATEGORY:m_category.SetString(str); break;
	//	case XVAL:				m_bXValidation = ToBool(str); break;
	//	default: ASSERT(false);
	//	}
	//}


	//************************************************************************
	//CCounter

	CCounter::CCounter(size_t no, size_t total)
	{
		m_no = no;
		m_total = total;
	}

	void CCounter::Reset()
	{
		m_no = 0;
		m_total = 0;
	}
	bool CCounter::operator ==(const CCounter& in)const
	{
		bool bEqual = true;
		if (&in != this)
		{
			if (m_no != in.m_no)bEqual = false;
			if (m_total != in.m_total)bEqual = false;
		}

		return bEqual;
	}

	istream& operator >> (istream& io, CCounter& count)
	{
		if (io)
		{
			count << io;
		}

		return io;
	}

	std::istream& CCounter::operator << (std::istream& io)
	{
		if (io)
		{
			io >> m_no;
			io >> m_total;

			//take the line feed
			char tmp[_MAX_PATH] = { 0 };
			io.getline(tmp, _MAX_PATH);

		}

		_ASSERTE(m_no >= 0 && m_no < m_total);
		return io;
	}


	//*****************************************************************************************
	CMonthDay::CMonthDay(size_t m, size_t d, size_t h)
	{
		m_month = m;
		m_day = d;
		m_hour = h;
	}

	void CMonthDay::Set(const std::string& date, const std::string& delimiter)
	{
		std::vector<int> tmp;
		std::stringstream ss(date);

		int i = 0;
		while (ss >> i)
		{
			tmp.push_back(i);

			//if (ss.peek() == ',')
			if (delimiter.find_first_of(ss.peek()) != string::npos)
				ss.ignore();
		}

		m_month = UNKNOWN_POS;
		m_day = UNKNOWN_POS;
		m_hour = UNKNOWN_POS;

		if (tmp.size() == 2)
		{
			m_month = tmp[0] - 1;
			m_day = tmp[1] - 1;
		}
		else if (tmp.size() == 3)
		{
			m_month = tmp[0] - 1;
			m_day = tmp[1] - 1;
			m_hour = tmp[2];
		}

		ASSERT(m_month == UNKNOWN_POS || m_month < 12);
		ASSERT(m_day == UNKNOWN_POS || m_day < 31);
		ASSERT(m_hour == UNKNOWN_POS || m_hour < 24);
	}

	string CMonthDay::Get(const string& delimiter)const
	{
		ASSERT(IsValid());

		std::vector<int> tmp;
		tmp.push_back(int(m_month) + 1);
		tmp.push_back(int(m_day) + 1);
		if (m_hour != NOT_INIT)
			tmp.push_back(int(m_hour));

		std::stringstream ss;
		copy(tmp.begin(), tmp.end(), ostream_iterator<int>(ss, delimiter.c_str()));

		return ss.str();
	}

	bool CMonthDay::IsValid()const
	{
		return m_month < 12 && m_day < GetNbDayPerMonth(m_month) && (m_hour==NOT_INIT||m_hour<24);
	}
}