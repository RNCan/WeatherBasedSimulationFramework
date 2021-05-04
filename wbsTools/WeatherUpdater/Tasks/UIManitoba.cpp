#include "StdAfx.h"
#include "UIManitoba.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>


#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "Basic/ExtractLocationInfo.h"
#include "Basic/CSV.h"
#include "Basic\json\json11.hpp"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/UtilWin.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;
using namespace json11;

//agriculture
//https://web43.gov.mb.ca/climate/HourlyReport.aspx


//mbpotatoes
//http://www.mbpotatoes.ca/weather.cfm?stn=325


//plus de donn…es sur:
//ftp://mawpvs.dyndns.org/Partners/AI/
//ftp://mawpvs.dyndns.org/DailySummary/



////HG - water level
//TW - water temperature
//TA - air temperature
//UD - wind direction
//US - wind speed
//UG - wind gust
//PC - precipitation
//XR - relative humidity
//PA - atmospheric pressure
//https://www.hydro.mb.ca/hydrologicalData/static/

//liste des stations
//https ://www.hydro.mb.ca/hydrologicalData/static/data/stationdata.json?

//coordonner des stations
//https://www.hydro.mb.ca/hydrologicalData/static/stations/05TG746/station.html?



//<table border="0"  cellspacing="0" cellpadding="0">
//</table>

//https://www.hydro.mb.ca/hydrologicalData/static/stations/05TG746/Parameter/HG/ContinuousWeek.xls
//https ://www.hydro.mb.ca/hydrologicalData/static/stations/05TG746/station.html?



//https://www.hydro.mb.ca/hydrologicalData/static/stations/05KL701/Parameter/TA/DayMeanYear.html
//https://www.hydro.mb.ca/hydrologicalData/static/stations/05KL701/Parameter/TA/DayMeanYear.html
//https://www.hydro.mb.ca/hydrologicalData/static/data/graph.json?v=20160613054508
//view-source:https://www.hydro.mb.ca/hydrologicalData/static/stations/05KL701/station.html?v=20160513054508
//https://www.hydro.mb.ca/hydrologicalData/static/

//Table60ElementPos	Table60ElementName	Table60ElementDesc	Table60ElementUnits
//1	TMSTAMP	time stamp of record at interval completion	CDST "yyyy-mm-dd hh:mm"
//2	RECNBR	seq table record number from prog change / start	int
//3	StnID	unique 3 digit Station Identifier for MAWP	int
//4	BatMin	minimum battery voltage prev 1 hour	volts(v)
//5	Air_T	avg of 5 sec temp vals prev 1 min	deg C(∞C)
//6	AvgAir_T	avg of 5 sec temp vals prev 1 hour	deg C(∞C)
//7	MaxAir_T	maximum of 1 min avg temp vals prev 1 hour	deg C(∞C)
//8	MinAir_T	minimum of 1 min avg temp vals prev 1 hour	deg C(∞C)
//9	RH	avg of 5 sec RH vals prev 1 min	% RH(%)
//10	AvgRH	avg of 5 sec RH vals prev 1 hour	% RH(%)
//11	Rain	total rain measured prev 1 hour(Primary gauge)	millimetres(m.m.)
//12	Rain24RT	total rain running total since midnight CDST	millimetres(m.m.)
//13	WS_10Min	scalar mean wind speed prev 10 min	metres per second(m / s)
//14	WD_10Min	derived vector wind dir prev 10 min	degrees true (∞)
//15	AvgWS	scalar mean wind speed prev 1 hour	metres per second(m / s)
//16	AvgWD	derived vector wind direction prev 1 hour	degrees true (∞)
//17	AvgSD	derived std deviation of vector dir prev 1 hour	degrees true (∞)
//18	MaxWS_10	maximum 10 min mean wind speed prev 1 hour	metres per second(m / s)
//19	MaxWD_10	derived vector wind direction for max 10min mean	degrees true (∞)
//20	MaxWS	maximum wind speed of 5 sec vals prev 1 hour	metres per second(m / s)
//21	HmMaxWS	timestamp of occurance of MaxWS	CDST "yyyy-mm-dd hh:mm:ss.msms"
//22	MaxWD	wind direction at MaxWS	degrees true (∞)
//23	Max5WS_10	maximum wind speed of 5 sec vals prev 10 min	metres per second(m / s)
//24	Max5WD_10	wind direction at Max5WS_10	degrees true (∞)
//25	WS_2Min	scalar mean wind speed prev 2 minutes	metres per second(m / s)
//26	WD_2Min	derived vector wind direction prev 2 minutes	degrees true (∞)
//27	Soil_T05	avg of 5 sec temp vals prev 1 min	deg C(∞C)
//28	AvgRS_kw	avg hourly solar flux density prev 1 hour	KW / m≤
//29	TotRS_MJ	total hourly solar fluxes prev 1 hour	MJ / m≤
//30	Rain2	total rain measured prev 1 hour(Secondary gauge)	millimetres(m.m.)
//31	Rain24RT2	total rain running total since midnight CDST(Secondary gauge)	millimetres(m.m.)




//Table24ElementPos	Table24ElementName	Table24ElementDesc	Table24ElementUnits
//1	TMSTAMP	time stamp of record at interval completion	CDST("yyyy-mm-dd hh:mm" - 1)
//2	RECNBR	sequential tabrecord number from prog change / start	int
//3	StnID	unique 3 digit Station Identifier for MAWP	int
//4	BatMin	minimum battery voltage prev 24 hours	volts(v)
//5	ProgSig	set by logger, changes if prog changes	int
//6	AvgAir_T	avg of prev 24 hours of 1 min avg's	deg C (∞C)
//7	MaxAir_T	max of prev 24 hours of 1 min avg's	deg C (∞C)
//8	HmMaxAir_T	timestamp of occurance of max temp	CDST "yyyy-mm-dd hh:mm:ss.msms"
//9	MinAir_T	min of prev 24 hours of 1 min avg's	deg C (∞C)
//10	HmMinAir_T	timestamp of occurance of min temp	CDST "yyyy-mm-dd hh:mm:ss.msms"
//11	AvgRH	avg of prev 24 hours of 1 min avg's	% RH (%)
//12	MaxRH	maximum of prev 24 hours of 1 min avg's	% RH (%)
//13	HmMaxRH	timestamp of occurance of max RH	CDST "yyyy-mm-dd hh:mm:ss.msms"
//14	MinRH	minimum of prev 24 hours of 1 min avg's	% RH (%)
//15	HmMinRH	timestamp of occurance of min RH	CDST "yyyy-mm-dd hh:mm:ss.msms"
//16	Rain	total rain measured prev 24 hours(primary)	millimetres(m.m.)
//17	MaxWS	max of 5 sec vals prev 24 hours	metres per second(m / s)
//18	HmMaxWS	timestamp of occurance of max wind	CDST "yyyy-mm-dd hh:mm:ss.msms"
//19	AvgWS	scalar mean wind speed prev 24 hours	metres per second(m / s)
//20	AvgWD	derived vector wind direction prev 24 hours	degrees true (∞)
//21	AvgSD	derived std deviation of vector dir prev 24 hours	degrees true (∞)
//22	AvgSoil_T05	avg of 5 sec vals prev 24 hours	deg C(∞C)
//23	MaxSoil_T05	maximum of 5 sec vals prev 24 hours	deg C(∞C)
//24	MinSoil_T05	minimum of 5 sec vals prev 24 hours	deg C(∞C)
//25	AvgRS_kw	avg hourly solar flux density	KW / m≤
//26	MaxRS_kw	maximum hourly solar flux density	KW / m≤
//27	HmMaxRS	timestamp of maximum hourly solar flux density	CDST "yyyy-mm-dd hh:mm:ss.msms"
//28	TotRS_MJ	total solar fluxes previous 24 hours	MJ / m≤
//29	Rain2	total rain measured prev 24 hours(secondary)	millimetres(m.m.)


namespace WBSF
{

	const char* CUIManitoba::SUBDIR_NAME[NB_NETWORKS] = { "Agriculture", "Agriculture2", "Fire", "Hydro", "Potato" };
	const char* CUIManitoba::NETWORK_NAME[NB_NETWORKS] = { "Manitoba Agriculture", "Manitoba Agriculture2", "Manitoba Fire", "Manitoba Hydro", "Manitoba Potatoes" };
	const char* CUIManitoba::PROVIDER_NAME[NB_NETWORKS] = { "Manitoba", "Manitoba", "Manitoba", "Manitoba Hydro", "Manitoba Potatoes" };

	const char* CUIManitoba::NETWORK_ABVR[NB_NETWORKS] = { "Agri", "Agri2", "Fire", "Hydro", "Potato" };
	const char* CUIManitoba::SERVER_NAME[NB_NETWORKS] = { "web43.gov.mb.ca", "mbagweather.ca", "www.gov.mb.ca", "www.hydro.mb.ca", "www.mbpotatoes.ca" };
	const char* CUIManitoba::SERVER_PATH[NB_NETWORKS] = { "climate/", "partners/CanAg/", "sd/fire/Wx-Display/weatherview/data/", "hydrologicalData/static/stations/", "/" };

	size_t CUIManitoba::GetNetwork(const string& network)
	{
		size_t n = NOT_INIT;

		for (size_t i = 0; i < NB_NETWORKS && n == NOT_INIT; i++)
		{
			if (IsEqualNoCase(network, NETWORK_ABVR[i]))
				n = i;
		}

		return n;
	}

	std::bitset<CUIManitoba::NB_NETWORKS> CUIManitoba::GetNetwork()const
	{
		std::bitset<NB_NETWORKS> network;

		StringVector str(Get(NETWORK), "|;,");
		if (str.empty())
		{
			network.set();
		}
		else
		{
			for (size_t i = 0; i < str.size(); i++)
			{
				size_t n = GetNetwork(str[i]);
				if (n < NB_NETWORKS)
					network.set(n);
			}
		}

		return network;
	}

	//*********************************************************************
	const char* CUIManitoba::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Network", "DataType" };
	const size_t CUIManitoba::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_COMBO_INDEX };
	const UINT CUIManitoba::ATTRIBUTE_TITLE_ID = IDS_UPDATER_MANITOBA_P;
	const UINT CUIManitoba::DESCRIPTION_TITLE_ID = ID_TASK_MANITOBA;

	const char* CUIManitoba::CLASS_NAME() { static const char* THE_CLASS_NAME = "Manitoba";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIManitoba::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIManitoba::CLASS_NAME(), (createF)CUIManitoba::create);



	CUIManitoba::CUIManitoba(void)
	{}

	CUIManitoba::~CUIManitoba(void)
	{}



	std::string CUIManitoba::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case NETWORK:	str = "Agri=Manitoba Agriculture|Agri2=Manitoba Agriculture (more stations, daily only)|Fire=Manitoba fire (hourly only)|Hydro=Manitoba Hydro|Potato=Manitoba Potatoes (daily only)"; break;
		case DATA_TYPE:	str = GetString(IDS_STR_DATA_TYPE); break;
		};
		return str;
	}

	std::string CUIManitoba::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Manitoba\\"; break;
		case NETWORK:	str = "Agri|Agri2|Fire|Hydro"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		case DATA_TYPE: str = "0"; break;
		};

		return str;
	}


	//****************************************************


	std::string CUIManitoba::GetStationsListFilePath(size_t network)const
	{
		static const char* FILE_NAME[NB_NETWORKS] = { "ManitobaAgriStations.csv", "ManitobaAgri2Stations.csv", "ManitobaFireStations.csv", "ManitobaHydroStations.csv",  "ManitobaPotatoStations.csv" };

		string path = network == FIRE ? GetDir(WORKING_DIR) + SUBDIR_NAME[network] + "\\" : WBSF::GetApplicationPath() + "Layers\\";
		string filePath = path + FILE_NAME[network];

		return filePath;
	}

	string CUIManitoba::GetOutputFilePath(size_t network, size_t type, const string& title, int year, size_t m)const
	{
		string strType = (type == HOURLY_WEATHER) ? "Hourly" : "Daily";

		string ext = ".csv";
		if (network == AGRI2)
		{
			ext = "";
			if (year == -999)
				year = stoi(title.substr(0, 4));
		}


		return GetDir(WORKING_DIR) + SUBDIR_NAME[network] + "\\" + strType + "\\" + ToString(year) + "\\" + (m != NOT_INIT ? FormatA("%02d\\", m + 1).c_str() : "") + title + ext;
	}


	ERMsg CUIManitoba::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);
		std::bitset<NB_NETWORKS> network = GetNetwork();

		string tstr = type == HOURLY_WEATHER ? "hourly" : "daily";
		callback.PushTask("Download " + tstr + " Manitoba Data (" + ToString(network.count()) + " networks)", network.count());

		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network[n])
			{
				string workingDir = GetDir(WORKING_DIR) + SUBDIR_NAME[n] + "\\";
				msg = CreateMultipleDir(workingDir);


				callback.AddMessage(GetString(IDS_UPDATE_DIR));
				callback.AddMessage(workingDir, 1);
				callback.AddMessage(GetString(IDS_UPDATE_FROM));
				callback.AddMessage(string(SERVER_NAME[n]) + "/" + SERVER_PATH[n], 1);
				callback.AddMessage("");

				switch (n)
				{
				case AGRI: msg = ExecuteAgri(callback); break;
				case AGRI2: msg = ExecuteAgri2(callback); break;
				case FIRE: msg = ExecuteFire(callback); break;
				case HYDRO:	msg = ExecuteHydro(callback); break;
				case POTATO: msg = ExecutePotato(callback); break;
				default: ASSERT(false);
				}

				msg += callback.StepIt();
			}//if selected network
		}//for all network


		callback.PopTask();


		return msg;
	}


	ERMsg CUIManitoba::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		std::bitset<NB_NETWORKS> network = GetNetwork();

		m_stations.clear();
		for (size_t n = 0; n < NB_NETWORKS; n++)
		{
			if (network.test(n))
			{
				CLocationVector locations;
				msg = locations.Load(GetStationsListFilePath(n));

				if (msg)
					msg += locations.IsValid();

				//Update network
				for (size_t i = 0; i < locations.size(); i++)
					locations[i].SetSSI("Network", NETWORK_NAME[n]);

				m_stations.insert(m_stations.end(), locations.begin(), locations.end());

				for (size_t i = 0; i < locations.size(); i++)
					if (locations[i].UseIt())
						stationList.push_back(ToString(n) + "/" + locations[i].m_ID);
			}
		}

		if (msg)
		{
			if (network.test(AGRI2))
			{
				LoadAgri2(callback);
			}

		}


		return msg;
	}

	ERMsg CUIManitoba::Finalize(TType type, CCallback& callback)
	{

		m_stations.clear();
		m_agri2Stations.clear();

		return ERMsg();
	}

	ERMsg CUIManitoba::GetWeatherStation(const std::string& NID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t n = ToSizeT(NID.substr(0, 1));
		string ID = NID.substr(2);

		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[it];


		size_t type = as<size_t>(DATA_TYPE);
		station.SetHourly(type == HOURLY_WEATHER);


		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			if (n == AGRI)
			{
				for (size_t m = 0; m < 12 && msg; m++)
				{
					string filePath = GetOutputFilePath(n, type, ID, year, m);
					if (FileExists(filePath))
						msg = station.LoadData(filePath, -999, false);

					msg += callback.StepIt(0);
				}
			}
			else if (n == AGRI2)
			{
				if (m_agri2Stations.find(ID) != m_agri2Stations.end())
					station = m_agri2Stations.at(ID);

				msg += callback.StepIt(0);
			}
			else
			{
				string filePath = GetOutputFilePath(n, type, ID, year);
				if (FileExists(filePath))
					msg = station.LoadData(filePath, -999, false);

				msg += callback.StepIt(0);
			}


		}

		station.CleanUnusedVariable("TN T TX P TD H WS WD R Z");
		//clean temperature under -60 and upper 60∞C

		CTPeriod p = station.GetEntireTPeriod();
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			if (station.IsHourly())
			{
				CHourlyData& data = station.GetHour(TRef);
				if ((data[H_TMIN] > -999 && (data[H_TMIN] < -60 || data[H_TMIN] > 60)) ||
					(data[H_TAIR] > -999 && (data[H_TAIR] < -60 || data[H_TAIR] > 60)) ||
					(data[H_TMAX] > -999 && (data[H_TMAX] < -60 || data[H_TMAX] > 60)))
				{
					data[H_TMIN] = -999;
					data[H_TAIR] = -999;
					data[H_TMAX] = -999;
				}

				if (data[H_TDEW] > -999 && (data[H_TDEW] < -60 || data[H_TDEW] > 60)) 
				{
					data[H_TDEW] = -999;
					data[H_RELH] = -999;
				}

				if (data[H_WNDS] > -999 && (data[H_WNDS] < 0 || data[H_WNDS] > 110))
				{
					data[H_WNDS] = -999;
					data[H_WNDD] = -999;
				}

			}
			else
			{
				CWeatherDay& data = station.GetDay(TRef);
				if ((!data[H_TMIN].empty() && (data[H_TMIN][LOWEST] < -60 || data[H_TMIN][HIGHEST] > 60)) ||
					(!data[H_TAIR].empty() && (data[H_TAIR][LOWEST] < -60 || data[H_TAIR][HIGHEST] > 60)) ||
					(!data[H_TMAX].empty() && (data[H_TMAX][LOWEST] < -60 || data[H_TMAX][HIGHEST] > 60)))
				{
					data[H_TMIN].Reset();
					data[H_TAIR].Reset();
					data[H_TMAX].Reset();
				}

				if (!data[H_TDEW].empty() && (data[H_TDEW][MEAN] < -60 || data[H_TDEW][MEAN] > 60)) 
				{
					data[H_TDEW] = -999;
					data[H_RELH] = -999;
				}

				if (!data[H_WNDS].empty() && (data[H_WNDS][MEAN] < 0 || data[H_WNDS][MEAN] > 110))
				{
					data[H_WNDS] = -999;
					data[H_WNDD] = -999;
				}
			}
		}




		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		station.SetSSI("Provider", PROVIDER_NAME[n]);


		return msg;
	}




	//******************************************************************************************************

	double CUIManitoba::GetWindDir(string compass)
	{
		static const char* COMPASS[16] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW" };

		double wd = -999;
		for (size_t i = 0; i < 16; i++)
		{
			if (compass == COMPASS[i])
			{
				wd = i * 22.5;
				break;
			}
		}

		return wd;
	}

	ERMsg CUIManitoba::DownloadAgriData(CHttpConnectionPtr& pConnection, size_t type, const std::string& ID, CTRef TRef, std::string& text)
	{
		ERMsg msg;

		CString URL;
		CStringA strParam;

		if (type == HOURLY_WEATHER)
		{
			URL = _T("climate/HourlyReport.aspx");
			string strDate = FormatA("%4d-%02d-%02d", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
			strParam += "__VIEWSTATE=%2FwEPDwULLTEwOTU4OTMxNzYPZBYCZg9kFgICAw9kFgICAQ9kFgQCAQ9kFgQCAQ9kFgICAQ9kFgJmDxAPFgYeDURhdGFUZXh0RmllbGQFC1N0YXRpb25OYW1lHg5EYXRhVmFsdWVGaWVsZAUFU3RuSUQeC18hRGF0YUJvdW5kZ2QQFSgGQWx0b25hBkFyYm9yZwZCaXJ0bGUKQm9pc3NldmFpbglEZWxvcmFpbmUGRHVnYWxkCUVsbSBDcmVlawlFcmlrc2RhbGUJRXRoZWxiZXJ0B0ZvcnJlc3QJR2xhZHN0b25lCEdsZW5ib3JvCUdyYW5kdmlldwdIYW1pb3RhCUtpbGxhcm5leQlMZXRlbGxpZXIHTWFuaXRvdQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgZNb3JyaXMHUGllcnNvbgxQb3J0YWdlIEVhc3QGUmVzdG9uB1J1c3NlbGwHU2Vsa2lyawhTb21lcnNldAZTb3VyaXMJU3QgUGllcnJlC1N0LiBBZG9scGhlCFN0YXJidWNrCVN0ZS4gUm9zZQlTdGVpbmJhY2gLU3dhbiBWYWxsZXkGVGV1bG9uCFRyZWhlcm5lBlZpcmRlbghXYXdhbmVzYQ1XaW5rbGVyIENNQ0RDCVdvb2RsYW5kcxUoAzI0NAMyMDYDMjE2AzIwOQMyNDEDMjE3AzIzNwMyMTgDMjEzAzIzMwMyMDQDMjM5AzIxOQMyMTQDMjIwAzIzOAMyNDIDNzQwAzIyMQMyMDUDMjIyAzIzMgMyMzUDMjQ1AzIxNQMyMTADMjQ2AzIwOAMyMDMDMjQzAzIwMgMyMTEDMjIzAzIzMQMyMDcDMjAxAzIyNAMyNDADMjMwAzIyNRQrAyhnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZGQCAg9kFgICAQ9kFgICAg8PZBYMHgtvbk1vdXNlT3ZlcgU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0hSRGF0ZScsICdvbk1vdXNlT3ZlcicpHgpvbk1vdXNlT3V0BTtDaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nSFJEYXRlJywgJ29uTW91c2VPdXQnKR4Lb25Nb3VzZURvd24FPUNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nSFJEYXRlJywgJ29uTW91c2VEb3duJykeCW9uTW91c2VVcAU7Q2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdIUkRhdGUnLCAnb25Nb3VzZVVwJykeB29uQ2xpY2sFLlNob3dDYWxlbmRhcignY3RsMDBfRGVmYXVsdENvbnRlbnRfdHh0SFJEYXRlJykeCm9uS2V5UHJlc3MFLlNob3dDYWxlbmRhcignY3RsMDBfRGVmYXVsdENvbnRlbnRfdHh0SFJEYXRlJylkAgUPPCsAEQEMFCsAAGQYAQUjY3RsMDAkRGVmYXVsdENvbnRlbnQkZ2RIb3VybHlSZXBvcnQPZ2TRbb1mBGIPVRtR3XPWKp688XF3xAKK0x9NkXgpIhq9DA%3D%3D&";
			strParam += "__VIEWSTATEGENERATOR=CF98EFE7&";
			strParam += "__EVENTVALIDATION=%2FwEdACvgny41miSOmWnk4YlEXzL%2F1Rn3s3zLvBRcK30Re68sg%2Bw3bZWK3uZ1o45Q8ttLL%2BhElpdnEBBGduxPsQe%2BTq5M5jIUpgpdAYi%2FSomLt87qnC1gPOjD4MEyBYFec0eHaMmQVi8J9YbTpvUiMniWBW%2BWDd%2B4ujXcVdIoCB0C0ROEtzPqgKglxt%2BEFeUakDwxeB1HLIR%2BpU4ymeiV0co43%2FDZxXLN72vPLmDhnillVUTyx13IxgHgqZ%2FY2Avglzxu5MNeVYMZh2Qu4wHYcoafWpV0RrL68yf5lt9NjJI9u2ToVXPrnew0SOn%2By0xPEvpEZMiBsjrg%2FI9hCHLnk8zmA%2Fhrw8dMhR%2BKIbXzMhr38%2BjH%2Fuvv099C83W9TYlSOck72FGOg6QfrcFy8rfrp%2F1QfX8NnrqhrMR0ByT%2FulhQ30qCRoNs7Ms5j7oo4JznHWB0nQj%2B1TCtCzw5t7H9RoT9Yoa08wMbwkJa5fZ%2BO5BN9OfE%2BW6khJlulOytY75Enbjru%2FTlxcDtEX5Gflb3iP5DuvT41UOZfD78YgUjaxHMxM%2FE0Giz%2BnOy85Bm1yuoe1GZWO56jHOPDUlFaljjcrWoeF0eb1oQKLPAzAmDaTaGTlQfyNtqyX%2FZWmk9DLR%2BLKpkICx3%2BlAcLamSb9t1%2B84LNnBEGaF2mtccVHpaV475B3jOuxIW0OF6ak7jDdPaA6f1fOVEgyeSjFZlNpmE6cA%2BA%2FK9cp1siLE2rSgPElxim%2FAWrzvNYXQweyiCnE%2BYo7Mju7jIuaTBmAK72WxgFGSO7U%2Fsf7oi48HejaSK6%2BydxApiT7%2BVxHkPq0z8rpD3CKXPlaiTXQyfE17BdXsTIngn1XzE9v7hFuR161sinSICSPGK%2FTaAWodHPdHBsOIc%2B3NozaF3bCuXn35XBZ2w%2Bo3R4YCg6INL%2B7aSWO7VNweb8oFVowgtoR6oZm1R2q1l5Zk%2FZxs%3D&";
			strParam += ("ctl00%24DefaultContent%24cboStationNames=" + ID + "&").c_str();
			strParam += ("ctl00%24DefaultContent%24txtHRDate=" + strDate + "&").c_str();
			strParam += "ctl00%24DefaultContent%24btn_HRSearch=Submit";
		}
		else
		{
			URL = _T("climate/DailyReport.aspx");
			string strDate1 = FormatA("%4d-%02d-%02d", TRef.GetYear(), TRef.GetMonth() + 1, 1);
			string strDate2 = FormatA("%4d-%02d-%02d", TRef.GetYear(), TRef.GetMonth() + 1, GetNbDayPerMonth(TRef.GetYear(), TRef.GetMonth()));
			strParam += "__VIEWSTATE=%2FwEPDwUJMjE5Njc0MjAyD2QWAmYPZBYCAgMPZBYCAgEPZBYIAgEPZBYGAgEPZBYCAgEPZBYCZg8QDxYGHg1EYXRhVGV4dEZpZWxkBQtTdGF0aW9uTmFtZR4ORGF0YVZhbHVlRmllbGQFBVN0bklEHgtfIURhdGFCb3VuZGdkEBU%2BBkFsdG9uYQZBcmJvcmcGQmlydGxlCkJvaXNzZXZhaW4HQnJhbmRvbghDYXJiZXJyeQZDYXJtYW4NQ3lwcmVzcyBSaXZlcgdEYXVwaGluCURlbG9yYWluZQZEdWdhbGQJRWxtIENyZWVrC0VtZXJzb24gQXV0CUVyaWtzZGFsZQlFdGhlbGJlcnQNRmlzaGVyIEJyYW5jaAdGb3JyZXN0BUdpbWxpCUdsYWRzdG9uZQhHbGVuYm9ybwlHcmFuZHZpZXcGR3JldG5hB0hhbWlvdGEJS2lsbGFybmV5CUxldGVsbGllcgdNYW5pdG91CE1jQ3JlYXJ5Bk1lbGl0YQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgpNb3JkZW4gQ0RBBk1vcnJpcwdQaWVyc29uC1BpbG90IE1vdW5kClBpbmF3YSBBdXQHUG9ydGFnZQxQb3J0YWdlIEVhc3QGUmVzdG9uClJvYmxpbiBBVVQHUnVzc2VsbAdTZWxraXJrClNob2FsIExha2UIU29tZXJzZXQGU291cmlzC1NwcmFndWUgQVVUCVN0IFBpZXJyZQtTdC4gQWRvbHBoZQhTdGFyYnVjawlTdGUuIFJvc2UJU3RlaW5iYWNoClN3YW4gUml2ZXILU3dhbiBWYWxsZXkGVGV1bG9uB1RoZSBQYXMIVHJlaGVybmUGVmlyZGVuCldhc2FnYW1pbmcIV2F3YW5lc2ENV2lua2xlciBDTUNEQxBXaW5uaXBlZyBBaXJwb3J0CVdvb2RsYW5kcxU%2BAzI0NAMyMDYDMjE2AzIwOQEyATQBNQE4ATkDMjQxAzIxNwMyMzcCMTEDMjE4AzIxMwI1NgMyMzMCMTQDMjA0AzIzOQMyMTkCMTcDMjE0AzIyMAMyMzgDMjQyAjI1AjI2Azc0MAMyMjEDMjA1AjI5AzIyMgMyMzICMzQCMzUCMzcDMjM1AzI0NQIzOAMyMTUDMjEwAjQwAzI0NgMyMDgCNDEDMjAzAzI0MwMyMDIDMjExAzIyMwI0NAMyMzEDMjA3AjQ1AzIwMQMyMjQCNTEDMjQwAzIzMAI1MgMyMjUUKwM%2BZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dkZAICD2QWAgIBD2QWAgICDw9kFgweC29uTW91c2VPdmVyBT9DaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nUGxhbnREYXRlJywgJ29uTW91c2VPdmVyJykeCm9uTW91c2VPdXQFPkNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZU91dCcpHgtvbk1vdXNlRG93bgVAQ2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZURvd24nKR4Jb25Nb3VzZVVwBT5DaGFuZ2VCb3JkZXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ1BsYW50RGF0ZScsICdvbk1vdXNlVXAnKR4Hb25DbGljawUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKR4Kb25LZXlQcmVzcwUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKWQCAw9kFgICAQ9kFgICAg8PZBYMHwMFPUNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdFbmREYXRlJywgJ29uTW91c2VPdmVyJykfBAU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0VuZERhdGUnLCAnb25Nb3VzZU91dCcpHwUFPkNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlRG93bicpHwYFPENoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlVXAnKR8HBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKR8IBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKWQCAw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%2BfIxvdGjFVCEs0K6GTIWA%2Bs80gVsOsX6WN3Wn3%2BZI%3D&";
			strParam += "__VIEWSTATEGENERATOR=31DECDD8&";
			strParam += "__EVENTVALIDATION=%2FwEdAEIRfgjWegxq%2BCMbI7zTjgUf1Rn3s3zLvBRcK30Re68sg%2Bw3bZWK3uZ1o45Q8ttLL%2BhElpdnEBBGduxPsQe%2BTq5M5jIUpgpdAYi%2FSomLt87qnBuYqHKC6crP0J4xwtZ4hlKyhKPgzgjinYykQeZCuOjVPIcqwtk5PzosS76ZIKUluUilO842d26iZl39%2F2D0cyqS%2FXOnUmzIIkIFRxnWExuqLWA86MPgwTIFgV5zR4doyZBWLwn1htOm9SIyeJYFb5YN37i6NdxV0igIHQLRE4S3AFrUrtqCFYWrQoxOsRlOVzPqgKglxt%2BEFeUakDwxeB1HLIR%2BpU4ymeiV0co43%2FDZf9mwziuVZpFM3rIPwJ0vLsVyze9rzy5g4Z4pZVVE8se4gfTH7xQ9FbGEM%2Bnq34XxXcjGAeCpn9jYC%2BCXPG7kw15VgxmHZC7jAdhyhp9alXRGsvrzJ%2FmW302Mkj27ZOhVrhK3Zc41sAoziXBMZ1tZyXPrnew0SOn%2By0xPEvpEZMiBsjrg%2FI9hCHLnk8zmA%2Fhrw8dMhR%2BKIbXzMhr38%2BjH%2Fuvv099C83W9TYlSOck72FHgveY7DcA3SMI9ApWnkHFTfPKtYOVOEdPvIjd3ADkQwI6DpB%2BtwXLyt%2Bun%2FVB9fw2euqGsxHQHJP%2B6WFDfSoJGg2zsyzmPuijgnOcdYHSdCMYc7XC6z%2BLpwGxEEWWHQ7j%2B1TCtCzw5t7H9RoT9Yoa08wMbwkJa5fZ%2BO5BN9OfE%2BQKWU2OmyhhGFtvtszIfyMDe6e9lkFFRSu0VND%2BhRMk7i48sKpqh%2FmPDn8Oq7eahrW6khJlulOytY75Enbjru%2FTlxcDtEX5Gflb3iP5DuvT4v88YBTp%2Ft8iCSaebOGciJNVDmXw%2B%2FGIFI2sRzMTPxNBos%2FpzsvOQZtcrqHtRmVjumNjSLqWXcfcJg%2F5swexjcnqMc48NSUVqWONytah4XR5vWhAos8DMCYNpNoZOVB%2FINKlXtU0M%2Ffok6JNDUZkWq9tqyX%2FZWmk9DLR%2BLKpkICx3%2BlAcLamSb9t1%2B84LNnBEGaF2mtccVHpaV475B3jOuxIW0OF6ak7jDdPaA6f1fOVEgyeSjFZlNpmE6cA%2BA%2FK9rrbZiJkDIXK683tOHErgoHKdbIixNq0oDxJcYpvwFq87zWF0MHsogpxPmKOzI7u4nW9uSq9EvljFVWSrjZgbsci5pMGYArvZbGAUZI7tT%2Bx%2FuiLjwd6NpIrr7J3ECmJPdOPHLDqnrzsfLGtj58A0ur%2BVxHkPq0z8rpD3CKXPlaiTXQyfE17BdXsTIngn1XzEa%2FC1DCzvQY4UCOdRo8X%2BTvb%2B4RbkdetbIp0iAkjxiv3HqQdHu7YxOFLra%2FgosKYckhbB5Q5GqqYnJo8poNhAbY0tFJ6FA5VYRXHzlEDStfDWgB2QNvezPyj8a6ybWcSz4moPAhgTzO3o%2BC0jBWanUw%3D%3D&";
			strParam += ("ctl00%24DefaultContent%24cboStationNames=" + ID + "&").c_str();
			strParam += ("ctl00%24DefaultContent%24txtPlantDate=" + strDate1 + "&").c_str();
			strParam += ("ctl00%24DefaultContent%24txtEndDate=" + strDate2 + "&").c_str();
			strParam += "ctl00%24DefaultContent%24btn_DRSearch=Submit";
		}
		//URL = _T("climate/DailyReport.aspx");
		//strParam = "__VIEWSTATE=%2FwEPDwUJMjE5Njc0MjAyD2QWAmYPZBYCAgMPZBYCAgEPZBYIAgEPZBYGAgEPZBYCAgEPZBYCZg8QDxYGHg1EYXRhVGV4dEZpZWxkBQtTdGF0aW9uTmFtZR4ORGF0YVZhbHVlRmllbGQFBVN0bklEHgtfIURhdGFCb3VuZGdkEBU%2BBkFsdG9uYQZBcmJvcmcGQmlydGxlCkJvaXNzZXZhaW4HQnJhbmRvbghDYXJiZXJyeQZDYXJtYW4NQ3lwcmVzcyBSaXZlcgdEYXVwaGluCURlbG9yYWluZQZEdWdhbGQJRWxtIENyZWVrC0VtZXJzb24gQXV0CUVyaWtzZGFsZQlFdGhlbGJlcnQNRmlzaGVyIEJyYW5jaAdGb3JyZXN0BUdpbWxpCUdsYWRzdG9uZQhHbGVuYm9ybwlHcmFuZHZpZXcGR3JldG5hB0hhbWlvdGEJS2lsbGFybmV5CUxldGVsbGllcgdNYW5pdG91CE1jQ3JlYXJ5Bk1lbGl0YQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgpNb3JkZW4gQ0RBBk1vcnJpcwdQaWVyc29uC1BpbG90IE1vdW5kClBpbmF3YSBBdXQHUG9ydGFnZQxQb3J0YWdlIEVhc3QGUmVzdG9uClJvYmxpbiBBVVQHUnVzc2VsbAdTZWxraXJrClNob2FsIExha2UIU29tZXJzZXQGU291cmlzC1NwcmFndWUgQVVUCVN0IFBpZXJyZQtTdC4gQWRvbHBoZQhTdGFyYnVjawlTdGUuIFJvc2UJU3RlaW5iYWNoClN3YW4gUml2ZXILU3dhbiBWYWxsZXkGVGV1bG9uB1RoZSBQYXMIVHJlaGVybmUGVmlyZGVuCldhc2FnYW1pbmcIV2F3YW5lc2ENV2lua2xlciBDTUNEQxBXaW5uaXBlZyBBaXJwb3J0CVdvb2RsYW5kcxU%2BAzI0NAMyMDYDMjE2AzIwOQEyATQBNQE4ATkDMjQxAzIxNwMyMzcCMTEDMjE4AzIxMwI1NgMyMzMCMTQDMjA0AzIzOQMyMTkCMTcDMjE0AzIyMAMyMzgDMjQyAjI1AjI2Azc0MAMyMjEDMjA1AjI5AzIyMgMyMzICMzQCMzUCMzcDMjM1AzI0NQIzOAMyMTUDMjEwAjQwAzI0NgMyMDgCNDEDMjAzAzI0MwMyMDIDMjExAzIyMwI0NAMyMzEDMjA3AjQ1AzIwMQMyMjQCNTEDMjQwAzIzMAI1MgMyMjUUKwM%2BZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dkZAICD2QWAgIBD2QWAgICDw9kFgweC29uTW91c2VPdmVyBT9DaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nUGxhbnREYXRlJywgJ29uTW91c2VPdmVyJykeCm9uTW91c2VPdXQFPkNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZU91dCcpHgtvbk1vdXNlRG93bgVAQ2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZURvd24nKR4Jb25Nb3VzZVVwBT5DaGFuZ2VCb3JkZXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ1BsYW50RGF0ZScsICdvbk1vdXNlVXAnKR4Hb25DbGljawUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKR4Kb25LZXlQcmVzcwUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKWQCAw9kFgICAQ9kFgICAg8PZBYMHwMFPUNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdFbmREYXRlJywgJ29uTW91c2VPdmVyJykfBAU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0VuZERhdGUnLCAnb25Nb3VzZU91dCcpHwUFPkNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlRG93bicpHwYFPENoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlVXAnKR8HBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKR8IBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKWQCAw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%2BfIxvdGjFVCEs0K6GTIWA%2Bs80gVsOsX6WN3Wn3%2BZI%3D&__VIEWSTATEGENERATOR=31DECDD8&__EVENTVALIDATION=%2FwEdAEIRfgjWegxq%2BCMbI7zTjgUf1Rn3s3zLvBRcK30Re68sg%2Bw3bZWK3uZ1o45Q8ttLL%2BhElpdnEBBGduxPsQe%2BTq5M5jIUpgpdAYi%2FSomLt87qnBuYqHKC6crP0J4xwtZ4hlKyhKPgzgjinYykQeZCuOjVPIcqwtk5PzosS76ZIKUluUilO842d26iZl39%2F2D0cyqS%2FXOnUmzIIkIFRxnWExuqLWA86MPgwTIFgV5zR4doyZBWLwn1htOm9SIyeJYFb5YN37i6NdxV0igIHQLRE4S3AFrUrtqCFYWrQoxOsRlOVzPqgKglxt%2BEFeUakDwxeB1HLIR%2BpU4ymeiV0co43%2FDZf9mwziuVZpFM3rIPwJ0vLsVyze9rzy5g4Z4pZVVE8se4gfTH7xQ9FbGEM%2Bnq34XxXcjGAeCpn9jYC%2BCXPG7kw15VgxmHZC7jAdhyhp9alXRGsvrzJ%2FmW302Mkj27ZOhVrhK3Zc41sAoziXBMZ1tZyXPrnew0SOn%2By0xPEvpEZMiBsjrg%2FI9hCHLnk8zmA%2Fhrw8dMhR%2BKIbXzMhr38%2BjH%2Fuvv099C83W9TYlSOck72FHgveY7DcA3SMI9ApWnkHFTfPKtYOVOEdPvIjd3ADkQwI6DpB%2BtwXLyt%2Bun%2FVB9fw2euqGsxHQHJP%2B6WFDfSoJGg2zsyzmPuijgnOcdYHSdCMYc7XC6z%2BLpwGxEEWWHQ7j%2B1TCtCzw5t7H9RoT9Yoa08wMbwkJa5fZ%2BO5BN9OfE%2BQKWU2OmyhhGFtvtszIfyMDe6e9lkFFRSu0VND%2BhRMk7i48sKpqh%2FmPDn8Oq7eahrW6khJlulOytY75Enbjru%2FTlxcDtEX5Gflb3iP5DuvT4v88YBTp%2Ft8iCSaebOGciJNVDmXw%2B%2FGIFI2sRzMTPxNBos%2FpzsvOQZtcrqHtRmVjumNjSLqWXcfcJg%2F5swexjcnqMc48NSUVqWONytah4XR5vWhAos8DMCYNpNoZOVB%2FINKlXtU0M%2Ffok6JNDUZkWq9tqyX%2FZWmk9DLR%2BLKpkICx3%2BlAcLamSb9t1%2B84LNnBEGaF2mtccVHpaV475B3jOuxIW0OF6ak7jDdPaA6f1fOVEgyeSjFZlNpmE6cA%2BA%2FK9rrbZiJkDIXK683tOHErgoHKdbIixNq0oDxJcYpvwFq87zWF0MHsogpxPmKOzI7u4nW9uSq9EvljFVWSrjZgbsci5pMGYArvZbGAUZI7tT%2Bx%2FuiLjwd6NpIrr7J3ECmJPdOPHLDqnrzsfLGtj58A0ur%2BVxHkPq0z8rpD3CKXPlaiTXQyfE17BdXsTIngn1XzEa%2FC1DCzvQY4UCOdRo8X%2BTvb%2B4RbkdetbIp0iAkjxiv3HqQdHu7YxOFLra%2FgosKYckhbB5Q5GqqYnJo8poNhAbY0tFJ6FA5VYRXHzlEDStfDWgB2QNvezPyj8a6ybWcSz4moPAhgTzO3o%2BC0jBWanUw%3D%3D&ctl00%24DefaultContent%24cboStationNames=244&ctl00%24DefaultContent%24txtPlantDate=2019-01-01&ctl00%24DefaultContent%24txtEndDate=2019-02-05&ctl00%24DefaultContent%24btn_DRSearch=Submit";
		//strParam = "__VIEWSTATE=%2FwEPDwUJMjE5Njc0MjAyD2QWAmYPZBYCAgMPZBYCAgEPZBYIAgEPZBYGAgEPZBYCAgEPZBYCZg8QDxYGHg1EYXRhVGV4dEZpZWxkBQtTdGF0aW9uTmFtZR4ORGF0YVZhbHVlRmllbGQFBVN0bklEHgtfIURhdGFCb3VuZGdkEBU%2BBkFsdG9uYQZBcmJvcmcGQmlydGxlCkJvaXNzZXZhaW4HQnJhbmRvbghDYXJiZXJyeQZDYXJtYW4NQ3lwcmVzcyBSaXZlcgdEYXVwaGluCURlbG9yYWluZQZEdWdhbGQJRWxtIENyZWVrC0VtZXJzb24gQXV0CUVyaWtzZGFsZQlFdGhlbGJlcnQNRmlzaGVyIEJyYW5jaAdGb3JyZXN0BUdpbWxpCUdsYWRzdG9uZQhHbGVuYm9ybwlHcmFuZHZpZXcGR3JldG5hB0hhbWlvdGEJS2lsbGFybmV5CUxldGVsbGllcgdNYW5pdG91CE1jQ3JlYXJ5Bk1lbGl0YQxNZWxpdGEgU291dGgJTWlubmVkb3NhCU1vb3NlaG9ybgpNb3JkZW4gQ0RBBk1vcnJpcwdQaWVyc29uC1BpbG90IE1vdW5kClBpbmF3YSBBdXQHUG9ydGFnZQxQb3J0YWdlIEVhc3QGUmVzdG9uClJvYmxpbiBBVVQHUnVzc2VsbAdTZWxraXJrClNob2FsIExha2UIU29tZXJzZXQGU291cmlzC1NwcmFndWUgQVVUCVN0IFBpZXJyZQtTdC4gQWRvbHBoZQhTdGFyYnVjawlTdGUuIFJvc2UJU3RlaW5iYWNoClN3YW4gUml2ZXILU3dhbiBWYWxsZXkGVGV1bG9uB1RoZSBQYXMIVHJlaGVybmUGVmlyZGVuCldhc2FnYW1pbmcIV2F3YW5lc2ENV2lua2xlciBDTUNEQxBXaW5uaXBlZyBBaXJwb3J0CVdvb2RsYW5kcxU%2BAzI0NAMyMDYDMjE2AzIwOQEyATQBNQE4ATkDMjQxAzIxNwMyMzcCMTEDMjE4AzIxMwI1NgMyMzMCMTQDMjA0AzIzOQMyMTkCMTcDMjE0AzIyMAMyMzgDMjQyAjI1AjI2Azc0MAMyMjEDMjA1AjI5AzIyMgMyMzICMzQCMzUCMzcDMjM1AzI0NQIzOAMyMTUDMjEwAjQwAzI0NgMyMDgCNDEDMjAzAzI0MwMyMDIDMjExAzIyMwI0NAMyMzEDMjA3AjQ1AzIwMQMyMjQCNTEDMjQwAzIzMAI1MgMyMjUUKwM%2BZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dnZ2dkZAICD2QWAgIBD2QWAgICDw9kFgweC29uTW91c2VPdmVyBT9DaGFuZ2VNb3VzZSgnY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nUGxhbnREYXRlJywgJ29uTW91c2VPdmVyJykeCm9uTW91c2VPdXQFPkNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZU91dCcpHgtvbk1vdXNlRG93bgVAQ2hhbmdlQm9yZGVyKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdQbGFudERhdGUnLCAnb25Nb3VzZURvd24nKR4Jb25Nb3VzZVVwBT5DaGFuZ2VCb3JkZXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ1BsYW50RGF0ZScsICdvbk1vdXNlVXAnKR4Hb25DbGljawUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKR4Kb25LZXlQcmVzcwUxU2hvd0NhbGVuZGFyKCdjdGwwMF9EZWZhdWx0Q29udGVudF90eHRQbGFudERhdGUnKWQCAw9kFgICAQ9kFgICAg8PZBYMHwMFPUNoYW5nZU1vdXNlKCdjdGwwMF9EZWZhdWx0Q29udGVudF9pbWdFbmREYXRlJywgJ29uTW91c2VPdmVyJykfBAU8Q2hhbmdlTW91c2UoJ2N0bDAwX0RlZmF1bHRDb250ZW50X2ltZ0VuZERhdGUnLCAnb25Nb3VzZU91dCcpHwUFPkNoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlRG93bicpHwYFPENoYW5nZUJvcmRlcignY3RsMDBfRGVmYXVsdENvbnRlbnRfaW1nRW5kRGF0ZScsICdvbk1vdXNlVXAnKR8HBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKR8IBS9TaG93Q2FsZW5kYXIoJ2N0bDAwX0RlZmF1bHRDb250ZW50X3R4dEVuZERhdGUnKWQCAw9kFgRmD2QWAmYPZBYCZg8PFgIeBFRleHQFJzxiPkRhaWx5IFdlYXRoZXIgU3VtbWFyeSBmb3IgQWx0b25hPC9iPmRkAgEPZBYCZg9kFgJmDw8WAh8JBTQ8Yj5Gcm9tOiBKYW51YXJ5IDAxLCAyMDE5ICAgVG86IEphbnVhcnkgMzEsIDIwMTk8L2I%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%2FGWujxijWU4r92z6X7EMzNoV2l7I&__VIEWSTATEGENERATOR=31DECDD8&__EVENTVALIDATION=%2FwEdAEJM%2BrcZpLyVtSfgIIC%2FO1N71Rn3s3zLvBRcK30Re68sg%2Bw3bZWK3uZ1o45Q8ttLL%2BhElpdnEBBGduxPsQe%2BTq5M5jIUpgpdAYi%2FSomLt87qnBuYqHKC6crP0J4xwtZ4hlKyhKPgzgjinYykQeZCuOjVPIcqwtk5PzosS76ZIKUluUilO842d26iZl39%2F2D0cyqS%2FXOnUmzIIkIFRxnWExuqLWA86MPgwTIFgV5zR4doyZBWLwn1htOm9SIyeJYFb5YN37i6NdxV0igIHQLRE4S3AFrUrtqCFYWrQoxOsRlOVzPqgKglxt%2BEFeUakDwxeB1HLIR%2BpU4ymeiV0co43%2FDZf9mwziuVZpFM3rIPwJ0vLsVyze9rzy5g4Z4pZVVE8se4gfTH7xQ9FbGEM%2Bnq34XxXcjGAeCpn9jYC%2BCXPG7kw15VgxmHZC7jAdhyhp9alXRGsvrzJ%2FmW302Mkj27ZOhVrhK3Zc41sAoziXBMZ1tZyXPrnew0SOn%2By0xPEvpEZMiBsjrg%2FI9hCHLnk8zmA%2Fhrw8dMhR%2BKIbXzMhr38%2BjH%2Fuvv099C83W9TYlSOck72FHgveY7DcA3SMI9ApWnkHFTfPKtYOVOEdPvIjd3ADkQwI6DpB%2BtwXLyt%2Bun%2FVB9fw2euqGsxHQHJP%2B6WFDfSoJGg2zsyzmPuijgnOcdYHSdCMYc7XC6z%2BLpwGxEEWWHQ7j%2B1TCtCzw5t7H9RoT9Yoa08wMbwkJa5fZ%2BO5BN9OfE%2BQKWU2OmyhhGFtvtszIfyMDe6e9lkFFRSu0VND%2BhRMk7i48sKpqh%2FmPDn8Oq7eahrW6khJlulOytY75Enbjru%2FTlxcDtEX5Gflb3iP5DuvT4v88YBTp%2Ft8iCSaebOGciJNVDmXw%2B%2FGIFI2sRzMTPxNBos%2FpzsvOQZtcrqHtRmVjumNjSLqWXcfcJg%2F5swexjcnqMc48NSUVqWONytah4XR5vWhAos8DMCYNpNoZOVB%2FINKlXtU0M%2Ffok6JNDUZkWq9tqyX%2FZWmk9DLR%2BLKpkICx3%2BlAcLamSb9t1%2B84LNnBEGaF2mtccVHpaV475B3jOuxIW0OF6ak7jDdPaA6f1fOVEgyeSjFZlNpmE6cA%2BA%2FK9rrbZiJkDIXK683tOHErgoHKdbIixNq0oDxJcYpvwFq87zWF0MHsogpxPmKOzI7u4nW9uSq9EvljFVWSrjZgbsci5pMGYArvZbGAUZI7tT%2Bx%2FuiLjwd6NpIrr7J3ECmJPdOPHLDqnrzsfLGtj58A0ur%2BVxHkPq0z8rpD3CKXPlaiTXQyfE17BdXsTIngn1XzEa%2FC1DCzvQY4UCOdRo8X%2BTvb%2B4RbkdetbIp0iAkjxiv3HqQdHu7YxOFLra%2FgosKYckhbB5Q5GqqYnJo8poNhAbY0tFJ6FA5VYRXHzlEDStfCYDqoSZQmNt0V68Ed3BHfthdfYonEFbE%2Fj2BH1Ps86lA%3D%3D&ctl00%24DefaultContent%24cboStationNames=206&ctl00%24DefaultContent%24txtPlantDate=2019-01-01&ctl00%24DefaultContent%24txtEndDate=2019-01-31&ctl00%24DefaultContent%24btn_DRSearch=Submit";
		DWORD HttpRequestFlags = INTERNET_FLAG_SECURE | INTERNET_FLAG_EXISTING_CONNECT;// | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;
		CHttpFile* pURLFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL, NULL, 1, NULL, NULL, HttpRequestFlags);


		bool bRep = false;

		if (pURLFile != NULL)
		{
			{
				TRY
				{
					CString strContentL;
					strContentL.Format(_T("Content-Length: %d\r\n"), strParam.GetLength());
					pURLFile->AddRequestHeaders(strContentL);
					pURLFile->AddRequestHeaders(CString(_T("Content-Type: application/x-www-form-urlencoded\r\n")));

					// send request
					bRep = pURLFile->SendRequest(0, 0, (void*)(const char*)strParam, strParam.GetLength()) != 0;
				}
					CATCH_ALL(e)

				{
					DWORD errnum = GetLastError();

					if (errnum == 12002 || errnum == 12029)
					{
						msg = UtilWin::SYGetMessage(*e);
					}
					else if (errnum == 12031 || errnum == 12111)
					{
						//throw a exception: server reset
						msg = UtilWin::SYGetMessage(*e);
					}
					else if (errnum == 12003)
					{
						msg = UtilWin::SYGetMessage(*e);

						DWORD size = 255;
						TCHAR cause[256] = { 0 };
						InternetGetLastResponseInfo(&errnum, cause, &size);
						if (_tcslen(cause) > 0)
							msg.ajoute(UtilWin::ToUTF8(cause));
					}
					else
					{
						CInternetException e(errnum);
						msg += UtilWin::SYGetMessage(e);
					}
				}

				END_CATCH_ALL
			}

			if (msg)
			{
				if (bRep)
				{
					const short MAX_READ_SIZE = 4096;
					pURLFile->SetReadBufferSize(MAX_READ_SIZE);

					std::string tmp;
					tmp.resize(MAX_READ_SIZE);
					UINT charRead = 0;
					while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE)) > 0)
						text.append(tmp.c_str(), charRead);
				}
				else
				{
					CString tmp;
					tmp.FormatMessage(IDS_CMN_UNABLE_LOAD_PAGE, URL);
					msg.ajoute(UtilWin::ToUTF8(tmp));
				}
			}

			pURLFile->Close();
			delete pURLFile;
		}

		return msg;
	}

	ERMsg CUIManitoba::GetAgriStationList(size_t dataType, StringVector& fileList, CCallback& callback)
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (msg)
		{
			string str;
			msg = UtilWWW::GetPageText(pConnection, "climate/" + string(dataType == HOURLY_WEATHER ? "HourlyReport.aspx" : "DailyReport.aspx"), str);
			if (msg)
			{
				string::size_type pos1 = str.find("<table");
				string::size_type pos2 = NOT_INIT;
				if (pos1 < str.size())
				{
					pos1 = str.find("<select", pos1);
					pos2 = str.find("</select>", pos1);
				}

				if (pos1 != string::npos && pos2 != string::npos)
				{
					try
					{
						string xml_str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
						zen::XmlDoc doc = zen::parse(xml_str);

						zen::XmlIn in(doc.root());
						for (zen::XmlIn it = in["option"]; it; it.next())
						{
							string value;
							it.get()->getAttribute("value", value);

							//ignore EnvCan weather station . ie ID <100
							int ID = ToInt(value);
							if (ID >= 100)
								fileList.push_back(value);
						}//for all station
					}
					catch (const zen::XmlParsingError& e)
					{
						// handle error
						msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
					}
				}
			}


			pConnection->Close();
			pSession->Close();

		}

		return msg;
	}

	ERMsg CUIManitoba::SaveAgriDailyStation(const std::string& filePath, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(false);

		enum THourlyColumns { C_DATE, C_TMAX, C_TMIN, C_TAVG, C_PPT, C_GDD, C_CHU, NB_COLUMNS };
		static const TVarH DAILY_VAR[NB_COLUMNS] = { H_SKIP, H_TMAX, H_TMIN, H_TAIR, H_PRCP, H_SKIP, H_SKIP };

		try
		{
			int ID = ToInt(GetFileTitle(filePath));
			WBSF::ReplaceString(str, "\t", "");

			zen::XmlDoc doc = zen::parse(str);

			zen::XmlIn in(doc.root());
			for (zen::XmlIn itTR = in["tr"]; itTR; itTR.next())
			{
				StringVector tmp;
				for (zen::XmlIn itTD = itTR["td"]; itTD; itTD.next())
				{
					string value;
					itTD["font"](value);
					Trim(value);
					tmp.push_back(value);
				}//for all columns


				if (tmp.size() == NB_COLUMNS)
				{

					StringVector date(tmp[C_DATE], " ");

					if (date.size() == 3)
					{

						int year = ToInt(date[2]);
						size_t month = WBSF::GetMonthIndex(date[0].c_str());
						size_t day = ToInt(date[1]) - 1;

						CTRef TRef(year, month, day);
						ASSERT(TRef.IsValid());

						for (size_t i = 0; i < NB_COLUMNS; i++)
						{
							if (DAILY_VAR[i] != H_SKIP && !tmp[i].empty())
							{
								double value = ToDouble(tmp[i]);
								data.GetDay(TRef).SetStat(DAILY_VAR[i], value);
							}
						}
					}
				}
			}


			if (msg)
			{
				ASSERT(data.size() == 1);
				//save annual data
				for (auto it1 = data.begin(); it1 != data.end(); it1++)
				{
					msg += it1->second->SaveData(filePath);
				}
			}//if msg
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}


		return msg;
	}


	ERMsg CUIManitoba::SaveAgriHourlyStation(const std::string& filePath, std::string str)
	{
		ERMsg msg;

		CWeatherYears data(true);

		enum THourlyColumns { C_HOUR, C_TEMP, C_RH, C_RAIN, C_WIND_SPEED, C_WIND_DIR, C_PEAK_WIND, C_SOIL_TEMP, NB_COLUMNS };
		static const TVarH HOURLY_VARS[NB_COLUMNS] = { H_SKIP, H_TAIR, H_RELH, H_PRCP, H_WNDS, H_WNDD, H_SKIP, H_ADD1 };

		try
		{
			int ID = ToInt(GetFileTitle(filePath));
			WBSF::ReplaceString(str, "\t", "");

			zen::XmlDoc doc = zen::parse(str);

			zen::XmlIn in(doc.root());
			for (zen::XmlIn itDay = in["table"]; itDay; itDay.next())
			{
				string date;
				itDay.get()->getAttribute("date", date);
				CTRef TRef;
				TRef.FromFormatedString(date);
				TRef.Transform(CTM(CTM::HOURLY));

				for (zen::XmlIn itTR = itDay["tr"]; itTR; itTR.next())
				{
					StringVector tmp;
					for (zen::XmlIn itTD = itTR["td"]; itTD; itTD.next())
					{
						string value;
						itTD["font"](value);
						Trim(value);
						tmp.push_back(value);
					}//for all columns


					if (tmp.size() == NB_COLUMNS)
					{
						string strHour = tmp[C_HOUR];
						size_t hour = ToSizeT(strHour.substr(0, 2));

						if (strHour == "12AM")
							hour = 0;
						if (strHour == "12PM")
							hour = 12;
						else if (strHour.find("PM") != string::npos)
							hour += 12;

						TRef.m_hour = hour;
						ASSERT(TRef.IsValid());

						for (size_t i = 0; i < NB_COLUMNS; i++)
						{
							if (HOURLY_VARS[i] != H_SKIP && !tmp[i].empty())
							{
								double value = (HOURLY_VARS[i] == H_WNDD) ? GetWindDir(tmp[i]) : ToDouble(tmp[i]);
								data[TRef].SetStat(HOURLY_VARS[i], value);
							}
						}
					}
				}
			}

			if (msg)
			{
				if (data.empty())
				{
					msg = data.SaveData(filePath);//save empty file to avoid download it again
				}
				else if (msg && data.size() == 1)
				{
					//save annual data
					const CWeatherYear& d = data[size_t(0)];
					msg = d.SaveData(filePath);
				}//if msg
				else
				{
					ASSERT(false);
				}
			}

		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}


		return msg;
	}

	ERMsg CUIManitoba::ExecuteAgri(CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		CTRef currentTRef = CTRef::GetCurrentTRef();


		StringVector stationsList;
		GetAgriStationList(type, stationsList, callback);


		int nbFiles = 0;
		int nbRun = 0;
		size_t curY = 0;


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		while (nbRun < 5 && curY < nbYears && msg)
		{
			size_t totalFiles = (lastYear < currentTRef.GetYear()) ? stationsList.size()*nbYears * 12 : stationsList.size()*(nbYears - 1) * 12 + stationsList.size()*(currentTRef.GetMonth() + 1);
			callback.PushTask("Download Manitoba agriculture data (" + ToString(totalFiles) + " station-month)", totalFiles);

			nbRun++;
			msg = GetHttpConnection(SERVER_NAME[AGRI], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
			if (msg)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 45000);//let more time to get the data...

				for (size_t y = curY; y < nbYears &&msg; y++)
				{
					int year = firstYear + int(y);
					size_t nbMonths = year < currentTRef.GetYear() ? 12 : currentTRef.GetMonth() + 1;

					for (size_t m = 0; m < nbMonths && msg; m++)
					{
						for (size_t i = 0; i < stationsList.size() && msg; i++)
						{

							bool bDownload = true;
							string filePath = GetOutputFilePath(AGRI, type, stationsList[i], year, m);
							CreateMultipleDir(GetPath(filePath));

							CFileStamp fileStamp(filePath);
							CTime lu = fileStamp.m_time;
							if (lu.GetTime() > 0)
							{
								int nbDays = CTRef(lu.GetYear(), lu.GetMonth() - 1, lu.GetDay() - 1) - CTRef(year, m, LAST_DAY);

								if (nbDays > 7)
								{
									bDownload = false;
								}
								else if (nbDays > 0)
								{
									//load the file and verify if the last day of the moth is present
									CWeatherStation junk;
									if (junk.LoadData(filePath))
									{
										size_t lastDay = GetNbDayPerMonth(year, m) - 1;
										//if (junk.IsHourly())
										//{
											//if (junk[year][m][lastDay][LAST_HOUR].HaveData())
												//bDownload = false;
										//}
										//else
										//{
										if (junk[year][m][lastDay].HaveData())
											bDownload = false;
										//}
									}
								}
							}


							if (bDownload)
							{
								if (type == HOURLY_WEATHER)
								{
									string source;
									size_t nbDays = year < currentTRef.GetYear() || m < currentTRef.GetMonth() ? GetNbDayPerMonth(year, m) : currentTRef.GetDay() + 1;
									callback.PushTask("Update station (ID=" + stationsList[i] + ") for " + GetMonthTitle(m) + " " + ToString(year) + " (" + ToString(nbDays) + " days)", nbDays);


									for (size_t d = 0; d < nbDays && msg; d++)
									{
										string str;

										CTRef TRef(year, m, d);
										msg = DownloadAgriData(pConnection, type, stationsList[i], TRef, str);
										string::size_type pos1 = str.find("Hourly Raw Data for");

										if (pos1 < str.size())
										{
											pos1 = str.find("<table", pos1);
											if (pos1 < str.size())
											{
												string::size_type pos2 = str.find("</table>", pos1);

												if (pos1 != string::npos && pos2 != string::npos)
												{
													source += ("<table date=\"" + TRef.GetFormatedString("%Y-%m-%d") + "\" " + str.substr(pos1 + 6, pos2 - (pos1 + 6)) + "</table>");
													msg += callback.StepIt();
												}
											}
										}
									}



									//split data in seperate files
									if (msg)
									{
										string tmp = string("<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n") + "<tables>" + source + "</tables>";
										msg += SaveAgriHourlyStation(filePath, tmp);
										nbFiles++;
										nbRun = 0;
									}

									callback.PopTask();
								}//if hourly
								else
								{
									string str;
									msg = DownloadAgriData(pConnection, type, stationsList[i], CTRef(year, m), str);

									//split data in separate files
									if (msg)
									{

										string::size_type pos1 = str.find("DefaultContent_gdDailySummaryResults");
										string::size_type pos2 = string::npos;

										if (pos1 < str.size())
											pos1 = str.find("<table", pos1);

										if (pos1 < str.size())
											pos2 = str.find("</table>", pos1);


										if (pos1 != string::npos && pos2 != string::npos)
										{
											string tmp = "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 9);
											msg += SaveAgriDailyStation(filePath, tmp);
											nbFiles++;
											nbRun = 0;
										}

									}//if msg
								}//data type
							}//need download

							msg += callback.StepIt();
						}//for all station
					}//for all months

					if (msg)
						curY++;
				}//for all years

				//if an error occur: try again
				if (!msg && !callback.GetUserCancel())
				{
					if (nbRun < 5)
					{
						callback.AddMessage(msg);
						msg = ERMsg();
						msg += WaitServer(10, callback);
					}
				}

				//clean connection
				pConnection->Close();
				pSession->Close();
			}//if get connection

			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbFiles), 1);
			callback.PopTask();
		}//while


		return msg;
	}



	//******************************************************************************************************
	//Manitoba Agri 2
	ERMsg CUIManitoba::ExecuteAgri2(CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);
		if (type == HOURLY_WEATHER)
		{
			callback.AddMessage("Hourly data is not available for Agriculture 2 network");
			return msg;
		}

		size_t nbRun = 0;
		size_t cur_i = 0;

		StringVector fileList;
		msg = GetAgri2Files(fileList, callback);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		while (nbRun < 5 && cur_i < fileList.size() && msg)
		{

			nbRun++;
			msg = GetHttpConnection(SERVER_NAME[AGRI2], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
			if (msg)
			{
				callback.AddMessage("Download Manitoba agriculture2 data (" + ToString(fileList.size()) + " files)");
				callback.PushTask("Download Manitoba agriculture2 data (" + ToString(fileList.size()) + " files)", fileList.size());

				for (size_t i = cur_i; i < fileList.size() && msg; i++)
				{
					string remoteFilePath = SERVER_PATH[AGRI2] + fileList[i];
					string outputFilePath = GetOutputFilePath(AGRI2, type, fileList[i]);

					CreateMultipleDir(GetPath(outputFilePath));

					msg += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_SECURE | INTERNET_FLAG_TRANSFER_BINARY);
					if (msg)
					{
						cur_i++;
					}


					msg += callback.StepIt();
				}

				//if an error occur: try again
				if (!msg && !callback.GetUserCancel())
				{
					if (nbRun < 5)
					{
						callback.AddMessage(msg);
						msg = ERMsg();
						msg += WaitServer(10, callback);
					}
				}

				//clean connection
				pConnection->Close();
				pSession->Close();
			}//if get connection

			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(cur_i), 1);
			callback.PopTask();
		}//while


		return msg;
	}

	ERMsg CUIManitoba::GetAgri2Files(StringVector& fileList, CCallback& callback)
	{
		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		//		CTRef currentTRef = CTRef::GetCurrentTRef();

		fileList.clear();
		try
		{
			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME[AGRI2], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
			if (msg)
			{
				string str;
				msg = UtilWWW::GetPageText(pConnection, SERVER_PATH[AGRI2], str, false, INTERNET_FLAG_SECURE | INTERNET_FLAG_EXISTING_CONNECT);
				if (msg)
				{
					string::size_type pos1 = str.find("<tbody>");
					string::size_type pos2 = str.find("</tbody>");
					if (pos1 < str.length() && pos2 < str.length())
					{
						str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" + str.substr(pos1, pos2 - pos1 + 8);
						WBSF::ReplaceString(str, "'", "\"");
						WBSF::ReplaceString(str, "\t", "");


						zen::XmlDoc doc = zen::parse(str);

						zen::XmlIn in(doc.root());
						for (zen::XmlIn itTR = in["tr"]; itTR; itTR.next())
						{
							string value;
							itTR["td"]["a"](value);
							if (value.length() == 14)
							{
								int year = stoi(value.substr(0, 4));
								if (year >= firstYear && year <= lastYear)
								{
									string filePath = GetOutputFilePath(AGRI2, DAILY_WEATHER, value);
									if (!FileExists(filePath))
										fileList.push_back(value);
								}
							}
						}
					}
				}
				pSession->Close();
				pConnection->Close();

			}
		}
		catch (CException* e)
		{
			msg = UtilWin::SYGetMessage(*e);
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}


		return msg;
	}

	ERMsg CUIManitoba::LoadAgri2(CCallback& callback)
	{
		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		//std::map<string, CWeatherYears> data;
		m_agri2Stations.clear();

		StringVector fileList;
		for (size_t y = 0; y < nbYears&& msg; y++)
		{
			int year = firstYear + int(y);
			string filePath = GetOutputFilePath(AGRI2, DAILY_WEATHER, "*.txt", year);
			StringVector tmp = GetFilesList(filePath);
			fileList.insert(fileList.end(), tmp.begin(), tmp.end());
		}

		callback.PushTask("Load Manitoba Agri2 data (" + to_string(fileList.size()) + ")", fileList.size());
		for (size_t i = 0; i < fileList.size() && msg; i++)
		{
			ifStream file;
			msg = file.open(fileList[i]);
			if (msg)
			{
				enum THourlyColumns { STNID, STNNAME, DATEDT, AVGAIR_T, MAXAIR_T, MINAIR_T, AVGRH, RAIN24, NB_COLUMNS };
				static const WBSF::HOURLY_DATA::TVarH COL_POS_H[NB_COLUMNS] = { H_SKIP, H_SKIP, H_SKIP, H_TAIR, H_TMAX, H_TMIN, H_RELH, H_PRCP };

				for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
				{
					if (!loop->empty())
					{
						StringVector time((*loop)[DATEDT], "-");
						ASSERT(time.size() == 3);

						int year = ToInt(time[0]);
						size_t month = ToInt(time[1]) - 1;
						size_t day = ToInt(time[2]) - 1;

						ASSERT(month >= 0 && month < 12);
						ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));

						CTRef TRef = CTRef(year, month, day);
						string ID = (*loop)[STNID];

						for (size_t v = 0; v < loop->size(); v++)
						{
							if (COL_POS_H[v] != H_SKIP)
							{
								double value = ToDouble((*loop)[v]);
								if (value > -99)
								{
									m_agri2Stations[ID][TRef][COL_POS_H[v]] = value;
								}
							}
						}
					}//empty

					msg += callback.StepIt(0);
				}//for all line 

				msg += callback.StepIt();
			}//if msg
		}//for all files (days)

		callback.PopTask();


		if (msg)
		{
			//save data
			for (auto it1 = m_agri2Stations.begin(); it1 != m_agri2Stations.end(); it1++)
			{
				size_t pos = m_stations.FindByID(it1->first);
				if (pos != NOT_INIT)
					((CLocation&)it1->second) = m_stations[pos];
				else
					callback.AddMessage("Data without station information: " + it1->first, 1);
			}
		}//if msg

		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(m_agri2Stations.size()), 1);
		//callback.PopTask();

		return msg;
	}

	//******************************************************************************************************
	//Manitoba fire

	ERMsg CUIManitoba::ExecuteFire(CCallback& callback)
	{
		ERMsg msg;


		size_t type = as<size_t>(DATA_TYPE);
		if (type == DAILY_WEATHER)
			return msg;


		if (!FileExists(GetStationsListFilePath(FIRE)))
		{
			CLocationVector stationListTmp;
			msg = UpdateFireStationsList(stationListTmp, callback);
			if (msg)
				msg = stationListTmp.Save(GetStationsListFilePath(FIRE), ',', callback);
		}


		if (!msg)
			return msg;




		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[FIRE]) + "/" + SERVER_PATH[FIRE], 1);
		callback.AddMessage("");

		string fileName = "wx_last48.csv";
		string remoteFilePath = SERVER_PATH[FIRE] + fileName;
		string outputFilePath = workingDir + fileName;


		int nbRun = 0;
		bool bDownloaded = false;

		while (!bDownloaded && nbRun < 5 && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			ERMsg msgTmp = GetHttpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
			if (msgTmp)
			{
				try
				{
					msgTmp += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_TRANSFER_BINARY);


					//split data in seperate files
					if (msgTmp)
					{
						ASSERT(FileExists(outputFilePath));
						msg = SplitFireData(outputFilePath, callback);
						RemoveFile(outputFilePath);

						msg += callback.StepIt();
						bDownloaded = true;
					}
				}
				catch (CException* e)
				{
					msgTmp = UtilWin::SYGetMessage(*e);
				}


				//clean connection
				pConnection->Close();
				pSession->Close();
			}
			else
			{
				if (nbRun > 1 && nbRun < 5)
				{
					msg += WaitServer(10, callback);
				}
			}
		}

		//callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI), 1);
		//callback.PopTask();

		return msg;
	}

	ERMsg CUIManitoba::SplitFireData(const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		CTM TM(CTM::HOURLY);

		std::map<string, CWeatherYears> data;

		ifStream file;
		msg = file.open(outputFilePath);
		if (msg)
		{
			callback.PushTask("Split data", file.length());

			CWeatherAccumulator stat(TM);
			string lastID;

			//station	datetime	tmp	rh	wd	cwd	ws	wsmax	rn_1
			enum THourlyColumns { C_STATION, C_DATETIME, C_TMP, C_RH, C_WD, C_CWD, C_WS, C_WSMAX, C_RN_1, NB_COLUMNS };
			static const WBSF::HOURLY_DATA::TVarH COL_POS_H[NB_COLUMNS] = { H_SKIP, H_SKIP, H_TAIR, H_RELH, H_WNDD, H_SKIP, H_WNDS, H_SKIP, H_PRCP };

			for (CSVIterator loop(file); loop != CSVIterator() && msg; ++loop)
			{
				if (!loop->empty())
				{
					StringVector time((*loop)[C_DATETIME], "-: T");
					ASSERT(time.size() == 8);

					int year = ToInt(time[0]);
					size_t month = ToInt(time[1]) - 1;
					size_t day = ToInt(time[2]) - 1;
					size_t hour = ToInt(time[3]);

					ASSERT(month >= 0 && month < 12);
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
					ASSERT(hour >= 0 && hour < 24);

					CTRef TRef = CTRef(year, month, day, hour);
					string ID = (*loop)[C_STATION];


					if (ID != lastID)
					{
						if (data.find(ID) == data.end())
						{
							data[ID] = CWeatherYears(true);
							//try to load old data before changing it...
							string filePath = GetOutputFilePath(FIRE, HOURLY_WEATHER, ID, year);
							data[ID].LoadData(filePath, -999, false);//don't erase other years when multiple years
						}

						lastID = ID;
					}

					if (stat.TRefIsChanging(TRef))
					{
						data[lastID][stat.GetTRef()].SetData(stat);
					}


					for (size_t v = 0; v < loop->size(); v++)
					{
						size_t cPos = COL_POS_H[v];
						if (cPos < NB_VAR_H && (*loop)[v] != "NULL")
						{
							double value = ToDouble((*loop)[v]);
							if (value > -99)
							{
								stat.Add(TRef, cPos, value);
								if (cPos == H_RELH && (*loop)[C_TMP] != "NULL")
								{
									double T = ToDouble((*loop)[C_TMP]);
									double Hr = ToDouble((*loop)[C_RH]);
									stat.Add(TRef, H_TDEW, Hr2Td(T, Hr));
								}
							}
						}
					}
				}//empty

				msg += callback.StepIt(loop->GetLastLine().length() + 2);
			}//for all line (


			if (stat.GetTRef().IsInit() && data.find(lastID) != data.end())
				data[lastID][stat.GetTRef()].SetData(stat);


			if (msg)
			{
				//save data
				for (auto it1 = data.begin(); it1 != data.end(); it1++)
				{
					for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
					{
						string filePath = GetOutputFilePath(FIRE, HOURLY_WEATHER, it1->first, it2->first);
						string outputPath = GetPath(filePath);
						CreateMultipleDir(outputPath);
						it2->second->SaveData(filePath, TM);
					}
				}
			}//if msg

			callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(data.size()), 1);
			callback.PopTask();
		}//if msg

		return msg;
	}

	ERMsg CUIManitoba::UpdateFireStationsList(CLocationVector& locations, CCallback& callback)
	{
		ERMsg msg;

		size_t nbDownload = 0;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		msg = GetHttpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (msg)
		{

			//http://www.gov.mb.ca/sd/fire/Wx-Display/weatherview/stns_geojson.js
			string str;
			msg = UtilWWW::GetPageText(pConnection, "sd/fire/Wx-Display/weatherview/stns_geojson.js", str);
			if (msg)
			{
				str = str.substr(13, str.size() - 14 - 1);

				string error;
				Json jsonfeatures = Json::parse(str, error);

				if (error.empty())
				{
					Json::array stations = jsonfeatures["features"].array_items();
					callback.PushTask("Update fire stations list", stations.size());

					for (Json::array::const_iterator it = stations.begin(); it != stations.end() && msg; it++)
					{
						Json::object metadata = it->object_items();

						CLocation location;

						location.m_ID = metadata["properties"]["Stn_ID"].string_value();
						location.m_name = WBSF::PurgeFileName(metadata["properties"]["Stn_Name"].string_value());
						location.m_lat = metadata["geometry"]["coordinates"][1].number_value();
						location.m_lon = metadata["geometry"]["coordinates"][0].number_value();

						location.SetSSI("Owner", metadata["properties"]["OWNER"].string_value());
						location.SetSSI("Country", "CAN");
						location.SetSSI("SubDivision", "MB");
						location.SetSSI("Region", metadata["properties"]["REGION"].string_value());
						location.SetSSI("Zone", metadata["properties"]["I_A_ZONE"].string_value());

						locations.push_back(location);

						msg += callback.StepIt();
					}//for all stations


					msg += callback.StepIt();
					nbDownload++;
				}
				else
				{
					msg.ajoute(error);
				}
			}//if msg
		}//if msg

		pConnection->Close();
		pSession->Close();

		//lat/lon is valid
		ASSERT(locations.IsValid(true));
		//if missing elevation, extract elevation at 30 meters
		if (!locations.IsValid(false))
			locations.ExtractOpenTopoDataElevation(false, COpenTopoDataElevation::NASA_SRTM30M, COpenTopoDataElevation::I_BILINEAR, callback);

		//if still missing elevation, extract elevation at 90 meters
		if (!locations.IsValid(false))
			locations.ExtractOpenTopoDataElevation(false, COpenTopoDataElevation::NASA_SRTM90M, COpenTopoDataElevation::I_BILINEAR, callback);

		if (!locations.IsValid(false))
		{

			for (CLocationVector::iterator it = locations.begin(); it != locations.end(); )
			{
				if (!it->IsValid(false))
				{
					callback.AddMessage("WARNING: invalid coordinate :" + it->m_name + "(" + it->m_ID + "), " + "lat=" + to_string(it->m_lat) + ", lon=" + to_string(it->m_lon) + ", elev=" + to_string(it->m_alt), 2);
					it = locations.erase(it);
				}
				else
				{
					it++;
				}
			}
		}

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(locations.size()), 2);
		callback.PopTask();


		return msg;
	}






	//******************************************************************************************************


	//HG - water level
	//TW - water temperature
	//TA - air temperature
	//UD - wind direction
	//US - wind speed
	//UG - wind gust
	//PC - precipitation
	//XR - relative humidity
	//PA - atmospheric pressure

	enum TVariables { H_WATER_LEVEL, H_WATER_TEMPERATURE, H_AIR_TEMPERATURE, H_WIND_DIRECTION, H_WIND_SPEED, H_WIND_GUST, H_PRECIPITATION, H_RELATIVE_HUMIDITY, H_ATMOSPHERIC_PRESSURE, NB_MAN_VARS };
	static char* HYDRO_VAR_NAME[NB_MAN_VARS] = { "HG", "TW", "TA", "UD", "US", "UG", "PC", "XR", "PA" };
	static const TVarH HYDRO_VAR[NB_MAN_VARS] = { H_ADD1, H_ADD2, H_TAIR, H_WNDD, H_WNDS, H_SKIP, H_PRCP, H_RELH, H_PRES };
	static size_t GetVar(string filePath)
	{
		size_t var = NOT_INIT;

		string title = GetFileTitle(filePath);
		string varID = title.substr(0, 2);
		MakeUpper(varID);
		for (size_t i = 0; i < NB_MAN_VARS && var == NOT_INIT; i++)
		{
			if (varID == HYDRO_VAR_NAME[i])
				var = HYDRO_VAR[i];
		}

		return var;
	}




	void ExcelSerialDateToDMY(double fSerialDate, int &nYear, size_t &nMonth, size_t &nDay, size_t &nHour, size_t &nMinute)
	{

		int nSerialDate = int(fSerialDate);

		// Excel/Lotus 123 have a bug with 29-02-1900. 1900 is not a
		// leap year, but Excel/Lotus 123 think it is...
		if (nSerialDate == 60)
		{
			nDay = 29;
			nMonth = 2;
			nYear = 1900;

			return;
		}
		else if (nSerialDate < 60)
		{
			// Because of the 29-02-1900 bug, any serial date 
			// under 60 is one off... Compensate.
			nSerialDate++;
		}

		// Modified Julian to DMY calculation with an addition of 2415019
		int l = nSerialDate + 68569 + 2415019;
		int n = int((4 * l) / 146097);
		l = l - int((146097 * n + 3) / 4);
		int i = int((4000 * (l + 1)) / 1461001);
		l = l - int((1461 * i) / 4) + 31;
		int j = int((80 * l) / 2447);
		nDay = l - int((2447 * j) / 80);
		l = int(j / 11);
		nMonth = j + 2 - (12 * l);
		nYear = 100 * (n - 49) + i + l;


		fSerialDate -= nSerialDate;
		fSerialDate *= 24;
		nHour = int(fSerialDate);
		fSerialDate -= nHour;
		fSerialDate *= 60;
		nMinute = int(fSerialDate + 0.5);
		if (nMinute == 60)
		{
			nHour++;
			nMinute = 0;
		}

		nMonth--;
		nDay--;
	}

	ERMsg CUIManitoba::ExecuteHydro(CCallback& callback)
	{
		ERMsg msg;

		//liste des stations
		//https ://www.hydro.mb.ca/hydrologicalData/static/data/stationdata.json?

		//</table>
		//https://www.hydro.mb.ca
		//hydrologicalData/static/stations/05TG746/Parameter/HG/ContinuousWeek.xls


		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath(HYDRO));

		if (msg)
			msg += locations.IsValid();

		if (msg)
		{

			size_t type = as<size_t>(DATA_TYPE);

			string fileName = type == HOURLY_WEATHER ? "ContinuousWeek.xls" : "DayMeanYear.xls";


			callback.PushTask("Update Manitoba Hydro weather data (" + ToString(locations.size()) + " stations)", locations.size()*NB_MAN_VARS);
			size_t curI = 0;
			int nbRun = 0;
			//bool bDownloaded = false;
			size_t nbDownload = 0;

			while (curI < locations.size() && nbRun < 5 && msg)
			{
				nbRun++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;

				ERMsg msgTmp = GetHttpConnection(SERVER_NAME[HYDRO], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
				if (msgTmp)
				{
					TRY
					{

						for (size_t i = curI; i < locations.size() && msg; i++)
						{
							if (locations[i].UseIt())
							{

								StringVector filePath;
								string ID = locations[i].m_ID;
								//string ID = "05UH737";
								for (size_t v = 0; v < NB_MAN_VARS&&msg; v++)
								{
									if (HYDRO_VAR[v] != NOT_INIT)
									{
										string remoteFilePath = "hydrologicalData/static/stations/" + ID + "/Parameter/" + HYDRO_VAR_NAME[v] + "/" + fileName;
										string outputFilePath = GetDir(WORKING_DIR) + SUBDIR_NAME[HYDRO] + "\\" + HYDRO_VAR_NAME[v] + "_" + fileName;

										if (FileExists(outputFilePath))
											msg += RemoveFile(outputFilePath);

										msgTmp += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY);

										//split data in seperate files
										if (msgTmp && FileExists(outputFilePath))
										{
											ifStream file;
											if (file.open(outputFilePath))
											{
												string str = file.GetText();
												file.close();

												if (str.find("Sorry, the page was not found") == NOT_INIT)
												{
													string tmpFilePath = outputFilePath;
													SetFileExtension(tmpFilePath, ".csv");

													if (FileExists(tmpFilePath))
														msg += RemoveFile(tmpFilePath);

													if (msg)
													{
														string xls2csv = GetApplicationPath() + "External\\xls2csv.exe";
														string command = xls2csv + " \"" + outputFilePath + "\" \"" + tmpFilePath + "\"";
														WinExecWait(command);

														if (FileExists(tmpFilePath))
															filePath.push_back(tmpFilePath);
														else
															msg.ajoute("Unable to convert " + GetFileName(outputFilePath) + " to " + GetFileName(tmpFilePath));
													}
												}
												else
												{
													ASSERT(FileExists(outputFilePath));
													RemoveFile(outputFilePath);
												}
											}//if valid file
										}//if file exist
									}//for all variables

									msg += callback.StepIt();

								}//for all var

								if (msg && !filePath.empty())
								{
									nbDownload++;
									msg += SplitHydroData(locations[i].m_ID, filePath, callback);
									if (msg)
									{
										//remove files
										for (size_t i = 0; i < filePath.size(); i++)
										{
											//delete .csv file
											msg += RemoveFile(filePath[i]);
											//delete .xls file
											msg = RemoveFile(SetFileExtension(filePath[i], ".xls"));
										}

									}
								}
							}//if used it
							else
							{
								msg += callback.StepIt(NB_MAN_VARS);
							}

							curI++;
						}//for all stations
					}
						CATCH_ALL(e)
					{
						msgTmp = UtilWin::SYGetMessage(*e);
					}
					END_CATCH_ALL

						//clean connection
						pConnection->Close();
					pSession->Close();
				}
				else
				{
					if (nbRun > 1 && nbRun < 5)
					{
						msg += WaitServer(10, callback);
					}
				}
			}


			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload), 1);
			callback.PopTask();
		}



		return msg;
	}


	ERMsg CUIManitoba::SplitHydroData(const string& ID, const StringVector& outputFilePath, CCallback& callback)
	{
		ASSERT(!outputFilePath.empty());
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);
		CTM TM(type == HOURLY_WEATHER ? CTM::HOURLY : CTM::DAILY);

		CWeatherYears data(type == HOURLY_WEATHER);

		vector<size_t> vars(outputFilePath.size());
		vector<ifStream> files(outputFilePath.size());
		for (size_t i = 0; i < outputFilePath.size(); i++)
		{
			msg += files[i].open(outputFilePath[i]);
			vars[i] = GetVar(outputFilePath[i]);
		}


		if (msg)
		{
			CWeatherAccumulator stat(TM);

			for (size_t i = 0; i < outputFilePath.size() && msg; i++)
			{
				string junk;
				for (int l = 0; l < 8; l++)
					getline(files[i], junk);

				for (CSVIterator loop(files[i]); loop != CSVIterator() && msg; ++loop)
				{
					if (loop->size() >= 2)
					{
						int year = 0;
						size_t month = 0;
						size_t day = 0;
						size_t hour = 0;
						size_t minute = 0;

						double fSerialDate = ToDouble((*loop)[0]);
						ExcelSerialDateToDMY(fSerialDate, year, month, day, hour, minute);


						ASSERT(month >= 0 && month < 12);
						ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
						ASSERT(hour >= 0 && hour < 24);
						ASSERT(minute >= 0 && minute < 60);

						CTRef TRef = type == HOURLY_WEATHER ? CTRef(year, month, day, hour) : CTRef(year, month, day);

						if (stat.TRefIsChanging(TRef))
						{

							if (!data.IsYearInit(TRef.GetYear()))
							{
								//try to load old data before changing it...
								string filePath = GetOutputFilePath(HYDRO, type, ID, year);
								data.LoadData(filePath, -999, false);//don't erase other years when multiple years
							}

							data[stat.GetTRef()].SetStat((TVarH)vars[i], stat.GetStat(vars[i]));
						}

						double value = ToDouble((*loop)[1]);
						if (value > -99)
						{

							if (vars[i] == H_PRES)
								value *= 10;//kPa --> hPa

							//if (cPos == H_RELH)
								//value = max(1.0, min(100.0, value));

							stat.Add(TRef, vars[i], value);

						}//if good

					}//size >= 2

					msg += callback.StepIt(0);
				}//for all line 

				if (stat.GetTRef().IsInit())
					data[stat.GetTRef()].SetStat((TVarH)vars[i], stat.GetStat(vars[i]));


			}//for all files (variables)


			if (msg)
			{
				//Compute Tdew if hourly data
				if (type == HOURLY_WEATHER)
				{
					CTPeriod p = data.GetEntireTPeriod();
					for (CTRef h = p.Begin(); h <= p.End(); h++)
					{
						CStatistic T = data[h][H_TAIR];
						CStatistic Hr = data[h][H_RELH];
						if (T.IsInit() && Hr.IsInit())
							data[h].SetStat(H_TDEW, Hr2Td(T[MEAN], Hr[MEAN]));
					}
				}

				//save all years 
				for (auto it = data.begin(); it != data.end(); it++)
				{
					string filePath = GetOutputFilePath(HYDRO, type, ID, it->first);
					string outputPath = GetPath(filePath);
					CreateMultipleDir(outputPath);
					it->second->SaveData(filePath, TM);
				}
			}//if msg


			//callback.PopTask();
		}//if msg

		return msg;
	}

	ERMsg CUIManitoba::UpdateHydroStationsList(CCallback& callback)
	{
		ERMsg msg;

		//get stations 

		//<table border="0"  cellspacing="0" cellpadding="0">
		//</table>
		//https://www.hydro.mb.ca
		//hydrologicalData/static/stations/05TG746/Parameter/HG/ContinuousWeek.xls
		//hydrologicalData/static/stations/05TG746/Parameter/HG/DayMeanYear.xls

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;
		// /hydrologicalData/static/
		msg = GetHttpConnection("www.hydro.mb.ca", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (msg)
		{

			string str;
			msg = UtilWWW::GetPageText(pConnection, "hydrologicalData/static/data/stationdata.json?", str);
			if (msg)
			{
				string error;
				std::vector<Json>& items = Json::parse_multi(str, error);
				//Json json = Json::parse(str, error);

				if (!error.empty())
				{
					msg.ajoute(error);
					return msg;
				}
				else if (items.size() == 1)
				{
					const Json& item = items.front();
					bool bTest = item.is_object();
					const Json& item2 = item["features"];
					CLocationVector locations;

					//const Json& item2 = item["features"];

					ASSERT(item2.is_array());
					for (size_t j = 0; j < item2.array_items().size(); j++)
					{
						const Json& item3 = item2[j]["attributes"];
						const Json& item4 = item2[j]["geometry"];
						//:{"x":-96.101388888889, "y" : 56.086388888889}
						string test = item3["station_latitude"].string_value();


						string name = item3["station_name"].string_value();
						string ID = item3["station_no"].string_value();
						double lat = item4["y"].number_value();
						double lon = item4["x"].number_value();
						CLocation location(name, ID, lat, lon, -999);
						location.SetSSI("Network", NETWORK_NAME[HYDRO]);
						location.SetSSI("StationNo", item3["station_id"].string_value());
						location.SetSSI("RiverName", item3["river_name"].string_value());
						location.SetSSI("InstallDate", item3["gen.shelterinstall"].string_value());
						locations.push_back(location);

						msg += callback.StepIt();

					}


					//temporary file path without elevation
					string filePath = GetDir(WORKING_DIR) + SUBDIR_NAME[HYDRO] + "\\Stations.csv";
					msg = locations.Save(filePath);

				}
			}


			//clean connection
			pConnection->Close();
			pSession->Close();
		}

		return msg;

	}


	ERMsg CUIManitoba::ExecutePotato(CCallback& callback)
	{
		ERMsg msg;

		size_t type = as<size_t>(DATA_TYPE);

		if (type == HOURLY_WEATHER)
		{
			callback.AddMessage("Hourly data is not available for MB potato network");
			return msg;
		}

		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath(POTATO));

		if (msg)
		{
			size_t nbDownloads = 0;
			callback.PushTask("Update Manitoba Potatoes weather data (" + ToString(locations.size()) + " stations)", locations.size());

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME[POTATO], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
			if (msg)
			{
				TRY
				{

					for (size_t i = 0; i < locations.size() && msg; i++)
					{
						StringVector filePath;
						string ID = locations[i].m_ID;

						string URL = FormatA("/weather.cfm?stn=%s", ID.c_str());

						string str;
						msg = UtilWWW::GetPageText(pConnection, URL, str);
						if (msg)
						{
							string::size_type pos1 = str.find("<table id=\"StationInfoTable\">");
							string::size_type pos2 = str.find("</table>");
							if (pos1 < str.size() && pos2 < str.size())
							{
								nbDownloads++;
								string::size_type count = (pos2 - pos1) + 8;
								string source = str.substr(pos1, count);
								WBSF::ReplaceString(source, "'", "\"");
								WBSF::ReplaceString(source, "\t", "");
								WBSF::ReplaceString(source, "<sup>2</sup>", "≤");
								WBSF::ReplaceString(source, "&deg;", "∞");


								msg += SplitPotatoData(locations[i].m_ID, source);
							}//if valid 
						}//msg

						msg += callback.StepIt();
					}//for all locations

				}
					CATCH_ALL(e)
				{
					msg = UtilWin::SYGetMessage(*e);
				}
				END_CATCH_ALL

					//clean connection
					pConnection->Close();
				pSession->Close();
			}//if msg

			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownloads), 1);
			callback.PopTask();
		}//if msg

		return msg;
	}

	ERMsg CUIManitoba::SplitPotatoData(const string& ID, const string& source)
	{
		ERMsg msg;

		try
		{
			CWeatherYears data(false);

			string xml_str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" + source;
			zen::XmlDoc doc = zen::parse(xml_str);

			std::pair<zen::XmlElement::ChildIter, zen::XmlElement::ChildIter> trs = doc.root().getChildren();
			if (std::distance(trs.first, trs.second) > 2)
			{
				zen::XmlElement::ChildIter trit = trs.first;
				trit++;

				std::pair<zen::XmlElement::ChildIter, zen::XmlElement::ChildIter> ths = trit->getChildren();

				size_t index = UNKNOWN_POS;
				zen::XmlElement::ChildIter th = ths.first;
				for (size_t i = 0; th != ths.second&&index == -1; i++, ++th)
				{
					string header;
					th->getValue(header);
					if (header.find("Solar Radiation") != string::npos)
						index = i;
				}

				if (index != UNKNOWN_POS)
				{
					trit++;
					for (; trit != trs.second; ++trit)
					{
						std::pair<zen::XmlElement::ChildIter, zen::XmlElement::ChildIter> tds = trit->getChildren();
						zen::XmlElement::ChildIter tdit = tds.first;

						string date_str;
						tdit->getValue(date_str);
						for (size_t i = 0; i < index; i++)
							tdit++;

						string str;
						tdit->getValue(str);
						double value = !str.empty() ? ToDouble(str) : 0;
						StringVector tmp(date_str, "/");
						if (value > 0 && tmp.size() == 3)
						{
							size_t m = ToSizeT(tmp[0]) - 1;
							size_t d = ToSizeT(tmp[1]) - 1;
							int year = ToInt(tmp[2]);
							CTRef Tref(year, m, d);


							if (!data.IsYearInit(year))
							{
								//try to load old data before changing it...
								string filePath = GetOutputFilePath(POTATO, DAILY_WEATHER, ID, year);
								data.LoadData(filePath, -999, false);//don't erase other years when multiple years
							}

							value = value < 1000 ? value * 10.0 : value / 100.0;
							if (value > 0 && value < 500)
								data[Tref].SetStat(H_SRAD, value);
						}// tmp == 3 and sRad is init
					}

					//save data
					for (CWeatherYearMap::iterator it = data.begin(); it != data.end(); it++)
					{
						string filePath = GetOutputFilePath(POTATO, DAILY_WEATHER, ID, it->first);
						string outputPath = GetPath(filePath);
						CreateMultipleDir(outputPath);
						it->second->SaveData(filePath);
					}
				}//if index != -1
			}//if > 2

		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
		}

		return msg;
	}
}



