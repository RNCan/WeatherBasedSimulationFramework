#include "StdAfx.h"
#include "UIRTMA.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"

#include "TaskFactory.h"
#include "WeatherBasedSimulationString.h"
#include "../Resource.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;


namespace WBSF
{
	
	//Real-Time Mesoscale Analysis (RTMA/URMA) Products (meme extent que Hires)
	//ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/rtma/prod/rtma2p5.20170616/

	//*********************************************************************
	const char* CUIURMA::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Product" };
	const size_t CUIURMA::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH,  T_COMBO_INDEX };
	const UINT CUIURMA::ATTRIBUTE_TITLE_ID = IDS_UPDATER_RTMA_P;
	const UINT CUIURMA::DESCRIPTION_TITLE_ID = ID_TASK_RTMA;

	const char* CUIURMA::CLASS_NAME(){ static const char* THE_CLASS_NAME = "URMA";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIURMA::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIURMA::CLASS_NAME(), (createF)CUIURMA::create);
	const char* CUIURMA::SERVER_NAME = "ftp.ncep.noaa.gov";// "para.nomads.ncep.noaa.gov";
	const char* CUIURMA::SERVER_PATH = "pub/data/nccf/com/%1/prod/%12p5.*";
	                                  //pub/data/nccf/com/urma/prod/

	CUIURMA::CUIURMA(void)
	{}
	
	CUIURMA::~CUIURMA(void)
	{}
	

	std::string CUIURMA::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case PRODUCT:	str = "Real-Time Mesoscale Analysis (RTMA)|Unrestricted Mesoscale Analysis (URMA)"; break;
		};
		return str;
	}

	std::string CUIURMA::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "URMA\\"; break;
		case PRODUCT: str = "1"; break;
		};

		return str;
	}
		
	

	//************************************************************************************************************
	//Load station definition list section


	//*************************************************************************************************


	ERMsg CUIURMA::Execute(CCallback& callback)
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
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
		if (!msg)
			return msg;

		
		size_t product = as<size_t>(PRODUCT);
		string p = product == RTMA ? "rtma" : "urma";
		CFileInfoVector fileList;
		string server_path = SERVER_PATH;
		ReplaceString(server_path, "%1", p.c_str());
//
//"rtma2p5.2018102800.pcp.184.grb2"
//"urma2p5.2018102800.pcp_01h.184.grb2"
//"urma2p5.t00z.2dvaranl_ndfd.grb2_wext"
//"rtma2p5.t00z.2dvaranl_ndfd.grb2_wexp"



		CFileInfoVector dir;
		msg = FindDirectories(pConnection, server_path, dir);

		callback.PushTask(string("Get RTMA/URMA files list from: ") + server_path, dir.size());
		for (CFileInfoVector::const_iterator it1 = dir.begin(); it1 != dir.end() && msg; it1++)
		{
			CFileInfoVector fileListTmp;
			//msg = FindFiles(pConnection, it1->m_filePath + "RTMA/URMA.t??z.arw_2p5km.f00.conus.grib2", fileListTmp);
			
			//msg = FindFiles(pConnection, it1->m_filePath + "rtma2p5.t??z.2dvaranl_ndfd.grb2_wexp", fileListTmp);
			msg = FindFiles(pConnection, it1->m_filePath + p + "2p5.t??z.2dvaranl_ndfd.grb2_wexp", fileListTmp);
			
			
			CFileInfoVector fileListPrcp;
			string URL = it1->m_filePath + p + "2p5.??????????.pcp" + (product == RTMA ? "" : "_01h") + ".184.grb2";
			msg = FindFiles(pConnection, URL, fileListPrcp);
			fileListTmp.insert(fileListTmp.end(), fileListPrcp.begin(), fileListPrcp.end());
			

			for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
			{
				
				string outputFilePath = GetLocalFilePath(it->m_filePath);
				CFileInfo info;


				ifStream stream;
				if (!stream.open(outputFilePath))
				{
					fileList.push_back(*it);
				}
				else
				{
					//verify if the file finish with 7777
					char test[5] = { 0 };
					stream.seekg(-4, ifstream::end);
					stream.read(&(test[0]), 4);

					if (string(test) != "7777")
					{
						fileList.push_back(*it);
					}
				}
			}

			msg += callback.StepIt();
		}


		callback.PopTask();


		callback.AddMessage("Number of RTMA/URMA gribs to download: " + ToString(fileList.size()));
		callback.PushTask("Download RTMA/URMA gribs (" + ToString(fileList.size()) + ")", fileList.size());

		size_t nbDownload = 0;
		for (CFileInfoVector::iterator it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			//string fileName = GetFileName(it->m_filePath);
			string outputFilePath = GetLocalFilePath(it->m_filePath);

			callback.PushTask("Download RTMA/URMA gribs:" + outputFilePath, NOT_INIT);

			CreateMultipleDir(GetPath(outputFilePath));
			msg = CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, false);
			if (msg && FileExists(outputFilePath))
			{
				nbDownload++;

			}

			callback.PopTask();
			msg += callback.StepIt();
		}

		pConnection->Close();
		pSession->Close();


		callback.AddMessage("Number of RTMA/URMA gribs downloaded: " + ToString(nbDownload));
		callback.PopTask();


		return msg;


		return msg;
	}


	ERMsg CUIURMA::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	ERMsg CUIURMA::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CUIURMA::GetGribsList(CTPeriod p, std::map<CTRef, std::string>& gribsList, CCallback& callback)
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

			for (size_t i = 0; i < list1.size(); i++)
			{
				CTRef TRef = GetTRef(list1[i]);
				if (p.IsInside(TRef) )
					gribsList[TRef] = list1[i];
			}
			
		}



		return msg;
	}

	CTRef CUIURMA::GetTRef(string filePath)
	{
		CTRef TRef;
		
		//rtma2p5.t17z.2dvaranl_ndfd_wexp.grib2
		//rtma2p5.2018102100.pcp.184.grib2
		string title = GetFileTitle(filePath);
		StringVector tmp(title, ".");
		ASSERT(tmp.size() == 3 || tmp.size() == 4);

		if (tmp.size() == 3)
		{
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
			size_t h = WBSF::as<int>(tmp[1].substr(1, 2));
			TRef = CTRef(year, m, d, h);
		}
		else if (tmp.size() == 4)
		{

			int year = WBSF::as<int>(tmp[1].substr(0, 4));
			size_t m = WBSF::as<int>(tmp[1].substr(4, 2)) - 1;
			size_t d = WBSF::as<int>(tmp[1].substr(6, 2)) - 1;
			size_t h = WBSF::as<int>(tmp[1].substr(8, 2));
			TRef = CTRef(year,m,d,h);
		}

		return TRef;
	}

	string CUIURMA::GetLocalFilePath(const string& remote)const
	{
		string workingDir = GetDir(WORKING_DIR);
		string dir = WBSF::GetLastDirName(GetPath(remote));
		string fileTitle = GetFileTitle(remote);
		string ext = GetFileExtension(remote);
		if (ext.find("_wexp") != string::npos)
			fileTitle += "_wexp";

		int year = WBSF::as<int>(dir.substr(8, 4));
		int month = WBSF::as<int>(dir.substr(12, 2));
		int day = WBSF::as<int>(dir.substr(14, 2));


		return FormatA("%s%d\\%02d\\%02d\\%s.grib2", workingDir.c_str(), year, month, day, fileTitle.c_str());
	}

}