#include "StdAfx.h"
#include "UIQuebec.h"

#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

namespace WBSF
{


	//meteocentre
	//http://meteocentre.com/cgi-bin/get_sao_tab?STN=CXSH&LANG=fr&DELT=24
	//http://meteocentre.com/cgi-bin/get_sao_tab?STN=CYKO&LANG=fr&DELT=150

	//IEM
	//https://mesonet.agron.iastate.edu/sites/obhistory.php?month=8&year=2016&day=22&network=CA_QC_ASOS&station=CWBS



	//http://www.climat-quebec.qc.ca/htdocs/data_dyna/xml_data/coord_stations_svi_cour_car_private.txt



	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
	const char* CUIQuebec::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Network", "DataType", "UpdateUntil", "UpdateStationsList", "UserNameSOPFEU", "PasswordSOPFEU", "UserNameMFFP", "PasswordMFFP" };
	const size_t CUIQuebec::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_COMBO_INDEX, T_STRING, T_BOOL, T_STRING, T_PASSWORD, T_STRING, T_PASSWORD};
	const UINT CUIQuebec::ATTRIBUTE_TITLE_ID = IDS_UPDATER_QUEBEC_P;
	const UINT CUIQuebec::DESCRIPTION_TITLE_ID = ID_TASK_QUEBEC;

	const char* CUIQuebec::CLASS_NAME(){ static const char* THE_CLASS_NAME = "Quebec";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIQuebec::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIQuebec::CLASS_NAME(), (createF)CUIQuebec::create);

	
	//SOPFEU, MDDELCC, HYDRO, MFFP, ALCAN, AGRI, METEO_CENTRE, NB_NETWORKS};
	const char* CUIQuebec::SERVER_NAME[NB_NETWORKS] = { "FTP3.sopfeu.qc.ca", "www.mddelcc.gouv.qc.ca", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org"};
	const char* CUIQuebec::NETWORK_NAME[NB_NETWORKS] = { "SOPFEU", "MDDELCC", "HYDRO", "MFFP", "ALCAN", "FADQ"};//, "METEO_CENTRE" 

	//size_t CUIQuebec::GetNetwork(const string& network)
	//{
	//	size_t n = NOT_INIT;

	//	for (size_t i = 0; i <NB_NETWORKS && n == NOT_INIT; i++)
	//	{
	//		if (IsEqualNoCase(network, NETWORK_NAME[i]))
	//			n = i;
	//	}

	//	return n;
	//}

	std::bitset<CUIQuebec::NB_NETWORKS> CUIQuebec::GetNetwork()const
	{
		StringVector networkV(Get(NETWORK), "|");
		bitset<NB_NETWORKS> networks;

		for (size_t n = 0; n < NB_NETWORKS; n++)
			if (networkV.empty() || networkV.Find(ToString(n)) != NOT_INIT)
				networks.set(n);

		return networks;
	}




	CUIQuebec::CUIQuebec(void)
	{}

	CUIQuebec::~CUIQuebec(void)
	{}


	std::string CUIQuebec::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORK: str = "0=SOPFEU|1=MDDELCC (daily)|2=Hydro-Quebec|3=MFFP|4=ALCAN|5=Financière Agricole"; break;
		case DATA_TYPE: str = "Hourly|Daily"; break;
		};

		return str;
	}

	std::string CUIQuebec::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Quebec\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case UPDATE_UNTIL: str = "15"; break;
		case UPDATE_STATIONS_LIST: str = "0"; break;
		};

		return str;
	}
	
	//*************************************************************************************************

	ERMsg CUIQuebec::Execute(CCallback& callback)
	{
		ERMsg msg;

		



		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);
		
		bitset<NB_NETWORKS> network = GetNetwork();
		size_t dataType = as<size_t>(DATA_TYPE);

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n])
			{
				switch (n)
				{
				case SOPFEU:  InitSOPFEU(m_SOPFEU); msg += m_SOPFEU.Execute(callback); break;
				case MDDELCC: InitMDDELCC(m_MDDELCC); msg += m_MDDELCC.Execute(callback); break;
				case HYDRO:
				case MFFP:	InitMFFP(m_MFFP); msg += m_MFFP.Execute(callback); break;
				case ALCAN:
				case FADQ:break;
				}

			}
		}
		
		return msg;
	}

	ERMsg CUIQuebec::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		bitset<NB_NETWORKS> network = GetNetwork();
		size_t dataType = as<size_t>(DATA_TYPE);

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n] )
			{
				StringVector station;

				switch (n)
				{
				case SOPFEU: InitSOPFEU(m_SOPFEU); msg += m_SOPFEU.GetStationList(station, callback); break;
				case MDDELCC:InitMDDELCC(m_MDDELCC);  msg += m_MDDELCC.GetStationList(station, callback); break;
				case HYDRO:
				case MFFP:	InitMFFP(m_MFFP);  msg += m_MFFP.GetStationList(station, callback); break;
				case ALCAN:
				case FADQ:break;
				}

				if (msg)
				{
					stationList.reserve(stationList.size() + station.size());
					for (StringVector::const_iterator it = station.begin(); it != station.end(); it++)
						stationList.push_back(	ToString(n) + *it);
				}
			}
		}


		return msg;
	}
	//
	void CUIQuebec::InitSOPFEU(CSOPFEU& obj)const
	{
		obj.m_workingDir = GetDir(WORKING_DIR) + "SOPFEU\\";
		obj.m_userName = Get(USER_NAME_SOPFEU);
		obj.m_password = Get(PASSWORD_SOPFEU);
		obj.m_firstYear = as<int>(FIRST_YEAR);
		obj.m_lastYear = as<int>(LAST_YEAR);
	}

	void CUIQuebec::InitMDDELCC(CMDDELCC& obj)const
	{
		obj.m_workingDir = GetDir(WORKING_DIR) + "MDDELCC\\";
		obj.m_firstYear = as<int>(FIRST_YEAR);
		obj.m_lastYear = as<int>(LAST_YEAR);
		obj.bForceUpdateList = as<bool>(UPDATE_STATIONS_LIST);
		obj.m_updateUntil = as<int>(UPDATE_UNTIL);
	}

	void CUIQuebec::InitMFFP(CMFFP& obj)const
	{
		obj.m_workingDir = GetDir(WORKING_DIR) + "MFFP\\";
		obj.m_userName = Get(USER_NAME_MFFP);
		obj.m_password = Get(PASSWORD_MFFP);
		obj.m_firstYear = as<int>(FIRST_YEAR);
		obj.m_lastYear = as<int>(LAST_YEAR);
	}

	ERMsg CUIQuebec::GetWeatherStation(const string& name, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		//bitset<NB_NETWORKS> network = GetNetwork();

		size_t n = WBSF::as<size_t>(name.substr(0,1));
		string ID = name.substr(1);
		
		switch (n)
		{
		case SOPFEU: msg += m_SOPFEU.GetWeatherStation(ID, TM, station, callback); break;
		case MDDELCC:  msg += m_MDDELCC.GetWeatherStation(ID, TM, station, callback); break;
		case HYDRO:
		case MFFP:	msg += m_MFFP.GetWeatherStation(ID, TM, station, callback); break;
		case ALCAN:
		case FADQ:  break;
		}

		return msg;
	}
	
}

