#include "StdAfx.h"
#include "GoogleDriveTask.h"
#include "basic/CallcURL.h"
#include "basic/GoogleDrive.h"
#include "TaskFactory.h"
#include "UI/Common/UtilWWW.h"

#include "../Resource.h"

using namespace std;
using namespace UtilWWW;

namespace WBSF
{


	//*********************************************************************
	const char* CGoogleDriveTask::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "GoogleDriveLink", "Local", "ShowProgress" };
	const size_t CGoogleDriveTask::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_URL, T_PATH, T_BOOL };
	const UINT CGoogleDriveTask::ATTRIBUTE_TITLE_ID = IDS_TOOL_GOOGLE_DRIVE_P;
	const UINT CGoogleDriveTask::DESCRIPTION_TITLE_ID = ID_TASK_GOOGLE_DRIVE;


	const char* CGoogleDriveTask::CLASS_NAME() { static const char* THE_CLASS_NAME = "GoogleDrive";  return THE_CLASS_NAME; }
	CTaskBase::TType CGoogleDriveTask::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CGoogleDriveTask::CLASS_NAME(), (createF)CGoogleDriveTask::create);



	CGoogleDriveTask::CGoogleDriveTask(void)
	{}

	CGoogleDriveTask::~CGoogleDriveTask(void)
	{}


	std::string CGoogleDriveTask::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
			//case DIRECTION:		str = GetString(IDS_FTP_DIRECTION); break;
			//case CONNECTION:	str = GetString(IDS_FTP_CONNECTION_TYPE); break;
		//default:;
		//};
		return str;
	}

	std::string CGoogleDriveTask::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case SHOW_PROGRESS:	str = "0"; break;
		default:;
		};

		return str;
	}


	ERMsg CGoogleDriveTask::Execute(CCallback& callback)
	{
		ERMsg msg;


		string file_id = Get(REMOTE);
		if (file_id.find("drive.google.com")!=string::npos)
			file_id = CGoogleDrive::GetFileIDFromURL(file_id);


		string file_name = CGoogleDrive::GetFileName(file_id);
		string path_out = Get(LOCAL);
		string file_path_out = path_out + "\\" + file_name;
		bool bShow = as<bool>(SHOW_PROGRESS);

		CreateMultipleDir(path_out);

		msg = CGoogleDrive::DownloadFile(file_id, file_path_out, bShow, callback);

		return msg;
	}



}