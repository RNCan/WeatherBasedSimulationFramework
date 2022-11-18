//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include <string>
#include "basic/ERMsg.h"


namespace WBSF
{

	class CCallcURL
	{
	public:

		CCallcURL(const std::string& exe_filepath="", DWORD bufsize = 4096);
		

		ERMsg get_text(const std::string& arg, std::string& str_out);
		ERMsg copy_file(const std::string& URL, std::string& outputFilePath, bool bShowCurl=false);
		static ERMsg CallApp(const std::string& cmdline, std::string& str, DWORD BUFSIZE = 4096);


		
		size_t m_timeout;
		DWORD m_bufsize;

	protected:

		
		static ERMsg CreateChildProcess(const std::string& cmdLine, HANDLE& g_hChildStd_ERR_Wr, HANDLE& g_hChildStd_OUT_Wr);
		static ERMsg ReadFromPipe(HANDLE& g_hChildStd_OUT_Rd, HANDLE& g_hChildStd_ERR_Rd, std::string& str, DWORD BUFSIZE);

		
		std::string m_exe_filepath;
	};
	
}
