//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#ifndef __BASEFILE_H
#define __BASEFILE_H

#pragma once

#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"

namespace WBSF
{

	class CBaseFile
	{
	public:


		CBaseFile(const std::string& path);
		virtual ~CBaseFile();

		virtual bool SetPath(const std::string& path);
		virtual const std::string& GetPath()const{ return m_path; }
		virtual std::string GetFilePath(const std::string& fileName, const std::string& fileExtention)const;

		std::string GetFullPath(const std::string& filename)const{ return m_path + filename; }
		bool FileExists(const std::string& filePath)const{ return WBSF::FileExists(filePath); }

		static StringVector GetFilesList(const std::string& filter, int type = FILE_PATH){ return WBSF::GetFilesList(filter, type); }

	protected:

		std::string m_path;
	};


}
#endif