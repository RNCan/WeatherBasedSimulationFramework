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
// Class: CDegreeHoursModel
//
// Description: CDegreeHoursModel is a BioSIM's model that computes heating degree hours.
//              The model accept any number of years and can return the 
//				hourly, degree hours value or the cumulative value.
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
// 20/09/2016	3.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 06/09/2016	3.0.3	Rémi Saint-Amant	Replace DregreDay (hour) by DegreHour
//*****************************************************************************

#include <array>
#include "ModelBase/EntryPoint.h"
#include "DegreeHoursModel.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
namespace WBSF
{

	//this line links this model with the entry point of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CDegreeHoursModel::CreateObject);

	 
	static const int ERROR_BEGINNING_DATE = ERROR_USER_BASE + 1;
	static const int ERROR_ENDING_DATE = ERROR_USER_BASE + 2;


	//The constructor of the class
	CDegreeHoursModel::CDegreeHoursModel()
	{
		//NB_INPUT_PARAMETER and VERSION are 2 framework variable
		NB_INPUT_PARAMETER = 8; //set the number of parameters for this model
		VERSION = "3.1.0 (2016)"; //set the version of this model

		//This model has 8 input parameters 
		CMonthDay m_firstDate = CMonthDay(FIRST_MONTH, FIRST_DAY);
		CMonthDay m_lastDate = CMonthDay(LAST_MONTH, LAST_DAY);
	}

	//The destructor of the class
	CDegreeHoursModel::~CDegreeHoursModel()
	{
	}


	//**************************
	//this method is called by the framework to load parameters
	ERMsg CDegreeHoursModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_weather.size() > 0);

		ERMsg msg;

		//read the 5 input parameters: must be in the same order than the 
		//model's interface. 

		int c = 0;

		parameters[c++].GetInt();//method
		m_DH.m_lowerThreshold = parameters[c++].GetReal();
		m_DH.m_upperThreshold = parameters[c++].GetReal();
		m_DH.m_cutoffType = parameters[c++].GetInt();

		m_firstDate = CMonthDay(parameters[c++].GetString()); 
		m_lastDate = CMonthDay(parameters[c++].GetString());
		m_DH.m_bCumulative = parameters[c++].GetBool();
		double notUse = parameters[c++].GetReal();//for reverse model

		//perform verification
		if (!m_firstDate.IsValid())
			return GetErrorMessage(ERROR_BEGINNING_DATE);
		if (!m_lastDate.IsValid())
			return GetErrorMessage(ERROR_ENDING_DATE);



		ASSERT(m_DH.m_cutoffType >= 0 && m_DH.m_cutoffType < CDegreeDays::NB_CUTOFF);
		

		return msg;
	}

	ERMsg CDegreeHoursModel::OnExecuteHourly()
	{
		ERMsg msg;

		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();

		 
		CModelStatVector stats;

		CTPeriod p = m_weather.GetEntireTPeriod();
		CTRef begin = m_firstDate.GetTRef(p.Begin().GetYear());
		CTRef end = m_lastDate.GetTRef(p.End().GetYear());

		p = CTPeriod(begin, end);

		m_output.Init(p, CDegreeHours::NB_OUTPUT, 0, CDegreeHours::HEADER);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			double dh = m_DH.GetDH(m_weather.GetHour(TRef));
			m_output[TRef][CDegreeHours::S_DH] = dh;
			if (m_DH.m_bCumulative && TRef != p.Begin())
				m_output[TRef][CDegreeHours::S_DH] += m_output[TRef - 1][CDegreeHours::S_DH];
		}


		return msg;
	}

	//Call by the framework to implement daily computation
	

	

	ERMsg CDegreeHoursModel::GetErrorMessage(int errorID)
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

}