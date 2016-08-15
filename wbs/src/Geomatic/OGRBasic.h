//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <vector>
#include <float.h>

#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"
#include "Basic/Statistic.h"
#include "Basic/UtilMath.h"


class OGRDataSource;

namespace WBSF
{


	class CGDALDatasetExVector;

	class COGRDataSource
	{
	public:

		COGRDataSource()
		{
			m_poDS = NULL;
		}

		~COGRDataSource()
		{
			Close();
		}

		ERMsg Open(const char * pszName, int bUpdate = FALSE);
		void Close();
		bool IsOpen()const{ return m_poDS != NULL; }

		OGRDataSource       *m_poDS;

		static bool FileExist(const char * filePathOut);
		
	};

	//*****************************************************************************************************
	//COGRBaseOption : parse base applications option 
	class COGRBaseOption
	{
	public:

		enum { OP_NULL = 0, OP_BASE = 1, OP_EXTENTS = 2, OP_SIZE = 4, OP_BANDS = 8, OP_SEPARATE = 16 };
		enum { BASE_OPTIONS = OP_BASE };

		COGRBaseOption(UINT maskOption = BASE_OPTIONS);
		void Reset();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		virtual std::string GetUsage()const;
		virtual std::string GetHelp()const;
		

		std::string m_format;
		StringVector m_createOptions;
		StringVector m_workOptions;
		StringVector m_filesPath;


		bool m_bMulti;
		bool m_bOverwrite;
		bool m_bQuiet;
		bool m_bNeedHelp;
		bool m_bVersion;
	

		UINT GetMaskOption()const{ return m_maskOption; }
		void SetMaskOption(UINT maskOption){ m_maskOption = maskOption; }

		bool IsUsed(UINT opt)const{ return (m_maskOption&opt) != 0; }


	protected:

		UINT m_maskOption;
	};

}