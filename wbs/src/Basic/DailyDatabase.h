//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/WeatherDatabase.h"

namespace WBSF
{

	class CDailyDatabase : public CDHDatabaseBase
	{
	public:

		static const int   VERSION;
		static const char* XML_FLAG;
		static const char* DATABASE_EXT;
		static const char* OPT_EXT;
		static const char* DATA_EXT;
		static const CTM   DATA_TM;
		virtual int GetVersion()const{ return VERSION; }
		virtual const char* GetXMLFlag()const{ return XML_FLAG; }
		virtual const char* GetDatabaseExtension()const{ return DATABASE_EXT; }
		virtual const char* GetOptimizationExtension()const{ return OPT_EXT; }
		virtual const char* GetDataExtension()const{ return DATA_EXT; }
		virtual const CTM	GetDataTM()const{ return DATA_TM; };


		CDailyDatabase(int cacheSize = 200) : CDHDatabaseBase(cacheSize)
		{}


		static ERMsg v4_to_v3(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback);
		static ERMsg v3_to_v4(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback);

		static int GetVersion(const std::string& filePath);
		static ERMsg CreateDatabase(const std::string& filePath);
		static ERMsg DeleteDatabase(const std::string& filePath, CCallback& callback = CCallback::DEFAULT_CALLBACK);
		static ERMsg RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback = CCallback::DEFAULT_CALLBACK);
		//static ERMsg AppendDatabase(const std::string& inputFilePath1, const std::string& inputFilePath2, CCallback& callback = CCallback::DEFAULT_CALLBACK);

	};



	typedef std::shared_ptr<CDailyDatabase> CDailyDatabasePtr;

}