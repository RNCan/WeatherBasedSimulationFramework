//***********************************************************
// 2026-09-08   1.0.0	Rémi Saint-Amant	Creation
//***********************************************************

//#include "Basic/DegreeDays.h"
//#include "ModelBase/DevRateEquation.h"


#include "SaintAmantModel.h"
#include "GypsyMothEggHatch.h"
#include "GypsyMothEggHatchEquations.h"



using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LDD;
using namespace std;

namespace WBSF
{ 

	CSaintAmantModel::CSaintAmantModel(const CGMEggParam& param) : CEggModel(param)
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		m_bApplyAttrition = true;
	}

	CSaintAmantModel::~CSaintAmantModel()
	{
	}

	
	ERMsg CSaintAmantModel::ComputeHatch(const CWeatherStation& weather, const CTPeriod& p)
	{
		assert(weather.IsHourly());

		ERMsg msg;

		m_eggState.Init(p.GetLength(), p.Begin());

		//This is where the model is actually executed
		CModelStatVector output(p, NB_STATS, 0);

		//simulation overall years
		for (size_t y = 0; y < weather.size(); y++)
		{
			ExecuteDaily(weather[y].GetTRef().GetYear(), weather, output);
		}

		for (CTRef TRef = p.Begin(); TRef <=  p.End(); TRef++)
		{
			m_eggState[TRef][DIAPAUSE] = MAXEGGS* output[TRef][S_DIAPAUSE_EGGS];
			m_eggState[TRef][POSDIAPAUSE] = MAXEGGS * output[TRef][S_EGGS] / 100.0;
			m_eggState[TRef][HATCH] = MAXEGGS * output[TRef][S_LARVAE] / 100.0;
			m_eggState[TRef][HATCHING] = MAXEGGS * output[TRef][S_EGGS_HATCH] / 100.0;
		}

		return msg;
	}

	void CSaintAmantModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output)
	{
		//Create stand
		assert(m_param.m_pModel);
		CLDDStand stand(m_param.m_pModel);
		stand.m_bApplyAttrition = m_bApplyAttrition;
				
		stand.init(year, weather);

		//Create host
		CLDDHostPtr pHost(new CLDDHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;

		pHost->Initialize<CGypsyMothEggHatch>(CInitialPopulation(400, 100, DIAPAUSE_EGG));

		//add host to stand			
		stand.m_host.push_front(pHost);

		CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			stand.Live(weather.GetDay(d));
			if (output.IsInside(d))
				stand.GetStat(d, output[d]);

			stand.AdjustPopulation();
		}

	}

}
