//*********************************************************************
//27/01/2015	Rémi Saint-Amant	Creation
//*********************************************************************
#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"
#include "BlueStainVariablesModel.h"
#include "BlueStainVariables.h"



using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBlueStainVariablesModel::CreateObject);


	CBlueStainVariablesModel::CBlueStainVariablesModel()
	{
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.1 (2017)";
	}

	CBlueStainVariablesModel::~CBlueStainVariablesModel()
	{
	}

	
	//This method is call to compute solution
	ERMsg CBlueStainVariablesModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CBlueStainVariables blueStainVariables;
		blueStainVariables.Execute(m_weather, m_output);
		return msg;
	}

}