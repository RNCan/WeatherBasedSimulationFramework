//*********************************************************************
// File: MakeBioSIMNormalsModel.cpp
//
// Class: CCreateBioSIMDatabase
//
//************** MODIFICATIONS  LOG ********************
// 08/04/2026   2.3	Rémi Saint-Amant    Update with BioSIM 11. 
// 31/10/2012   2.2	Rémi Saint-Amant    Update with BioSIM 10.2.4
// 01/09/2003   1.0	Rémi Saint-Amant    Creation
//*********************************************************************

#include <fstream>
#include <cstdio>
#include <cmath>
#include <cassert>



#include "ModelBase/EntryPoint.h"
#include "MakeBioSIMNormalsModel.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/DailyDatabase.h"

#include "Basic/AdvancedNormalStation.h"






//using namespace DAILY_DATA;
//using namespace UtilWin;

namespace WBSF
{

	using namespace NORMALS_DATA;

	//**************************
	//put the number of input parameters
	//NB_INPUT_PARAMETER is used to determine if the dll
	//uses the same number of parameters than the model interface

	//static const int IDS_BAD_PARAMETER = ERROR_USER_BASE + 1;

	//**************************
	//these two lines define the main class 
	//create the main object of your class 
	//very important: don't change the name of pGlobalModel
	static const bool bRegistred =
		CModelFactory::RegisterModel(CCreateBioSIMDatabase::CreateObject);


	CNormalsDatabase* m_pNormalDB = NULL;
	CDailyDatabase* m_pDailyDB = NULL;
	//C2DimArray<UtilWin::CStdioFileEx> m_file;
	//UtilWin::CStdioFileEx m_fileElev;


	CCreateBioSIMDatabase::CCreateBioSIMDatabase()
	{
		NB_INPUT_PARAMETER = 2;
		VERSION = "2.3 (2026)";
	}

	CCreateBioSIMDatabase::~CCreateBioSIMDatabase()
	{
		//if( m_pDailyDB )
			//delete m_pDailyDB;

		//m_pDailyDB=NULL;
	}


	//**************************
	//this method is called to load parameters in your variables
	ERMsg CCreateBioSIMDatabase::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;
		m_bDeleteOldDB = parameters[c++].GetBool();
		m_filePath = parameters[c++].GetString();

		if (m_info.m_paramCounter.GetTotal() != 1 ||
			m_info.m_repCounter.GetTotal() != 1)
		{
			msg.ajoute("Number of replications must be 1 to create Normals database. No parameters variation allowed");
		}

		if (m_weather.GetNbYears() < 10)
		{
			msg.ajoute("At least 10 years if data is needed to create a Normals' database");
		}

		return msg;
	}





	//typedef CModelStatVectorTemplate<NORMAL_DATA::NB_FIELDS> CNormalStatVector;
	//**************************
	//This method is called to compute the solution
	ERMsg CCreateBioSIMDatabase::OnExecuteMonthly()
	{
		assert(m_weather.GetNbYears() >= 10);

		ERMsg msg;


		CAdvancedNormalStation normalStation;
		msg = normalStation.FromDaily(m_weather, 2);


		if (msg)
		{
			CTRef start(0, 0, 0, 0, CTM(CTM::MONTHLY, CTM::OVERALL_YEARS));
			m_output.Init(12, start, NORMALS_DATA::NB_FIELDS);
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t j = 0; j < NORMALS_DATA::NB_FIELDS; j++)
					m_output[m][j] = normalStation[m][j];
			}


			if (m_info.m_locCounter.IsFirst() &&
				m_info.m_paramCounter.IsFirst() &&
				m_info.m_repCounter.IsFirst())
			{
				WBSF::CreateMultipleDir(WBSF::GetPath(m_filePath));
				if(GetFileExtension(m_filePath).empty())
					SetFileExtension(m_filePath, ".NormalsDB");

				if (m_bDeleteOldDB)
				{
					CNormalsDatabase::DeleteDatabase(m_filePath);
				}

				m_pNormalDB = new CNormalsDatabase;
				m_pNormalDB->SetPeriod(m_weather.GetFirstYear(), m_weather.GetLastYear());
				msg += m_pNormalDB->Open(m_filePath, CNormalsDatabase::modeEdit);
			}

			if (msg)
			{
				msg += m_pNormalDB->Add(normalStation);
			}


			if (m_info.m_locCounter.IsLast() &&
				m_info.m_paramCounter.IsLast() &&
				m_info.m_repCounter.IsLast())
			{
				m_pNormalDB->Close();
				delete m_pNormalDB;
				m_pNormalDB = NULL;
			}



		}



		return msg;
	}

	ERMsg CCreateBioSIMDatabase::OnExecuteDaily()
	{
		ERMsg msg;

		//	m_CS.Enter();

		if (m_info.m_locCounter.IsFirst() &&
			m_info.m_paramCounter.IsFirst() &&
			m_info.m_repCounter.IsFirst())
		{
			SetFileExtension(m_filePath, ".DailyDB");
			if (m_bDeleteOldDB)
				CDailyDatabase::DeleteDatabase(m_filePath);

			m_pDailyDB = new CDailyDatabase;
			msg += m_pDailyDB->Open(m_filePath, CDailyDatabase::modeEdit);
		}


		if (msg)
		{
			msg += m_pDailyDB->Add(m_weather);
		}


		if (m_info.m_locCounter.IsLast() &&
			m_info.m_paramCounter.IsLast() &&
			m_info.m_repCounter.IsLast())
		{
			m_pDailyDB->Close();
			delete m_pDailyDB;
			m_pDailyDB = NULL;
		}

		//m_CS.Leave();

		CModelStatVectorTemplate<1> junk(1, CTRef(0, 0, 0), -9999);
		SetOutput(junk);

		//m_outputFile << 0<<1<<1<<-9999;
		//m_outputFile.EndLine();

		return msg;
	}



}