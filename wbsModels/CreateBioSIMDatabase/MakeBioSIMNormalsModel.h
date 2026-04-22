//*********************************************************************
// File: MakeBioSIMNormalsModel.h
//
// Class: CCreateBioSIMDatabase
//
// Description: CCreateBioSIMDatabase it's a BioSIM model that compute 
//              seasonality stability of mountain pine beetle
//

#pragma once


#include "string.h"

#include "Basic/WeatherStation.h"
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	//#include "DailyFile.h"
	//#include "2DimArray.h"
	//#include "UtilWin.h"
	//#include <StlLock.h>

	//*******************************************************
	//CCreateBioSIMDatabase

	class CCreateBioSIMDatabase : public CBioSIMModelBase
	{
	public:

		enum TType { NORAMAL, DAILY };

		CCreateBioSIMDatabase();
		virtual ~CCreateBioSIMDatabase();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		//virtual ERMsg GetErrorMessage(int errorID);


		static CBioSIMModelBase* CreateObject() { return new CCreateBioSIMDatabase; }



	private:

		void GetDailyStation(CWeatherStation& station);



		bool m_bDeleteOldDB;
		std::string m_filePath;

	};

}