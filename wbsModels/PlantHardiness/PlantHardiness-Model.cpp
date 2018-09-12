//*********************************************************************
//27/01/2010	1.2.0	Rémi Saint-Amant	Integrate into BioSIM 11
//27/01/2010			Rémi Saint-Amant	Creation
//*********************************************************************
#include "PlantHardiness-Model.h"
#include "PlantHardiness.h"
#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"


using namespace std;

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CPlantHardinessModel::CreateObject);


	CPlantHardinessModel::CPlantHardinessModel()
	{
		// initialise your variable here (optionnal)
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.2.0 (2018)";
	}

	CPlantHardinessModel::~CPlantHardinessModel()
	{
	}

	//This method is call to compute solution
	ERMsg CPlantHardinessModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CPlantHardiness model;
		model.Compute(m_weather, m_output);

		return msg;
	}

	//this method is call to load your parameter in your variable
	/*ERMsg CPlantHardinessModel::ProcessParameter(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		short c=0;
		m_firstDay = parameters[c++].GetInt()-1;
		m_ffmc = parameters[c++].GetReal();
		m_dmc = parameters[c++].GetReal();
		m_dc = parameters[c++].GetReal();
		m_lastDay = parameters[c++].GetInt()-1;

		return msg;
	}
	*/
}