//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 19-03-2021	Rémi Saint-Amant	Bug correction in reading buffer
// 01-10-2020	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include "stdafx.h"
#include "Basic/CallcURL.h"
#include "Basic/UtilStd.h"

using namespace std;

namespace WBSF
{

	CCallcURL::CCallcURL(const std::string& exe_filepath, DWORD bufsize)
	{
		
		m_exe_filepath = exe_filepath;
		m_bufsize = bufsize;
		m_timeout = 0;

		if (m_exe_filepath.empty())
			m_exe_filepath = GetApplicationPath() + "External\\curl.exe";
	}

	


	ERMsg CCallcURL::get_text(const std::string& arg, std::string& str_out)
	{
		ERMsg msg;

		str_out.clear();

		string cmdline = "\"" + m_exe_filepath + "\" " + arg;
		msg = CallApp(cmdline, str_out, m_bufsize);

		return msg;
	}


	ERMsg CCallcURL::copy_file(const std::string& URL, const std::string& output_filepath, bool bShowCurl)
	{
		ERMsg msg;


		//string strHeaders = "-H \"Content-Type: application/x-www-form-urlencoded\"";
		string argument = string(bShowCurl ? "" : "-s ") + "-k --ssl-no-revoke ";
		if (m_timeout > 0)
			argument += "--connect-timeout " + to_string(m_timeout) + " ";
		
		argument += "\"" + URL + "\" --output \"" + output_filepath + "\"";
		//--max-time

		string command= "\"" + m_exe_filepath + "\" " + argument;


		DWORD exit_code;
		msg = WinExecWait(command, "", bShowCurl ? SW_SHOW : SW_HIDE, &exit_code);
		if (exit_code != 0 && !FileExists(output_filepath))
		{

			msg.ajoute("Unable to download file:");
			msg.ajoute(output_filepath);
			
		}

		return msg;
	}



	ERMsg CCallcURL::CallApp(const std::string& cmdline, std::string& str, DWORD BUFSIZE)
	{
		ERMsg msg;

		HANDLE g_hChildStd_ERR_Rd = NULL;
		HANDLE g_hChildStd_ERR_Wr = NULL;
		HANDLE g_hChildStd_OUT_Rd = NULL;
		HANDLE g_hChildStd_OUT_Wr = NULL;

		SECURITY_ATTRIBUTES saAttr = { 0 };

		// Set the bInheritHandle flag so pipe handles are inherited. 
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		// Create a pipe for the child process's STDOUT. 

		if (msg && !CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
			msg.ajoute("CreatePipe fail");

		// Ensure the read handle to the pipe for STDOUT is not inherited.

		if (msg && !SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
			msg.ajoute("SetHandleInformation fail");

		// Create a pipe for the child process's STDERR. 

		if (msg && !CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0))
			msg.ajoute("CreatePipe fail");

		// Ensure the write handle to the pipe for STDERR is not inherited. 

		if (msg && !SetHandleInformation(g_hChildStd_ERR_Wr, HANDLE_FLAG_INHERIT, 0))
			msg.ajoute("SetHandleInformation fail");

		if (msg)
		{
			// Create the child process. 
			CreateChildProcess(cmdline, g_hChildStd_ERR_Wr, g_hChildStd_OUT_Wr);

			// Read from pipe that is the standard output for child process. 
			//string error;
			msg = ReadFromPipe(g_hChildStd_OUT_Rd, g_hChildStd_ERR_Rd, str, BUFSIZE);

			//string error;
			//ReadFromPipe(g_hChildStd_ERR_Rd, error);
			//if (!error.empty())
			//{
				//msg.ajoute(error);//humm if warning!!!
			//}

			//close handles
			CloseHandle(g_hChildStd_OUT_Rd);
			CloseHandle(g_hChildStd_ERR_Rd);

		}

		return msg;
	}

	ERMsg CCallcURL::CreateChildProcess(const std::string& cmdLine, HANDLE& g_hChildStd_ERR_Wr, HANDLE& g_hChildStd_OUT_Wr)
		// Create a child process that uses the previously created pipes for STDIN and STDOUT.
	{
		ERMsg msg;


		//TCHAR szCmdline[] = TEXT("\"E:/Project/bin/Releasex64/External/curl.exe\" -s \"https://climat.meteo.gc.ca/historical_data/search_historic_data_stations_f.html?searchType=stnProv&timeframe=1&lstProvince=QC&optLimit=yearRange&StartYear=2020&EndYear=2020&Year=2020&Month=9&Day=29&selRowPerPage=10\"");
		PROCESS_INFORMATION piProcInfo;
		STARTUPINFO siStartInfo;
		BOOL bSuccess = FALSE;

		// Set up members of the PROCESS_INFORMATION structure. 

		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		// Set up members of the STARTUPINFO structure. 
		// This structure specifies the STDIN and STDOUT handles for redirection.

		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.hStdError = g_hChildStd_ERR_Wr;
		siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		siStartInfo.wShowWindow = SW_HIDE;

		// Create the child process. 
		typedef std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>, wchar_t> CONVERT_STRING_DEF;
		CONVERT_STRING_DEF CONVERT_STRING;
		std::wstring wcommand = CONVERT_STRING.from_bytes(cmdLine);
		wcommand.resize(wcslen(wcommand.c_str()));

		bSuccess = CreateProcess(NULL,
			&(wcommand[0]),     // command line 
			NULL,          // process security attributes 
			NULL,          // primary thread security attributes 
			TRUE,          // handles are inherited 
			0,             // creation flags 
			NULL,          // use parent's environment 
			NULL,          // use parent's current directory 
			&siStartInfo,  // STARTUPINFO pointer 
			&piProcInfo);  // receives PROCESS_INFORMATION 

		 // If an error occurs, exit the application. 
		if (bSuccess)
		{
			// Close handles to the child process and its primary thread.
			// Some applications might keep these handles to monitor the status
			// of the child process, for example. 

			CloseHandle(piProcInfo.hProcess);
			CloseHandle(piProcInfo.hThread);

			// Close handles to the stdin and stdout pipes no longer needed by the child process.
			// If they are not explicitly closed, there is no way to recognize that the child process has ended.

			CloseHandle(g_hChildStd_OUT_Wr);
			CloseHandle(g_hChildStd_ERR_Wr);
		}
		else
		{
			msg.ajoute("CreateProcess fail with command:");
			msg.ajoute(cmdLine);
		}

		return msg;
	}


	ERMsg CCallcURL::ReadFromPipe(HANDLE& g_hChildStd_OUT_Rd, HANDLE& g_hChildStd_ERR_Rd, std::string& str, DWORD BUFSIZE)
		// Read output from the child process's pipe for STDOUT
		// Stop when clhild process close pipe
	{
		//static const int BUFSIZE = 4096;

		ERMsg msg;

		DWORD dwRead;
		//CHAR chBuf[BUFSIZE] = { 0 };
		std::string chBuf;
		chBuf.resize(BUFSIZE, 0);

		BOOL bSuccess = FALSE;

		for (;;)
		{
			//read error
			ReadFile(g_hChildStd_ERR_Rd, &(chBuf[0]), BUFSIZE, &dwRead, NULL);
			if (dwRead > 0)
			{
				msg.ajoute(chBuf);
				break;
			}

			//read data
			bSuccess = ReadFile(g_hChildStd_OUT_Rd, &(chBuf[0]), BUFSIZE, &dwRead, NULL);

			if (!bSuccess)
				break;

			str.append(chBuf.c_str(), dwRead);
		}

		return msg;
	}

	//void ErrorExit(LPCTSTR lpszFunction)

	//	// Format a readable error message, display a message box, 
	//	// and exit from the application.
	//{
	//	LPVOID lpMsgBuf;
	//	LPVOID lpDisplayBuf;
	//	DWORD dw = GetLastError();

	//	FormatMessage(
	//		FORMAT_MESSAGE_ALLOCATE_BUFFER |
	//		FORMAT_MESSAGE_FROM_SYSTEM |
	//		FORMAT_MESSAGE_IGNORE_INSERTS,
	//		NULL,
	//		dw,
	//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	//		(LPTSTR)&lpMsgBuf,
	//		0, NULL);

	//	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
	//		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	//	StringCchPrintf((LPTSTR)lpDisplayBuf,
	//		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
	//		TEXT("%s failed with error %d: %s"),
	//		lpszFunction, dw, lpMsgBuf);
	//	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	//	LocalFree(lpMsgBuf);
	//	LocalFree(lpDisplayBuf);
	//	ExitProcess(1);
	//}



}//namespace WBSF