//*********************************************************************
// 21/01/2016	2.4.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 03/02/2012	1.0		Rémi Saint-Amant	Creation
//
// Equation from THE ASCE STANDARDIZED REFERENCE EVAPOTRANSPIRATION EQUATION
//		http://www.kimberly.uidaho.edu/water/asceewri/ascestzdetmain2005.pdf
//*********************************************************************
#include <algorithm>
#include "Basic/WeatherDefine.h"
#include "Basic/DegreeDays.h"
#include "Basic/ASCE_ETc.h"
#include "Modelbase/EntryPoint.h"
#include "ETcModel.h"


using namespace std;
//using namespace WBSF::HOURLY_DATA;

namespace WBSF
{
	

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CASCE_ETcModel::CreateObject);

	//enum TOutput { O_ETc, O_ETsz, O_Kc, O_Ke, O_Kcb, O_Kc_max, O_Ks, O_Kr, O_De, O_Zr, O_Dr, O_Dr_adj, O_RAW, O_TAW, O_PIRn, O_DPe, NB_OUTPUT };
	//typedef CModelStatVectorTemplate<NB_OUTPUT> CETStatVector;
	//	typedef CModelStatVectorTemplate<NB_OUTPUT_FULL> CETFullStatVector;

	CASCE_ETcModel::CASCE_ETcModel()
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER = 5;
		VERSION = "1.2.0 (2016)";

		m_crop = BROCCOLI;
		m_Jplant = CMonthDay(0,0);
		m_soil = SILT_LOAM;
		m_granularity = FROM_SOIL;
		m_irrigation = I_PRECIPITATION;
		m_bExtended = false;

	}

	CASCE_ETcModel::~CASCE_ETcModel()
	{}


	//ERMsg CASCE_ETcModel::OnExecuteHourly()
	//{
	//	ERMsg msg;

	//	if (!m_weather.IsHourly())
	//		m_weather.ComputeHourlyVariables();

	//	CTPeriod p = m_weather.GetEntireTPeriod();
	//	((CETStatVector&)m_output).Init(p);


	//	CASCE_ETsz ASCE2005;
	//	//ASCE2005.SetOptions(m_options);

	//	CModelStatVector Etsz;
	//	ASCE2005.Execute(m_weather, Etsz);

	//	//Compute DegreeDays 5°C fro crop coefficient
	//	CDegreeDays DD(CDegreeDays::AT_HOURLY, CDegreeDays::BIOSIM_HOURLY, 5);
	//	CModelStatVector DD5;
	//	DD.Execute(m_weather, DD5);

	//	CMonthDay March15(MARCH, 15 - 1);


	//	CStatistic statDD5;
	//	//Compute corrected evapotranpiration 
	//	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	//	{
	//		if (TRef >= March15)
	//			statDD5 += DD5[TRef][0] / 24.0;//convert DH to DD

	//		//double Etsz = Etsz[TRef][0];
	//		//double Kc = CKropCoeficient::GetKc(m_Kc, statDD5[SUM]);
	//		//if (m_bZeroUnderSnow && m_weather[TRef][H_SNDH][SUM]>0)//Is it true???
	//		//Kc = 0;

	//		//m_output[TRef][O_ETc] = Kc*Etsz[TRef][0];
	//		//m_output[TRef][O_Kc] = Kc;
	//		//m_output[TRef][O_ETsz] = Etsz[TRef][0];


	//	}

	//	return msg;
	//}


	ERMsg CASCE_ETcModel::OnExecuteDaily()
	{
		ERMsg msg;

		//compute evapotranpiration of reference
		CASCE_ETc ASCE_ETc(m_Jplant, m_crop, m_soil, m_granularity, m_irrigation, m_bExtended);
		ASCE_ETc.Execute(m_weather, m_output);

		return msg;
	}
	
	ERMsg CASCE_ETcModel::OnExecuteMonthly()
	{
		ERMsg msg;

		CASCE_ETc ASCE_ETc(m_Jplant, m_crop, m_soil, m_granularity, m_irrigation, m_bExtended);

		CModelStatVector stat;
		ASCE_ETc.Execute(m_weather, stat);


		CTTransformation TT(stat.GetTPeriod(), CTM(CTM::MONTHLY));
		//ASCE_ETc.Transform(TT, stat, m_output);


		return msg;
	}

	ERMsg CASCE_ETcModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CASCE_ETc ASCE_ETc(m_Jplant, m_crop, m_soil, m_granularity, m_irrigation, m_bExtended);

		CModelStatVector stat;
		ASCE_ETc.Execute(m_weather, stat);

		CTTransformation TT(stat.GetTPeriod(), CTM(CTM::ANNUAL));
		//ASCE_ETc.Transform(TT, stat, m_output);

		return msg;
	}



	//this method is call to load your parameter in your variable
	ERMsg CASCE_ETcModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;

		//member
		m_Jplant = parameters[c++].GetString();//plantation date
		m_crop = (TCrop)parameters[c++].GetInt();//crop type
		m_soil = (TSoil)parameters[c++].GetInt();//soil type
		m_granularity = (TGranularity)parameters[c++].GetInt();//soil granularity
		m_irrigation = (TWettingEvent)parameters[c++].GetInt();//irregeration type
		
		if (m_info.m_modelName.find("Ex", 0) != -1)
			m_bExtended = true;
		

		return msg;
	}

}