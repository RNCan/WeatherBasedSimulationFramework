#include "StdAfx.h"
#include "CopyFTP.h"
#include "TaskFactory.h"
#include "UI/Common/UtilWWW.h"

#include "../Resource.h"

using namespace std;
using namespace UtilWWW;

namespace WBSF
{


	//*********************************************************************
	const char* CCopyFTP::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Direction", "Server", "Remote", "Local", "UserName", "Password", "ConnectionTimeout", "Passive", "ShowProgress" };
	const size_t CCopyFTP::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_COMBO_INDEX, T_STRING, T_STRING, T_FILEPATH, T_STRING, T_PASSWORD, T_STRING, T_BOOL, T_BOOL };
	const UINT CCopyFTP::ATTRIBUTE_TITLE_ID = IDS_TOOL_DOWNLOAD_UPLOAD_P;
	const UINT CCopyFTP::DESCRIPTION_TITLE_ID = ID_TASK_DOWNLOAD_UPLOAD;
	

	const char* CCopyFTP::CLASS_NAME(){ static const char* THE_CLASS_NAME = "FTPTransfer";  return THE_CLASS_NAME; }
	CTaskBase::TType CCopyFTP::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CCopyFTP::CLASS_NAME(), (createF)CCopyFTP::create);

	

	CCopyFTP::CCopyFTP(void)
	{}

	CCopyFTP::~CCopyFTP(void)
	{}


	std::string CCopyFTP::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case DIRECTION:		str = GetString(IDS_FTP_DIRECTION); break;
		//case CONNECTION:	str = GetString(IDS_FTP_CONNECTION_TYPE); break;
		};
		return str;
	}

	std::string CCopyFTP::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case DIRECTION:		str = ToString(D_DOWNLOAD); break;
		//case CONNECTION:	str = ToString(USE_PRECONFIG); break;
		case CONNECTION_TIMEOUT:str = "15000"; break;
		//case LIMIT:	str = "0"; break;
		//case ASCII:	str = "0"; break;
		case PASSIVE:	str = "1"; break;
		case SHOW_PROGRESS:	str = "0"; break;
		};

		return str;
	}

	
	ERMsg CCopyFTP::Execute(CCallback& callback)
	{
		ERMsg msg;

		
		size_t direction = as<size_t>(DIRECTION);

		string userName = Get(USER_NAME);
		string password = Get(PASSWORD);
		string connectionTimeout = Get(CONNECTION_TIMEOUT);

		if (userName.empty()) 
			userName = "anonymous";

		if (password.empty())
			password = "anonymous%40example.com";



		string input = direction == D_DOWNLOAD ? Get(REMOTE) : Get(LOCAL);
		string output = direction == D_DOWNLOAD ? Get(LOCAL) : Get(REMOTE);

		callback.AddMessage("FTPTransfer from:");
		callback.AddMessage(input, 1);
		callback.AddMessage("To:");
		callback.AddMessage(output, 1);
		callback.AddMessage("");

		if (direction == D_DOWNLOAD)
			msg = CreateMultipleDir(GetPath(Get(LOCAL)));

		callback.PushTask("FTPTransfer", NOT_INIT);

		
		//
		
		string workingDir = GetPath(Get(LOCAL));
		string scriptFilePath = workingDir + m_name + "_script.txt";
		WBSF::RemoveFile(scriptFilePath + ".log");



		ofStream stript;
		msg = stript.open(scriptFilePath);
		if (msg)
		{
			stript << "open ftp://"+ userName+":"+ password+"@" << Get(SERVER) << endl;

			stript << "cd \"" << GetPath(Get(REMOTE)) << "\"" << endl;
			stript << "lcd \"" << GetPath(Get(LOCAL)) << "\"" << endl;
			stript << (direction == D_DOWNLOAD ? "get" : "put") << " \"" << GetFileName(Get(LOCAL)) << "\"" << endl;
			stript << "exit" << endl;
			stript.close();

			UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;
			bool bShow = as<bool>(SHOW_PROGRESS);
			string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + "/timeout="+ connectionTimeout +" /passive="+ (as<bool>(PASSIVE)?"on":"off")+" /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath+"\"";
			
			DWORD exitCode=0;
			msg = WinExecWait(command.c_str(), GetApplicationPath().c_str(), show, &exitCode);
			if (msg && exitCode != 0)
			{
				msg.ajoute("WinSCP as exit with error code " + ToString((int)exitCode));
				msg.ajoute("See log file: " + scriptFilePath + ".log");
			}
			
		}

		msg += callback.StepIt();
		callback.PopTask();


		return msg;
	}



}