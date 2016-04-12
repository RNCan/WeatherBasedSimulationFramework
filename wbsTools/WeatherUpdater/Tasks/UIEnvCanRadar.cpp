#include "StdAfx.h"
#include "UIEnvCanRadar.h"
#include "Basic/FileStamp.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "UI/Common/SYShowMessage.h"
#include "../Resource.h"
#include "TaskFactory.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;


namespace WBSF
{


	//Canada
	//http://climate.weather.gc.ca/radar/image.php?time=04-MAR-14%2005.21.06.293480%20PM&site=WBI
	//USA 
	//http://radar.weather.gov/ridge/Conus/RadarImg/
	//http://www.ncdc.noaa.gov/nexradinv/
	//http://www.ncdc.noaa.gov/has/HAS.FileSelect
	//http://www.ncdc.noaa.gov/cdo-web/review
	//http://gis.ncdc.noaa.gov/map/viewer/#app=cdo&cfg=radar&theme=radar&display=nexrad

	const char* CCanadianRadar::RADARS[NB_RADAR][3] =
	{
		{ "ATL", "Atlantique", "" },
		{ "ONT", "Ontario", "" },
		{ "PNR", "Prairies", "" },
		{ "PYR", "Pacifique", "" },
		{ "QUE", "Québec", "" },
		{ "WBI", "Britt (Sudbury)", "-83.6373658665 47.8026521191 -77.4570436502 43.6539154098" },
		{ "WGJ", "Montreal River (Sault Ste. Marie)", "-87.7639114726 49.2568691017 -81.4584394308 45.099335919" },
		{ "WHK", "Carvel (Edmonton)", "-117.771880828 55.5695382581 -110.533182931 51.4120050754" },
		{ "WHN", "Jimmy Lake (Cold Lake)", "-113.711924973 56.9286079503 -106.239864469 52.7805952563" },
		{ "WKR", "King City (Toronto)", "" },
		{ "WMB", "Lac Castor (Saguenay)", "-73.9371157616 50.5745075074 -67.4182258339 46.438482943" },
		{ "WMN", "McGill (Montréal)", "-77.0121013446 47.4253803845 - 70.8752490569 43.2885607365" },
		{ "WSO", "Exeter (London)", "" },
		{ "WTP", "Holyrood (St. John's)", "-56.348300196 49.324926196 -50.0023250342, 45.1729382328" },
		{ "WUJ", "Aldergrove (Vancouver)", "" },
		{ "WVY", "Villeroy (Trois-Rivières)", "-75.0347285857 48.4508393775 -68.8006773355 44.299509455" },
		{ "WWW", "Spirit River (Grande Prairie)" },
		{ "XAM", "Val d'Irène (Mont-Joli)", "-70.8421322368 50.483980897 -64.3696725512 46.333428227" },
		{ "XBE", "Bethune (Regina)", "-108.603437034 52.5769462466 -101.801364087 48.4346069724" },
		{ "XBU", "Schuler (Medicine Hat)", "" },
		{ "XDR", "Dryden", "-96.1458510275 51.8657231731 -89.4801234175 47.7097893492" },
		{ "XFT", "Franktown (près d'Ottawa)", "-79.1701568375 47.0576542482 -73.0774316862 42.8890312587" },
		{ "XFW", "Foxwarren (Brandon)", "-104.479010139 52.5571459713 -97.7033266122 48.4148066972" },
		{ "XGO", "Halifax", "-66.7628782277 47.0966133415 -60.6403374437 42.9586010682" },
		{ "XLA", "Landrienne (Rouyn-Noranda)", "-81.0739843691 50.5393250609 -74.5533055035 46.4323210456" },
		{ "XMB", "Marion Bridge (Sydney)", "" },
		{ "XME", "Marble Mountain (Corner Brook)", "-61.1134121488 50.9354754337 -54.5607311707 46.7759959048" },
		{ "XNC", "Chipman (Fredericton)", "-68.8209519657 48.2272221325 -62.5795461928 44.0828491909" },
		{ "XNI", "Supérieur Ouest (Thunder Bay)", "-92.4118480452 50.8658240476 -85.8344850086 46.709490384" },
		{ "XPG", "Prince George", "" },
		{ "XRA", "Radisson (Saskatoon)", "" },
		{ "XSI", "Victoria", "" },
		{ "XSM", "Strathmore (Calgary)", "" },
		{ "XSS", "Silver Star Mountain (Vernon)", "" },
		{ "XTI", "Nord-est de l'Ontario (Timmins)", "-85.0798140042 51.2865048157 -78.5076534795 47.14570975" },
		{ "XWL", "Woodlands (Winnipeg)", "-101.1382495 52.1588556519 -94.439735035 48.0025219883" },
	};


//L'atlantique|Ontario|Les Prairies|Le Pacifique|Québec|Britt (Sudbury)|Montreal River (Sault Ste. Marie)|Carvel (près d'Edmonton)|Jimmy Lake (Cold Lake)|King City (Toronto)|Lac Castor (Saguenay)|McGill (Montréal)|Exeter (London)|Holyrood (St. John's)|Aldergrove (Vancouver)|Villeroy (Trois-Rivières)|Spirit River (Grande Prairie)|Val d'Irène (Mont-Joli)|Bethune (Regina)|Schuler (Medicine Hat)|Dryden|Franktown (près d'Ottawa)|Foxwarren (Brandon)|Halifax|Landrienne (Rouyn-Noranda)|Marion Bridge (Sydney)|Marble Mountain (Corner Brook)|Chipman (Fredericton)|Supérieur Ouest (Thunder Bay)|Prince George|Radisson (Saskatoon)|Victoria|Strathmore (Calgary)|Silver Star Mountain (Vernon)|Nord-est de l'Ontario (Timmins)|Woodlands (Winnipeg)

//Atlantic Region
//Ontario Region
//Pacific Region
//Quebec Region
//Prairies Region
//
//Aldergrove(near Vancouver)
//Prince George
//Silver Star Mountain(near Vernon)
//Victoria
//
//Bethune(near Regina)
//Carvel(near Edmonton)
//Foxwarren(near Brandon)
//Jimmy Lake(near Cold Lake)
//Radisson(near Saskatoon)
//Schuler(near Medicine Hat)
//Spirit River(near Grande Prairie)
//Strathmore(near Calgary)
//Woodlands(near Winnipeg)
//
//Britt(Near Sudbury)
//Montreal River(near Sault Ste Marie)
//Dryden
//Exeter(near London)
//Franktown(near Ottawa)
//King City(near Toronto)
//
//Northeast Ontario(near Timmins)
//Superior West(near Thunder Bay)
//
//Lac Castor(near Saguenay)
//Landrienne(near Rouyn - Noranda)
//McGill(near Montréal)
//Val d'Irène (near Mont Joli)
//Villeroy(near Trois - Rivières)
//
//Chipman(near Fredericton)
//Halifax
//Holyrood(near St.John's)
//Marble Mountain
//Marion Bridge(near Sydney)

//*********************************************************************
	const char* CUIEnvCanRadar::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Type", "Radar", "FirstDate", "LastDate" };
	const size_t CUIEnvCanRadar::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_POSITION, T_STRING_BROWSE, T_DATE, T_DATE };
	const UINT CUIEnvCanRadar::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_RADAR_P;
	const UINT CUIEnvCanRadar::DESCRIPTION_TITLE_ID = ID_TASK_EC_RADAR;

	const char* CUIEnvCanRadar::CLASS_NAME(){ static const char* THE_CLASS_NAME = "EnvCanRadar";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanRadar::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIEnvCanRadar::CLASS_NAME(), (createF)CUIEnvCanRadar::create);
	
	const char* CUIEnvCanRadar::SERVER_NAME[2] = { "dd.weather.gc.ca", "climate.weather.gc.ca"};
	const char* CUIEnvCanRadar::SERVER_PATH = "radar/PRECIPET/GIF/";
	
	const char* CUIEnvCanRadar::TYPE_NAME[NB_TYPE] = { "PRECIP_SNOW", "PRECIP_RAIN" };



	CUIEnvCanRadar::CUIEnvCanRadar(void)
	{}

	CUIEnvCanRadar::~CUIEnvCanRadar(void)
	{}


	std::string CUIEnvCanRadar::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		//case TYPE:	str = "06|24"; break;// GetString(IDS_PROPERTIES_ENV_CAN_PRCP_RADAR); break;
		case TYPE:		str = "Current|Historical"; break;
		case PRCP_TYPE:	str = "PRECIP_SNOW|PRECIP_RAIN"; break;
		case RADAR:		str = "L'atlantique|Ontario|Les Prairies|Le Pacifique|Québec|Britt (près de Sudbury)|Montreal River (près de Sault Ste. Marie)|Carvel (près d'Edmonton) | Jimmy Lake(près de Cold Lake) | King City(près de Toronto) | Lac Castor(près de Saguenay) | McGill(près de Montréal) | Exeter(près de London) | Holyrood(près de St.John's)|Aldergrove (près de Vancouver)|Villeroy (près de Trois-Rivières)|Spirit River (près de Grande Prairie)|Val d'Irène(près de Mont - Joli) | Bethune(près de Regina) | Schuler(près de Medicine Hat) | Dryden | Franktown(près d'Ottawa)|Foxwarren (près de Brandon)|Halifax|Landrienne (près de Rouyn-Noranda)|Marion Bridge (près de Sydney)|Marble Mountain (près de Corner Brook)|Chipman (près de Fredericton)|Supérieur Ouest (près de Thunder Bay)|Prince George|Radisson (près de Saskatoon)|Victoria|Strathmore (près de Calgary)|Silver Star Mountain (près de Vernon)|Nord-est de l'Ontario(près de Timmins) | Woodlands(près de Winnipeg)"; break;
		};
		return str;
	}

	std::string CUIEnvCanRadar::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		//case TYPE:	str = ToString(TYPE_06HOURS); break;
		case TYPE:		str = ToString(CURRENT_RADAR); break;
		case PRCP_TYPE:	str = ToString(T_SNOW); break;
		case RADAR:		str = "11111111111111111111111111111111111"; break;
		case FIRST_DATE:
		case LAST_DATE:	str = CTRef::GetCurrentTRef(CTM::HOURLY).GetFormatedString(); break;

		};

		return str;
	}

	ERMsg CUIEnvCanRadar::Execute(CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	bool CUIEnvCanRadar::NeedDownload(const CFileInfo& info, const string& filePath)const
	{
		bool bDownload = true;

		CFileStamp fileStamp(filePath);
		__time64_t lastUpdate = fileStamp.m_time;
		if (lastUpdate > 0 && info.m_time < lastUpdate)
			bDownload = false;

		return bDownload;
	}


	string CUIEnvCanRadar::GetOutputFilePath(size_t t, const string& URL)const
	{
		string path;
		if (t == CURRENT_RADAR)
		{
			string year = URL.substr(0, 4);
			string month = URL.substr(4, 2);
			string day = URL.substr(6, 2);
			string region = URL.substr(13, 3);
			path = GetDir(WORKING_DIR) + region + "\\" + year + "\\" + month + "\\" + day + "\\" + URL;
		}
		else if (t == HISTORICAL_RADAR)
		{
			ASSERT(URL.length() == 67);
			//'/lib/radar/image.php?time=15-JUL-13%2010.20.37.922195%20PM&site=XAM'
			string year = "20" + URL.substr(33, 2);
			size_t m = GetMonthIndex(URL.substr(29, 3).c_str()); ASSERT(m < 12);
			string month = FormatA("%02d", m + 1);
			string day = URL.substr(26, 2);
			string hour = URL.substr(38, 2);
			string min = URL.substr(41, 2);
			string region = URL.substr(64, 3);
			string type = TYPE_NAME[as<size_t>(PRCP_TYPE)];
			string pm = URL.substr(56, 2);

			if (pm == "PM")
			{
				if (hour != "12")
					hour = ToString(ToInt(hour) + 12);
			}
			else if (hour == "12")
			{
				hour = "00";
			}
			GetDir(WORKING_DIR) + region + "\\" + year + "\\" + month + "\\" + day + "\\" + year + month + day + hour + min + "_" + region + "_" + type + ".gif";

		}

		return path;
	}


	CTPeriod CUIEnvCanRadar::GetPeriod()const
	{
		CTPeriod p;
		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 4 && t2.size()==4)
		{
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, ToSizeT(t1[3])), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, ToSizeT(t2[3])));
		}

		return p;
	}

	
	ERMsg CUIEnvCanRadar::GetRadarList(StringVector& stationList, CCallback& callback)const
	{
		ERMsg msg;


		//Interface attribute index to attribute index
		//sample for Québec
		//http://climate.weather.gc.ca/radar/index_e.html?RadarSite=XAM&sYear=2013&sMonth=7&sDay=15&sHour=22&sMin=00&Duration=2&ImageType=PRECIP_SNOW_WEATHEROFFICE&scale=14

		static const char pageFormat[] =
			"radar/index_e.html?"
			"RadarSite=%s&"
			"sYear=%d&"
			"sMonth=%d&"
			"sDay=%d&"
			"sHour=%d&"
			"sMin=00&"
			"Duration=2&"
			"ImageType=%s&"
			"scale=14";


		std::bitset<NB_RADAR> selection(Get(RADAR));
		CTPeriod period = GetPeriod();

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), selection.count() * period.size() / 2);
		//callback.SetNbStep(selection.count() * period.size() / 2);

		size_t type = as<size_t>(TYPE);
		size_t prcpType = as<size_t>(PRCP_TYPE);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[type], pConnection, pSession);
		if (!msg)
			return msg;


		ASSERT(period.GetTM().Type() == CTM::HOURLY);

		std::set<string> tmpList;
		//loop each 2 hours
		for (size_t i = 0; i < selection.size() && msg; i++)
		{
			if (selection[i])
			{
				for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef += 2)
				{
					string URL = FormatA(pageFormat, CCanadianRadar::RADARS[i][0], TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), TYPE_NAME[prcpType]);
					URL.resize(strlen(URL.c_str()));

					string source;
					UtilWWW::GetPageText(pConnection, URL, source, true);


					string::size_type posEnd = 0;

					string::size_type posBegin = source.find("<div id=\"radar");

					while (posBegin != string::npos)
					{
						string image = FindString(source, "src='", "'", posBegin);
						tmpList.insert(image);
						posBegin = source.find("<div id=\"radar", posBegin);
					}

					msg += callback.StepIt();
				}
			}
		}

		pConnection->Close();
		pSession->Close();


		stationList.insert(stationList.begin(), tmpList.begin(), tmpList.end());
		callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(stationList.size()));
		callback.PopTask();

		return msg;
	}



	ERMsg CUIEnvCanRadar::CleanRadarList(StringVector& radarList, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_CLEAN_LIST), radarList.size());
		//callback.SetNbStep(radarList.size());

		for (StringVector::iterator it = radarList.begin(); it != radarList.end() && msg;)
		{
			if (FileExists(GetOutputFilePath(as<size_t>(TYPE), *it)))
				it = radarList.erase(it);
			else
				it++;

			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}

	ERMsg CUIEnvCanRadar::ExecuteCurrent(CCallback& callback)
	{
		ERMsg msg;
		string workingDir = GetDir(WORKING_DIR);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[CURRENT_RADAR], pConnection, pSession);
		if (!msg)
			return msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME[CURRENT_RADAR], 1);
		callback.AddMessage("");


		CFileInfoVector dirList;
		msg = UtilWWW::FindDirectories(pConnection, SERVER_PATH, dirList);
		callback.PushTask("Find file", dirList.size());
		//callback.SetNbStep(dirList.size());

		CFileInfoVector clearedList;
		{
			CFileInfoVector fileList;
			for (size_t i = 0; i < dirList.size() && msg; i++)
			{
				CFileInfoVector fileListTmp;
				msg = UtilWWW::FindFiles(pConnection, dirList[i].m_filePath + "/*.gif", fileListTmp);
				fileList.insert(fileList.begin(), fileListTmp.begin(), fileListTmp.end());
				msg += callback.StepIt();
			}

			callback.AddMessage("Number of images found: " + ToString(fileList.size()));
			callback.PushTask("Clear list", fileList.size());
			//callback.SetNbStep(fileList.size());



			for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg; it++)
			{
				string fileTitle = GetFileTitle(it->m_filePath);
				if (Find(fileTitle, "_COMP_PRECIPET_SNOW_A11Y") || Find(fileTitle, "_COMP_PRECIPET_RAIN_A11Y") ||
					Find(fileTitle, "_COMP_PRECIPET_SNOW") || Find(fileTitle, "_COMP_PRECIPET_RAIN"))
				{
					//dont use
				}
				else if (Find(fileTitle, "PRECIPET_SNOW_A11Y") || Find(fileTitle, "PRECIPET_RAIN_A11Y"))
				{
					string fileName = GetFileName(it->m_filePath);
					string filePath = GetOutputFilePath(as<size_t>(TYPE), fileName);
					if (NeedDownload(*it, filePath))
						clearedList.push_back(*it);
				}
				else if (Find(fileTitle, "PRECIPET_SNOW") || Find(fileTitle, "PRECIPET_RAIN"))
				{
					string fileName = GetFileName(it->m_filePath);
					string filePath = GetOutputFilePath(as<size_t>(TYPE), fileName);
					if (NeedDownload(*it, filePath))
						clearedList.push_back(*it);
				}
				msg += callback.StepIt();
			}

			callback.PopTask();
		}

		callback.PopTask();

		callback.AddMessage("Number of images to download after clearing: " + ToString(clearedList.size()));
		callback.PushTask("Download images", clearedList.size());
		//callback.SetNbStep(clearedList.size());

		int nbDownload = 0;
		for (size_t i = 0; i < clearedList.size() && msg; i++)
		{
			string filePath = GetOutputFilePath(as<size_t>(TYPE), GetFileName(clearedList[i].m_filePath));
			CreateMultipleDir(GetPath(filePath));
			msg = UtilWWW::CopyFile(pConnection, clearedList[i].m_filePath, filePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
			if (msg)
				nbDownload++;

			msg += callback.StepIt();
		}

		pConnection->Close();
		pSession->Close();


		callback.AddMessage("Number of images downloaded: " + ToString(nbDownload));
		callback.PopTask();

		return msg;
	}



	ERMsg CUIEnvCanRadar::ExecuteHistorical(CCallback& callback)
	{
		ERMsg msg;
		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[as<size_t>(TYPE)], pConnection, pSession);
		if (!msg)
			return msg;


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME[as<size_t>(TYPE)], 1);
		callback.AddMessage("");


		//Get remote station list
		StringVector imageList;
		if (msg)
			msg = GetRadarList(imageList, callback);

		if (msg)
			msg = CleanRadarList(imageList, callback);

		if (!msg)
			return msg;


		callback.PushTask("Download images", imageList.size());
		//callback.SetNbStep(imageList.size());


		int nbRun = 0;
		int curI = 0;

		while (curI<imageList.size() && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME[as<size_t>(TYPE)], pConnection, pSession);

			if (msg)
			{
				TRY
				{
					for (int i = curI; i<imageList.size() && msg; i++)
					{
						string filePath = GetOutputFilePath(as<size_t>(TYPE), imageList[i]);
						msg += CreateMultipleDir(GetPath(filePath));
						//msg += CopyFile(pConnection, imageList[i], filePath, INTERNET_FLAG_TRANSFER_BINARY | WININET_API_FLAG_SYNC);

						CString URL(imageList[i].c_str());
						CHttpFile* pURLFile = pConnection->OpenRequest(_T("GET"), URL, NULL, 1, NULL, NULL, INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_TRANSFER_BINARY | WININET_API_FLAG_SYNC | INTERNET_FLAG_NEED_FILE);

						//CStdioFile* pURLFile = pSession->OpenURL(UtilWin::Convert(imageList[i]), 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT);

						bool bRep = false;

						if (pURLFile != NULL)
						{
							if (pURLFile->SendRequest() != 0)
							{
								CArray<char> source;
								int len = 0;
								CArray<char> tmp;
								tmp.SetSize(50);

								DWORD dwStatusCode = 0;
								pURLFile->QueryInfoStatusCode(dwStatusCode);

								ULONGLONG length = pURLFile->GetLength();
								while (length > 0)
								{
									pURLFile->Read(tmp.GetData(), 50);
									source.Append(tmp);

									length = pURLFile->GetLength();
								}

								pURLFile->QueryInfoStatusCode(dwStatusCode);
								pURLFile->Close();

								ofStream file;

								msg = file.open(filePath, ios::out | ios::binary);
								if (msg)
								{
									if (!source.IsEmpty())
										file.write(source.GetData(), (UINT)source.GetSize());

									file.close();
								}
							}
						}

						if (msg)
						{
							curI++;
							nbRun = 0;
							msg += callback.StepIt();
						}
					}
				}
				CATCH_ALL(e)
				{
					msg = UtilWin::SYGetMessage(*e);
				}
				END_CATCH_ALL

					//if an error occur: try again
					if (!msg && !callback.GetUserCancel())
					{
						//callback.AddTask(1);//one step more

						if (nbRun < 5)
						{
							callback.AddMessage(msg);
							msg.asgType(ERMsg::OK);
							Sleep(1000);//wait 1 sec
						}
					}

				//clean connection
				pConnection->Close();
				pSession->Close();
			}
		}


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI));
		callback.PopTask();


		return msg;
	}

}