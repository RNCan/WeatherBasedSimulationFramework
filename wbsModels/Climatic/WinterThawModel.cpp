//*****************************************************************************
// StringFrost
// 
//*****************************************************************************
//*****************************************************************************
// File: WinterThawModel.cpp
//
// Class: CWinterThawModel
//
// Description: CWinterThawModel computes number of thaw (Tmin > 0) and total number of thaw days.
//
// Input parameters:
//		First date: First date of the period (Decemeber first by default)
//		Last Date: last date of the period (Macth 31 by default).
//
// Output variable:
//		number of thaw.
//		total number of thaw days.
//
//*****************************************************************************
// 08/02/2008	1.0.0	Rémi Saint-Amant    Creation from old code
//*****************************************************************************

#include <array>
#include "ModelBase/EntryPoint.h"
#include "WinterThawModel.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
namespace WBSF
{

	//this line links this model with the entry point of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CWinterThawModel::CreateObject);

	 
	enum TOutput {O_NB_THANS, O_NB_THAN_DAYS, NB_OUTPUTS};

	//The constructor of the class
	CWinterThawModel::CWinterThawModel()
	{
		//NB_INPUT_PARAMETER and VERSION are 2 framework variable
		NB_INPUT_PARAMETER = 2; //set the number of parameters for this model
		VERSION = "1.0.0 (2021)"; //set the version of this model
	}

	//The destructor of the class
	CWinterThawModel::~CWinterThawModel()
	{
	}

	//**************************
	//this method is called by the framework to load parameters
	ERMsg CWinterThawModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_weather.size() > 0);

		ERMsg msg;

		int c = 0;

		m_begin.Set(parameters[c++].GetString());
		m_end.Set(parameters[c++].GetString());
		
		if (!m_begin.IsValid())
			msg.ajoute("Invalid begin");

		if (!m_end.IsValid())
			msg.ajoute("Invalid end");


		return msg;
	}



	ERMsg CWinterThawModel::OnExecuteAnnual()
	{
		ERMsg msg;


		CModelStatVector stats;
		m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_OUTPUTS,-9999);
		

		for (size_t y = 1; y < m_weather.GetNbYears(); y++)
		{
			//CTPeriod p = m_weather[y].GetEntireTPeriod();

			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod p(m_begin.GetTRef(year - 1), m_end.GetTRef(year));

			CTRef lastThaw;
			//size_t nbThaws = 0;
			//size_t nbDaysThaw = 0;
			vector<size_t> thaws;
			for (CTRef TRef = p.Begin(); TRef<=p.End(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);

				if (!lastThaw.IsInit() && wDay[H_TMIN][HIGHEST] > 0.0)
				{
					lastThaw = TRef;
				}

				if (wDay[H_TMIN][HIGHEST] <= 0.0 && lastThaw.IsInit())
				{
					thaws.push_back(TRef - lastThaw);
					lastThaw.clear();
				}
			}

			if (lastThaw.IsInit())
			{
				thaws.push_back(p.End() - lastThaw);
				lastThaw.clear();
			}


			m_output[y][O_NB_THANS] = thaws.size();
			m_output[y][O_NB_THAN_DAYS] = std::accumulate(thaws.begin(), thaws.end(),decltype(thaws)::value_type(0));

		}
 

		return msg;
	}

}