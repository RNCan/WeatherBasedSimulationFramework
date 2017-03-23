#include "StdAfx.h"
#include "UIQuebec.h"
#include "SOPFEU.h"
#include "MDDELCC.h"

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

	//MesoWest
	//http://gl1.chpc.utah.edu/cgi-bin/droman/stn_state.cgi?state=QC&order=status
	//http://mesowest.utah.edu/cgi-bin/droman/raws_ca_monitor.cgi?state=QC&rawsflag=3
	//http://mesowest.utah.edu/cgi-bin/droman/download_api2.cgi?stn=CWZS&year1=2017&day1=23&month1=3&hour1=1&timetype=GMT&unit=0


	//http://www.climat-quebec.qc.ca/htdocs/data_dyna/xml_data/coord_stations_svi_cour_car_private.txt



	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
	const char* CUIQuebec::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "FirstYear", "LastYear", "Network", "DataType", "UpdateUntil", "UpdateStationsList" };
	const size_t CUIQuebec::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_COMBO_INDEX, T_STRING, T_BOOL };
	const UINT CUIQuebec::ATTRIBUTE_TITLE_ID = IDS_UPDATER_QUEBEC_P;
	const UINT CUIQuebec::DESCRIPTION_TITLE_ID = ID_TASK_QUEBEC;

	const char* CUIQuebec::CLASS_NAME(){ static const char* THE_CLASS_NAME = "Quebec";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIQuebec::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIQuebec::CLASS_NAME(), (createF)CUIQuebec::create);

	
	//SOPFEU, MDDELCC, HYDRO, MFFP, ALCAN, AGRI, METEO_CENTRE, NB_NETWORKS};
	const char* CUIQuebec::SERVER_NAME[NB_NETWORKS] = { "FTP3.sopfeu.qc.ca", "www.mddelcc.gouv.qc.ca", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org"};//, "meteocentre.com" 
	const char* CUIQuebec::NETWORK_NAME[NB_NETWORKS] = { "SOPFEU", "MDDELCC", "HYDRO", "MFFP", "ALCAN", "AGRI"};//, "METEO_CENTRE" 

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
		case NETWORK: str = "0=SOPFEU (hourly)|1=MDDELCC (daily)|2=Hydro-Quebec|3=MFFP|4=ALCAN|5=AGRI"; break;
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

	string CUIQuebec::GetStationListFilePath(size_t n)
	{
		string filePath;

		string workingDir = GetDir(WORKING_DIR);

		switch (n)
		{
		case SOPFEU:  filePath = workingDir + "SOPFEU\\DailyStationsList.csv"; break;
		case MDDELCC: filePath = GetApplicationPath() + "Layers\\SOPFEUStations.csv"; break;
		case HYDRO:
		case MFFP:
		case ALCAN:
		case AGRI: break;
		}

		return filePath;
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
				case SOPFEU:
				{
					if (dataType == HOURLY_WEATHER)
					{
						CSOPFEU obj;
						obj.m_workingDir = workingDir + "SOPFEU\\";
						obj.m_firstYear = as<int>(FIRST_YEAR);
						obj.m_lastYear = as<int>(LAST_YEAR);
						obj.m_userName = Get(USER_NAME);
						obj.m_password = Get(PASSWORD);
						msg += obj.Execute(callback);
						break;
					}
				}
				case MDDELCC:
				{
					if (dataType == DAILY_WEATHER)
					{
						CMDDELCC obj;
						obj.m_workingDir = workingDir + "MDDELCC\\";
						obj.m_firstYear = as<int>(FIRST_YEAR);
						obj.m_lastYear = as<int>(LAST_YEAR);
						obj.m_updateUntil = as<int>(UPDATE_UNTIL);
						obj.bForceUpdateList = as<bool>(UPDATE_STATIONS_LIST);

						msg += obj.Execute(callback);
						break;
					}
				}
				case HYDRO:
				case MFFP:
				case ALCAN:
				case AGRI:break;
				}

			}
		}
		
		return msg;
	}

	ERMsg CUIQuebec::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		bitset<NB_NETWORKS> network = GetNetwork();

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n])
			{
				CLocationVector stations;
				msg = stations.Load(GetStationListFilePath(n));

				if (msg)
				{
					for (CLocationVector::const_iterator it = m_stations.begin(); it != m_stations.end(); it++)
					{
						if (!it->m_ID.empty() && it->m_ID[0] != '0')
							stationList.push_back(it->m_ID);
					}
				}
			}
		}


		return msg;
	}


	ERMsg CUIQuebec::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		bitset<NB_NETWORKS> network = GetNetwork();


		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n])
			{
				callback.AddMessage(GetString(IDS_UPDATE_DIR));
				callback.AddMessage(workingDir, 1);
				callback.AddMessage(GetString(IDS_UPDATE_FROM));
				callback.AddMessage(SERVER_NAME[n], 1);
				callback.AddMessage("");

				switch (n)
				{
				case SOPFEU:
				{
					CSOPFEU obj;
					obj.m_workingDir = workingDir + "SOPFEU\\";
					obj.m_firstYear = as<int>(FIRST_YEAR);
					obj.m_lastYear = as<int>(LAST_YEAR);
					msg += obj.GetWeatherStation(ID, TM, station, callback);
					break;
				}
				case MDDELCC:
				{
					CMDDELCC obj;
					obj.m_workingDir = workingDir + "MDDELCC\\";
					obj.m_firstYear = as<int>(FIRST_YEAR);
					obj.m_lastYear = as<int>(LAST_YEAR);
					msg += obj.GetWeatherStation(ID, TM, station, callback);
					break;
				}
				case HYDRO:
				case MFFP:
				case ALCAN:
				case AGRI:break;
				}
			}
		}

		return msg;
	}
	
}

