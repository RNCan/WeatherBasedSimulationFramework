//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 15-03-2024	Rémi Saint-Amant	Creation
//******************************************************************************

#include "stdafx.h"
#include "Basic/GoogleDrive.h"
#include "Basic/CallcURL.h"
#include "Basic/Callback.h"

#include <chrono>
#include <ctime>    

using namespace std;

namespace WBSF
{

	string CGoogleDrive::GetPartID(string name)
	{
		StringVector parts(name, ":");
		ASSERT(parts.size() == 2);
		return parts[0];
	}

	string CGoogleDrive::GetPartName(string name)
	{
		StringVector parts(name, ":");
		ASSERT(parts.size() == 2);
		return parts[1];
	}

	string CGoogleDrive::GetURLFromFileID(string id)
	{
		return "https://drive.google.com/file/d/" + id + "/view?usp=drive_link";
	}

	std::string CGoogleDrive::GetURLFromFolderID(std::string id)
	{
		return "https://drive.google.com/drive/folders/" + id;
	}

	std::string CGoogleDrive::GetFileIDFromURL(std::string URL)
	{
		string file_id;

		bool bGoogleDrive = URL.find("drive.google.com") != string::npos;
		size_t begin = URL.find("/d/");
		size_t end = begin;
		if (bGoogleDrive && begin != string::npos)
		{
			begin += 3;
			end = URL.find('/', begin);
		}


		if (bGoogleDrive && begin != string::npos && end != string::npos)
		{
			file_id = URL.substr(begin, end - begin);
		}

		return file_id;
	}

	static std::string GetFolderIDFromURL(std::string URL)
	{
		string folder_id;

		bool bGoogleDrive = URL.find("drive.google.com") != string::npos;
		if (bGoogleDrive)
			folder_id = GetFileName(URL);

		return folder_id;
	}

	ERMsg CGoogleDrive::GetFolderFileList(const std::string& folder_id, WBSF::CFileInfoVector& fileList)
	{
		ERMsg msg;
		CCallcURL cURL;

		string URL = GetURLFromFolderID(folder_id);
		string argument = "-s -k --ssl-no-revoke \"" + URL + "\"";

		string source;
		msg = cURL.get_text(argument, source);
		if (msg)
			msg = ParseFolderFileList(source, fileList);

		return msg;
	}

	//Parse a HTML page to extract file, last modification and size
	ERMsg CGoogleDrive::ParseFolderFileList(const std::string& source, WBSF::CFileInfoVector& fileList)
	{



		ERMsg msg;
		string::size_type posBegin = source.find("<tr data-selectable");



		while (posBegin != string::npos)
		{
			string::size_type posEnd = posBegin;

			string file_id = FindString(source, "data-id=\"", "\"", posBegin, posEnd);

			string file_name_class = FindString(source, "aria-label=\"", "\"", posBegin, posEnd);
			string file_date_class = FindString(source, "aria-label=\"", "\"", posBegin, posEnd);
			string file_size_class = FindString(source, "aria-label=\"", "\n", posBegin, posEnd);


			string file_name_str = WBSF::ReplaceString(file_name_class, " Compressed archive", "");
			string file_date_str = WBSF::ReplaceString(file_date_class, "Modified ", "");
			string file_size_str = WBSF::ReplaceString(file_size_class, "Size: ", "");

			if (!file_name_str.empty() && !file_date_str.empty() && !file_size_str.empty())
			{
				//StringVector file_typeV(file_type_str, ":");
				//ASSERT(file_typeV.size() == 2);
				///ASSERT(file_typeV[0] == "Compressed archive");

				std::time_t time = 0;
				if (file_date_str.find("a.m.") != string::npos || file_date_str.find("p.m.") != string::npos)
				{
					ReplaceString(file_date_str, u8" a.m.", " AM");//before a.m. isn't not a space but a special UTF8 character
					ReplaceString(file_date_str, u8" p.m.", " PM");//before p.m. isn't not a space but a special UTF8 character

					std::time_t t = std::time(0);   // get time now
					struct std::tm tm = *std::localtime(&t);


					std::istringstream ss(file_date_str);
					ss.imbue(std::locale("en_US.UTF-8"));
					ss >> std::get_time(&tm, "%H:%M %p"); // or just %T in this case
					ASSERT(!ss.fail());

					std::time_t time = mktime(&tm);
				}
				else
				{
					struct std::tm tm = {};
					std::istringstream ss(file_date_str);
					ss.imbue(std::locale("en_US.UTF-8"));
					ss >> std::get_time(&tm, "%b %d, %Y");
					ASSERT(!ss.fail());

					time = mktime(&tm);
				}


				StringVector file_sizeV(Trim(file_size_str), " ");
				ASSERT(file_sizeV.size() == 2);
				double file_size = ToDouble(file_sizeV[0]);


				if (file_sizeV[1].find("bytes") != string::npos)
					file_size *= 1;
				else if (file_sizeV[1].find("KB") != string::npos)
					file_size *= 1000;
				else if (file_sizeV[1].find("MB") != string::npos)
					file_size *= 1000000;
				else if (file_sizeV[1].find("GB") != string::npos)
					file_size *= 1000000000;

				CFileInfo info;
				info.m_filePath = file_id + ":" + Trim(file_name_str);
				info.m_time = time;
				info.m_size = __int64(file_size);


				fileList.push_back(info);
			}



			posBegin = source.find("<tr data-selectable", posEnd);
		}

		return msg;
	}

	//Parse a HTML page to extract file, last modification and size
	std::string CGoogleDrive::GetFileName(const std::string& file_id)
	{
		std::string file_name;

		string URL = GetURLFromFileID(file_id);
		CCallcURL cURL;
		string argument = "-s -k --ssl-no-revoke \"" + URL + "\"";

		string source;
		if (cURL.get_text(argument, source))
		{
			file_name = FindString(source, "<meta itemprop=\"name\" content=\"", "\">");
		}

		return file_name;
	}


	ERMsg CGoogleDrive::DownloadFile(const std::string& file_id, const std::string& file_path_out, bool bShow, CCallback& callback)
	{
		ERMsg msg;

		//string URL = GetURLFromFileID(file_id);
		//bool bGoogleDrive = URL.find("drive.google.com") != string::npos;
		//size_t begin = URL.find("/d/");
		//size_t end = begin;
		/*if (bGoogleDrive && begin != string::npos)
		{
			begin += 3;
			end = URL.find('/', begin);
		}

		if (bGoogleDrive && begin != string::npos && end != string::npos)
		{*/
		//string file_id = URL.substr(begin, end - begin);


		std::string file_name = CGoogleDrive::GetFileName(file_id);
		callback.PushTask("Download Google Drive file: " + file_id + " (" + file_name + ")", NOT_INIT);

		string new_URL = "https://drive.usercontent.google.com/download?id=" + file_id + "&confirm=xxx";

		CCallcURL cURL;
		msg = cURL.copy_file(new_URL, file_path_out, bShow);

		if (msg)
		{
			if (FileExists(file_path_out) && WBSF::GetFileInfo(file_path_out).m_size > 10000)
			{
				string URL = GetURLFromFileID(file_id);
				callback.AddMessage("File successfully downloaded: " + file_path_out);
				callback.AddMessage("From URL: " + URL);
			}
			else
			{
				msg.ajoute("File was not downloaded: " + file_path_out);
			}
		}
		msg += callback.StepIt();
		callback.PopTask();
		//	}
			/*else
			{
				msg.ajoute("The link \"" + URL + "\" not seem to be an Google Drive link.");
				msg.ajoute("URL must be in the form: https://drive.google.com/file/d/<File ID>/view?usp=drive_link");
			}*/

		return msg;
	}


}//namespace WBSF


