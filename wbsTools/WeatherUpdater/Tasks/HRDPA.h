#pragma once


#include "TaskBase.h"
#include "Basic/Callback.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/UtilWWW.h"

namespace WBSF
{
	class CHRDPA 
	{
	public:

		enum TPrcpPeriod { TYPE_06HOURS, TYPE_24HOURS };
		enum TProduct { RDPA, HRDPA, NB_PRODUCTS };
		
		CHRDPA(void);

		ERMsg Execute(CCallback& callback);
		
		bool m_bByHRDPS;
		int m_max_hours;
		std::string m_workingDir;
		TPrcpPeriod m_type;
		TProduct m_product;

		//proptree param
		std::string GetOutputFilePath(const std::string& fileName)const;
		bool NeedDownload(const CFileInfo& info, const std::string& filePath)const;

		static const UINT DESCRIPTION_TITLE_ID;
		static const char* SERVER_NAME;
		static const char* SERVER_PATH;


		static CTRef GetTRef(std::string file_path);
	};

}