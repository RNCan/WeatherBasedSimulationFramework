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
		//string connection = Get(CONNECTION);
		string connectionTimeout = Get(CONNECTION_TIMEOUT);
		//string proxy = Get(PROXY);
		//int limit = as<int>(LIMIT);

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
			//CTRef TRef = GetTRef(s, fileList[i].m_filePath);
			//string outputFilePath = GetLocaleFilePath(source, fileList[i].m_filePath);
			//string tmpFilePaht = GetPath(outputFilePath) + GetFileName(fileList[i].m_filePath);
			//CreateMultipleDir(GetPath(outputFilePath));

			stript << "open ftp://"+ userName+":"+ password+"@" << Get(SERVER) << endl;
				
				
			//if (as<bool>(ASCII))
//				command += "-Ascii ";
			

			stript << "cd \"" << GetPath(Get(REMOTE)) << "\"" << endl;
			stript << "lcd \"" << GetPath(Get(LOCAL)) << "\"" << endl;
			stript << (direction == D_DOWNLOAD ? "get" : "put") << " \"" << GetFileName(Get(LOCAL)) << "\"" << endl;
			stript << "exit" << endl;
			stript.close();

			UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;
			bool bShow = as<bool>(SHOW_PROGRESS);
			string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + "/timeout="+ connectionTimeout +" /passive="+ (as<bool>(PASSIVE)?"on":"off")+" /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath+"\"";
			//if (!proxy.empty())
			//{
			//	ProxyMethod = 3 ProxyHost = proxy
			//}


			DWORD exitCode=0;
			msg = WinExecWait(command.c_str(), GetApplicationPath().c_str(), show, &exitCode);
			if (msg && exitCode != 0)
			{
				msg.ajoute("FTPTransfer as exit with error code " + ToString((int)exitCode));
				msg.ajoute("See log file: " + scriptFilePath + ".log");
			}
				

			//if (msg)
			//{
			//	callback.AddMessage("FTP Verification");

			//	//verify date and size on the FTP side
			//	CInternetSessionPtr pSession;
			//	CFtpConnectionPtr pConnection;

			//	string server = Get(SERVER);
			//	string remote = Get(REMOTE);
			//	string local = Get(LOCAL);

			//	msg = GetFtpConnection(server, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
			//	if (msg)
			//	{
			//		CFileInfoVector fileList;
			//		msg = FindFiles(pConnection, remote, fileList, false, callback);
			//		if (fileList.size() == 1)
			//		{
			//			CFileInfo info;
			//			if (GetFileInfo(local, info))
			//			{
			//				__int64 diffSize = __int64(info.m_size) - fileList.front().m_size;
			//				__int64 diffTime = __int64(info.m_time) - fileList.front().m_time;

			//				callback.AddMessage("diff size (Ko):" + ToString(diffSize / 1024.0, 1));
			//				callback.AddMessage("diff time (h):" + ToString(diffTime / 3600.0, 1));
			//			}
			//			else
			//			{
			//				msg.ajoute("ERROR: Locale file not find : " + local);
			//			}
			//		}
			//		else
			//		{
			//			msg.ajoute("ERROR: Remote file not find: " + remote);
			//		}
			//	}//msg
			//}
		}

		msg += callback.StepIt();
		callback.PopTask();


		return msg;
	}

	//ERMsg CCopyFTP::Execute(CCallback& callback)
	//{
	//	ERMsg msg;

	//	size_t direction = as<size_t>(DIRECTION);

	//	string userName = Get(USER_NAME);
	//	string password = Get(PASSWORD);
	//	string connection = Get(CONNECTION);
	//	string connectionTimeout = Get(CONNECTION_TIMEOUT);
	//	string proxy = Get(PROXY);
	//	int limit = as<int>(LIMIT);
	//	
	//	string input = direction == D_DOWNLOAD ? Get(REMOTE) : Get(LOCAL);
	//	string output = direction == D_DOWNLOAD ? Get(LOCAL) : Get(REMOTE);

	//	callback.AddMessage("FTPTransfer from:");
	//	callback.AddMessage(input, 1);
	//	callback.AddMessage("To:");
	//	callback.AddMessage(output, 1);
	//	callback.AddMessage("");

	//	if (direction == D_DOWNLOAD)
	//		msg = CreateMultipleDir(GetPath(output));

	//	callback.PushTask("FTPTransfer", NOT_INIT);

	//	string command = GetApplicationPath() + "External\\FTPTransfer.exe ";
	//	command += "-Server \"" + Get(SERVER) + "\" ";
	//	command += "-Remote \"" + Get(REMOTE) + "\" ";
	//	command += "-Local \"" + Get(LOCAL) + "\" ";
	//	if (!userName.empty())
	//		command += "-UserName \"" + userName + "\" ";
	//	if (!password.empty())
	//		command += "-Password \"" + password + "\" ";
	//	command += "-Connection " + connection + " ";
	//	command += "-ConnectionTimeout " + connectionTimeout + " ";
	//	if (!proxy.empty())
	//		command += "-Proxy \"" + proxy + "\" ";
	//	if (limit> 0)
	//		command += "-Limit " + ToString(limit) + " ";
	//	if (as<bool>(ASCII))
	//		command += "-Ascii ";
	//	if (as<bool>(PASSIVE))
	//		command += "-Passive ";

	//	command += (direction == D_DOWNLOAD ? "-Download" : "-Upload");

	//	UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;

	//	DWORD exitCode;
	//	msg = WinExecWait(command.c_str(), GetApplicationPath().c_str(), show, &exitCode);
	//	if (msg && exitCode != 0)
	//		msg.ajoute("FTPTransfer as exit with error code " + ToString((int)exitCode));

	//	if (msg)
	//	{
	//		callback.AddMessage("FTP Verification");

	//		//verify date and size on the FTP side
	//		CInternetSessionPtr pSession;
	//		CFtpConnectionPtr pConnection;

	//		string server = Get(SERVER);
	//		string remote = Get(REMOTE);
	//		string local = Get(LOCAL);

	//		msg = GetFtpConnection(server, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
	//		if (msg)
	//		{
	//			CFileInfoVector fileList;
	//			msg = FindFiles(pConnection, remote, fileList, false, callback);
	//			if (fileList.size() == 1)
	//			{
	//				CFileInfo info;
	//				if (GetFileInfo(local, info))
	//				{
	//					__int64 diffSize = __int64(info.m_size) - fileList.front().m_size;
	//					__int64 diffTime = __int64(info.m_time) - fileList.front().m_time;

	//					callback.AddMessage("diff size (Ko):" + ToString(diffSize / 1024.0, 1));
	//					callback.AddMessage("diff time (h):" + ToString(diffTime / 3600.0, 1));
	//					/*if (direction == D_DOWNLOAD)
	//					{
	//					}
	//					else
	//					{
	//					}*/
	//				}
	//				else
	//				{
	//					msg.ajoute("ERROR: Locale file not find : " + local);
	//				}
	//			}
	//			else
	//			{
	//				msg.ajoute("ERROR: Remote file not find: " + remote);
	//			}
	//		}//msg
	//	}

	//	msg += callback.StepIt();
	//	callback.PopTask();


	//	return msg;
	//}



}