#include "StdAfx.h"
#include "UIEnvCanPrcpRadar.h"
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


	//*********************************************************************
	const char* CUIEnvCanPrcpRadar::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Type"};
	const size_t CUIEnvCanPrcpRadar::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX};
	const UINT CUIEnvCanPrcpRadar::ATTRIBUTE_TITLE_ID = IDS_UPDATER_EC_PRCP_RADAR_P;
	const UINT CUIEnvCanPrcpRadar::DESCRIPTION_TITLE_ID = ID_TASK_EC_PRCP_RADAR;

	const char* CUIEnvCanPrcpRadar::CLASS_NAME(){ static const char* THE_CLASS_NAME = "EnvCanRadarPrcp";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIEnvCanPrcpRadar::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIEnvCanPrcpRadar::CLASS_NAME(), (createF)CUIEnvCanPrcpRadar::create);

	
	const char* CUIEnvCanPrcpRadar::SERVER_NAME = "dd.weather.gc.ca";
	const char* CUIEnvCanPrcpRadar::SERVER_PATH = "analysis/precip/rdpa/grib2/polar_stereographic/";



	CUIEnvCanPrcpRadar::CUIEnvCanPrcpRadar(void)
	{}

	CUIEnvCanPrcpRadar::~CUIEnvCanPrcpRadar(void)
	{}
	
	std::string CUIEnvCanPrcpRadar::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case TYPE:	str = "06|24"; break;
		};
		return str;
	}

	std::string CUIEnvCanPrcpRadar::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "EnvCan\\PrcpRadar\\"; break;
		case TYPE: str = ToString(TYPE_06HOURS); break;
		};

		return str;
	}


	bool CUIEnvCanPrcpRadar::NeedDownload(const CFileInfo& info, const string& filePath)const
	{
		bool bDownload = true;

		CFileStamp fileStamp(filePath);
		__time64_t lastUpdate = fileStamp.m_time;
		if (lastUpdate > 0 && info.m_time < lastUpdate)
			bDownload = false;

		return bDownload;
	}

	string CUIEnvCanPrcpRadar::GetOutputFilePath(const string& fileName)const
	{

		string year = Right(fileName, 20).substr(0, 4);
		string type = as<size_t>(TYPE) == 0 ? "06" : "24";

		return GetDir(WORKING_DIR) + type + "\\" + year + "\\" + fileName;
	}


	ERMsg CUIEnvCanPrcpRadar::Execute(CCallback& callback)
	{
		ERMsg msg;
		string workingDir = GetDir(WORKING_DIR);

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		string type = as<size_t>(TYPE) == TYPE_06HOURS ? "06" : "24";
		string path = SERVER_PATH + type + "/*.grib2";


		CFileInfoVector fileList;
		msg = UtilWWW::FindFiles(pConnection, path, fileList);

		callback.AddMessage("Number of images found: " + ToString(fileList.size()));
		//keep only 10km grid
		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end();)
		{
		//	string fileTitle = GetFileTitle(it->m_filePath);
//			if (Find(fileTitle, "ps10km"))
	//		{
		//		it = fileList.erase(it);
//			}
	//		else
		//	{
			string fileName = GetFileName(it->m_filePath);
			string filePath = GetOutputFilePath(fileName);
			if (!NeedDownload(*it, filePath))
				it = fileList.erase(it);
			else
				it++;
			//}

			msg += callback.StepIt(0);
		}

		//remove up to date file

		//for(int i=fileList.size()-1; i>=0; i--)
		//{
		//	string fileName = GetFileName(fileList[i].m_filePath);
		//	string filePath = GetOutputFilePath(fileName);
		//	if( !NeedDownload(fileList[i], filePath) )
		//		fileList.erase(fileList.begin() + i);

		//	msg += callback.StepIt(0);
		//}



		callback.AddMessage("Number of images to download after clearing: " + ToString(fileList.size()));
		callback.PushTask("Download precipitation images + (" + ToString(fileList.size() )+ ")", fileList.size());
		//callback.SetNbStep(fileList.size());

		int nbDownload = 0;
		for (size_t i = 0; i < fileList.size() && msg; i++)
		{
			string filePath = GetOutputFilePath(GetFileName(fileList[i].m_filePath));
			CreateMultipleDir(GetPath(filePath));
			msg = UtilWWW::CopyFile(pConnection, fileList[i].m_filePath, filePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
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


	//ERMsg CUIEnvCanPrcpRadar::GetStationList(StringVector& stationList, CCallback& callback)
	//{
	//	ERMsg msg;
	//	return msg;
	//}

}