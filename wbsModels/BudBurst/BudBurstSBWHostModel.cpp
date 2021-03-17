//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurstSBWHostModel.h"
#include "BudBurstSBWHost.h"
//#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSBWHostBudBurstModel::CreateObject);


	CSBWHostBudBurstModel::CSBWHostBudBurstModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = 2;
		VERSION = "1.0.0 (2021)";
	}

	CSBWHostBudBurstModel::~CSBWHostBudBurstModel()
	{
	}

	//This method is call to compute solution
	ERMsg CSBWHostBudBurstModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//attention : a refaire poour l'ensembled
		CSBWHostBudBurst model;
		model.m_species = m_species;
		/*for (size_t y = 0; y < m_weather.size(); y++)
		{
			static const double DEFOL[5][11] =
			{
				{ 0,0,0,0,0,0,96.7,98.3,98.3,98.3 },
				{0},
				{0, 0, 0, 0, 0, 0, 8.3, 71.7, 71.7, 71.7},
				{0},
				{0},
			};

			model.m_defioliation[m_weather[y].GetTRef().GetYear()] = DEFOL[m_species][y]/100.0;

		}
*/
		for (size_t y = 0; y < m_weather.size(); y++)
			model.m_defioliation[m_weather[y].GetTRef().GetYear()] = m_defoliation;

		msg = model.Execute(m_weather, m_output);


		return msg;
	}

	//this method is call to load your parameter in your variable
	ERMsg CSBWHostBudBurstModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;
		m_species = parameters[c++].GetInt();
		m_defoliation = parameters[c++].GetReal()/100.0;



		return msg;
	}

}