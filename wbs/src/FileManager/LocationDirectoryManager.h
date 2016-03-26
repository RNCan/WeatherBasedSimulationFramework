//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/Location.h"
#include "FileManager/DirectoryManager.h"


namespace WBSF
{

	class CLocationDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;

		CLocationDirectoryManager(const std::string& projectPath = "");
		CLocationDirectoryManager(const CLocationDirectoryManager& DM);
		virtual ~CLocationDirectoryManager(void);

		//virtual StringVector GetFilesList()const;
		ERMsg Get(const std::string& fileName, CLocationVector& locArray)const;
		ERMsg Set(const std::string& fileName, CLocationVector& locArray)const;


		CLocationDirectoryManager& operator=(const CLocationDirectoryManager& NDM);

		void ConvertLoc2CSV()const;


	};

}