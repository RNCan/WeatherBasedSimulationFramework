//************** M O D I F I C A T I O N S   L O G ********************
//Creation: 2005 by Jacques Régnière and Rémi Saint-Amant
//Modification: 
// 11/05/2016   Rémi Saint-Amant    New compilation with WBSF
// 27/03/2013   Rémi Saint-Amant    New compilation
// 02/08/2012   Rémi Saint-Amant    Stream compatible
// 26/05/2009   Rémi Saint-Amant    Compile with the new BioSIMModelBase (Compatible with hxGrid)
//**********************************************************************
#include "MPB-CTModel.h"
#include "../MPBColdTolerance.h"


#include <time.h>
#include <stdio.h>
#include <math.h>
#include <crtdbg.h>
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CMPB_CT_model::CreateObject);

	enum TDaily{ D_TMIN, D_TMAX, D_P1, D_P2, D_P3, D_CT, D_LT50, D_P_SURV, D_P_MORT, NB_DAILY_OUTPUT };
	enum TAnnual { A_TMIN, A_P_SURV, NB_ANNUAL_OUTPUT };
	typedef CModelStatVectorTemplate<NB_DAILY_OUTPUT> CDailyOutputVector;
	typedef CModelStatVectorTemplate<NB_ANNUAL_OUTPUT> CAnnualOutputVector;

	//First day of cold-tolerance accumulation: Aug 1. (date, base 0)
	static const int T_0 = 212; //This is t sub 0 in Equation [7]
	static const int T_1 = 364; //This is t sub 1 in Equation [7]


	CMPB_CT_model myModel;
	CBioSIMModelBase* pGlobalModel = &myModel;

	CMPB_CT_model::CMPB_CT_model()
	{
		NB_INPUT_PARAMETER = 1;
		VERSION = "2.3.0 (2016)";

		//	Model parameters
		//m_RhoG=0.311;
		//m_MuG=-5;
		//m_SigmaG=8.716;
		//m_KappaG=-39.3;
		//m_RhoL=0.791;
		//m_MuL=33.9;
		//m_SigmaL=3.251;
		//m_KappaL=-32.7;
		//m_Lambda0=0.254;
		//m_Lambda1=0.764;
		//	Generate phloem temperatures from air min/max
		m_bMicroClimate = true;
	}

	CMPB_CT_model::~CMPB_CT_model()
	{
	}



	//Annual version of the model (outputs on line per year)
	ERMsg CMPB_CT_model::OnExecuteAnnual()
	{
		ERMsg msg;

		CMPBColdTolerance CT;
		CT.m_bMicroClimate = m_bMicroClimate;
		CT.ComputeAnnual(m_weather);

		const CMPBCTResultVector& result = CT.GetResult();
		_ASSERT(result.size() == m_weather.GetNbYears());

		enum TAnnual { A_TMIN, A_P_SURV, NB_ANNUAL_OUTPUT };

		m_output.Init(m_weather.GetNbYears() - 1, CTRef(m_weather[size_t(1)].GetTRef().GetYear()), NB_ANNUAL_OUTPUT);

		for (int y = 1; y < (int)result.size(); y++)
		{
			m_output[y - 1][A_TMIN] = result[y].m_Tmin;
			m_output[y - 1][A_P_SURV] = result[y].m_Psurv * 100;
		}

		//SetOutput(output);

		return msg;
	}


	//Daily version of the model (outputs on line per day)
	ERMsg CMPB_CT_model::OnExecuteDaily()
	{
		ERMsg msg;

		//BUG: les paramètre ne sont pas propager à CT. RSA 29/07/2012
		//input parameter
		CMPBColdTolerance CT;
		CT.m_bMicroClimate = m_bMicroClimate;
		CT.ComputeDaily(m_weather);

		const CMPBCTResultVector& result = CT.GetResult();
		_ASSERTE(result.size() == m_weather.GetNbDays());

		CDailyOutputVector output(m_weather.GetEntireTPeriod(CTM::DAILY));
		for (size_t d = 0; d<result.size(); d++)
		{
			output[d][D_TMIN] = result[d].m_Tmin;
			output[d][D_TMAX] = result[d].m_Tmax;
			output[d][D_P1] = result[d].m_p1>-999 ? result[d].m_p1 * 100 : -999;
			output[d][D_P2] = (result[d].m_p1 > -999 && result[d].m_p3 > -999) ? (1 - (result[d].m_p1 + result[d].m_p3)) * 100 : -999;
			output[d][D_P3] = result[d].m_p3 > -999 ? result[d].m_p3 * 100 : -999;
			output[d][D_CT] = result[d].m_Ct;
			output[d][D_LT50] = result[d].m_LT50;
			output[d][D_P_SURV] = result[d].m_Psurv > -999 ? result[d].m_Psurv * 100 : -999;
			output[d][D_P_MORT] = result[d].m_Pmort > -999 ? result[d].m_Pmort * 100 : -999;
		}

		SetOutput(output);

		return msg;
	}



	//this method is call to load your parameter in your variable
	ERMsg CMPB_CT_model::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//  if( parameters.size() == NB_INPUT_PARAMETER1 ||
		//parameters.size() == NB_INPUT_PARAMETER2)
		//  {
		//transfer your parameters here
		int cur = 0;
		//if( parameters.size() == NB_INPUT_PARAMETER1 )
		//	{
		m_bMicroClimate = parameters[cur++].GetBool();
		//		}
		//else if( parameters.size() == NB_INPUT_PARAMETER2 )
		//{
		//	m_RhoG = parameters[cur++].GetReal();
		//	m_MuG = parameters[cur++].GetReal();
		//	m_SigmaG = parameters[cur++].GetReal();
		//	m_KappaG = parameters[cur++].GetReal();
		//	m_RhoL = parameters[cur++].GetReal();
		//	m_MuL = parameters[cur++].GetReal();
		//	m_SigmaL = parameters[cur++].GetReal();
		//	m_KappaL = parameters[cur++].GetReal();
		//	m_Lambda0 = parameters[cur++].GetReal();
		//	m_Lambda1 = parameters[cur++].GetReal();
		//	m_bbMicroclimate = parameters[cur++].GetBool();
		//}
		//}
		//else
		//{
		//	//the number of input parameters in the model is incorrect
		//	msg = GetErrorMessage( ERROR_BAD_NUMBER_PARAMETER );
		//}

		return msg;
	}
}