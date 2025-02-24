#include "StdAfx.h"
#include "UISolutionMesonetDaily.h"
#include "TaskFactory.h"
#include "Basic/DailyDatabase.h"
#include "UI/Common/SYShowMessage.h"
#include <boost\filesystem.hpp>
#include "../Resource.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;


namespace WBSF
{

	//*********************************************************************

	const char* CUISolutionMesonetDaily::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "UserName", "Password", "WorkingDir", "FirstYear", "LastYear" };
	const size_t CUISolutionMesonetDaily::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_STRING, T_PASSWORD, T_PATH, T_STRING, T_STRING };
	const UINT CUISolutionMesonetDaily::ATTRIBUTE_TITLE_ID = IDS_UPDATER_SM_DAILY_P;
	const UINT CUISolutionMesonetDaily::DESCRIPTION_TITLE_ID = ID_TASK_SM_DAILY;

	const char* CUISolutionMesonetDaily::CLASS_NAME() { static const char* THE_CLASS_NAME = "SolutionMesonetDaily";  return THE_CLASS_NAME; }
	CTaskBase::TType CUISolutionMesonetDaily::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUISolutionMesonetDaily::CLASS_NAME(), (createF)CUISolutionMesonetDaily::create);
	static size_t OLD_CLASS_ID = CTaskFactory::RegisterTask("MesonetQuebec", (createF)CUISolutionMesonetDaily::create);


	const char* CUISolutionMesonetDaily::SERVER_NAME = "horus.mesonet-quebec.org";
	const char* CUISolutionMesonetDaily::SUB_DIR = "/";


	CUISolutionMesonetDaily::CUISolutionMesonetDaily(void)
	{}


	CUISolutionMesonetDaily::~CUISolutionMesonetDaily(void)
	{}


	std::string CUISolutionMesonetDaily::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Solution-Mesonet\\Daily\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	ERMsg CUISolutionMesonetDaily::Execute(CCallback& callback)
	{
		ERMsg msg;

		//int firstYear = as<int>(FIRST_YEAR);
		//int lastYear = as<int>(LAST_YEAR);
		//size_t nbYears = lastYear - firstYear + 1;
		//callback.PushTask(m_name + " : " + GetString(IDS_UPDATE_FILE), nbYears);
		//callback.AddMessage(GetString(IDS_UPDATE_FILE));

		string workingDir = GetDir(WORKING_DIR);
		CTime today = CTime::GetCurrentTime();

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		//verify that all data file have an entry

		msg = UpdateStationList(callback);
		if (msg)
			msg = LoadStationList(callback);

		if (!msg)
			return msg;

		//open a connection on the server



		CFileInfoVector dirList;
		msg += GetDirList(dirList, callback);


		CFileInfoVector fileList;
		if (msg && !dirList.empty())
			msg += GetFileList(dirList, fileList, callback);

		if (msg && !fileList.empty())
		{
			callback.PushTask("Update Solution-Mesonet files: " + ToString(fileList.size()) + " files", fileList.size());
			callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(fileList.size()));

			//StringVector tmp(IDS_FTP_DIRECTION, "|;");

			//download all files
			size_t nbTry = 0;
			size_t curI = 0;

			while (curI < fileList.size() && msg)
			{
				nbTry++;

				//open a connection on the server
				CInternetSessionPtr pSession;
				CFtpConnectionPtr pConnection;

				msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);
				if (msg)
				{
					try
					{
						while (curI < fileList.size() && msg)
						{
							string filePath = fileList[curI].m_filePath;
							string dirName = GetLastDirName(GetPath(filePath));
							string yearPage = SUB_DIR + dirName;
							string outputPath = workingDir + dirName + "/";
							CreateMultipleDir(outputPath);



							string fileTitle = GetFileTitle(filePath);
							string filePathZip = outputPath + fileTitle + ".zip";
							string filePathWea = outputPath + fileTitle + ".wea";
							string stationPage = yearPage + "/" + fileTitle + ".zip";
							if (m_stations.find(fileTitle) == m_stations.end())
								callback.AddMessage("data file without metadata : " + fileTitle);

							msg = UtilWWW::CopyFile(pConnection, stationPage.c_str(), filePathZip.c_str(), INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true);
							if (msg)
							{
								ASSERT(FileExists(filePathZip));
								string app = GetApplicationPath() + "External\\7za.exe";
								string param = " e \"" + filePathZip + "\" -y -o\"" + outputPath + "\""; ReplaceString(param, "\\", "/");
								string command = app + param;
								msg += WinExecWait(command.c_str());
								msg += RemoveFile(filePathZip);
								if (msg)
								{
									//update time stamp to the zip file
									boost::filesystem::path p(filePathWea);
									if (boost::filesystem::exists(p))
										boost::filesystem::last_write_time(p, fileList[curI].m_time);

									curI++;
									nbTry = 0;
									msg += callback.StepIt();
								}
							}
						}//while
					}
					catch (CException* e)
					{
						//if an error occur: try again
						if (nbTry < 5)
						{
							callback.AddMessage(UtilWin::SYGetMessage(*e));
							msg = WaitServer(5, callback);

						}
						else
						{
							msg = UtilWin::SYGetMessage(*e);
						}
					}

					pConnection->Close();
					pSession->Close();

				}//if msg
			}//while


			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI));
			callback.PopTask();
		}//if list not empty

		return msg;
	}


	ERMsg CUISolutionMesonetDaily::GetDirList(CFileInfoVector & dirList, CCallback& callback)
	{
		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		string workingDir = GetDir(WORKING_DIR);

		bool bEnd = false;
		size_t nbTry = 0;
		while (!bEnd && msg)
		{
			nbTry++;

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);

			if (msg)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 35000);

				try
				{
					CFileInfoVector dirListTmp;
					msg = UtilWWW::FindDirectories(pConnection, SUB_DIR, dirListTmp);
					if (msg)
					{
						ASSERT(dirList.empty());

						bEnd = true;
						for (size_t i = 0; i < dirListTmp.size(); i++)
						{
							string dirName = GetLastDirName(dirListTmp[i].m_filePath);
							int year = ToInt(dirName);
							if (year >= firstYear && year <= lastYear)
								dirList.push_back(dirListTmp[i]);
						}
					}
					//else
					//{
					//	if (nbTry < 5 && !callback.GetUserCancel())
					//	{
					//		callback.AddMessage(msg);
					//		msg = ERMsg();
					//		callback.PushTask("Waiting 2 seconds for server...", 40);
					//		for (size_t i = 0; i < 40 && msg; i++)
					//		{
					//			Sleep(50);//wait 50 milisec
					//			msg += callback.StepIt();
					//		}
					//		callback.PopTask();

					//	}
					//}
				}
				catch (CException* e)
				{
					//if an error occur: try again
					if (nbTry < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));
						msg = WaitServer(5, callback);
					}
					else
					{
						msg = UtilWin::SYGetMessage(*e);
					}
				}
			}//if connect

			pConnection->Close();
			pSession->Close();

		}//try many time

		return msg;
	}

	ERMsg CUISolutionMesonetDaily::GetFileList(const CFileInfoVector & dirList, CFileInfoVector & fileList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		callback.PushTask("Get file list: " + ToString(dirList.size()) + " years", dirList.size());
		//callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(fileList.size()));


		size_t nbFound = 0;
		size_t nbTry = 0;
		size_t curI = 0;
		while (curI < dirList.size() && msg)
		{
			nbTry++;

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);

			if (msg)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 35000);

				try
				{
					while (curI < dirList.size() && msg)
					{
						string dirName = GetLastDirName(dirList[curI].m_filePath);
						string yearPage = SUB_DIR + dirName;
						string outputPath = workingDir + dirName + "/";

						//Load files list

						CFileInfoVector fileListTmp;
						msg += UtilWWW::FindFiles(pConnection, (yearPage + "/*.zip"), fileListTmp, true, callback);
						if (msg)
						{
							//add file to update
							for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
							{
								string fileTitle = GetFileTitle(it->m_filePath);
								string filePathZip = outputPath + fileTitle + ".zip";
								string filePathWea = outputPath + fileTitle + ".wea";

								if (!UtilWWW::IsFileUpToDate(*it, filePathWea, false))
									fileList.push_back(*it);
							}

							curI++;
							nbTry = 0;
							nbFound += fileListTmp.size();
							msg += callback.StepIt();
						}
						//else
						//{
						//	if (nbTry < 5 && !callback.GetUserCancel())
						//	{
						//		callback.AddMessage(msg);
						//		msg = ERMsg();
						//		callback.PushTask("Waiting 2 seconds for server...", 40);
						//		for (size_t i = 0; i < 40 && msg; i++)
						//		{
						//			Sleep(50);//wait 50 milisec
						//			msg += callback.StepIt();
						//		}
						//		callback.PopTask();

						//	}
						//}

					}//for all directories
				}
				catch (CException* e)
				{
					//if an error occur: try again
					if (nbTry < 5)
					{
						callback.AddMessage(UtilWin::SYGetMessage(*e));
						msg = WaitServer(5, callback);
					}
					else
					{
						msg = UtilWin::SYGetMessage(*e);
					}
				}
			}//try many time

			pConnection->Close();
			pSession->Close();
		}//if msg


		callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(nbFound));
		callback.PopTask();

		return msg;
	}



	string CUISolutionMesonetDaily::GetStationListFilePath()const
	{
		return GetDir(WORKING_DIR) + "Stations.xml";
	}

	ERMsg CUISolutionMesonetDaily::UpdateStationList(CCallback& callback)const
	{
		ERMsg msg;


		bool bEnd = false;
		size_t nbTry = 0;
		while (!bEnd && msg)
		{
			nbTry++;

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, Get(USER_NAME), Get(PASSWORD), true, 5, callback);

			if (msg)
			{
				string FTPFilePath = string(SUB_DIR) + "stations.xml";
				string localFilePath = GetStationListFilePath();

				if (UtilWWW::IsFileUpToDate(pConnection, FTPFilePath.c_str(), localFilePath.c_str()), true, true)
				{
					bEnd = true;
				}
				else
				{
					try
					{
						CreateMultipleDir(GetPath(localFilePath));
						msg = UtilWWW::CopyFile(pConnection, FTPFilePath.c_str(), localFilePath.c_str(), INTERNET_FLAG_EXISTING_CONNECT, true);
						if(msg)
							callback.AddMessage("Stations list updated.");

						bEnd = true;

					}
					catch (CException* e)
					{
						if (nbTry < 5)
						{
							callback.AddMessage(UtilWin::SYGetMessage(*e));
							msg = WaitServer(5, callback);
						}
						else
						{
							msg = UtilWin::SYGetMessage(*e);
						}
					
					//if (msg)
					//{
						//update time stamp to the zip file

						/*boost::filesystem::path p(localFilePath);
						ASSERT(boost::filesystem::exists(p));
						boost::filesystem::last_write_time(p, FTPFilePath.m_time);
*/
						//callback.AddMessage("Stations list updated.");
						//bEnd = true;
					//}
					//else
					//{
						//if (nbTry < 5 && !callback.GetUserCancel())
						//{
						//	callback.AddMessage(msg);
						//	msg = ERMsg();
						//	WaitServer(5, callback);
						//	//callback.PushTask("Waiting 2 seconds for server...", 40);
						//	//for (size_t i = 0; i < 40 && msg; i++)
						//	//{
						//	//	Sleep(50);//wait 50 milisec
						//	//	msg += callback.StepIt();
						//	//}
						//	//callback.PopTask();

						//}
					}
				}

				//if (msg)
				//{
				//	msg = LoadStationList(callback);
				//	CLocationVector stations;
				//	stations.reserve(m_stations.size());
				//	for (auto it = m_stations.begin(); it != m_stations.end(); it++)
				//		stations.push_back(it->second);

				//	WBSF::SetFileExtension(localFilePath, ".csv");
				//	msg = stations.Save(localFilePath);
				//}

				pSession->Close();
				pConnection->Close();
			}
		}

		return msg;
	}

	ERMsg CUISolutionMesonetDaily::LoadStationList(CCallback& callback)
	{
		ERMsg msg;

		m_stations.clear();
		string filePath = GetStationListFilePath();


		ifStream file;
		file.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<char>()));
		msg = file.open(filePath);
		if (msg)
		{
			try
			{
				string str = file.GetText();
				std::replace(str.begin(), str.end(), '\'', '�');

				zen::XmlDoc doc = zen::parse(str);

				zen::XmlIn in(doc.root());
				for (zen::XmlIn child = in["station"]; child; child.next())
				{
					CLocation tmp;
					child.attribute("fullname", tmp.m_name);
					std::replace(tmp.m_name.begin(), tmp.m_name.end(), '�', '\'');
					//remove coma in name
					tmp.m_name.erase(std::remove(tmp.m_name.begin(), tmp.m_name.end(), ','), tmp.m_name.end());
					tmp.m_name.erase(std::remove(tmp.m_name.begin(), tmp.m_name.end(), '.'), tmp.m_name.end());
					tmp.m_name.erase(std::remove(tmp.m_name.begin(), tmp.m_name.end(), ';'), tmp.m_name.end());
					child.attribute("name", tmp.m_ID);
					child.attribute("lat", tmp.m_lat);
					child.attribute("lon", tmp.m_lon);
					child.attribute("elev", tmp.m_elev);
					m_stations[tmp.m_ID] = tmp;
				}

			}
			catch (const zen::XmlFileError& e)
			{
				// handle error
				msg.ajoute(GetErrorDescription(e.lastError));
			}
			catch (const zen::XmlParsingError& e)
			{
				// handle error
				msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
			}
		}

		//add other stations for historical load
		string otherStations = GetApplicationPath() + "Layers\\QuebecStations.csv";
		CLocationVector stations;
		msg = stations.Load(otherStations);
		for (auto it = stations.begin(); it != stations.end(); it++)
		{
			it->m_ID = WBSF::MakeLower(it->m_ID);
			if (m_stations.find(it->m_ID) != m_stations.end())
			{
				m_stations[it->m_ID].SetSSI("Network", it->GetSSI("Network"));
				m_stations[it->m_ID].SetSSI( "Country", it->GetSSI("Country") );
				m_stations[it->m_ID].SetSSI("SubDivision", it->GetSSI("SubDivision"));
			}
			else
			{
				m_stations[it->m_ID] = *it;
			}
				
		}

		return msg;
	}

	ERMsg CUISolutionMesonetDaily::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;
		msg = LoadStationList(callback);

		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		//find all station in the directories
		for (CLocationMap::const_iterator it = m_stations.begin(); it != m_stations.end() && msg; it++)
		{
			bool bAdd = false;
			for (size_t y = 0; y < nbYears && !bAdd&&msg; y++)
			{
				int year = firstYear + int(y);

				string filePath = workingDir + ToString(year) + "\\" + it->first + ".wea";
				if (FileExists(filePath))
					bAdd = true;

				msg += callback.StepIt(0);
			}

			if (bAdd)
				stationList.push_back(it->first.c_str());
		}

		return msg;
	}

	ERMsg CUISolutionMesonetDaily::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;


		((CLocation&)station) = m_stations[ID];
		Trim(station.m_name);

		bool bFirst = true;
		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = workingDir + ToString(year) + "\\" + ID + ".wea";
			if (FileExists(filePath))
				msg += station.LoadData(filePath, -996, false);

			msg += callback.StepIt(0);
		}//for all year


		if (msg)
		{
			//verify station is valid
			if (station.HaveData())
				msg = station.IsValid();
		}

		station.SetSSI("Provider", "Solution-Mesonet");


		return msg;
	}



}