//*****************************************************************************
// DegreeDay summation
// 
// Jacques Régnière
// Canadian Forest Service
// 
// Programmer: Rémi Saint-Amant
// 
//*****************************************************************************
//*****************************************************************************
// File: DegreDaysModel.cpp
//
// Class: CDegreeDaysModel
//
// Description: CDegreeDaysModel is a BioSIM's model that computes heating degree day.
//              The model accept any number of years and can return the 
//				hourly, daily, monthly, annual degree day value or the cumulative value.
//
// Input parameters:
//		FirstDay: the first day of summation (1..365)
//		LastDay: the last day of summation (1..366)
//		Threshold: base threshold of the summation.
//		SummationType: can be cumulative or not.
//
//
// Output variable:
//		Degree day summation.
//
//*****************************************************************************
// 19/03/2024	3.2.1	Rémi Saint-Amant    Compile with VS 2022
// 11/04/2018	3.2.0	Rémi Saint-Amant    Compile with VS 2017
// 01/09/2017	3.1.1	Rémi Saint-Amant    Bug correction in monthly and annual when start and end is define
// 20/09/2016	3.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 06/09/2016	3.0.3	Rémi Saint-Amant	Replace DregreDay (hour) by DegreHour
// 21/01/2016	3.0.2	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 08/05/2015			Rémi Saint-Amant	New BioSIM 11 version
// 12/05/2013			Rémi Saint-Amant	Add hourly DD
// 07/04/2013			Rémi Saint-Amant	Add multiple Degree day model
// 28/03/2012			Rémi Saint-Amant    Add allen wave
// 06/06/2011			Rémi Saint-Amant    Update
// 08/02/2008			Rémi Saint-Amant    Creation from old code
//*****************************************************************************

#include <array>
#include "ModelBase/EntryPoint.h"
#include "DegreeDaysModel.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
namespace WBSF
{

	//this line links this model with the entry point of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CDegreeDaysModel::CreateObject);

	 
	static const int ERROR_BEGINNING_DATE = ERROR_USER_BASE + 1;
	static const int ERROR_ENDING_DATE = ERROR_USER_BASE + 2;


	//The constructor of the class
	CDegreeDaysModel::CDegreeDaysModel()
	{
		//NB_INPUT_PARAMETER and VERSION are 2 framework variable
		NB_INPUT_PARAMETER = 8; //set the number of parameters for this model
		VERSION = "3.2.1 (2024)"; //set the version of this model

		//This model has 8 input parameters 
		CMonthDay m_firstDate = CMonthDay(FIRST_MONTH, FIRST_DAY);
		CMonthDay m_lastDate = CMonthDay(LAST_MONTH, LAST_DAY);
		m_summationType = CUMULATIVE;
	}

	//The destructor of the class
	CDegreeDaysModel::~CDegreeDaysModel()
	{
	}

	//**************************
	//this method is called by the framework to load parameters
	ERMsg CDegreeDaysModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_weather.size() > 0);

		ERMsg msg;

		//read the 5 input parameters: must be in the same order than the 
		//model's interface. 

		int c = 0;

		m_DD.m_method = parameters[c++].GetInt();
		m_DD.m_lowerThreshold = parameters[c++].GetReal();
		m_DD.m_upperThreshold = parameters[c++].GetReal();
		m_DD.m_cutoffType = parameters[c++].GetInt();

		m_firstDate = CMonthDay(parameters[c++].GetString());
		m_lastDate = CMonthDay(parameters[c++].GetString());
		m_summationType = parameters[c++].GetInt();
		double notUse = parameters[c++].GetReal();//for reverse model

		//perform verification
		if (!m_firstDate.IsValid())
			return GetErrorMessage(ERROR_BEGINNING_DATE);
		if (!m_lastDate.IsValid())
			return GetErrorMessage(ERROR_ENDING_DATE);



		ASSERT(m_DD.m_cutoffType >= 0 && m_DD.m_cutoffType < CDegreeDays::NB_CUTOFF);
		ASSERT(m_summationType >= 0 && m_summationType < NB_SUMMATION_TYPE);

		return msg;
	}



	void CDegreeDaysModel::ComputeFinal(CTM TM, const CModelStatVector& input, CModelStatVector& output)
	{
		CTPeriod p = input.GetTPeriod();
		CTPeriod pIn = p;
		if (m_firstDate!=CMonthDay(FIRST_MONTH, FIRST_DAY) || m_lastDate != CMonthDay(LAST_MONTH, LAST_DAY))
			pIn = CTPeriod(CTRef(p.Begin().GetYear(), m_firstDate.m_month, m_firstDate.m_day, m_firstDate.m_hour, p.GetTM()), CTRef(p.End().GetYear(), m_lastDate.m_month, m_lastDate.m_day, m_lastDate.m_hour, p.GetTM()), CTPeriod::FOR_EACH_YEAR);

		CTTransformation TT(pIn, TM);
		CTStatMatrix stats(input, TT);
		
		//output.Init(stats.m_period, CDegreeDays::NB_OUTPUT, CBioSIMModelBase::VMISS, CDegreeDays::HEADER);
		output.Init(stats.m_period, CDegreeDays::NB_OUTPUT, CBioSIMModelBase::VMISS, CDegreeDays::HEADER);
	
		int year = stats.m_period.GetFirstYear();
		CStatistic DDsum;

		for (CTRef TRef = stats.m_period.Begin(); TRef <= stats.m_period.End(); TRef++)
		//for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			if (TRef.GetYear() != year)
			{
				year = TRef.GetYear();
				DDsum.Reset();
			}

			if (TRef >= m_firstDate && TRef <= m_lastDate)
			{
				if (m_summationType == CUMULATIVE)
				{
					if (stats(TRef, CDegreeDays::S_DD).IsInit())
						DDsum += stats(TRef, CDegreeDays::S_DD)[SUM];

					if (DDsum.IsInit())
						output[TRef][CDegreeDays::S_DD] = DDsum[SUM];
				}
				else
				{
					if (stats(TRef, CDegreeDays::S_DD).IsInit())
						output[TRef][CDegreeDays::S_DD] = stats(TRef, CDegreeDays::S_DD)[SUM];
				}
			}
		}
	}

	//ERMsg CDegreeDaysModel::OnExecuteHourly()
	//{
	//	ASSERT(m_DD.m_method == 0);

	//	ERMsg msg;

	//	m_firstDate.m_hour = FIRST_HOUR;
	//	m_lastDate.m_hour = LAST_HOUR;

	//	CModelStatVector stats;
	//	//m_DD.m_aType = CDegreeDays::AT_HOURLY;
	//	m_DD.m_method = CDegreeDays::BIOSIM_HOURLY;
	//	m_DD.Execute(m_weather, stats);
	//	ComputeFinal(CTM(CTM::HOURLY), stats, m_output);

	//	return msg;
	//}

	//Call by the framework to implement daily computation
	ERMsg CDegreeDaysModel::OnExecuteDaily()
	{
		ERMsg msg;//define error message result

		CModelStatVector stats; 
		m_DD.Execute(m_weather, stats);
		ComputeFinal(CTM(CTM::DAILY), stats, m_output);

		return msg;
	}


	ERMsg CDegreeDaysModel::OnExecuteMonthly()
	{
		ERMsg msg;

		CModelStatVector stats;
		m_DD.Execute(m_weather, stats);
		ComputeFinal(CTM(CTM::MONTHLY), stats, m_output);

		return msg;
	}

	ERMsg CDegreeDaysModel::OnExecuteAnnual()
	{
		ERMsg msg;


		CModelStatVector stats;
		m_DD.Execute(m_weather, stats);
		ComputeFinal(CTM(CTM::ANNUAL), stats, m_output);
		 

		return msg;
	}


	ERMsg CDegreeDaysModel::GetErrorMessage(int errorID)
	{
		ERMsg msg;

		short language = m_info.m_language;
		if (language == FRENCH)
		{
			switch (errorID)
			{
			case ERROR_BEGINNING_DATE: msg.ajoute("Erreur du modèle : date de début invalide."); break;
			case ERROR_ENDING_DATE: msg.ajoute("Erreur du modèle : date de fin invalide."); break;
			default: msg = CBioSIMModelBase::GetErrorMessage(errorID);
			}
		}
		else if (language == ENGLISH)
		{
			switch (errorID)
			{
			case ERROR_BEGINNING_DATE: msg.ajoute("Model error: beginning date is invalid."); break;
			case ERROR_ENDING_DATE: msg.ajoute("Model error: ending date is invalid."); break;
			default: msg = CBioSIMModelBase::GetErrorMessage(errorID);
			}
		}

		return msg;
	}

	//
	//
	//
	//Daily Average;Daily Average (adjusted);Daily Average (strict);Modified Allen Wave (1976);Hourly (select hourly model)
	//Single Triangle;Double Triangle;Single Sine;Double Sine;BioSIM
	//Horizontal;Intermediate;Vertical
	//
}