//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "FileManager/DirectoryManager.h"

namespace WBSF
{

	class CModelInput;

	class CModelInputDirectoryManager : public CDirectoryManager//Base
	{
	public:

		static const char* SUB_DIR_NAME;

		CModelInputDirectoryManager(const std::string& projectPath = "");
		CModelInputDirectoryManager(const CModelInputDirectoryManager& NDM);
		virtual ~CModelInputDirectoryManager(void);

		//Get file with extension
		ERMsg Get(const std::string& fileName, CModelInput& modelInput)const;
		ERMsg Set(const std::string& fileName, CModelInput& modelInput)const;



	};

}