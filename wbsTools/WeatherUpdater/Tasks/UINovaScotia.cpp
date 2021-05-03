#include "StdAfx.h"
#include "UINovaScotia.h"
#include <boost\dynamic_bitset.hpp>
#include <boost\filesystem.hpp>

#include "UI/Common/UtilWin.h"
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Basic\CSV.h"

#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;


namespace WBSF
{

	//https://novascotia.ca/natr/forestprotection/wildfire/fwi/Fire-Weather-Forecast-Actuals.asp
	//https://novascotia.ca/natr/forestprotection/wildfire/fwi/FWIActuals.xml
	//https://www.novascotia.ca/natr/forestprotection/wildfire/fwi/stations/pdf/Milton_Hourly.pdf

	//Stations coordinate take from :
	//https://www.google.fr/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&uact=8&ved=0ahUKEwjmqpPqy-vSAhXL8RQKHffuAIoQFggaMAA&url=http%3A%2F%2Fbatchgeo.com%2Fmap%2Fkml%2Ffts_raws2&usg=AFQjCNE51Cd65d-iB3BjhjgHI2I53sch4w&sig2=wfhSgE1V_JF0c8rXI-X7Pg&bvm=bv.150475504,d.amc

	const char* CUINovaScotia::SUBDIR_NAME[NB_NETWORKS] = { "Fire" };
	const char* CUINovaScotia::NETWORK_NAME[NB_NETWORKS] = { "Nova Scotia Fire" };
	const char* CUINovaScotia::SERVER_NAME[NB_NETWORKS] = { "Ftpque.nrcan.gc.ca" };
	const char* CUINovaScotia::SERVER_PATH[NB_NETWORKS] = { "/NSWeather/" };

	size_t CUINovaScotia::GetNetwork(const string& network)
	{
		size_t n = NOT_INIT;

		for (size_t i = 0; i < NB_NETWORKS && n == NOT_INIT; i++)
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

	const char* CUINovaScotia::CLASS_NAME() { static const char* THE_CLASS_NAME = "NovaScotia";  return THE_CLASS_NAME; }
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
		
		string filePath = GetApplicationPath() + "Layers\\NovaScotiaStations.csv";
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


		//StringVector fileList;

//		msg += UpdateStationList(callback);

	//	if (!msg)
		//	return msg;


		size_t nbDownloads = 0;

		int firstYear = WBSF::as<int>(Get(FIRST_YEAR));
		int lastYear = WBSF::as<int>(Get(LAST_YEAR));
		size_t nbYears = lastYear - firstYear + 1;

		//callback.AddMessage(GetString(IDS_NUMBER_FILES) + ToString(nbYears), 1);
		callback.AddMessage("");

		callback.PushTask("Update Nova-Scotia weather data (" + ToString(nbYears) + " years)", nbYears);

		//open a connection on the server
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);
		if (msg)
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 45000);

			//add station list
			for (size_t y = 0; y < nbYears; y++)
			{
				int year = firstYear + int(y);

				string filter = "/hydromanitoba/NSWeather/" + ToString(year) + "/*.CSV";

				CFileInfoVector fileList;
				msg = FindFiles(pConnection, filter, fileList, false, callback);

				if (msg)
				{
					callback.PushTask("Update Nova-Scotia weather data for year : " + to_string(year) + "(" + to_string(fileList.size()) + " files)", fileList.size());

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

					callback.PopTask();
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

		msg = m_stations.Load(GetStationsListFilePath(FIRE));

		for (size_t i = 0; i < m_stations.size(); i++)
			stationList.push_back(m_stations[i].m_ID);

		return msg;
	}

	ERMsg CUINovaScotia::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//Get station information
		size_t it = m_stations.FindByID(ID);
		ASSERT(it != NOT_INIT);

		((CLocation&)station) = m_stations[it];

		int firstYear = WBSF::as<int>(Get("FirstYear"));
		int lastYear = WBSF::as<int>(Get("LastYear"));
		size_t nbYears = lastYear - firstYear + 1;

		station.SetHourly(true);
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




		/*string network = station.GetSSI("Network");
		string country = station.GetSSI("Country");
		string subDivisions = station.GetSSI("SubDivision");
		station.m_siteSpeceficInformation.clear();
		station.SetSSI("Network", network);
		station.SetSSI("Country", country);
		station.SetSSI("SubDivision", subDivisions);*/

		station.SetSSI("Provider", "Nova-Scotia");


		return msg;
	}


	//******************************************************************************************************

	//ERMsg CUINovaScotia::UpdateStationList(CCallback& callback)
	//{
	//	ERMsg msg;

	//	CInternetSessionPtr pSession;
	//	CFtpConnectionPtr pConnection;

	//	msg = GetFtpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);

	//	if (msg)
	//	{
	//		string path = "/hydromanitoba/NSWeather/NS_Wx_Stations_List.csv";

	//		CFileInfoVector fileList;
	//		msg = FindFiles(pConnection, path, fileList, false, callback);

	//		if (msg)
	//		{
	//			ASSERT(fileList.size() == 1);

	//			string outputFilePath = GetStationsListFilePath(FIRE);
	//			if (!IsFileUpToDate(fileList.front(), outputFilePath))
	//			{
	//				CreateMultipleDir(GetPath(outputFilePath));
	//				msg = CopyFile(pConnection, fileList.front().m_filePath, outputFilePath, INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
	//				if (msg)
	//				{
	//					//
	//					//replace header line
	//					ifStream file;
	//					msg = file.open(outputFilePath);
	//					if (msg)
	//					{
	//						string line;
	//						getline(file, line);
	//						string text = "Name,ID,Type,Latitude,Longitude,LatDeg,LongDeg,Easting,Northing,Elevation\n";

	//						while (getline(file, line))
	//						{
	//							text += line + "\n";
	//						}

	//						file.close();

	//						ofStream file;
	//						msg = file.open(outputFilePath);
	//						if (msg)
	//						{
	//							file << text;
	//							file.close();
	//						}
	//					}
	//				}
	//			}

	//		}

	//		pConnection->Close();
	//		pSession->Close();

	//	}

	//	return msg;
	//}



	enum TColumns { C_STATIONNAME, C_DATE_TIME, C_RH, C_RHMAX, C_RHMAX24, C_RHMIN, C_RHMIN24, C_RN24, C_RNTOTAL, C_RN_1, C_TMAX, C_TMAX24, C_TMIN, C_TMIN24, C_TEMP, C_WSPD, C_MAX_SPD, C_MAX_DIR, C_DIR, NB_COLUMNS };
	static const char* NS_COLUMN_NAME[NB_COLUMNS] = { "StationName", "DateTime", "Rh", "RhMax", "RhMax24", "RhMin", "RhMin24", "Rn24", "RnTotal", "Rn_1", "TMax", "TMax24", "TMin", "TMin24", "Temp", "Wspd", "Max_Spd", "Max_Dir", "Dir" };
	//static const TVarH NS_FIRE_VAR[NB_COLUMNS] = { H_SKIP, H_SKIP, H_RELH, H_RELH, H_SKIP, H_RELH, H_SKIP, H_SKIP, H_PRCP, H_TMAX, H_SKIP, H_TMIN, H_SKIP, H_TAIR, H_WNDS, H_SKIP, H_SKIP, H_WNDD };
	//Rn24, RHMin24, RHMax24, TMIN24 and TMAX24 is valid only at noon and is NOON to NOON computed


	static size_t GetNSColumn(const string& header)
	{
		size_t c = NOT_INIT;
		for (size_t i = 0; i < NB_COLUMNS&&c == NOT_INIT; i++)
		{
			if (IsEqual(header, NS_COLUMN_NAME[i]))
				c = i;
		}

		ASSERT(c != NOT_INIT);
		return c;
	}

	static vector<size_t> GetNSColumns(const StringVector& header)
	{
		vector<size_t> columns(header.size());


		for (size_t c = 0; c < header.size(); c++)
			columns[c] = GetNSColumn(header[c]);

		return columns;
	}
	static TVarH GetNSVariable(size_t type)
	{
		TVarH v = H_SKIP;

		if (type == C_RHMAX || type == C_RHMIN || type == C_RH)
			v = H_RELH;
		else if (type == C_DIR)
			v = H_WNDD;
		else if (type == C_WSPD)
			v = H_WNDS;
		else if (type == C_RN_1)
			v = H_PRCP;
		else if (type == C_TMIN)
			v = H_TMIN;
		else if (type == C_TEMP)
			v = H_TAIR;
		else if (type == C_TMAX)
			v = H_TMAX;

		return v;
	}

	static vector<TVarH>  GetNSVariables(const vector<size_t>& columns)
	{
		vector<TVarH> vars(columns.size());


		for (size_t c = 0; c < columns.size(); c++)
			vars[c] = GetNSVariable(columns[c]);

		return vars;
	}


	CTRef CUINovaScotia::GetTRef(string str)
	{
		CTRef TRef;

		//2017/01/01 00:00
		StringVector vec(str, " :/-");
		ASSERT(vec.size() >= 4);

		if (vec.size() >= 4)
		{

			size_t day = WBSF::as<size_t>(vec[0]) - 1;
			size_t month = WBSF::as<size_t>(vec[1]) - 1;
			int year = WBSF::as<int>(vec[2]);
			size_t hour = WBSF::as<size_t>(vec[3]);
			if (day > 2000 && day < 2100 && year <= 31)
			{
				//time format have change
				day = WBSF::as<size_t>(vec[2]) - 1;
				year = WBSF::as<int>(vec[0]);
			}

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
			//CWeatherAccumulator stat(TM);
			double lastRnTotal = 0;
			size_t TMaxCol = NOT_INIT;
			size_t RnTotalCol = NOT_INIT;
			vector<TVarH> variables;

			for (CSVIterator loop(file); loop != CSVIterator(); ++loop)
			{
				if (variables.empty())
				{
					StringVector head = loop.Header();
					vector<size_t> columns = GetNSColumns(head);
					variables = GetNSVariables(columns);
					TMaxCol = std::distance( columns.begin(),find(columns.begin(), columns.end(), C_TMAX));
					RnTotalCol = std::distance( columns.begin(),find(columns.begin(), columns.end(), C_RNTOTAL));
					ASSERT(TMaxCol<columns.size());
					ASSERT(RnTotalCol<columns.size());
				}
				 
				if (loop->size() == variables.size())
				{
					CTRef TRef = GetTRef((*loop)[C_DATE_TIME]);
					if (TRef.IsInit())
					{
						//if (stat.TRefIsChanging(TRef))
							//data[stat.GetTRef()].SetData(stat);
						bool bOnlyZero = true;
						std::vector<double> values(variables.size(), -999);
						for (size_t c = 0; c < loop->size(); c++)
						{
							if (c < variables.size() && variables[c] != H_SKIP && !(*loop)[c].empty())
							{
								values[c] = ToDouble((*loop)[c]);
								if (values[c] != 0)
									bOnlyZero = false;
							}
						}

						if (!bOnlyZero)
						{
							//there is a bug in the generation of the NS files 
							//to detect it: when RnTotal < previous and Tmax is empty
							bool bBuggedLine = false;
							if (!(*loop)[RnTotalCol].empty())
							{
								double RnTotal = ToDouble((*loop)[RnTotalCol]);
								bBuggedLine = RnTotal < lastRnTotal/* && (*loop)[TMaxCol].empty()*/;
								lastRnTotal = RnTotal;
							}

							if (!bBuggedLine)
							{
								for (size_t c = 0; c < values.size(); c++)
								{
									if (values[c] > -999)
										data[TRef].SetStat(variables[c], values[c]);
								}//for all columns

								if (values[C_RH] > -999 && values[C_TEMP] > -999)
								{
									double Hr = values[C_RH];
									double T = values[C_TEMP];
									ASSERT(T > -99 && T < 99);
									data[TRef].SetStat(H_TDEW, Hr2Td(T, Hr));
								}
								else if (values[C_RHMIN] > -999 && values[C_TMAX] > -999)
								{
									double Hr = values[C_RHMIN];
									double T = values[C_TMAX];

									ASSERT(T > -99 && T < 99);
									data[TRef].SetStat(H_TDEW, Hr2Td(T, Hr));
								}
								else if (values[C_RHMAX] > -999 && values[C_TMIN] > -999)
								{
									double Hr = values[C_RHMAX];
									double T = values[C_TMIN];

									ASSERT(T > -99 && T < 99);
									data[TRef].SetStat(H_TDEW, Hr2Td(T, Hr));
								}
							}
							else
							{
								callback.AddMessage("Invalid data line : " + loop->GetLastLine());
							}
						}//TRef is init
					}//only zero
				}//if the same number of columns
				msg += callback.StepIt(0);
			}//for all line (

			//if (stat.GetTRef().IsInit())
				//data[stat.GetTRef()].SetData(stat);

		}//if load 

		return msg;
	}


}
