//*********************************************************************
//05/06/2020	1.0		Rémi Saint-Amant	Creation
//*********************************************************************
#include "SummerMoistureModel.h"
#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"

using namespace WBSF::HOURLY_DATA; 
using namespace std;

namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSummerMoistureModel::CreateObject);

	typedef CModelStatVectorTemplate<1> CDCStatVector;

	CSummerMoistureModel::CSummerMoistureModel()
	{
		// initialise your variable here (optionnal)
		
		VERSION = "1.0.0 (2020)";

		//m_bAutoSelect = true;
		//m_FFMC = 85.0;
		//m_DMC = 6.0;
		//m_DC = 15.0;
		//

		//m_carryOverFraction = 1;
		//m_effectivenessOfWinterPrcp = 0.75;

		//m_nbDaysStart = 3;
		//m_TtypeStart = CGSInfo::TT_TNOON;
		//m_thresholdStart = 12;
		//m_nbDaysEnd = 3;
		//m_TtypeEnd = CGSInfo::TT_TMAX;
		//m_thresholdEnd = 5;
		//m_carryOverFraction = 1;
		//m_effectivenessOfWinterPrcp = 0.75;
		//m_method = CFWI::NOON_CALCULATION;
	}

	CSummerMoistureModel::~CSummerMoistureModel()
	{}

	//this method is call to load your parameter in your variable
	ERMsg CSummerMoistureModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//if (WBSF::Find(m_info.m_modelName, "fixed"))
		//{
		//	m_bAutoSelect = false;

		//	//transfer your parameter here
		//	size_t c = 0;
		//	m_firstDay = CMonthDay(parameters[c++].GetString());
		//	m_lastDay = CMonthDay(parameters[c++].GetString());
		//	m_FFMC = parameters[c++].GetReal();
		//	m_DMC = parameters[c++].GetReal();
		//	m_DC = parameters[c++].GetReal();
		//	m_carryOverFraction = parameters[c++].GetReal();
		//	m_effectivenessOfWinterPrcp = parameters[c++].GetReal();

		//	if (!GetFileData(0).empty())
		//		msg = m_init_values.Load(GetFileData(0));

		//}
		//else //if (parameters.size() == 8)
		//{
		//	m_bAutoSelect = true;

		//	size_t c = 0;
		//	m_nbDaysStart = parameters[c++].GetInt();
		//	m_TtypeStart = parameters[c++].GetInt();
		//	m_thresholdStart = parameters[c++].GetReal();
		//	m_nbDaysEnd = parameters[c++].GetInt();
		//	m_TtypeEnd = parameters[c++].GetInt();
		//	m_thresholdEnd = parameters[c++].GetReal();
		//	m_carryOverFraction = parameters[c++].GetReal();
		//	m_effectivenessOfWinterPrcp = parameters[c++].GetReal();
		//	//m_method = parameters[c++].GetInt();
		//}

		size_t c = 0;
		m_SM.m_bOverwinter = parameters[c++].GetBool();
		m_SM.m_a = parameters[c++].GetReal();
		m_SM.m_b = parameters[c++].GetReal();

		
		return msg;
	}


	//This method is call to compute solution
	ERMsg CSummerMoistureModel::OnExecuteMonthly()
	{
		return m_SM.Execute(m_weather, m_output);
	}





}