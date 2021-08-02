//*****************************************************************************
// StringFrost
// 
//*****************************************************************************
//*****************************************************************************
// File: StringFrostModel.cpp
//
// Class: CStringFrostModel
//
// Description: CStringFrostModel computes heating degree day up to a define frost event.
//              The model return the 5 last events/year
//
// Input parameters:
//		DD_type: the type of DD
//		DD_Threshold: base threshold of the summation.
//		event_var: Variable of event [Tmin, Tair, Tmax]
//		event_op: operator: Lower (0), or grather(1)
//		event_threshold: event threshold
//
//
// Output variable:
//		Date of event
//		Degree day summation at events.
//		Temperature of event.
//
//*****************************************************************************
// 31/07/2021	1.0.0	Rémi Saint-Amant    Creation
//*****************************************************************************

#include <array>
#include "ModelBase/EntryPoint.h"
#include "SpringFrostModel.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
namespace WBSF
{

	//this line links this model with the entry point of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CStringFrostModel::CreateObject);

	 
	enum TOutput {O_TREF, O_CDD, O_T, ONE_EVENT_SIZE, NB_EVENTS=5,NB_OUTPUTS= ONE_EVENT_SIZE * NB_EVENTS};

	//The constructor of the class
	CStringFrostModel::CStringFrostModel()
	{
		//NB_INPUT_PARAMETER and VERSION are 2 framework variable
		NB_INPUT_PARAMETER = 5; //set the number of parameters for this model
		VERSION = "1.0.0 (2021)"; //set the version of this model
	}

	//The destructor of the class
	CStringFrostModel::~CStringFrostModel()
	{
	}

	//**************************
	//this method is called by the framework to load parameters
	ERMsg CStringFrostModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_weather.size() > 0);

		ERMsg msg;

		size_t c = 0;

		m_DD.m_method = parameters[c++].GetInt();
		m_DD.m_lowerThreshold = parameters[c++].GetReal();
		m_params.m_var = (WBSF::HOURLY_DATA::TVarH)parameters[c++].GetInt();
		m_params.m_op = parameters[c++].GetInt();
		m_params.m_threshold = parameters[c++].GetReal();
		m_params.m_prec = 1;

		return msg;
	}



	ERMsg CStringFrostModel::OnExecuteAnnual()
	{
		ERMsg msg;


		CModelStatVector stats;
		m_DD.Execute(m_weather, stats);
		m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_OUTPUTS, 0);
		

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod();
			p.End() = CTRef(p.End().GetYear(), JUNE, DAY_30);

			double CDD = 0;
			std::vector<CFrostEventOutput> events;
			for (CTRef TRef = p.Begin(); TRef<=p.End(); TRef++)
			{
				const CDataInterface& data = m_weather.Get(TRef);
				
				if( m_params.is_throw(data) )
					events.push_back(CFrostEventOutput(TRef, CDD, data[m_params.m_var][WBSF::MEAN]));
					
				CDD += stats[TRef][CDegreeDays::S_DD];
			}

			std::vector<CFrostEventOutput>::const_reverse_iterator it = events.crbegin();
			for(size_t i=0; i<NB_EVENTS; i++)
			{
				m_output[y][ONE_EVENT_SIZE*i + O_TREF] = CTRef().GetRef();
				m_output[y][ONE_EVENT_SIZE*i + O_CDD] = -999;
				m_output[y][ONE_EVENT_SIZE*i + O_T] = -999;

				if (it != events.crend())
				{
					m_output[y][ONE_EVENT_SIZE*i + O_TREF] = it->m_TRef.GetRef();
					m_output[y][ONE_EVENT_SIZE*i + O_CDD] = it->m_CDD;
					m_output[y][ONE_EVENT_SIZE*i + O_T] = it->m_T;
					it++;
				}
			}
		}
 

		return msg;
	}

}