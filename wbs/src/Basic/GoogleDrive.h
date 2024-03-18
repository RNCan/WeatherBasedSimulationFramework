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
#include "basic/UtilStd.h"
#include "basic/callback.h"


namespace WBSF
{

	class CGoogleDrive
	{
	public:

		static std::string GetURLFromFileID(std::string file_id);
		static std::string GetURLFromFolderID(std::string folder_id);
		static std::string GetPartName(std::string name);
		static std::string GetPartID(std::string name);
		static std::string GetFileIDFromURL(std::string URL);
		static std::string GetFolderIDFromURL(std::string URL);

		static ERMsg GetFolderFileList(const std::string& folder_id, WBSF::CFileInfoVector& fileList);
		static ERMsg ParseFolderFileList(const std::string& source, WBSF::CFileInfoVector& file);
		static std::string GetFileName(const std::string& file_id);
		static ERMsg DownloadFile(const std::string& file_id, const std::string& file_path_out, bool bShow, CCallback& callback = CCallback::DEFAULT_CALLBACK);
	protected:

	};
	
}
