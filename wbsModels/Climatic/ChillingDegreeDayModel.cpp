//*****************************************************************************
// DegreeDay summation
// Rémi Saint-Amant
// 
//*****************************************************************************
//*****************************************************************************
// File: ChillingDegreDaysModel.cpp
//
// Class: CChillingDegreeDaysModel
//
// Description: CChillingDegreeDaysModel is a BioSIM's model that computes cooling degree day.
//
// Input parameters:
//		FirstDay: the first day of summation (1..365)
//		LastDay: the last day of summation (1..366)
//		Threshold: base threshold of the summation.
//
//
// Output variable:
//		Chilling degree day summation.
//
//*****************************************************************************
// 25/10/2022	1.0.0	Rémi Saint-Amant    Creation
//*****************************************************************************

#include <array>
#include "ModelBase/EntryPoint.h"
#include "ChillingDegreeDayModel.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;



namespace WBSF
{

	//this line links this model with the entry point of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CChillingDegreeDaysModel::CreateObject);

	//The constructor of the class
	CChillingDegreeDaysModel::CChillingDegreeDaysModel()
	{
		NB_INPUT_PARAMETER = 3; //set the number of parameters for this model
		VERSION = "1.0.0 (2022)"; //set the version of this model

		//This model has 8 input parameters 
		m_firstDate = CMonthDay(AUGUST, DAY_01);
		m_lastDate = CMonthDay(APRIL, DAY_30);
		m_threshold = 0;
	}

	//The destructor of the class
	CChillingDegreeDaysModel::~CChillingDegreeDaysModel()
	{
	}

	//**************************
	//this method is called by the framework to load parameters
	ERMsg CChillingDegreeDaysModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_weather.size() > 0);

		ERMsg msg;

		int c = 0;
		m_threshold = parameters[c++].GetReal();//for reverse model
		m_firstDate = CMonthDay(parameters[c++].GetString());
		m_lastDate = CMonthDay(parameters[c++].GetString());
		

		
		if (!m_firstDate.IsValid())
			msg.ajoute("Model error: beginning date is invalid."); 
	
		if (!m_lastDate.IsValid())
			msg.ajoute("Model error: ending date is invalid.");
			
		//perform verification
	//	if (m_firstDate.GetTRef(0) < m_lastDate.GetTRef(0))
		//	msg.ajoute("Model error: beginning date must be greater than ending date.");


		return msg;
	}



	//Call by the framework to implement daily computation
	ERMsg CChillingDegreeDaysModel::OnExecuteDaily()
	{
		ERMsg msg;//define error message result

		
		m_output.Init(m_weather.GetEntireTPeriod(CTM::DAILY), 1, -999);

		
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			
			int year = m_weather.GetFirstYear() + int(y);
			
			CTRef begin = m_firstDate.GetTRef(year);
			CTRef end = m_lastDate.GetTRef(year);
			if(end<= begin)
				end = m_lastDate.GetTRef(year+1);


			if (y == m_weather.GetNbYears() - 1)
				end = min( end, CTRef(year, DECEMBER, DAY_31));

			//CTPeriod p(begin, end);

			double FDD = 0;
			for (CTRef TRef = begin; TRef <= end; TRef++)
			{
				double Tair = m_weather.GetDay(TRef)[H_TNTX][MEAN];
				if (Tair < m_threshold)
					FDD += -(Tair - m_threshold);

				m_output[TRef][0] = FDD;
			}
		}

		return msg;
	}


	//ERMsg CChillingDegreeDaysModel::OnExecuteMonthly()
	//{
	//	ERMsg msg;

	//	CModelStatVector stats;
	//	m_DD.Execute(m_weather, stats);
	//	ComputeFinal(CTM(CTM::MONTHLY), stats, m_output);

	//	return msg;
	//}

	//ERMsg CChillingDegreeDaysModel::OnExecuteAnnual()
	//{
	//	ERMsg msg;


	//	CModelStatVector stats;
	//	m_DD.Execute(m_weather, stats);
	//	ComputeFinal(CTM(CTM::ANNUAL), stats, m_output);
	//	 

	//	return msg;
	//}


	/*ERMsg CChillingDegreeDaysModel::GetErrorMessage(int errorID)
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
	}*/

	//
	//
	//
	//Daily Average;Daily Average (adjusted);Daily Average (strict);Modified Allen Wave (1976);Hourly (select hourly model)
	//Single Triangle;Double Triangle;Single Sine;Double Sine;BioSIM
	//Horizontal;Intermediate;Vertical
	//
}