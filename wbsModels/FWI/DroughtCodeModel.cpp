//*********************************************************************
//29/03/2017	2.1.1	Rémi Saint-Amant    Bug correction in Drought Code
//20/09/2016	2.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
//08/02/2016	2.0.0   Rémi Saint-Amant	Include in WBSF
//08/02/2012	1.1     Rémi Saint-Amant	Recompile with new interface and new FWI model
//17/05/2011	1.0		Rémi Saint-Amant	Creation
//*********************************************************************
#include "DroughtCodeModel.h"
#include "FWI.h"
#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/GrowingSeason.h"

using namespace WBSF::HOURLY_DATA; 
using namespace std;

namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CDroughtCode::CreateObject);

	typedef CModelStatVectorTemplate<1> CDCStatVector;

	CDroughtCode::CDroughtCode()
	{
		// initialise your variable here (optionnal)
		
		VERSION = "2.1.1 (2017)";

		m_bAutoSelect = true;
		m_FFMC = 85.0;
		m_DMC = 6.0;
		m_DC = 15.0;
		

		m_carryOverFraction = 1;
		m_effectivenessOfWinterPrcp = 0.75;

		m_nbDaysStart = 3;
		m_TtypeStart = CGSInfo::TT_TNOON;
		m_thresholdStart = 12;
		m_nbDaysEnd = 3;
		m_TtypeEnd = CGSInfo::TT_TMAX;
		m_thresholdEnd = 5;
		m_carryOverFraction = 1;
		m_effectivenessOfWinterPrcp = 0.75;
		m_method = CFWI::NOON_CALCULATION;
	}

	CDroughtCode::~CDroughtCode()
	{}

	//this method is call to load your parameter in your variable
	ERMsg CDroughtCode::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		if (parameters.size() == 8)
		{
			m_bAutoSelect = true;

			short c = 0;

			m_nbDaysStart = parameters[c++].GetInt();
			m_TtypeStart = parameters[c++].GetInt();
			m_thresholdStart = parameters[c++].GetReal();
			m_nbDaysEnd = parameters[c++].GetInt();
			m_TtypeEnd = parameters[c++].GetInt();
			m_thresholdEnd = parameters[c++].GetReal();
			m_carryOverFraction = parameters[c++].GetReal();
			m_effectivenessOfWinterPrcp = parameters[c++].GetReal();
			//m_method = parameters[c++].GetInt();
		}
		else if (parameters.size() == 7)
		{
			m_bAutoSelect = false;

			short c = 0;
			m_firstDay = parameters[c++].GetString();
			m_lastDay = parameters[c++].GetString();
			m_FFMC = parameters[c++].GetReal();
			m_DMC = parameters[c++].GetReal();
			m_DC = parameters[c++].GetReal();
			m_carryOverFraction = parameters[c++].GetReal();
			m_effectivenessOfWinterPrcp = parameters[c++].GetReal();
		}
		else
		{
			msg = GetErrorMessage(ERROR_BAD_NUMBER_PARAMETER);
		}

		return msg;
	}

	ERMsg CDroughtCode::ExecuteDaily(CModelStatVector& output)
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//manual setting 
		CFWI FWI;
		FWI.m_method = (CFWI::TMethod)m_method;
		FWI.m_firstDay = m_firstDay;
		FWI.m_lastDay = m_lastDay;
		FWI.m_FFMC = m_FFMC;
		FWI.m_DMC = m_DMC;
		FWI.m_DC = m_DC;
		//automatic setting
		FWI.m_bAutoSelect = m_bAutoSelect;
		FWI.m_nbDaysStart = m_nbDaysStart;
		FWI.m_TtypeStart = m_TtypeStart;
		FWI.m_thresholdStart = m_thresholdStart;
		FWI.m_nbDaysEnd = m_nbDaysEnd;
		FWI.m_TtypeEnd = m_TtypeEnd;
		FWI.m_thresholdEnd = m_thresholdEnd;
		//common setting
		FWI.m_carryOverFraction = m_carryOverFraction;
		FWI.m_effectivenessOfWinterPrcp = m_effectivenessOfWinterPrcp;


		FWI.Execute(m_weather, output);

		return msg;
	}

	//This method is call to compute solution
	ERMsg CDroughtCode::OnExecuteDaily()
	{
		ERMsg msg;

		
		//Compute FWI
		CModelStatVector output;
		ExecuteDaily(output);
		
		//Create output from result
		m_output.Init(output.GetTPeriod(), 1, -9999);
		
		for (CTRef d = output.GetFirstTRef(); d <= output.GetLastTRef(); d++)
			m_output[d][0] = output[d][CFWIStat::DC];

		
		return msg;
	}

	ERMsg CDroughtCode::OnExecuteMonthly()
	{
		ERMsg msg;

		CFWIDStatVector resultD;
		ExecuteDaily(resultD);

		CFWIMStatVector resultM;
		CFWIStat::Covert2M(resultD, resultM);

		m_output.Init(resultM.GetTPeriod(), 1, -9999);

		for (size_t mm = 0; mm < resultM.size(); mm++)
			m_output[mm][0] = resultM[mm][CFWIStat::DC];

		


		return msg;
	}

	ERMsg CDroughtCode::OnExecuteAnnual()
	{
		ERMsg msg;

		CFWIDStatVector resultD;
		ExecuteDaily(resultD);


		CFWIAStatVector resultA;
		CFWIStat::Covert2A(resultD, resultA);

		m_output.Init(resultA.GetTPeriod(), 1, -9999);

		for (size_t y = 0; y < resultA.size(); y++)
			m_output[y][0] = resultA[y][CFWIStat::DC];

		

		return msg;
	}


	//double CDroughtCode::ComputeIndice(int year, size_t m, double& DCMo, double Rm, double Tm)
	//{
	//	double Lf[12] = { -1.6, -1.6, -1.6, 0.9, 3.8, 5.8, 6.4, 5.0, 2.4, 0.4, -1.6, -1.6 };

	//	if (Tm < -2.8)
	//		Tm = -2.8;


	//	size_t N = GetNbDayPerMonth(year, m);
	//	double Vm = max(0.0, N*(0.36*Tm + Lf[m])) / 2;

	//	double DChalf = DCMo + 0.5*Vm; // add in only half of drying before the rain influence

	//	double MDR = DChalf;
	//	if (Rm > 2.8)
	//	{
	//		double RMeff = 0.83*Rm;
	//		double smi = 800.0*exp(-1.0*DCMo / 400);
	//		MDR = DChalf - 400.0*log(1.0 + 3.937*RMeff / smi);

	//		if (MDR < 0)
	//			MDR = 0;
	//	}

	//	double MDC = MDR + 0.5*Vm;   // this adds the other half of the drying influence

	//	double MDCavg = (DCMo + MDC) / 2;

	//	DCMo = MDC;

	//	return MDCavg;
	//}



}