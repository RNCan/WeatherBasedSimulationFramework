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
	const char* CCopyFTP::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "Direction", "Server", "Remote", "Local", "UserName", "Password", "Connection", "ConnectionTimeout", "Proxy", "Limit", "Ascii", "Passive", "ShowProgress" };
	const size_t CCopyFTP::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_COMBO_POSITION, T_STRING, T_STRING, T_FILEPATH, T_STRING, PASSWORD, T_COMBO_POSITION, T_STRING, T_STRING, T_STRING, T_BOOL, T_BOOL, T_BOOL };
	const UINT CCopyFTP::ATTRIBUTE_TITLE_ID = IDS_TOOL_DOWNLOAD_UPLOAD_P;
	

	const char* CCopyFTP::CLASS_NAME(){ static const char* THE_CLASS_NAME = "FTPTransfer";  return THE_CLASS_NAME; }
	CTaskBase::TType CCopyFTP::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterClass(CCopyFTP::CLASS_NAME(), CCopyFTP::create);

	

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
		case CONNECTION:	str = GetString(IDS_FTP_CONNECTION_TYPE); break;
		};
		return str;
	}

	std::string CCopyFTP::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case DIRECTION:		str = ToString(D_DOWNLOAD); break;
		case CONNECTION:	str = ToString(USE_PRECONFIG); break;
		case CONNECTION_TIMEOUT:str = "15000"; break;
		case LIMIT:	str = "0"; break;
		case ASCII:	str = "0"; break;
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
		string connection = Get(CONNECTION);
		string connectionTimeout = Get(CONNECTION_TIMEOUT);
		string proxy = Get(PROXY);
		int limit = as<int>(LIMIT);
		
		string input = direction == D_DOWNLOAD ? Get(REMOTE) : Get(LOCAL);
		string output = direction == D_DOWNLOAD ? Get(LOCAL) : Get(REMOTE);

		callback.AddMessage("FTPTransfer from:");
		callback.AddMessage(input, 1);
		callback.AddMessage("To:");
		callback.AddMessage(output, 1);
		callback.AddMessage("");

		if (direction == D_DOWNLOAD)
			msg = CreateMultipleDir(GetPath(output));

		//string password = Decrypt(m_password);


		callback.PushTask("FTPTransfer", NOT_INIT);

		string command = GetApplicationPath() + "External\\FTPTransfer.exe ";
		command += "-Server \"" + Get(SERVER) + "\" ";
		command += "-Remote \"" + Get(REMOTE) + "\" ";
		command += "-Local \"" + Get(LOCAL) + "\" ";
		if (!userName.empty())
			command += "-UserName \"" + userName + "\" ";
		if (!password.empty())
			command += "-Password \"" + password + "\" ";
		command += "-Connection " + connection + " ";
		command += "-ConnectionTimeout " + connectionTimeout + " ";
		if (!proxy.empty())
			command += "-Proxy \"" + proxy + "\" ";
		if (limit> 0)
			command += "-Limit " + ToString(limit) + " ";
		if (as<bool>(ASCII))
			command += "-Ascii ";
		if (as<bool>(PASSIVE))
			command += "-Passive ";

		command += (direction == D_DOWNLOAD ? "-Download" : "-Upload");



		DWORD exitCode;
		msg = WinExecWait(command.c_str(), GetApplicationPath().c_str(), as<bool>(SHOW_PROGRESS)?SW_SHOW:SW_HIDE, &exitCode);
		if (msg && exitCode != 0)
			msg.ajoute("FTPTransfer as exit with error code " + ToString((int)exitCode));


		msg += callback.StepIt();
		callback.PopTask();


		return msg;
	}



}