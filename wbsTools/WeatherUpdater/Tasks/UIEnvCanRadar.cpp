#include "StdAfx.h"
#include "UIEnvCanRadar.h"
#include "Basic/FileStamp.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/CSV.h"
#include "Basic/CallcURL.h"
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
	// other site ofr radar: http://hpfx.collab.science.gc.ca/
	//USA 
	//http://radar.weather.gov/ridge/Conus/RadarImg/
	//http://www.ncdc.noaa.gov/nexradinv/
	//http://www.ncdc.noaa.gov/has/HAS.FileSelect
	//http://www.ncdc.noaa.gov/cdo-web/review
	//http://gis.ncdc.noaa.gov/map/viewer/#app=cdo&cfg=radar&theme=radar&display=nexrad

	const char* CCanadianRadar::DEFAULT_LIST[NB_RADARS][NB_INFO] =
	{
		{ "NAT", "NAT", "National", "" },
		{ "ATL", "ATL", "Atlantique", "" },
		{ "ONT", "ONT", "Ontario", "" },
		{ "PNR", "PNR", "Prairies", "" },
		{ "PYR", "PYR", "Pacifique", "" },
		{ "QUE", "QUE", "Québec", "" },
		{ "WUJ", "CASAG", "Aldergrove (Vancouver)", "" },
		{ "XBE", "CASBE", "Bethune (Regina)", "-108.603437034 52.5769462466 -101.801364087 48.4346069724" },
		{ "WBI", "CASBI", "Britt (Sudbury)", "-83.6373658665 47.8026521191 -77.4570436502 43.6539154098" },
		{ "WHK", "CASCV", "Carvel (Edmonton)", "-117.771880828 55.5695382581 -110.533182931 51.4120050754" },
		{ "XNC", "CASCM", "Chipman (Fredericton)", "-68.8209519657 48.2272221325 -62.5795461928 44.0828491909" },
		{ "XDR", "CASDR", "Dryden", "-96.1458510275 51.8657231731 -89.4801234175 47.7097893492" },
		{ "WSO", "CASET", "Exeter (London)", "" },
		{ "XFW", "CASFW", "Foxwarren (Brandon)", "-104.479010139 52.5571459713 -97.7033266122 48.4148066972" },
		{ "XFT", "CASFT", "Franktown (près d'Ottawa)", "-79.1701568375 47.0576542482 -73.0774316862 42.8890312587" },
		{ "XGO", "CASGO", "Halifax", "-66.7628782277 47.0966133415 -60.6403374437 42.9586010682" },
		{ "WTP", "CASHR", "Holyrood (St. John's)", "-56.348300196 49.324926196 -50.0023250342, 45.1729382328" },
		{ "WHN", "CASJL", "Jimmy Lake (Cold Lake)", "-113.711924973 56.9286079503 -106.239864469 52.7805952563" },
		{ "WKR", "CASKR", "King City (Toronto)", "" },
		{ "WMB", "CASMA", "Lac Castor (Saguenay)", "-73.9371157616 50.5745075074 -67.4182258339 46.438482943" },
		{ "XLA", "CASLA", "Landrienne (Rouyn-Noranda)", "-81.0739843691 50.5393250609 -74.5533055035 46.4323210456" },
		{ "XME", "CASMM", "Marble Mountain (Corner Brook)", "-61.1134121488 50.9354754337 -54.5607311707 46.7759959048" },
		{ "XMB", "CASMB", "Marion Bridge (Sydney)", "" },
		{ "WMN", "CASBV", "McGill (Montréal)", "-77.0121013446 47.4253803845 - 70.8752490569 43.2885607365" },
		{ "WGJ", "CASMR", "Montreal River (Sault Ste. Marie)", "-87.7639114726 49.2568691017 -81.4584394308 45.099335919" },
		{ "XTI", "CASRF", "Nord-est de l'Ontario (Timmins)", "-85.0798140042 51.2865048157 -78.5076534795 47.14570975" },
		{ "XPG", "CASPG", "Prince George", "" },
		{ "XRA", "CASRA", "Radisson (Saskatoon)", "" },
		{ "XBU", "CASSU", "Schuler (Medicine Hat)", "" },
		{ "XSS", "CASSS", "Silver Star Mountain (Vernon)", "" },
		{ "WWW", "CASSR", "Spirit River (Grande Prairie)" },
		{ "XSM", "CASSM", "Strathmore (Calgary)", "" },
		{ "XNI", "CASLL", "Supérieur Ouest (Thunder Bay)", "-92.4118480452 50.8658240476 -85.8344850086 46.709490384" },
		{ "XAM", "CASVD", "Val d'Irène (Mont-Joli)", "-70.8421322368 50.483980897 -64.3696725512 46.333428227" },
		{ "XSI", "CASSI", "Victoria", "" },
		{ "WVY", "CASVY", "Villeroy (Trois-Rivières)", "-75.0347285857 48.4508393775 -68.8006773355 44.299509455" },
		{ "XWL", "CASWL", "Woodlands (Winnipeg)", "-101.1382495 52.1588556519 -94.439735035 48.0025219883" },
	};

	size_t CCanadianRadar::GetRadar(const std::string& in, size_t t)
	{
		size_t r = -1;
		std::string tmp(in);
		Trim(tmp);
		MakeUpper(tmp);
		for (size_t i = 0; i < NB_RADARS; i++)
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
		for (size_t i = 0; i < NB_RADARS; i++)
		{
			str += i != 0 ? "|" : "";
			if (bAbvr)
				str += DEFAULT_LIST[i][ABRV2];
			if (bAbvr && bName)
				str += "=";
			if (bName)
				str += DEFAULT_LIST[i][NAME];
		}

		return str;
	}

	string CCanadianRadar::GetName(size_t r, size_t t)
	{
		ASSERT(r < NB_RADARS);
		ASSERT(t >= 0 && t < NB_INFO);
		return DEFAULT_LIST[r][t];
	}

	string CCanadianRadar::ToString()const
	{
		string str;
		if (!none() && !all())
		{
			for (size_t i = 0; i < NB_RADARS; i++)
			{
				if (test(i))
				{
					str += DEFAULT_LIST[i][ABRV2];
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
		size_t p = GetRadar(in, ABRV2);
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
	const char* CUIEnvCanRadar::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Type", "Radar", "Composite", "PrcpType", "Background", "FirstDate", "LastDate" };
	const size_t CUIEnvCanRadar::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING_SELECT, T_BOOL, T_COMBO_INDEX, T_COMBO_INDEX, T_DATE, T_DATE };
	const UINT CUIEnvCanRadar::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_RADAR_P;
	const UINT CUIEnvCanRadar::DESCRIPTION_TITLE_ID = ID_TASK_EC_RADAR;

	const char* CUIEnvCanRadar::CLASS_NAME() { static const char* THE_CLASS_NAME = "EnvCanRadar";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanRadar::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIEnvCanRadar::CLASS_NAME(), (createF)CUIEnvCanRadar::create);

	const char* CUIEnvCanRadar::SERVER_NAME[NB_TEMPORAL_TYPE] = { "dd.weather.gc.ca", "climat.meteo.gc.ca" };
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
		case TYPE:		str = "Current|Historical|New National"; break;
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
		case TYPE:		str = ToString(NEW_NATIONAL_RADAR); break;
		case PRCP_TYPE:	str = ToString(T_SNOW); break;
		case BACKGROUND:str = ToString(B_BROWN); break;
		case RADAR:		str = ""; break;
		case FIRST_DATE:
		case LAST_DATE:	str = CTRef::GetCurrentTRef(CTM::HOURLY).GetFormatedString("%Y-%m-%d"); break;
		case COMPOSITE: str = "0"; break;
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
		case NEW_NATIONAL_RADAR:msg = ExecuteNational(callback); break;
		default: ASSERT(false);
		}
		

		return msg;
	}

	
	

	bool CUIEnvCanRadar::NeedDownload(const CFileInfo& info, const string& filePath)const
	{
		return !FileExists(filePath);
		/*bool bDownload = true;

		CFileStamp fileStamp(filePath);
		__time64_t lastUpdate = fileStamp.m_time;
		if (lastUpdate > 0 && info.m_time < lastUpdate)
			bDownload = false;

		return bDownload;*/
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
		//Get path in UTC
		string path;
		if (t == CURRENT_RADAR)
		{
			StringVector tmp(URL, "_");
			ASSERT(tmp.size() == 4 || tmp.size() == 5);

			string year = tmp[0].substr(0, 4);
			string month = tmp[0].substr(4, 2);
			string day = tmp[0].substr(6, 2);
			string hour = tmp[0].substr(8, 2);
			string min = tmp[0].substr(10, 2);
			string radar_id = tmp[1];
			
			path = GetDir(WORKING_DIR) + radar_id + "\\" + year + "\\" + month + "\\" + day + "\\" + URL;
		}
		else if (t == HISTORICAL_RADAR)
		{
			size_t pos = URL.find("|");
			ASSERT(pos != string::npos);


			StringVector tmp(URL.substr(0, pos), "_");
			ASSERT(tmp.size() == 5);

			string date = FindString(URL, "time=", "&");
			string year = date.substr(0, 4);
			string month = date.substr(4, 2);
			string day = date.substr(6, 2);
			string hour = date.substr(8, 2);
			string min = date.substr(10, 2);
			string radar_id = tmp[1];

			string type = tmp[2] + "_" + tmp[3];

			path = GetDir(WORKING_DIR) + radar_id + "\\" + year + "\\" + month + "\\" + day + "\\" + year + month + day + hour + min + "_" + radar_id + "_" + type + ".gif";

		}

		return path;
	}


	CTPeriod CUIEnvCanRadar::GetPeriod()const
	{
		CTPeriod p;
		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
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
		if (test2.size() == 18 && test3.size() == 9)
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
		//https://climat.meteo.gc.ca/radar/index_f.html?site=CASVD&year=2017&month=8&day=1&hour=00&minute=00&duration=6&image_type=PRECIPET_SNOW_WEATHEROFFICE


		static const char pageFormat[] =
			"https://climate.weather.gc.ca/radar/index_e.html?"
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


		ASSERT(period.GetTM().Type() == CTM::HOURLY);

		std::set<string> tmpList;
		//loop each 2 hours
		for (size_t i = 0; i < radar.size() && msg; i++)
		{
			if (radar.none() || radar[i])
			{
				for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef += 2)
				{
					string radar_id = CCanadianRadar::GetName(i, CCanadianRadar::ABRV2);
					string typeName = TRef.GetYear() < 2014 ? TYPE_NAME_OLD[prcpType] : TYPE_NAME_NEW[prcpType];
					string URL = FormatA(pageFormat, radar_id.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), typeName.c_str());
					URL.resize(strlen(URL.c_str()));

					string source;
					//UtilWWW::GetPageText(pConnection, URL, source, true);
					string argument = "-s -k \"" + URL + "\"";
					string exe = GetApplicationPath() + "External\\curl.exe";
					CCallcURL cURL(exe);

					//string source;
					msg = cURL.get_text(argument, source);



					size_t begin = source.find("blobArray = [") + 13;//skip the first
					string fileList1 = FindString(source, "blobArray = [", "]", begin);
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
		bool bComposite = as<bool>(COMPOSITE);


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[CURRENT_RADAR], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
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
			string radar_id = GetLastDirName(dirList[i].m_filePath);
			if (radar.at(radar_id))
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
			string fileName = GetFileName(it->m_filePath);

			bool bIsCompsite = Find(fileName, "_COMP_");

			bool bOk0 = bComposite == bIsCompsite;
			bool bOk1 = bUseRain && bUseWhite && Find(fileName, "PRECIPET_RAIN_A11Y.gif");
			bool bOk2 = bUseRain && bUseBrown && Find(fileName, "PRECIPET_RAIN.gif");
			bool bOk3 = bUseSnow && bUseWhite && Find(fileName, "PRECIPET_SNOW_A11Y.gif");
			bool bOk4 = bUseSnow && bUseBrown && Find(fileName, "PRECIPET_SNOW.gif");


			if (bOk0 && (bOk1 || bOk2 || bOk3 || bOk4))
			{
				//this file is needed, is it up to date?
				string filePath = GetOutputFilePath(as<size_t>(TYPE), fileName);
				if (NeedDownload(*it, filePath))
					clearedList.push_back(*it);
			}


			msg += callback.StepIt();
		}

		callback.PopTask();

		callback.AddMessage("Number of images to download after clearing: " + ToString(clearedList.size()));
		callback.PushTask("Download images " + ToString(clearedList.size()) + " images", clearedList.size());
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


		callback.PushTask("Download historical radar images (" + ToString(imageList.size()) + ")", imageList.size());



		int nbRun = 0;
		int curI = 0;

		while (curI < imageList.size() && msg)
		{
			nbRun++;

			//CInternetSessionPtr pSession;
			//CHttpConnectionPtr pConnection;

			//msg = GetHttpConnection(SERVER_NAME[as<size_t>(TYPE)], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

			if (msg)
			{
				//TRY
			//	{
				for (int i = curI; i < imageList.size() && msg; i++)
				{
					string filePath = GetOutputFilePath(as<size_t>(TYPE), imageList[i]);
					msg += CreateMultipleDir(GetPath(filePath));


					StringVector name(imageList[i], "|"); ASSERT(name.size() == 2);
					string URL = "https://climate.weather.gc.ca" + name[1];

					string exe = "\"" + GetApplicationPath() + "External\\curl.exe\"";
					string argument = "-s -k \"" + URL + "\" --output \"" + filePath + "\"";
					string command = exe + " " + argument;

					DWORD exit_code;
					msg = WinExecWait(command, "", SW_HIDE, &exit_code);
					if (exit_code == 0 && FileExists(filePath))
					{
						ifStream file;
						msg += file.open(filePath);
						if (msg)
						{

							string line;
							std::getline(file, line);

							file.close();

							if (WBSF::Find(line, "GIF89"))
							{
								msg += callback.StepIt();
								curI++;
								nbRun = 0;
							}
							else
							{
								msg = WBSF::RemoveFile(filePath);
							}
						}

					}
					//}
					//}
					//CATCH_ALL(e)
					//{
					//	msg = UtilWin::SYGetMessage(*e);
					//}
					//END_CATCH_ALL

						//if an error occur: try again
					if (!msg && !callback.GetUserCancel())
					{
						//callback.AddTask(1);//one step more

						if (nbRun < 5)
						{
							callback.AddMessage(msg);
							msg = ERMsg();
							Sleep(1000);//wait 1 sec
						}
					}
				}
			}
		}


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI));
		callback.PopTask();


		return msg;
	}


	CRadarListNational CUIEnvCanRadar::GetNationalRadarListToUpdate()const
	{
		CRadarListNational image_list;

		time_t ltime;
		tm today = { 0 };

		_tzset();
		time(&ltime);
		_gmtime64_s(&today, &ltime);

		//month is zero base and day is in 1 base
		CTRef now = CTRef(1900 + today.tm_year, today.tm_mon, today.tm_mday - 1, today.tm_hour, CTM::HOURLY);
		int minute = int(today.tm_min/6)*6;
		//CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);
		CTPeriod p(now-3, now);//4 hours are available on the site
		
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			int mb = (TRef == p.Begin()) ? minute : 0;
			int me = (TRef == p.End()) ? minute : 60;
			for (int m = mb; m < me; m += 6)//images every 6 minutes
			{
				string date = FormatA("%04d-%02d-%02dT%02d:%02d:00Z", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), m);
				string path = FormatA("%s%s\\%04d\\%02d\\%02d\\%02d\\National_%04d-%02d-%02dT%02d%02d00Z.tif", GetDir(WORKING_DIR).c_str(), "National", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), m);
				
				if (!FileExists(path))
					image_list[date] = path;
			}
		}

		return image_list;
	}

	ERMsg CUIEnvCanRadar::ExecuteNational(CCallback& callback)
	{
		ERMsg msg;


		CRadarListNational images_list = GetNationalRadarListToUpdate();

		//https://geo.meteo.gc.ca/geomet?lang=fr&SERVICE=WMS&REQUEST=GetMap&FORMAT=image/png&TRANSPARENT=TRUE&STYLES=&VERSION=1.3.0&LAYERS=RADAR_1KM_RRAI&WIDTH=1871&HEIGHT=700&CRS=EPSG:3978&BBOX=-6991528.601092203,-1478754.0562611124,7859563.601092203,4077507.0562611124&TIME=2024-06-17T13%3A30%3A00Z
		//https://geo.meteo.gc.ca/geomet?lang=fr&SERVICE=WMS&REQUEST=GetMap&FORMAT=image/png&TRANSPARENT=TRUE&STYLES=&VERSION=1.3.0&LAYERS=RADAR_1KM_RRAI&WIDTH=1871&HEIGHT=700&CRS=EPSG:3978&BBOX=-6991528.601092203,-1478754.0562611124,7859563.601092203,4077507.0562611124&TIME=2024-06-18T9:00:00Z

		static const char PAGE_FORMAT_NAT[] =
			"https://geo.meteo.gc.ca/geomet?"
			"lang=fr&"
			"SERVICE=WMS&"
			"REQUEST=GetMap&"
			"FORMAT=image/png&"
			"TRANSPARENT=TRUE&"
			"STYLES=&VERSION=1.3.0&"
			"LAYERS=RADAR_1KM_RRAI&"
			"WIDTH=1871&"
			"HEIGHT=700&"
			"CRS=EPSG:3978&"
			"BBOX=-6991528.601092203,-1478754.0562611124,7859563.601092203,4077507.0562611124&"
			"TIME=%s";


		for (auto it = images_list.begin(); it != images_list.end()&&msg; it++)
		{
			string URL = FormatA(PAGE_FORMAT_NAT, it->first.c_str());
			string output_file_path = it->second;

			CreateMultipleDir( GetPath(output_file_path));

			string argument = "-s -k \"" + URL + "\"";
			
			CCallcURL cURL;
			msg += cURL.copy_file(URL, output_file_path + ".png");

			if (msg)
			{
				//verify that is a valid png file
				WBSF::ifStream file;
				file.open(output_file_path + ".png");
				char buffer[5] = { 0 };
				file.read(buffer, 4);
				file.close();

				if (string(buffer) == "‰PNG")
				{
					//gdal_translate - of GTiff - a_srs EPSG : 3978 - a_ullr - 6991528.601092203  4077507.0562611124 7859563.601092203 - 1478754.0562611124 "10;19.png" "OUTPUT3.tif"
					string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
					string projlib_path = GetApplicationPath() + "External\\projlib";
					string plugin_path = GetApplicationPath() + "External\\gdalplugins";

					string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";
					string argument = "-co COMPRESS=LZW -co TILED=YES -a_srs EPSG:3978 -a_ullr -6991528.601092203 4077507.0562611124 7859563.601092203 -1478754.0562611124";
					string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + option + " " + argument + " \"" + output_file_path + ".png" + "\" \"" + output_file_path + "\"";
					msg += WinExecWait(command);
				}


				msg += RemoveFile(output_file_path + ".png");
			}
		}

		return msg;
	}


	ERMsg CUIEnvCanRadar::GetRadarList(CTPeriod p, std::map<std::string, StringVector>& imageList, CCallback& callback)
	{
		ASSERT(p.GetTType() == CTM::DAILY);

		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		CCanadianRadar radar(Get(RADAR));
		

		StringVector radars = GetDirectoriesList(GetDir(WORKING_DIR) + "*");
		for (size_t d = 0; d < radars.size(); d++)
		{
			if (radar.at(radars[d]))
			{
				string radar_id = radars[d];
				string inputDir = GetDir(WORKING_DIR) + radar_id + "\\";

				StringVector years = GetDirectoriesList(inputDir + "*");
				
				for (size_t y = 0; y < years.size() && msg; y++)
				{
					int year = ToInt(years[y]);
					if (p.as(CTM::ANNUAL).IsInside(year))
					{
						string type = as<bool>(COMPOSITE) ? "COMP_" : "";
						if (year <= 2013)
							type += (as<size_t>(PRCP_TYPE) == T_RAIN) ? "PRECIP_RAIN" : "PRECIP_SNOW";
						else
							type += (as<size_t>(PRCP_TYPE) == T_RAIN) ? "PRECIPET_RAIN" : "PRECIPET_SNOW";

						if (as<size_t>(BACKGROUND) == B_WHITE)
							type += "_A11Y";


						StringVector tmpList = GetFilesList(inputDir + years[y] + "*_" + radar_id + "_" + type + ".gif", 2, true);
						for (size_t i = 0; i < tmpList.size() && msg; i++)
						{

							StringVector tmp(GetFileTitle(tmpList[i]), "_");
							ASSERT(tmp.size() == 4 || tmp.size() == 5);

							string year = tmp[0].substr(0, 4);
							string month = tmp[0].substr(4, 2);
							string day = tmp[0].substr(6, 2);
							//string hour = tmp[0].substr(8, 2);


							CTRef TRef(ToInt(year), ToSizeT(month) - 1, ToSizeT(day) - 1);

							if (p.as(CTM::DAILY).IsInside(TRef))
								imageList[radar_id + "_" + type].push_back(tmpList[i]);

							msg += callback.StepIt(0);

						}
					}

					msg += callback.StepIt();
				}
			}//is radar selected
		}//for all directories


		return msg;
	}


}