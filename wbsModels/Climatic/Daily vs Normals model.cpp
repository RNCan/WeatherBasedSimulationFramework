//*****************************************************************************
// TemperatureIndex
// 
//*****************************************************************************
//*****************************************************************************
// File: TemperatureIndexModel.cpp
//
// Class: CTemperatureIndexModel
//
// Description: CTemperatureIndexModel computes the ratio between the daily value and daily normals.
//
// Input parameters:
//
// Output variable:
//		Temperature Index.
//
//*****************************************************************************
// 31/07/2021	1.0.0	Rémi Saint-Amant    Creation
//*****************************************************************************

#include <array>
#include "ModelBase/EntryPoint.h"
#include "Basic/NormalsDatabase.h"
#include "Simulation/WeatherGeneration.h"
#include "Daily vs Normals model.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
namespace WBSF
{

	//this line links this model with the entry point of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CTemperatureIndexModel::CreateObject);

	 
	enum TOutput {O_TMIN, O_TMIN_NORMALS, O_TMIN_NORMALS_SD, O_TMIN_SCORE, O_TMAX, O_TMAX_NORMALS, O_TMAX_NORMALS_SD, O_TMAX_SCORE,NB_OUTPUTS};

	//The constructor of the class
	CTemperatureIndexModel::CTemperatureIndexModel()
	{
		//NB_INPUT_PARAMETER and VERSION are 2 framework variable
		NB_INPUT_PARAMETER = 0; //set the number of parameters for this model
		VERSION = "1.0.0 (2021)"; //set the version of this model
	}

	//The destructor of the class
	CTemperatureIndexModel::~CTemperatureIndexModel()
	{
	}

	//**************************
	//this method is called by the framework to load parameters
	ERMsg CTemperatureIndexModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_weather.size() > 0);

		ERMsg msg;

		size_t c = 0;

		return msg;
	}

	CModelStatVector CTemperatureIndexModel::GenerateNormals()
	{
		ERMsg msg;
		CCallback callback;
		CModelStatVector output(366, CTRef(0,0,0), 4);

		//open normal database
		CNormalsDatabasePtr pNormalDB;
		
		pNormalDB.reset(new CNormalsDatabase);
		msg = pNormalDB->Open(m_info.m_normalDBFilePath, CNormalsDatabase::modeRead, callback, true);
		if (msg)
			msg = pNormalDB->OpenSearchOptimization(callback);//open here to be thread safe
		

		CWGInput WGInput = m_info.m_WG;
		CWeatherGenerator WG;
		WG.SetWGInput(m_info.m_WG);
		WG.SetNbReplications(1);//no replication in daily db
		WG.SetTarget(m_info.m_loc);
		WG.SetNormalDB(pNormalDB);
	
		CNormalsStation normals;
		msg = WG.GetNormals(normals, callback);//create data
		normals.AdjustMonthlyMeans();
		
		for (size_t d = 0; d < 366; d++)
		{
			output[d][0] = normals.Interpole(d, NORMALS_DATA::TMIN_MN);
			output[d][1] = normals.Interpole(d, NORMALS_DATA::TMIN_SD);
			output[d][2] = normals.Interpole(d, NORMALS_DATA::TMAX_MN);
			output[d][3] = normals.Interpole(d, NORMALS_DATA::TMAX_SD);
		}
		
		return output;
	}

	ERMsg CTemperatureIndexModel::OnExecuteDaily()
	{
		ERMsg msg;


		CModelStatVector Normals = GenerateNormals();
		m_output.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_OUTPUTS, 0);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM::DAILY);
			for (CTRef TRef = p.Begin(); TRef<=p.End(); TRef++)
			{
				const CWeatherDay& data = m_weather.GetDay(TRef);

				m_output[TRef][O_TMIN] = data[H_TMIN][MEAN];
				m_output[TRef][O_TMIN_NORMALS] = Normals[TRef.GetJDay()][0];
				m_output[TRef][O_TMIN_NORMALS_SD] = Normals[TRef.GetJDay()][1];
				m_output[TRef][O_TMIN_SCORE] = (data[H_TMIN][MEAN] - Normals[TRef.GetJDay()][0]) / Normals[TRef.GetJDay()][1];
				m_output[TRef][O_TMAX] = data[H_TMAX][MEAN];
				m_output[TRef][O_TMAX_NORMALS] = Normals[TRef.GetJDay()][2];
				m_output[TRef][O_TMAX_NORMALS_SD] = Normals[TRef.GetJDay()][3];
				m_output[TRef][O_TMAX_SCORE] = (data[H_TMAX][MEAN] - Normals[TRef.GetJDay()][2]) / Normals[TRef.GetJDay()][3];
			}
		}
  

		return msg;
	}



}