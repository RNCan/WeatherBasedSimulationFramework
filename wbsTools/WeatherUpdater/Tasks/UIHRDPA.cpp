#include "StdAfx.h"
#include "UIHRDPA.h"
#include "HRDPA.h"
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



	//http://climate.weather.gc.ca/radar/image.php?time=04-MAR-14%2005.21.06.293480%20PM&site=WBI
	////HRDPA:
	//http://dd.weather.gc.ca/analysis/precip/hrdpa/grib2/polar_stereographic/24/


	//*********************************************************************
	const char* CUIHRDPA::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Product", "Type", "MaxHour" };
	const size_t CUIHRDPA::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX, T_STRING };
	const UINT CUIHRDPA::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_PRCP_RADAR_P;
	const UINT CUIHRDPA::DESCRIPTION_TITLE_ID = ID_TASK_EC_PRCP_RADAR;

	const char* CUIHRDPA::CLASS_NAME(){ static const char* THE_CLASS_NAME = "HRDPA";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIHRDPA::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIHRDPA::CLASS_NAME(), (createF)CUIHRDPA::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("EnvCanRadarPrcp", (createF)CUIHRDPA::create);
	

	
	//const char* CUIHRDPA::SERVER_NAME = "dd.weather.gc.ca";
	//const char* CUIHRDPA::SERVER_PATH = "analysis/precip/%1/grib2/polar_stereographic/";



	CUIHRDPA::CUIHRDPA(void)
	{}

	CUIHRDPA::~CUIHRDPA(void)
	{}
	
	std::string CUIHRDPA::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case PRODUCT:	str = "RDPA|HRDPA"; break;
		case TYPE:	str = "06|24"; break;
		};
		return str;
	}

	std::string CUIHRDPA::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\HRDPA\\"; break;
		case PRODUCT: str = "1"; break;
		case TYPE: str = ToString(TYPE_06HOURS); break;
		case MAX_HOUR: str = "48"; break;
		};

		return str;
	}

/*
	bool CUIHRDPA::NeedDownload(const CFileInfo& info, const string& filePath)const
	{
		bool bDownload = true;
		if (GoodGrib(filePath))
		{
			CFileStamp fileStamp(filePath);
			__time64_t lastUpdate = fileStamp.m_time;
			if (lastUpdate > 0 && info.m_time < lastUpdate)
				bDownload = false;
		}

		return bDownload;
	}

	string CUIHRDPA::GetOutputFilePath(const string& fileName)const
	{

		string year = Right(fileName, 20).substr(0, 4);
		string type = as<size_t>(TYPE) == 0 ? "06" : "24";

		return GetDir(WORKING_DIR) + type + "\\" + year + "\\" + fileName;
	}
*/

	ERMsg CUIHRDPA::Execute(CCallback& callback)
	{
		CHRDPA HRDPA;
		HRDPA.m_bByHRDPS = false;
		HRDPA.m_workingDir = GetDir(WORKING_DIR);
		HRDPA.m_type = (CHRDPA::TPrcpPeriod)as<size_t>(TYPE);
		HRDPA.m_product = (CHRDPA::TProduct)as<size_t>(PRODUCT);
		HRDPA.m_max_hours = as<int>(MAX_HOUR);

		//size_t product = 
//string type = == TYPE_06HOURS ? "06" : "24";


		return HRDPA.Execute(callback);
//		ERMsg msg;
		
		//string workingDir = GetDir(WORKING_DIR);

		//CInternetSessionPtr pSession;
		//CHttpConnectionPtr pConnection;

		//msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		//if (!msg)
		//	return msg;

		//callback.AddMessage(GetString(IDS_UPDATE_DIR));
		//callback.AddMessage(workingDir, 1);
		//callback.AddMessage(GetString(IDS_UPDATE_FROM));
		//callback.AddMessage(SERVER_NAME, 1);
		//callback.AddMessage("");

		//size_t product = as<size_t>(PRODUCT);
		//string type = as<size_t>(TYPE) == TYPE_06HOURS ? "06" : "24";
		//string path = SERVER_PATH + type + "/*.grib2";
		//ReplaceString(path, "%1", product == RDPA ? "rdpa":"hrdpa");


		//CFileInfoVector fileList;
		//msg = UtilWWW::FindFiles(pConnection, path, fileList);

		//callback.AddMessage("Number of images found: " + ToString(fileList.size()));
		////keep only 10km grid
		//for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end();)
		//{
		//	string fileName = GetFileName(it->m_filePath);
		//	string filePath = GetOutputFilePath(fileName);

		//	if (!CHRDPA::NeedDownload(*it, filePath))
		//		it = fileList.erase(it);
		//	else
		//		it++;

		//	msg += callback.StepIt(0);
		//}

		//callback.AddMessage("Number of images to download after clearing: " + ToString(fileList.size()));
		//callback.PushTask("Download RDPA/HRDPA precipitation images (" + ToString(fileList.size() )+ ")", fileList.size());

		//int nbDownload = 0;
		//for (size_t i = 0; i < fileList.size() && msg; i++)
		//{
		//	string filePath = GetOutputFilePath(GetFileName(fileList[i].m_filePath));
		//	CreateMultipleDir(GetPath(filePath));
		//	msg = UtilWWW::CopyFile(pConnection, fileList[i].m_filePath, filePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
		//	if (msg&&GoodGrib(filePath))
		//	{
		//		nbDownload++;
		//	}
		//	else
		//	{
		//		callback.AddMessage("corrupt file, remove: " + filePath);
		//		msg = WBSF::RemoveFile(filePath);
		//	}

		//	msg += callback.StepIt();
		//}

		//pConnection->Close();
		//pSession->Close();


		//callback.AddMessage("Number of images downloaded: " + ToString(nbDownload));
		//callback.PopTask();

	//	return msg;
	}



}