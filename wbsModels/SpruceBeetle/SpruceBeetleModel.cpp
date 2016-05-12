//**********************************************************************
//15/02/2007	Rémi Saint-Amant	create from matlab(.m) file 
//08/09/2011	Rémi Saint-Amant	update to new BioSIM 10 
//27/03/2013	Rémi Saint-Amant	New compilation
//**********************************************************************
#include "SpruceBeetleModel.h"
#include "ModelBase/EntryPoint.h"

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSpruceBeetleModel::CreateObject);

	//define your errorID here
	//for each errorID define here, go to the GetErrorMessage() method
	//and add the appropriate error msg string
	//#define NEED_2YEARS (ERROR_USER_BASE + 1)


	//this two lines define the main class 
	//create your main objet of your class 
	//very important: don't change the name of pGlobalModel
	CSpruceBeetleModel myModel;
	CBioSIMModelBase* pGlobalModel = &myModel;

	// Set true if you need extra computation( Radiation or VPD )
	CSpruceBeetleModel::CSpruceBeetleModel()
	{
		//put the number of input parameters
		//NB_INPUT_PARAMETER is use to determine if the dll
		//use the same number of parameter than the model interface
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.3.0 (2016)";

	}

	CSpruceBeetleModel::~CSpruceBeetleModel()
	{
	}

	//This method is call to compute solution
	ERMsg CSpruceBeetleModel::OnExecuteAnnual()
	{
		ASSERT(m_weather.GetNbYears() >= 2);

		ERMsg msg;

		CSpruceBeetle spruceBeetle;

		enum TAnnualStat{ PRI15, DAY15, PEAK, HR17, VOLTINISM, NB_STATS };
		//typedef CModelStatVectorTemplate<NB_STATS> CStatVector;

		m_output.Init(m_weather.GetNbYears() - 1, CTRef(m_weather[size_t(1)].GetTRef().GetYear()), NB_STATS, 0);

		for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		{
			//double prop_tree = 
			spruceBeetle.Compute(m_weather[y], m_weather[y + 1]);

			m_output[y][PRI15] = spruceBeetle.m_pri15;
			m_output[y][DAY15] = spruceBeetle.m_day15;
			m_output[y][PEAK] = spruceBeetle.m_peak + 1;
			m_output[y][HR17] = spruceBeetle.m_Hr17;
			m_output[y][VOLTINISM] = spruceBeetle.m_propTree;
		}

		//SetOutput(output);

		return msg;
	}

	//this method is call to load your parameter in your variable
	ERMsg CSpruceBeetleModel::ProcessParameter(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		//m_param.m_surrogateThreshold = parameters[c++].GetReal();
		//m_param.m_semi = parameters[c++].GetInt();
		//m_param.m_semiIndex = parameters[c++].GetInt();
		//m_param.m_mix = parameters[c++].GetInt();
		//m_param.m_mixIndex = parameters[c++].GetInt();
		//m_param.m_uniIndex = parameters[c++].GetInt();

		//m_param.m_intercept = parameters[c++].GetReal();
		//m_param.m_slope = parameters[c++].GetReal();
		//m_param.m_northEffect = parameters[c++].GetReal();
		//m_param.m_htEffect = parameters[c++].GetReal();
		//m_param.m_lowEffect = parameters[c++].GetReal();

		//CSpruceBeetle::INTERCEPT=parameters[c++].GetReal();
		//CSpruceBeetle::SLOPE=parameters[c++].GetReal();
		//CSpruceBeetle::NORTH_EFFECT=parameters[c++].GetReal();
		//CSpruceBeetle::HIGT_EFFECT=parameters[c++].GetReal();
		//CSpruceBeetle::LOW_EFFECT=parameters[c++].GetReal();


		return msg;
	}

	
}