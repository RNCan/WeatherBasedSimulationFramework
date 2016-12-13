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

	
	const char* CCanadianRadar::DEFAULT_LIST[NB_RADAR][NB_INFO] =
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

	size_t CCanadianRadar::GetRadar(const std::string& in, size_t t)
	{
		size_t r = -1;
		std::string tmp(in);
		Trim(tmp);
		MakeUpper(tmp);
		for (size_t i = 0; i < NB_RADAR; i++)
		{
			if (tmp == DEFAULT_LIST[i][t])
			{
				r = i;
				break;
			}
		}

		return r;
	}


	std::string CCanadianRadar::GetAllPossibleValue(bool bAbvr, bool bName)
	{
		string str;
		for (size_t i = 0; i < NB_RADAR; i++)
		{
			str += i != 0 ? "|" : "";
			if (bAbvr)
				str += DEFAULT_LIST[i][ABVR];
			if (bAbvr && bName)
				str += "=";
			if (bName)
				str += DEFAULT_LIST[i][NAME];
		}

		return str;
	}
	
	string CCanadianRadar::GetName(size_t r, size_t t)
	{
		ASSERT(r < NB_RADAR);
		ASSERT(t >= 0 && t < NB_INFO);
		return DEFAULT_LIST[r][t];
	}

	string CCanadianRadar::ToString()const
	{
		string str;
		if (!none() && !all())
		{
			for (size_t i = 0; i < NB_RADAR; i++)
			{
				if (at(i))
				{
					str += DEFAULT_LIST[i][ABVR];
					str += '|';
				}
			}
		}

		return str;
	}

	ERMsg CCanadianRadar::FromString(const string& in)
	{
		ERMsg msg;

		reset();

		StringVector tmp(in, "|;");
		for (size_t i = 0; i < tmp.size(); i++)
			msg += set(tmp[i]);

		return msg;
	}

	ERMsg CCanadianRadar::set(const std::string& in)
	{
		ERMsg message;
		size_t p = GetRadar(in);
		if (p < size())
		{
			set(p);
		}
		else
		{
			message.ajoute("This radar is invalid: %s" + in);
		}

		return message;
	}

//*********************************************************************
	const char* CUIEnvCanRadar::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Type", "PrcpType", "Background", "Radar", "FirstDate", "LastDate" };
	const size_t CUIEnvCanRadar::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX, T_COMBO_INDEX, T_STRING_SELECT, T_DATE, T_DATE };
	const UINT CUIEnvCanRadar::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_RADAR_P;
	const UINT CUIEnvCanRadar::DESCRIPTION_TITLE_ID = ID_TASK_EC_RADAR;

	const char* CUIEnvCanRadar::CLASS_NAME(){ static const char* THE_CLASS_NAME = "EnvCanRadar";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanRadar::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIEnvCanRadar::CLASS_NAME(), (createF)CUIEnvCanRadar::create);
	
	const char* CUIEnvCanRadar::SERVER_NAME[2] = { "dd.weather.gc.ca", "climate.weather.gc.ca"};
	const char* CUIEnvCanRadar::SERVER_PATH = "radar/PRECIPET/GIF/";
	
	const char* CUIEnvCanRadar::TYPE_NAME_OLD[NB_TYPE] = { "PRECIP_SNOW_WEATHEROFFICE", "PRECIP_RAIN_WEATHEROFFICE" };
	const char* CUIEnvCanRadar::TYPE_NAME_NEW[NB_TYPE] = { "PRECIPET_SNOW_WEATHEROFFICE", "PRECIPET_RAIN_WEATHEROFFICE" };
	


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
		case PRCP_TYPE:	str = "Snow|Rain"; break;
		case BACKGROUND:str = "White|Brown"; break;
		case RADAR:		str = CCanadianRadar::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIEnvCanRadar::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\Radar\\"; break;
		case TYPE:		str = ToString(CURRENT_RADAR); break;
		case PRCP_TYPE:	str = ToString(T_SNOW); break;
		case BACKGROUND:str = ToString(B_BROWN); break;
		case RADAR:		str = ""; break;
		case FIRST_DATE:
		case LAST_DATE:	str = CTRef::GetCurrentTRef(CTM::HOURLY).GetFormatedString(); break;

		};

		return str;
	}

	ERMsg CUIEnvCanRadar::Execute(CCallback& callback)
	{
		ERMsg msg;

		switch (as<int>(TYPE))
		{
		case CURRENT_RADAR: msg = ExecuteCurrent(callback); break;
		case HISTORICAL_RADAR: msg = ExecuteHistorical(callback); break;
		default: ASSERT(false);
		}


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


	string CUIEnvCanRadar::GetRadarID(size_t t, const string& URL)
	{
		//string ID;
		//size_t pos = (t == CURRENT_RADAR) ? 13 : 64;
		//ID = URL.substr(pos, 3);
		return GetLastDirName(URL);
		//return ID;
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
//			ASSERT(URL.length() == 63);
			//static const char* OLD_TYPE_NAME[NB_TYPE] = { "PRECIP_SNOW", "PRECIP_RAIN" };
			
			/*string year = "20" + URL.substr(33, 2);
			size_t m = GetMonthIndex(URL.substr(29, 3).c_str()); ASSERT(m < 12);
			string month = FormatA("%02d", m + 1);
			string day = URL.substr(26, 2);
			string hour = URL.substr(36, 2);
			string min = URL.substr(39, 2);
			string region = URL.substr(60, 3);
			string type = OLD_TYPE_NAME[as<size_t>(PRCP_TYPE)];
			string pm = URL.substr(52, 2);

			if (pm == "PM")
			{
				if (hour != "12")
					hour = ToString(ToInt(hour) + 12);
			}
			else if (hour == "12")
			{
				hour = "00";
			}*/
			
			string year = URL.substr(0, 4);
			string month = URL.substr(4, 2);
			string day = URL.substr(6, 2);
			string hour = URL.substr(8, 2);
			string min = URL.substr(10, 2);
			string region = URL.substr(13, 3);
			string type = URL.substr(17, URL.find("|")-17);

			path = GetDir(WORKING_DIR) + region + "\\" + year + "\\" + month + "\\" + day + "\\" + year + month + day + hour + min + "_" + region + "_" + type + ".gif";

		}

		return path;
	}


	CTPeriod CUIEnvCanRadar::GetPeriod()const
	{
		CTPeriod p;
		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size()==3)
		{
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, 0), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, 23));
		}
		else if (t1.size() == 4 && t2.size() == 4)
		{
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, ToSizeT(t1[3])), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, ToSizeT(t2[3])));
		}


		return p;
	}

	string GetDescription(const string& desc)
	{
		string out;

		//ReplaceString(desc, "&amp;", "|");
		//site=XAM|year=2011|month=7|day=10|hour=08|minute=20|duration=2|image_type=PRECIP_SNOW_WEATHEROFFICE|image=1">PRECIP - Snow - 2011-07-10, 04:20 EDT, 1/13
		//size_t pos = desc.find_last_of(">");
		//if (pos != string::npos)
		StringVector test1(desc, ">");
		StringVector test2(test1[0], "&=");
		StringVector test3(test1[1], " -:,");
		if (test2.size() == 18 && test3.size()==9)
		{
			int year = ToInt(test3[2]);
			int month = ToInt(test3[3]);
			int day = ToInt(test3[4]);
			int hour = ToInt(test3[5]);
			int min = ToInt(test3[6]);

			out = WBSF::FormatA("%4d%02d%02d%02d%02d_%s_%s", year, month, day, hour, min, test2[1].c_str(), test2[15].c_str());
			
		}

		return out;
	}

	ERMsg CUIEnvCanRadar::GetRadarList(StringVector& radarList, CCallback& callback)const
	{
		ERMsg msg;


		//Interface attribute index to attribute index
		//sample for Québec
		//http://climate.weather.gc.ca/radar/index_e.html?site=XAM&year=2015&month=7&day=25&hour=13&minute=20&duration=2&image_type=PRECIPET_SNOW_WEATHEROFFICE
		//http://climate.weather.gc.ca/radar/index_e.html?site=XAM&sYear=2013&sMonth=7&sDay=15&sHour=22&sMin=00&Duration=2&ImageType=PRECIP_SNOW_WEATHEROFFICE&scale=14
		//http://climate.weather.gc.ca/radar/index_e.html?site=XAM&year=2015&month=7&day=10&hour=20&minute=40&duration=2&image_type=PRECIPET_SNOW_WEATHEROFFICE
		//http://climate.weather.gc.ca/radar/index_e.html?site=XAM&year=2012&month=7&day=10&hour=20&minute=40&duration=2&image_type=PRECIP_RAIN_WEATHEROFFICE

		static const char pageFormat[] =
			"radar/index_e.html?"
			"site=%s&"
			"year=%d&"
			"month=%d&"
			"day=%d&"
			"hour=%02d&"
			"minute=00&"
			"duration=2&"
			"image_type=%s";


		CCanadianRadar radar(Get(RADAR));
		CTPeriod period = GetPeriod();

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), radar.count() * period.size() / 2);

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
		for (size_t i = 0; i < radar.size() && msg; i++)
		{
			if (radar.none() || radar[i])
			{
				for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef += 2)
				{
					string typeName = TRef.GetYear() < 2014 ? TYPE_NAME_OLD[prcpType] : TYPE_NAME_NEW[prcpType];
					string URL = FormatA(pageFormat, CCanadianRadar::GetName(i, 0).c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), typeName.c_str());
					URL.resize(strlen(URL.c_str()));

					string source;
					UtilWWW::GetPageText(pConnection, URL, source, true);
				//http://climate.weather.gc.ca/radar/index_e.html?site=XAM&year=2013&month=7&day=5&hour=00&minute=00&duration=2&image_type=PRECIPET_SNOW_WEATHEROFFICE
				//http://climate.weather.gc.ca/radar/index_e.html?site=XAM&year=2013&month=7&day=6&hour=00&minute=00&duration=2&image_type=PRECIP_SNOW_WEATHEROFFICE
				//http://climat.weather.gc.ca/radar/index_e.html?site=XAM&year=2015&month=7&day=10&hour=20&minute=00&duration=2&image_type=PRECIPET_SNOW_WEATHEROFFICE
					//string::size_type posEnd = 0;
					string fileList1 = FindString(source, "blobArray = [", "]");
					if (!fileList1.empty())
					{


						size_t begin = source.find("<p>Please enable JavaScript to view the animation.</p>");
						string fileList2 = FindString(source, "<ul>", "</ul>", begin);
						string::size_type posBegin1 = 0;
						string::size_type posBegin2 = 0;

						while (posBegin1 != string::npos && posBegin2 != string::npos)
						{
							string image = FindString(fileList1, "'", "'", posBegin1);
							string desc = GetDescription(FindString(fileList2, "<li><a href=\"/radar/index_e.html?", "</a></li>", posBegin2));
							if (!image.empty() && !desc.empty())
							{
								tmpList.insert(desc + "|" + image);
							}
								
							posBegin1 = fileList1.find(",", posBegin1);
							posBegin2 = fileList2.find("<li><a href=\"/radar/index_e.html?", posBegin2);
							
						}
					}
					msg += callback.StepIt();
				}
			}
		}

		pConnection->Close();
		pSession->Close();


		radarList.insert(radarList.end(), tmpList.begin(), tmpList.end());
		callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(radarList.size()));
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

		CCanadianRadar radar(Get(RADAR));
		bool bUseRain = as<size_t>(PRCP_TYPE) == T_RAIN;
		bool bUseSnow = as<size_t>(PRCP_TYPE) == T_SNOW;
		bool bUseWhite = as<size_t>(BACKGROUND) == B_WHITE;
		bool bUseBrown = as<size_t>(BACKGROUND) == B_BROWN;
		

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
		callback.PushTask("Find current radar images", dirList.size());
		//callback.SetNbStep(dirList.size());

		CFileInfoVector fileList;
		for (size_t i = 0; i < dirList.size() && msg; i++)
		{
			string ID = GetLastDirName(dirList[i].m_filePath); //GetRadarID(CURRENT_RADAR, dirList[i].m_filePath);
			if (radar.at(ID))
			{
				CFileInfoVector fileListTmp;
				msg = UtilWWW::FindFiles(pConnection, dirList[i].m_filePath + "/*.gif", fileListTmp);
				fileList.insert(fileList.begin(), fileListTmp.begin(), fileListTmp.end());
			}

			msg += callback.StepIt();
		}

		callback.AddMessage("Number of images found: " + ToString(fileList.size()));
		callback.PopTask();

		
		callback.PushTask("Clear list", fileList.size());
		CFileInfoVector clearedList;

		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			//string fileTitle = GetFileTitle(it->m_filePath);
			string fileName = GetFileName(it->m_filePath);
			
			bool bOk1 = bUseRain && bUseWhite && Find(fileName, "PRECIPET_RAIN_A11Y.gif");
			bool bOk2 = bUseRain && bUseBrown && Find(fileName, "PRECIPET_RAIN.gif");
			bool bOk3 = bUseSnow && bUseWhite && Find(fileName, "PRECIPET_SNOW_A11Y.gif");
			bool bOk4 = bUseSnow && bUseBrown && Find(fileName, "PRECIPET_SNOW.gif");

			if (Find(fileName, "_COMP_PRECIPET_SNOW_A11Y.gif") || Find(fileName, "_COMP_PRECIPET_RAIN_A11Y.gif") ||
				Find(fileName, "_COMP_PRECIPET_SNOW.gif") || Find(fileName, "_COMP_PRECIPET_RAIN.gif"))
			{
				//dont use
			}
			else if (bOk1 || bOk2 || bOk3 || bOk4)
			{
				
				string filePath = GetOutputFilePath(as<size_t>(TYPE), fileName);
				if (NeedDownload(*it, filePath))
					clearedList.push_back(*it);
			}
			
			msg += callback.StepIt();
		}

		callback.PopTask();

		callback.AddMessage("Number of images to download after clearing: " + ToString(clearedList.size()));
		callback.PushTask("Download images " + ToString(clearedList.size() )+ " images", clearedList.size());
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


		callback.PushTask("Download historical radar images (" + ToString(imageList.size())+ ")", imageList.size());
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

						StringVector name(imageList[i], "|"); ASSERT(name.size()==2);
						CString URL(name[1].c_str());
						CHttpFile* pURLFile = pConnection->OpenRequest(_T("GET"), URL, NULL, 1, NULL, NULL, INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_TRANSFER_BINARY | WININET_API_FLAG_SYNC | INTERNET_FLAG_NEED_FILE);

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

									//convert image to GeoTiff
									//ConvertToGeotiff(filePath, CCanadianRadar(CCanadianRadar::coord));
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