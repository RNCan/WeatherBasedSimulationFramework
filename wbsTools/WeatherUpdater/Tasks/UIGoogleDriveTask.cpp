#include "StdAfx.h"
#include "UIGoogleDriveTask.h"
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
	const char* CGoogleDriveTask::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "GoogleDriveLink", "DownloadPath", "UnzipFile", "OutputPath", "ShowProgress" };
	const size_t CGoogleDriveTask::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_URL, T_PATH, T_BOOL, T_PATH, T_BOOL };
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
		case UNZIP_FILE:	str = "1"; break;
		case SHOW_PROGRESS:	str = "0"; break;
		default:;
		};

		return str;
	}


	ERMsg CGoogleDriveTask::Execute(CCallback& callback)
	{
		ERMsg msg;


		string file_id = Get(URL_LINK);
		if (file_id.find("drive.google.com") != string::npos)
			file_id = CGoogleDrive::GetFileIDFromURL(file_id);


		string file_name = CGoogleDrive::GetFileName(file_id);
		string download_path = Get(DOWNLOAD_PATH);
		string download_filepath = download_path + "\\" + file_name;
		bool bShow = as<bool>(SHOW_PROGRESS);
		bool bUnzip = as<bool>(UNZIP_FILE);

		msg += CreateMultipleDir(download_path);
		if(msg)
			msg += CGoogleDrive::DownloadFile(file_id, download_filepath, bShow, callback);

		if (msg && bUnzip)
		{
			ASSERT(FileExists(download_filepath));

			string path_out = Get(OUTPUT_PATH);

			msg = CreateMultipleDir(path_out);
			if (msg)
			{
				string command = GetApplicationPath() + "External\\7za.exe x \"" + download_filepath + "\" -aoa -o\"" + path_out + "\"";

				callback.PushTask(GetString(IDS_UNZIP_FILE) + ": "+ GetFileName(download_filepath), NOT_INIT);
				callback.AddMessage(GetString(IDS_UNZIP_FILE) );
				callback.AddMessage(path_out);

				msg = WinExecWait(command.c_str(), "", bShow ? SW_SHOW : SW_HIDE);

				callback.PopTask();
			}


		}

		return msg;
	}



}