//*********************************************************************
// 21/01/2016	2.4.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 03/02/2012	1.0		Rémi Saint-Amant	Creation
//
// Equation from THE ASCE STANDARDIZED REFERENCE EVAPOTRANSPIRATION EQUATION
//		http://www.kimberly.uidaho.edu/water/asceewri/ascestzdetmain2005.pdf
//*********************************************************************

#include "Basic/WeatherDefine.h"
#include "Basic/DegreeDays.h"
#include "Modelbase/EntryPoint.h"
#include "ASCE-ETsz.h"

using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CASCE_ETszModel::CreateObject);

	enum TOutput { O_ET_SZ, NB_OUTPUT };
	enum TOutputFull { O_Δ = NB_OUTPUT, O_γ, O_RA, O_RSO, O_RS, O_FCF, O_RNS, O_RNL, O_RN, NB_OUTPUT_FULL };



	typedef CModelStatVectorTemplate<NB_OUTPUT> CETStatVector;
	typedef CModelStatVectorTemplate<NB_OUTPUT_FULL> CETFullStatVector;

	CASCE_ETszModel::CASCE_ETszModel()
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER = 1;
		VERSION = "1.2.0 (2016)";

		m_referenceType = CASCE_ETsz::SHORT_REF;
		m_extended = false;
	}

	CASCE_ETszModel::~CASCE_ETszModel()
	{}


	ERMsg CASCE_ETszModel::OnExecuteHourly()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		CTPeriod p = m_weather.GetEntireTPeriod();
		((CETStatVector&)m_output).Init(p);

		CASCE_ETsz ASCE2005(m_referenceType, m_extended);
		
		ASCE2005.Execute(m_weather, m_output);


		return msg;
	}

	ERMsg CASCE_ETszModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Init output 
		CTPeriod p = m_weather.GetEntireTPeriod();
		//((CETStatVector&)m_output).Init(p);

		if (m_weather.IsHourly())
		{
			OnExecuteHourly();
			m_output.Transform(CTM(CTM::DAILY), SUM);
		}
		else
		{
			/*m_weather.m_lat = 50.8;
			m_weather.m_elev = 100;
			CWeatherDay& wDay = m_weather.GetDay(CTRef(2014, 7 - 1, 6 - 1));
			wDay[H_TAIR] = (12.3 + 21.5) / 2;
			wDay[H_TRNG] = 21.5 - 12.3;
			wDay[H_RELH] = 63;
			wDay[H_RELH] += 84;
			wDay[H_ES] = 1000*(CASCE_ETsz::e°(12.3) + CASCE_ETsz::e°(21.5)) / 2;
			wDay[H_EA] = 1408.623802;
			wDay[H_WND2] = CASCE_ETsz::GetWindProfileRelationship(10.0, 10);
			wDay[H_SRAD] = 22.072;*/


			//compute evapotranpiration of reference
			CASCE_ETsz ETsz(m_referenceType, m_extended);
			ETsz.Execute(m_weather, m_output);
		}
	

		return msg;
	}
	
	ERMsg CASCE_ETszModel::OnExecuteMonthly()
	{
		ERMsg msg;

		
		CASCE_ETsz ETsz(m_referenceType, m_extended);

		//CModelStatVector stat;
		ETsz.Execute(m_weather, m_output);

		//CTTransformation TT(stat.GetTPeriod(), CTM(CTM::MONTHLY));
		//ETsz.Transform(TT, stat, m_output);
		m_output.Transform(CTM(CTM::MONTHLY), SUM);


		return msg;
	}

	ERMsg CASCE_ETszModel::OnExecuteAnnual()
	{
		ERMsg msg;

		
		CASCE_ETsz ETsz(m_referenceType, m_extended);

		//CModelStatVector stat;
		ETsz.Execute(m_weather, m_output);

		//CTTransformation TT(stat.GetTPeriod(), CTM(CTM::ANNUAL));
		//ETsz.Transform(TT, stat, m_output);
		m_output.Transform(CTM(CTM::ANNUAL), SUM);

		return msg;
	}



	//this method is call to load your parameter in your variable
	ERMsg CASCE_ETszModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;

		m_referenceType = (size_t)parameters[c++].GetInt();

		if (m_info.m_modelName.find("Ex", 0) != -1)
			m_extended = true;
		

		return msg;
	}

}