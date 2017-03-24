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
	const char* CUINovaScotia::SERVER_NAME[NB_NETWORKS] = { "novascotia.ca"};
	const char* CUINovaScotia::SERVER_PATH[NB_NETWORKS] = { "natr/forestprotection/wildfire/fwi/FWIActuals.xml"};

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
	const char* CUINovaScotia::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir"};
	const size_t CUINovaScotia::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH};
	const UINT CUINovaScotia::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOVASCOTIA_P;
	const UINT CUINovaScotia::DESCRIPTION_TITLE_ID = ID_TASK_NOVASCOTIA;

	const char* CUINovaScotia::CLASS_NAME(){ static const char* THE_CLASS_NAME = "NovaScotia";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINovaScotia::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINovaScotia::CLASS_NAME(), (createF)CUINovaScotia::create);



	CUINovaScotia::CUINovaScotia(void)
	{}

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
		};

		return str;
	}

	//****************************************************


	std::string CUINovaScotia::GetStationsListFilePath(size_t network)const
	{
		static const char* FILE_NAME[NB_NETWORKS] = { "NovaScotiaFireStations.csv" };

		string filePath = WBSF::GetApplicationPath() + "Layers\\" + FILE_NAME[network];
		return filePath;
	}

	string CUINovaScotia::GetOutputFilePath(size_t network, bool nHourly, const string& ID, int year)const
	{
		return GetDir(WORKING_DIR) + SUBDIR_NAME[network] + (nHourly?"\\Hourly\\":"\\Daily\\") + ToString(year) + "\\" + ID + ".csv";
	}


	ERMsg CUINovaScotia::Execute(CCallback& callback)
	{
		ERMsg msg;

		
		string workingDir = GetDir(WORKING_DIR) + SUBDIR_NAME[FIRE] + "\\";
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME[FIRE]) + "/" + SERVER_PATH[FIRE], 1);
		callback.AddMessage("");

		msg = ExecuteFire(callback);

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
			locations[i].SetSSI("Network", NETWORK_NAME[FIRE]);

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

		station.m_name = PurgeFileName(station.m_name);

		//now extract data 
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(n, TM.IsHourly(), ID, year);
			if (FileExists(filePath))
				msg = station.LoadData(filePath, -999, false);

			msg += callback.StepIt(0);
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		return msg;
	}


	//******************************************************************************************************

	enum TColumns{ C_WXSTATION, C_DATE, C_TEMP, C_RH, C_WSPD, C_MAX_SPD, C_DIR, C_RN1, C_RN24, C_FFMC, C_HFFMC, C_DMC, C_DC, C_ISI, C_HISI, C_BUI, C_FWI, NB_COLUMNS };
	static const char* COLUMN_NAME[NB_COLUMNS] = { "wxStation", "Date", "Temp", "Rh", "Wspd", "Max_Spd", "Dir", "Rn1", "Rn24", "FFMC", "hFFMC", "DMC", "DC", "ISI", "hISI", "BUI", "FWI" };
	static const TVarH FIRE_VAR[NB_COLUMNS] = { H_SKIP, H_SKIP, H_TAIR2, H_RELH, H_WNDS, H_SKIP, H_WNDD, H_PRCP, H_PRCP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP, H_SKIP};


	ERMsg CUINovaScotia::ExecuteFire(CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask("Update Nova-Scotia fire weather data (1 day)", 1 );

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[FIRE], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (msg)
		{
			TRY
			{
				string remoteFilePath = "natr/forestprotection/wildfire/fwi/FWIActuals.xml";
				string outputFilePath = GetDir(WORKING_DIR) + SUBDIR_NAME[FIRE] + "\\FWIActuals.xml";

				msg += CopyFile(pConnection, remoteFilePath, outputFilePath, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_PRAGMA_NOCACHE);
				if (msg)
					msg += SplitFireData(outputFilePath, callback);

			}
			CATCH_ALL(e)
			{
				msg = UtilWin::SYGetMessage(*e);
			}
			END_CATCH_ALL

				//clean connection
				pConnection->Close();
			pSession->Close();
		}

		
		callback.PopTask();
	

		return msg;
	}
	

	
	ERMsg CUINovaScotia::SplitFireData(const std::string& outputFilePath, CCallback& callback)
	{
		ASSERT(!outputFilePath.empty());

		ERMsg msg;
		
		size_t nbStations  = 0;
		
		ifStream files;
		msg += files.open(outputFilePath);

		if (msg)
		{
			zen::XmlDoc doc = zen::parse(files.GetText());

			
			zen::XmlIn in(doc.root());
			for (zen::XmlIn it = in["weather"]; it&&msg; it.next())
			{
				CWeatherYears dataH(true);
				CWeatherYears dataD(false);

				//nbStations

				std::array<string, NB_COLUMNS> values;
				for (size_t i= 0; i < NB_COLUMNS; i++)
					it[COLUMN_NAME[i]](values[i]);

				StringVector date(values[C_DATE], "/");
				ASSERT(date.size() == 3);

				
				size_t day = ToInt(date[0]) - 1;
				size_t month = ToInt(date[1]) - 1;
				int year = ToInt(date[2]);
				


				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
				
				string ID = values[C_WXSTATION];
				string filePathH = GetOutputFilePath(FIRE, true, ID, year);
				string filePathD = GetOutputFilePath(FIRE, false, ID, year);

				if (FileExists(filePathH))
					msg += dataH.LoadData(filePathH);

				if (FileExists(filePathD))
					msg += dataD.LoadData(filePathD);

				CTRef TRefH = CTRef(year, month, day, 13);
				CTRef TRefD = CTRef(year, month, day);

				for (size_t i = 0; i < NB_COLUMNS; i++)
				{
					if (FIRE_VAR[i] != H_SKIP && values[i] != "NA")
					{
						if (i == C_RN24)
							dataD.GetDay(TRefD).SetStat(FIRE_VAR[i], ToDouble(values[i]));
						else
							dataH.GetHour(TRefH).SetStat(FIRE_VAR[i], ToDouble(values[i]));
					}
				}

				//set Tdew
				if (dataH[TRefH][H_TAIR2].IsInit() && dataH[TRefH][H_RELH].IsInit() )
					dataH.GetHour(TRefH).SetStat(H_TDEW, WBSF::Hr2Td(dataH[TRefH][H_TAIR2][MEAN], dataH[TRefH][H_RELH][MEAN]));

				//create directory
				CreateMultipleDir(GetPath(filePathH));
				CreateMultipleDir(GetPath(filePathD));

				//save data
				msg += dataH.SaveData(filePathH);
				msg += dataD.SaveData(filePathD);
				nbStations++;
			}

		}//if msg


		callback.AddMessage(GetString(IDS_NB_STATIONS) + ToString(nbStations), 1);


		return msg;
	}

	
}