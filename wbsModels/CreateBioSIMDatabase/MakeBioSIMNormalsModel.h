//*********************************************************************
// File: MakeBioSIMNormalsModel.h
//
// Class: CCreateBioSIMDatabase
//
// Description: CCreateBioSIMDatabase it's a BioSIM model that compute 
//              seasonality stability of mountain pine beetle
//

#pragma once


#include "BioSIMModelBase.h"
#include "StdString.h"
#include "NormalFile.h"
#include "DailyFile.h"
#include "2DimArray.h"
#include "UtilWin.h"
//#include <StlLock.h>

//*******************************************************
//CCreateBioSIMDatabase

class CCreateBioSIMDatabase : public CBioSIMModelBase
{
public:

	enum TType {NORAMAL, DAILY};

    CCreateBioSIMDatabase();
    virtual ~CCreateBioSIMDatabase();

	virtual ERMsg OnExecuteDaily();
    virtual ERMsg OnExecuteMonthly();
	//virtual ERMsg OnExecuteUntemporal();
    virtual ERMsg ProcessParameter(const CParameterVector& parameters);
    virtual ERMsg GetErrorMessage(int errorID);


	static CBioSIMModelBase* CreateObject(){ return new CCreateBioSIMDatabase; }


    
private:

	void GetDailyStation(CDailyStation& station);

	

	bool m_bDeleteOldDB;
	//static bool m_bExportFile;
	CStdString m_filePath;
	


	//CCritSec m_CS;
	
};
