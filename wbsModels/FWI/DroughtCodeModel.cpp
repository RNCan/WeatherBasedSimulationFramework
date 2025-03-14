//*********************************************************************
//15/05/2018	2.2.0	R�mi Saint-Amant    Add min and max
//23/03/2018	2.1.2	R�mi Saint-Amant    Compile with VS 2017
//29/03/2017	2.1.1	R�mi Saint-Amant    Bug correction in Drought Code
//20/09/2016	2.1.0	R�mi Saint-Amant    Change Tair and Trng by Tmin and Tmax
//08/02/2016	2.0.0   R�mi Saint-Amant	Include in WBSF
//08/02/2012	1.1     R�mi Saint-Amant	Recompile with new interface and new FWI model
//17/05/2011	1.0		R�mi Saint-Amant	Creation
//*********************************************************************
#include "DroughtCodeModel.h"
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
		
		VERSION = "2.3.0 (2020)";

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

		if (WBSF::Find(m_info.m_modelName, "fixed"))
		{
			m_bAutoSelect = false;

			//transfer your parameter here
			size_t c = 0;
			m_firstDay = CMonthDay(parameters[c++].GetString());
			m_lastDay = CMonthDay(parameters[c++].GetString());
			m_FFMC = parameters[c++].GetReal();
			m_DMC = parameters[c++].GetReal();
			m_DC = parameters[c++].GetReal();
			m_carryOverFraction = parameters[c++].GetReal();
			m_effectivenessOfWinterPrcp = parameters[c++].GetReal();

			if (!GetFileData(0).empty())
				msg = m_init_values.Load(GetFileData(0));

		}
		else //if (parameters.size() == 8)
		{
			m_bAutoSelect = true;

			size_t c = 0;
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
		FWI.m_init_values = m_init_values;

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


		msg = FWI.Execute(m_weather, output);

		return msg;
	}

	//This method is call to compute solution
	ERMsg CDroughtCode::OnExecuteDaily()
	{
		ERMsg msg;

		
		//Compute FWI
		CModelStatVector output;
		msg = ExecuteDaily(output);
		
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
		msg = ExecuteDaily(resultD);

		CFWIMStatVector resultM;
		CFWIStat::Covert2M(resultD, resultM);

		enum TMOutput { M_DC_MIN, M_DC_MEAN, M_DC_MAX, NB_M_OUTPUTS };
		m_output.Init(resultM.GetTPeriod(), NB_M_OUTPUTS, -9999);

		static const size_t VAR[NB_M_OUTPUTS] = { CFWIStat::DC_MIN, CFWIStat::DC, CFWIStat::DC_MAX };

		for (size_t mm = 0; mm < resultM.size(); mm++)
		{
			for (size_t v = 0; v < NB_M_OUTPUTS; v++)
				m_output[mm][v] = resultM[mm][VAR[v]];
		}

		


		return msg;
	}

	ERMsg CDroughtCode::OnExecuteAnnual()
	{
		ERMsg msg;

		CFWIDStatVector resultD;
		msg = ExecuteDaily(resultD);


		CFWIAStatVector resultA;
		CFWIStat::Covert2A(resultD, resultA);

		m_output.Init(resultA.GetTPeriod(), 1, -9999);

		for (size_t y = 0; y < resultA.size(); y++)
			m_output[y][0] = resultA[y][CFWIStat::DC];

		

		return msg;
	}




}