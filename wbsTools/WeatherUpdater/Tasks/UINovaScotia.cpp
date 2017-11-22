#include "StdAfx.h"
#include "UINovaScotia.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "UI/Common/UtilWin.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"
//#include "json\json11.hpp"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;
//using namespace json11;

namespace WBSF
{

	//https://novascotia.ca/natr/forestprotection/wildfire/fwi/Fire-Weather-Forecast-Actuals.asp
	//https://novascotia.ca/natr/forestprotection/wildfire/fwi/FWIActuals.xml
	//https://www.novascotia.ca/natr/forestprotection/wildfire/fwi/stations/pdf/Milton_Hourly.pdf

	//Stations coordinate take from :
	//https://www.google.fr/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&uact=8&ved=0ahUKEwjmqpPqy-vSAhXL8RQKHffuAIoQFggaMAA&url=http%3A%2F%2Fbatchgeo.com%2Fmap%2Fkml%2Ffts_raws2&usg=AFQjCNE51Cd65d-iB3BjhjgHI2I53sch4w&sig2=wfhSgE1V_JF0c8rXI-X7Pg&bvm=bv.150475504,d.amc

	const char* CUINovaScotia::SUBDIR_NAME[NB_NETWORKS] = { "Fire" };
	const char* CUINovaScotia::NETWORK_NAME[NB_NETWORKS] = { "Nova Scotia Fire" };
	const char* CUINovaScotia::SERVER_NAME[NB_NETWORKS] = { "Ftpque.nrcan.gc.ca"};
	const char* CUINovaScotia::SERVER_PATH[NB_NETWORKS] = { "/NSWeather/"};

	size_t CUINovaScotia::GetNetwork(const string& network)
	{
		size_t n = NOT_INIT;

		for (size_t i = 0; i <NB_NETWORKS && n == NOT_INIT; i++)
		{
			if (IsEqualNoCase(network, NETWORK_NAME[i]))
				n = i;
		}

		return n;
	}

	//*********************************************************************
	const char* CUINovaScotia::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UsderName", "Password", "WorkingDir", "FirstYear", "LastYear" };
	const size_t CUINovaScotia::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING }; 
	const UINT CUINovaScotia::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOVASCOTIA_P;
	const UINT CUINovaScotia::DESCRIPTION_TITLE_ID = ID_TASK_NOVASCOTIA;

	const char* CUINovaScotia::CLASS_NAME(){ static const char* THE_CLASS_NAME = "NovaScotia";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINovaScotia::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINovaScotia::CLASS_NAME(), (createF)CUINovaScotia::create);



	CUINovaScotia::CUINovaScotia(void)
	{
		string year = ToString(WBSF::GetCurrentYear());

		Set("FirstYear", year);
		Set("LastYear", year);

	}

	CUINovaScotia::~CUINovaScotia(void)
	{}



	std::string CUINovaScotia::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string CUINovaScotia::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Nova-Scotia\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//****************************************************


	std::string CUINovaScotia::GetStationsListFilePath(size_t network)const
	{
		string filePath = GetDir(WORKING_DIR) + "NS_Wx_Stations_List.csv";
		return filePath;
	}

	string CUINovaScotia::GetOutputFilePath(int year, const string& name)const
	{
		//+ SUBDIR_NAME[network] 
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + name + ".csv";
	}


	ERMsg CUINovaScotia::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[FIRE]) + "/", 1);
		callback.AddMessage("");


		StringVector fileList;

		msg += UpdateStationList(callback);

		if (!msg)
			return msg;


		size_t nbDownloads = 0;

		int firstYear = WBSF::as<int>(Get(FIRST_YEAR));
		int lastYear = WBSF::as<int>(Get(LAST_YEAR));
		size_t nbYears = lastYear - firstYear + 1;

		//callback.AddMessage(GetString(IDS_NUMBER_FILES) + ToString(nbYears), 1);
		callback.AddMessage("");

		callback.PushTask("Update Nova-Scotia weather data (" + ToString(nbYears) + " files)", nbYears);

		//open a connection on the server
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true);
		if (msg)
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 45000);

			//add station list
			for (size_t y = 0; y < nbYears; y++)
			{
				int year = firstYear + int(y);

				string filter = "/hydromanitoba/NSWeather/" + ToString(year) + "/*.CSV";

				CFileInfoVector fileList;
				msg = FindFiles(pConnection, filter, fileList, callback);
				

				for (size_t i = 0; i < fileList.size(); i++)
				{
					string outputFilePath = GetOutputFilePath(year, GetFileTitle(fileList[i].m_filePath));
					if (!IsFileUpToDate(fileList[i], outputFilePath))
					{
						CreateMultipleDir(GetPath(outputFilePath));
						msg = CopyFile(pConnection, fileList[i].m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);

						if (msg)
						{
							nbDownloads++;
							msg += callback.StepIt();
						}
					}
				}
			}


			pConnection->Close();
			pSession->Close();
		}



		callback.AddMessage("Number of file downloaded: " + ToString(nbDownloads));
		callback.PopTask();

		return msg;
	}

	ERMsg CUINovaScotia::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

	
		m_stations.clear();
	
		CLocationVector locations;
		msg = locations.Load(GetStationsListFilePath(FIRE));

		if (msg)
			msg += locations.IsValid();

		//Update network
		for (size_t i = 0; i < locations.size(); i++)
		{
			//#locations[i].m_ID = locations[i].GetSSI("NESDIS ID");
			//locations[i].SetSSI("NESDIS ID", "");
			locations[i].SetSSI("Network", NETWORK_NAME[FIRE]);
		}
			

		m_stations.insert(m_stations.end(), locations.begin(), locations.end());
				
		for (size_t i = 0; i < locations.size(); i++)
			stationList.push_back(ToString(FIRE)+"/"+locations[i].m_ID);
	

		return msg;
	}

	ERMsg CUINovaScotia::GetWeatherStation(const std::string& NID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		size_t n = ToSizeT(NID.substr(0,1));
		string ID = NID.substr(2);

		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[it];

		int firstYear = WBSF::as<int>(Get("FirstYear"));
		int lastYear = WBSF::as<int>(Get("LastYear"));
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string name = station.m_name;
			string filePath = GetOutputFilePath(year, name);
			if (FileExists(filePath))
				msg = ReadDataFile(filePath, TM, station, callback);

			msg += callback.StepIt(0);
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		ReplaceString(station.m_name, "_", " ");
		return msg;
	}


	//******************************************************************************************************

	ERMsg CUINovaScotia::UpdateStationList(CCallback& callback)
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true);

		if (msg)
		{
			string path = "/hydromanitoba/NSWeather/NS_Wx_Stations_List.csv";

			CFileInfoVector fileList;
			msg = FindFiles(pConnection, path, fileList, callback);

			if (msg)
			{
				ASSERT(fileList.size() == 1);

				string outputFilePath = GetStationsListFilePath(FIRE);
				if (!IsFileUpToDate(fileList.front(), outputFilePath))
				{
					CreateMultipleDir(GetPath(outputFilePath));
					msg = CopyFile(pConnection, fileList.front().m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
					if (msg)
					{
						//
						//replace header line
						ifStream file;
						msg = file.open(outputFilePath);
						if (msg)
						{
							string line;
							getline(file, line);
							string text = "Name,ID,Type,Latitude,Longitude,LatDeg,LongDeg,Easting,Northing,Elevation\n";

							while (getline(file, line))
							{
								text += line + "\n";
							}
							
							file.close();
							
							ofStream file;
							msg = file.open(outputFilePath);
							if(msg)
							{
								file << text;
								file.close();
							}
						}
					}
				}

			}

			pConnection->Close();
			pSession->Close();

		}

		return msg;
	}



	enum TColumns{ C_STATIONNAME, C_DATE_TIME, C_RH, C_RHMAX, C_RHMAX24, C_RHMIN, C_RHMIN24, C_RNTOTAL, C_RN_1, C_TMAX, C_TMAX24, C_TMIN, C_TMIN24, C_TEMP, C_WSPD, C_MAX_SPD, C_MAX_DIR, C_DIR, NB_COLUMNS };
	//static const char* COLUMN_NAME[NB_COLUMNS] = { "wxStation", "Date", "Temp", "Rh", "Wspd", "Max_Spd", "Dir", "Rn1", "Rn24", "FFMC", "hFFMC", "DMC", "DC", "ISI", "hISI", "BUI", "FWI" };
	static const char* COLUMN_NAME[NB_COLUMNS] = { "StationName", "DateTime", "Rh", "RhMax", "RhMax24", "RhMin", "RhMin24", "RnTotal", "Rn_1", "TMax", "TMax24", "TMin", "TMin24", "Temp", "Wspd", "Max_Spd", "Max_Dir", "Dir" };
	static const TVarH FIRE_VAR[NB_COLUMNS] = { H_SKIP, H_SKIP, H_RELH, H_RELH, H_SKIP, H_RELH, H_SKIP, H_SKIP, H_PRCP, H_TMAX2, H_SKIP, H_TAIR2, H_SKIP, H_TMIN2, H_WNDS, H_SKIP, H_SKIP, H_WNDD };
	//Rn24, RHMin24, RHMax24, TMIN24 and TMAX24 is valid only at noon and is NOON to NOON computed

	
	CTRef CUINovaScotia::GetTRef(string str)
	{
		CTRef TRef;

		//2017/01/01 00:00:00
		StringVector vec(str, " :/");
		if (vec.size() == 6)
		{
			int year = WBSF::as<int>(vec[0]);
			size_t month = WBSF::as<size_t>(vec[1]) - 1;
			size_t day = WBSF::as<size_t>(vec[2]) - 1;
			size_t hour = WBSF::as<size_t>(vec[3]);

			ASSERT(month >= 0 && month < 12);
			ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
			ASSERT(hour >= 0 && hour < 24);


			TRef = CTRef(year, month, day, hour);// +1;
		}


		return TRef;

	}

	ERMsg CUINovaScotia::ReadDataFile(const string& filePath, CTM TM, CWeatherYears& data, CCallback& callback)const
	{
		ERMsg msg;
	
		//now extract data 
		ifStream file;

		msg = file.open(filePath);

		if (msg)
		{
			CWeatherAccumulator stat(TM);
			double lastPrcp = 0;

			for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
			{
				CTRef TRef = GetTRef((*loop)[C_DATE_TIME]);
				if (TRef.IsInit())
				{
					if (stat.TRefIsChanging(TRef))
						data[stat.GetTRef()].SetData(stat);

					for (size_t c = 0; c < loop->size(); c++)
					{
						if (c<NB_COLUMNS && FIRE_VAR[c] != H_SKIP && !(*loop)[c].empty())
						{
							double value = ToDouble((*loop)[c]);

							if (c==C_RNTOTAL && TM.IsDaily())
							{

							}
							else
							{
								stat.Add(TRef, FIRE_VAR[c], value);

								if (c == C_RH)
								{
									double T = ToDouble((*loop)[C_TEMP]);
									ASSERT(T > -99 && T < 99);
									stat.Add(TRef, H_TDEW, Hr2Td(T, value));
								}
								else if (c == C_RHMIN)
								{
									double T = ToDouble((*loop)[C_TMAX]);//hourly maximum
									ASSERT(T > -99 && T < 99);
									stat.Add(TRef, H_TDEW, Hr2Td(T, value));
								}
								else if (c == C_RHMAX)
								{
									double T = ToDouble((*loop)[C_TMIN]);//hourly minimum
									ASSERT(T > -99 && T < 99);
									stat.Add(TRef, H_TDEW, Hr2Td(T, value));
								}
							}
						}//if valid value
					}//for all columns
				}//TRef is init

				msg += callback.StepIt(0);
			}//for all line (

			if (stat.GetTRef().IsInit())
				data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}


}
//ERMsg CUINovaScotia::ExecuteFire(CCallback& callback)
//{
//	ERMsg msg;

//	callback.PushTask("Update Nova-Scotia fire weather data (1 day)", 1 );

//	CInternetSessionPtr pSession;
//	CHttpConnectionPtr pConnection;

//	msg = GetHttpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
//	if (msg)
//	{
//		TRY
//		{
//			string remoteFilePath = "natr/forestprotection/wildfire/fwi/FWIActuals.xml";
//			string outputFilePath = GetDir(WORKING_DIR) + SUBDIR_NAME[FIRE] + "\\FWIActuals.xml";

//			msg += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE);
//			if (msg)
//				msg += SplitFireData(outputFilePath, callback);

//		}
//		CATCH_ALL(e)
//		{
//			msg = UtilWin::SYGetMessage(*e);
//		}
//		END_CATCH_ALL

//			//clean connection
//			pConnection->Close();
//		pSession->Close();
//	}

//	
//	callback.PopTask();
//

//	return msg;
//}
//

//
//ERMsg CUINovaScotia::SplitFireData(const std::string& outputFilePath, CCallback& callback)
//{
//	ASSERT(!outputFilePath.empty());

//	ERMsg msg;
//	
//	size_t nbStations  = 0;
//	
//	ifStream files;
//	msg += files.open(outputFilePath);

//	if (msg)
//	{
//		zen::XmlDoc doc = zen::parse(files.GetText());

//		
//		zen::XmlIn in(doc.root());
//		for (zen::XmlIn it = in["weather"]; it&&msg; it.next())
//		{
//			CWeatherYears dataH(true);
//			CWeatherYears dataD(false);

//			//nbStations

//			std::array<string, NB_COLUMNS> values;
//			for (size_t i= 0; i < NB_COLUMNS; i++)
//				it[COLUMN_NAME[i]](values[i]);

//			if (values[C_DATE] != "NA")
//			{
//				StringVector date(values[C_DATE], "/");
//				ASSERT(date.size() == 3);


//				size_t day = ToInt(date[0]) - 1;
//				size_t month = ToInt(date[1]) - 1;
//				int year = ToInt(date[2]);



//				ASSERT(month >= 0 && month < 12);
//				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));

//				string ID = values[C_WXSTATION];
//				string filePathH = GetOutputFilePath(FIRE, true, ID, year);
//				string filePathD = GetOutputFilePath(FIRE, false, ID, year);

//				if (FileExists(filePathH))
//					msg += dataH.LoadData(filePathH);

//				if (FileExists(filePathD))
//					msg += dataD.LoadData(filePathD);

//				CTRef TRefH = CTRef(year, month, day, 13);
//				CTRef TRefD = CTRef(year, month, day);

//				for (size_t i = 0; i < NB_COLUMNS; i++)
//				{
//					if (FIRE_VAR[i] != H_SKIP && values[i] != "NA")
//					{
//						if (i == C_RN24)
//							dataD.GetDay(TRefD).SetStat(FIRE_VAR[i], ToDouble(values[i]));
//						else
//							dataH.GetHour(TRefH).SetStat(FIRE_VAR[i], ToDouble(values[i]));
//					}
//				}

//				//set Tdew
//				if (dataH[TRefH][H_TAIR2].IsInit() && dataH[TRefH][H_RELH].IsInit())
//					dataH.GetHour(TRefH).SetStat(H_TDEW, WBSF::Hr2Td(dataH[TRefH][H_TAIR2][MEAN], dataH[TRefH][H_RELH][MEAN]));

//				//create directory
//				CreateMultipleDir(GetPath(filePathH));
//				CreateMultipleDir(GetPath(filePathD));

//				//save data
//				msg += dataH.SaveData(filePathH);
//				msg += dataD.SaveData(filePathD);
//				nbStations++;
//			}
//		}

//	}//if msg


//	callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(nbStations), 1);


//	return msg;
//}
