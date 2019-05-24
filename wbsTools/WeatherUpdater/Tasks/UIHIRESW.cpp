#include "StdAfx.h"
#include "UIHIRESW.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Geomatic/SfcGribsDatabase.h"

#include "TaskFactory.h"
#include "WeatherBasedSimulationString.h"
#include "../Resource.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;


namespace WBSF
{
	
	//pour estimation de surface, c'est très bien et beaucoup plus petit que HRRR
	//High-Resolution Window (HIRESW) Forecast System 
	//ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/hiresw/prod/hiresw.20170616/
	//Real-Time Mesoscale Analysis (RTMA) Products (meme extent que Hires)
	//ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/rtma/prod/rtma2p5.20170616/

	////ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/hiresw/prod/hiresw.20181026/


	//WARNING: this product don't have precipitation


	//*********************************************************************
	const char* CUIHIRESW::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "DynamicalCores" };
	const size_t CUIHIRESW::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH,  T_COMBO_INDEX };
	const UINT CUIHIRESW::ATTRIBUTE_TITLE_ID = IDS_UPDATER_HIRESW_P;
	const UINT CUIHIRESW::DESCRIPTION_TITLE_ID = ID_TASK_HIRESW;

	const char* CUIHIRESW::CLASS_NAME(){ static const char* THE_CLASS_NAME = "HIRESW";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIHIRESW::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIHIRESW::CLASS_NAME(), (createF)CUIHIRESW::create);
	const char* CUIHIRESW::SERVER_NAME = "ftp.ncep.noaa.gov";// "ftpprd.ncep.noaa.gov";
	const char* CUIHIRESW::SERVER_PATH = "pub/data/nccf/com/hiresw/prod/hiresw.*";

	CUIHIRESW::CUIHIRESW(void)
	{}
	
	CUIHIRESW::~CUIHIRESW(void)
	{}
	

	std::string CUIHIRESW::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DYNAMICAL_CORES:	str = "Advanced Research Weather Research and Forecasting (WRF-ARW)|Nonhydrostatic Multiscale Model on B-grid (NMMB)"; break;
		};
		return str;
	}

	std::string CUIHIRESW::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "HIRESW\\"; break;
		case DYNAMICAL_CORES: str = "1"; break;
		};

		return str;
	}
		
	

	//************************************************************************************************************
	//Load station definition list section


	//*************************************************************************************************


	ERMsg CUIHIRESW::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);
		

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (!msg)
			return msg;

		size_t core = as<size_t>(DYNAMICAL_CORES);

		callback.PushTask(string("Get HIRESW files list from: ") + SERVER_PATH, 2);
		CFileInfoVector fileList;


		CFileInfoVector dir;
		msg = FindDirectories(pConnection, SERVER_PATH, dir);
		for (CFileInfoVector::const_iterator it1 = dir.begin(); it1 != dir.end() && msg; it1++)
		{
			CFileInfoVector fileListTmp;
			//msg = FindFiles(pConnection, it1->m_filePath + "hiresw.t??z.arw_2p5km.f00.conus.grib2", fileListTmp);
			msg = FindFiles(pConnection, it1->m_filePath + +"hiresw.t??z." + (core==0?"arw":"nmmb") +"_2p5km.f??.conus.grb2", fileListTmp);

			for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
			{
				
				string outputFilePath = GetLocalFilePath(it->m_filePath);
				if (!GoodGrib(outputFilePath))
					fileList.push_back(*it);
				
				//CFileInfo info;


				//ifStream stream;
				//if (!stream.open(outputFilePath))
				//{
				//	fileList.push_back(*it);
				//}
				//else
				//{
				//	//verify if the file finish with 7777
				//	char test[5] = { 0 };
				//	stream.seekg(-4, ifstream::end);
				//	stream.read(&(test[0]), 4);

				//	if (string(test) != "7777")
				//	{
				//		fileList.push_back(*it);
				//	}
				//}
			}

			msg += callback.StepIt();
		}


		callback.PopTask();


		callback.AddMessage("Number of HIRESW gribs to download: " + ToString(fileList.size()));
		callback.PushTask("Download HIRESW gribs (" + ToString(fileList.size()) + ")", fileList.size());

		int nbDownload = 0;
		for (CFileInfoVector::iterator it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			//string fileName = GetFileName(it->m_filePath);
			string outputFilePath = GetLocalFilePath(it->m_filePath);

			callback.PushTask("Download HIRESW gribs:" + outputFilePath, NOT_INIT);

			CreateMultipleDir(GetPath(outputFilePath));
			msg = CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
			if (msg && FileExists(outputFilePath))
			{
				nbDownload++;

			}

			callback.PopTask();
			msg += callback.StepIt();
		}

		pConnection->Close();
		pSession->Close();


		callback.AddMessage("Number of HIRESW gribs downloaded: " + ToString(nbDownload));
		callback.PopTask();


		return msg;


		return msg;
	}


	ERMsg CUIHIRESW::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	ERMsg CUIHIRESW::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CUIHIRESW::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		
		
		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;
		
		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);


			StringVector list1;
			list1 = GetFilesList(workingDir + ToString(year) + "\\*.grib2", FILE_PATH, true);
			//StringVector list2;
			//list2 = GetFilesList(workingDir + ToString(year) + "\\*.grib2.idx", FILE_PATH, true);

			for (size_t i = 0; i < list1.size(); i++)
			{
				CTRef TRef = GetTRef(list1[i]);
				if (p.IsInside(TRef) )
					gribsList[TRef] = list1[i];
			}
			
		}



		return msg;
	}

	CTRef CUIHIRESW::GetTRef(string filePath)
	{
		CTRef TRef;

		string name = GetFileTitle(filePath);
		filePath = GetPath(filePath);
		string dir1 = WBSF::GetLastDirName(filePath);
		while (WBSF::IsPathEndOk(filePath))
			filePath = filePath.substr(0, filePath.length() - 1);
		filePath = GetPath(filePath);
		string dir2 = WBSF::GetLastDirName(filePath);
		while (WBSF::IsPathEndOk(filePath))
			filePath = filePath.substr(0, filePath.length() - 1);
		filePath = GetPath(filePath);
		string dir3 = WBSF::GetLastDirName(filePath);

		int year = WBSF::as<int>(dir3);
		size_t m = WBSF::as<int>(dir2) - 1;
		size_t d = WBSF::as<int>(dir1) - 1;
		size_t h = WBSF::as<int>(name.substr(8, 2));
		TRef = CTRef(year, m, d, h);

		return TRef;
	}

	string CUIHIRESW::GetLocalFilePath(const string& remote)const
	{
		string workingDir = GetDir(WORKING_DIR);
		string dir = WBSF::GetLastDirName(GetPath(remote));
		string fileName = GetFileName(remote);

		int year = WBSF::as<int>(dir.substr(7, 4));
		int month = WBSF::as<int>(dir.substr(11, 2));
		int day = WBSF::as<int>(dir.substr(13, 2));


		return FormatA("%s%d\\%02d\\%02d\\%s", workingDir.c_str(), year, month, day, fileName.c_str());
	}

}