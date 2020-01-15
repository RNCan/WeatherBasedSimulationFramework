//*********************************************************************
//27/01/2020	1.3.0	Rémi Saint-Amant	Add USA Plant Hardiness
//27/01/2018	1.2.0	Rémi Saint-Amant	Integrate into BioSIM 11
//27/01/2010			Rémi Saint-Amant	Creation
//*********************************************************************
#include "PlantHardiness-Model.h"
#include "PlantHardinessCanada.h"
#include "PlantHardinessUSA.h"
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
		NB_INPUT_PARAMETER = 1;
		VERSION = "1.3.0 (2020)";
	}

	CPlantHardinessModel::~CPlantHardinessModel()
	{
	}

	//This method is call to compute solution
	ERMsg CPlantHardinessModel::OnExecuteAnnual()
	{
		ERMsg msg;

		if (m_country == C_CANADA)
		{
			CPlantHardinessCanada model;
			model.Compute(m_weather, m_output);
		}
		else if(m_country == C_USA)
		{
			CPlantHardinessUSA model;
			model.Compute(m_weather, m_output);
		}

		return msg;
	}

	//this method is call to load your parameter in your variable
	ERMsg CPlantHardinessModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c=0;
		m_country = parameters[c++].GetInt();
		if (m_country >= NB_COUNTRY)
			msg.ajoute("Bad input parameter, Country model is not valid");

		
		return msg;
	}
	
}