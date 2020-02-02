#include "stdafx.h"
#include "ZipUnzip.h"
#include "TaskFactory.h"
#include "../Resource.h"
#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{

	//*********************************************************************
	const char* CZipUnzip::ATTRIBUTE_NAME[] = { "Command", "ZipFilepath", "Directory", "Filter", "AddSubDirectory" };
	const size_t CZipUnzip::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_COMBO_INDEX, T_FILEPATH, T_PATH, T_STRING, T_BOOL };
	const UINT CZipUnzip::ATTRIBUTE_TITLE_ID = IDS_TOOL_ZIP_UNZIP_P;
	const UINT CZipUnzip::DESCRIPTION_TITLE_ID = ID_TASK_ZIP_UNZIP;

	const char* CZipUnzip::CLASS_NAME(){ static const char* THE_CLASS_NAME = "ZipUnzip";  return THE_CLASS_NAME; }
	CTaskBase::TType CZipUnzip::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CZipUnzip::CLASS_NAME(), (createF)CZipUnzip::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("Zip", (createF)CZipUnzip::create);
	

	CZipUnzip::CZipUnzip(void)
	{}

	CZipUnzip::~CZipUnzip(void)
	{}



	std::string CZipUnzip::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case COMMAND:		str = GetString(IDS_STR_ZIP_COMMAND); break;
		case ZIP_FILEPATH:	str = GetString(IDS_STR_FILTER_ZIP); break;
		case COPY_SUB_DIRECTORY: str = "0"; break;
		};

		return str;
	}

	std::string CZipUnzip::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case COMMAND:	str = ToString(UNZIP); break;
		case FILTER:	str = "*.*"; break;
		case COPY_SUB_DIRECTORY: str = "0"; break;
		};

		return str;
	}


	ERMsg CZipUnzip::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t command = as<size_t>(COMMAND);

		if (command == ZIP)
			msg = Zip(callback);
		else if (command == UNZIP)
			msg = Unzip(callback);
		else
			msg.ajoute("Unknown zip command");

		return msg;
	}

	ERMsg CZipUnzip::Zip(CCallback& callback)
	{
		ERMsg msg;

		string filepath = Get(ZIP_FILEPATH);
		string dir = GetDir(DIRECTORY);
		string filter = Get(FILTER);

		if (dir.empty())
			dir = ".\\";
		dir = GetAbsolutePath(GetProjectPath(), dir);

		if (FileExists(filepath))
			RemoveFile(filepath);

		string command = GetApplicationPath() + "External\\7za.exe a ";
		if (as<bool>(COPY_SUB_DIRECTORY) )
			command += "-r ";

		command += "\"" + filepath + "\" \"" + dir + filter + "\"";

		callback.PushTask(GetString(IDS_ZIP_FILE), NOT_INIT);
		callback.AddMessage(GetString(IDS_ZIP_FILE));

		msg = WinExecWait(command.c_str());

		if (msg)
		{
			CFileInfo info;
			if (GetFileInfo(filepath, info) && info.m_size> 100)
			{
				callback.AddMessage(FormatMsg(IDS_BSC_FILE_CREATE_SUCCESS, filepath));
			}
			else
			{
				msg.ajoute(FormatMsg(IDS_BSC_FILE_CREATE_FAILED, filepath));
			}
		}

		callback.PopTask();
		return msg;
	}



	ERMsg CZipUnzip::Unzip(CCallback& callback)
	{
		ERMsg msg;

		string filepath = Get(ZIP_FILEPATH);
		string dir = GetDir(DIRECTORY);

		if (FileExists(filepath))
		{
			msg = CreateMultipleDir(dir);
			if (msg)
			{
				string command = GetApplicationPath() + "External\\7za.exe x \"" + filepath + "\" -aoa -o\"" + dir + "\"";

				callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);
				callback.AddMessage(GetString(IDS_UNZIP_FILE));

				msg = WinExecWait(command.c_str());

				callback.PopTask();
			}
		}
		else
		{
			msg.ajoute(FormatMsg(IDS_BSC_FILE_NOT_EXIST, filepath));
		}



		return msg;
	}



}