//*********************************************************************
//12/02/2016	v3.0.0	R�mi Saint-Amant	Using WBSF. Hourly model.
//06/02/2012	v2.3	R�mi Saint-Amant	Correction of a bug in strating date
//18/05/2011    v2.2    R�mi Saint-Amant	Use of new FWI class
//11/03/2009	v1.1	R�mi Saint-Amant	Change input order and add startThreshold 
//05/03/2009	v1.0	R�mi Saint-Amant	Compile with new FWI kernel
//27/02/2009	----	R�mi Saint-Amant	Include new variable for montly model
//25/11/2008	----	R�mi Saint-Amant	Add Annual model
//12/12/2007	----	R�mi Saint-Amant	Creation
//*********************************************************************

#include "Basic/WeatherDefine.h"
#include "Basic/GrowingSeason.h"
#include "Basic/SnowAnalysis.h"
#include "ModelBase/EntryPoint.h"
#include "FWI.h"
#include "FWI-Model.h"


using namespace std;
using namespace WBSF::HOURLY_DATA; 


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CFWIModel::CreateObject);


	CFWIModel::CFWIModel() 
	{
		// initialise your variable here (optionnal)
		//	NB_INPUT_PARAMETER=8;//7 or 8 param
		VERSION = "3.0.0 (2016)";

		m_bAutoSelect = true;
		m_firstDay = NOT_INIT;
		m_lastDay = NOT_INIT;
		m_FFMC = 85.0;
		m_DMC = 6.0;
		m_DC = 15.0;
		m_nbDaysStart = 3;
		m_TtypeStart = CGSInfo::TT_TNOON;
		m_thresholdStart = 12;
		m_nbDaysEnd = 3;
		m_TtypeEnd = CGSInfo::TT_TMAX;
		m_thresholdEnd = 5;
		m_carryOverFraction = 1;
		m_effectivenessOfWinterPrcp = 0.75;
	}

	CFWIModel::~CFWIModel()
	{}

	//This method is call to compute solution
	ERMsg CFWIModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Init class member
		CFWI FWI;
		msg = ExecuteDaily(FWI, m_output);

		return msg;
	}

	ERMsg CFWIModel::ExecuteDaily(CFWI& FWI, CModelStatVector& output)
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();


		FWI.m_bAutoSelect = m_bAutoSelect;
		FWI.m_nbDaysStart = m_nbDaysStart;
		FWI.m_TtypeStart = m_TtypeStart;
		FWI.m_thresholdStart = m_thresholdStart;
		FWI.m_nbDaysEnd = m_nbDaysEnd;
		FWI.m_TtypeEnd = m_TtypeEnd;
		FWI.m_thresholdEnd = m_thresholdEnd;
		//manual setting 
		FWI.m_firstDay = m_firstDay;
		FWI.m_lastDay = m_lastDay;
		FWI.m_FFMC = m_FFMC;
		FWI.m_DMC = m_DMC;
		FWI.m_DC = m_DC;

		//common setting
		FWI.m_carryOverFraction = m_carryOverFraction;
		FWI.m_effectivenessOfWinterPrcp = m_effectivenessOfWinterPrcp;
	
		FWI.Execute(m_weather, output);


		return msg;
	}

	ERMsg CFWIModel::OnExecuteMonthly()
	{
		ERMsg msg;


		//Init class member
		CFWI FWI;
		CFWIMStatVector resultD;
		ExecuteDaily(FWI, resultD);


		CFWIMStatVector resultM;
		CFWIStat::CovertD2M(resultD, resultM);

		SetOutput(resultM);


		return msg;
	}

	ERMsg CFWIModel::OnExecuteAnnual()
	{
		ERMsg msg;

		//Init class member
		CFWI FWI;
		CSnowAnalysis snow;

		CFWIDStatVector resultD;
		ExecuteDaily(FWI, resultD);

		CFWIAStatVector resultA;
		CFWIStat::CovertD2A(resultD, resultA);

		for (size_t y = 0; y < resultA.size(); y++)
		{
			resultA[y][CFWIStat::SNOW_MELT] = snow.GetLastSnowTRef(m_weather[y]).GetJDay() + 1;
			CTRef Tref = snow.GetFirstSnowTRef(m_weather[y]);
			resultA[y][CFWIStat::SNOW_FALL] = Tref.IsInit() ? Tref.GetJDay() + 1 : 367;
		}


		SetOutput(resultA);

		return msg;
	}


	double CFWIModel::ComputeIndice(int year, int m, double& DCMo, double Rm, double Tm)
	{
		double Lf[12] = { -1.6, -1.6, -1.6, 0.9, 3.8, 5.8, 6.4, 5.0, 2.4, 0.4, -1.6, -1.6 };

		if (Tm < -2.8)
			Tm = -2.8;


		int N = GetNbDayPerMonth(year, m);
		double Vm = max(0.0, N*(0.36*Tm + Lf[m])) / 2.0;

		double DChalf = DCMo + 0.5*Vm; // add in only half of drying before the rain influence

		double MDR = DChalf;
		if (Rm > 2.8)
		{
			double RMeff = 0.83*Rm;
			double smi = 800.0*exp(-1.0*DCMo / 400);
			MDR = DChalf - 400.0*log(1.0 + 3.937*RMeff / smi);

			if (MDR < 0)
				MDR = 0;
		}

		double MDC = MDR + 0.5*Vm;   // this adds the other half of the drying influence

		double MDCavg = (DCMo + MDC) / 2;

		DCMo = MDC;

		return MDCavg;
	}


	//this method is call to load your parameter in your variable
	ERMsg CFWIModel::ProcessParameter(const CParameterVector& parameters)
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
		}
		else if (parameters.size() == 7)
		{
			m_bAutoSelect = false;
			//transfer your parameter here
			short c = 0;
			m_firstDay = CMonthDay(parameters[c++].GetString());
			m_lastDay = CMonthDay(parameters[c++].GetString());
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

}