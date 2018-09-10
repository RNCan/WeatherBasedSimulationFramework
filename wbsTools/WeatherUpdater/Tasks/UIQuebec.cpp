#include "StdAfx.h"
#include "UIQuebec.h"

#include "basic/WeatherStation.h"
#include "basic/CSV.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"
#include "json\json11.hpp"
#include "Geomatic/TimeZones.h"
#include "mosa.h"


using namespace WBSF::HOURLY_DATA;
using namespace Mosa;
using namespace std;
using namespace UtilWWW;
using namespace json11;


namespace WBSF
{

	//*********************************************************************

	static const DWORD FLAGS = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
	const char* CUIQuebec::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Network", "DataType", "UpdateUntil", "UpdateStationsList", "UserNameSOPFEU", "PasswordSOPFEU", "UserNameMFFP", "PasswordMFFP" };
	const size_t CUIQuebec::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_COMBO_INDEX, T_STRING, T_BOOL, T_STRING, T_PASSWORD, T_STRING, T_PASSWORD };
	const UINT CUIQuebec::ATTRIBUTE_TITLE_ID = IDS_UPDATER_QUEBEC_P;
	const UINT CUIQuebec::DESCRIPTION_TITLE_ID = ID_TASK_QUEBEC;

	const char* CUIQuebec::CLASS_NAME() { static const char* THE_CLASS_NAME = "Quebec";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIQuebec::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIQuebec::CLASS_NAME(), (createF)CUIQuebec::create);

	const char* CUIQuebec::SERVER_NAME[NB_NETWORKS] = { "FTP3.sopfeu.qc.ca", "www.mddelcc.gouv.qc.ca", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org", "horus.mesonet-quebec.org" };
	const char* CUIQuebec::NETWORK_NAME[NB_NETWORKS] = { "SOPFEU", "MDDELCC", "HYDRO", "MFFP", "ALCAN", "FADQ", "SM" };


	std::bitset<CUIQuebec::NB_NETWORKS> CUIQuebec::GetNetwork()const
	{
		StringVector networkV(Get(NETWORK), "|;,");
		bitset<NB_NETWORKS> networks;

		for (size_t n = 0; n < NB_NETWORKS; n++)
			if (networkV.empty() || networkV.Find(ToString(NETWORK_NAME[n])) != NOT_INIT)
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
		case NETWORK: str = "SOPFEU=SOPFEU|MDDELCC=MDDELCC (daily)|HYDRO=Hydro-Quebec|MFFP=MFFP|ALCAN=Alcan|FADQ=Financière Agricole|SM=Solution-Mesonet"; break;
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
		case UPDATE_UNTIL: str = "7"; break;
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
		
		callback.PushTask(GetString(IDS_UPDATE_FILE) + " Quebec " + (dataType==HOURLY_WEATHER?"hourly":"daily") + " (" + to_string(network.count()) + ")", network.count());

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n])
			{
				switch (n)
				{
				case SOPFEU:  InitSOPFEU(m_SOPFEU); msg += m_SOPFEU.Execute(callback); break;
				case MDDELCC: InitMDDELCC(m_MDDELCC); msg += m_MDDELCC.Execute(callback); break;
				case MFFP:    if (InitMFFP(m_MFFP)) { msg += m_MFFP.Execute(callback); break; }
				default: Init(n); msg += Execute(n, callback);
				}

				msg += callback.StepIt();
			}
			
			if (callback.GetUserCancel())
				break;
		}

		callback.PopTask();
		return msg;
	}


	ERMsg CUIQuebec::GetStationList(size_t n, StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		string filePath = GetApplicationPath() + "Layers\\QuebecStations.csv";
		msg = m_stations.Load(filePath);

		for (size_t i = 0; i < m_stations.size(); i++)
		{
			if (m_stations[i].GetSSI("Network") == NETWORK_NAME[n])
			{
				stationList.push_back(m_stations[i].m_ID);
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
			if (network[n])
			{
				StringVector station;

				switch (n)
				{
				case SOPFEU: InitSOPFEU(m_SOPFEU); msg += m_SOPFEU.GetStationList(station, callback); break;
				case MDDELCC:InitMDDELCC(m_MDDELCC);  msg += m_MDDELCC.GetStationList(station, callback); break;
				case MFFP:    if (InitMFFP(m_MFFP)) { msg += m_MFFP.GetStationList(station, callback); break; }
				default: Init(n); msg += GetStationList(n, station, callback); break;
				}

				if (msg)
				{
					stationList.reserve(stationList.size() + station.size());
					for (StringVector::const_iterator it = station.begin(); it != station.end(); it++)
						stationList.push_back(ToString(n) + *it);
				}
			}
		}

		
		return msg;
	}


	ERMsg CUIQuebec::Execute(size_t n, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		size_t dataType = as<size_t>(DATA_TYPE);
		if (dataType == HOURLY_WEATHER)
		{
			callback.AddMessage(GetString(IDS_UPDATE_DIR)); 
			callback.AddMessage(workingDir, 1);
			callback.AddMessage("");

			string userName = Get(USER_NAME_MFFP);
			string password = Get(PASSWORD_MFFP);
			int firstYear = as<int>(FIRST_YEAR);
			int lastYear = as<int>(LAST_YEAR);
			int updateUntil = as<int>(UPDATE_UNTIL);
			msg = ExecuteFTP(workingDir, n, userName, password, firstYear, lastYear, updateUntil, callback);
		}

		return msg;
	}

	string CUIQuebec::GetWorkingDir(size_t n)const
	{
		return GetDir(WORKING_DIR) + NETWORK_NAME[n] + "\\";
	}

	string CUIQuebec::GetOutputFilePath(size_t n, string id, int year)const
	{
		return GetWorkingDir(n) + ToString(year) + "\\" + id + ".csv";
	}

	void CUIQuebec::Init(size_t n)
	{
		m_firstYear = as<int>(FIRST_YEAR);
		m_lastYear = as<int>(LAST_YEAR);
		
		
	}

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

	bool CUIQuebec::InitMFFP(CMFFP& obj)const
	{
		obj.m_workingDir = GetDir(WORKING_DIR) + "MFFP\\";
		obj.m_userName = Get(USER_NAME_MFFP);
		obj.m_password = Get(PASSWORD_MFFP);
		obj.m_firstYear = as<int>(FIRST_YEAR);
		obj.m_lastYear = as<int>(LAST_YEAR);

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		return GetFtpConnection(SERVER_NAME[MFFP], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, obj.m_userName, obj.m_password, true, 2);
	}

	ERMsg CUIQuebec::GetWeatherStation(const string& name, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		size_t n = WBSF::as<size_t>(name.substr(0, 1));
		string ID = name.substr(1);

		switch (n)
		{
		case SOPFEU: msg += m_SOPFEU.GetWeatherStation(ID, TM, station, callback); break;
		case MDDELCC:  msg += m_MDDELCC.GetWeatherStation(ID, TM, station, callback); break;
		case MFFP:    if (InitMFFP(m_MFFP)) { msg += m_MFFP.GetWeatherStation(ID, TM, station, callback); break; }
		default:  msg += GetWeatherStation(n, ID, TM, station, callback); break;
		}

		return msg;
	}

	ERMsg CUIQuebec::GetWeatherStation(size_t n, const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		//string workingDir = GetWorkingDir(n);

		size_t pos = m_stations.FindByID(ID);
		ASSERT(pos != NOT_INIT);

		((CLocation&)station) = m_stations[pos];
		station.m_name = PurgeFileName(station.m_name);

		station.SetHourly(true);

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(n, ID, year);
			if (FileExists(filePath))
				msg = station.LoadData(filePath, -999, false);

			msg += callback.StepIt(0);
		}

		station.CleanUnusedVariable("TN T TX P TD H WS WD R Z W2");

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}


		return msg;
	}
}

