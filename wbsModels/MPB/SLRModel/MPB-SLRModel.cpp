//*********************************************************************
// File: SafranyikModel.cpp
//
// Class: CMPBModel
//
//************** MODIFICATIONS  LOG ********************
// 11/05/2016	Rémi Saint-Amant    New compile with WBSF
// 17/03/2012   Rémi Saint-Amant    Build with new BioSIMModelBase. varaible by memory.
// 31/03/2010	Rémi Saint-Amant    Add original Safranyik computation
// 26/05/2009   Rémi Saint-Amant    Compile with the new BioSIMModelBase (Compatible with hxGrid)
// 03/02/2009   Rémi Saint-Amant    Add gProduct3: Logan2b and cold tolerance
// 01/09/2006   Rémi Saint-Amant    Creation




//*********************************************************************
#include "MPB-SLRModel.h"
#include "SLR.h"
#include "ModelBase/EntryPoint.h"


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CMPBModel::CreateObject);




	//**************************
	//set if needs MTClim computation( Radiation, Vapor Pressure Defficit)
	CMPBModel::CMPBModel()
	{
		//**************************
		//put the number of input parameters
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 7;
		VERSION = "2.3.0 (2016)";
	}

	CMPBModel::~CMPBModel()
	{
	}

	enum TAnnualStat{ O_LOGAN, O_LOGAN2b, O_SAFRANYIK, O_SAFRANYIK_P3P4, O_COLD_TOLERANCE, O_HYBRID_CT, O_PROD1, O_PROD2, O_PROD3, NB_OUTPUT_STATS };

	typedef CModelStatVectorTemplate<NB_OUTPUT_STATS> CAnnualStatVector;


	//**************************
	//This method is called to compute the solution
	ERMsg CMPBModel::OnExecuteAnnual()
	{
		_ASSERTE(m_weather.GetNbYears() >= 2);

		ERMsg msg;

		CSLR model;
		model.ProcessParameter(m_parameters);
		model.Execute(m_weather);

		CAnnualStatVector stat(model.GetNbYears(), CTRef((short)model.GetYear(0)));

		// save result to disk
		for (int y = 0; y < (int)model.GetNbYears(); y++)
		{
			for (int i = 0; i < NB_OUTPUT; i++)
				stat[y][i] = model.GetF(i, y);

			double product1 = model.GetF(LOGAN, y)*model.GetF(SAFRANYIK_P3P4, y)*model.GetF(COLD_TOLERANCE, y);
			if (product1 < 0.00001)
				product1 = 0;

			stat[y][O_PROD1] = product1;

			double product2b = model.GetF(LOGAN2b, y)*model.GetF(SAFRANYIK_P3P4, y)*model.GetF(COLD_TOLERANCE, y);
			double gProduct2b = pow(product2b, 1.0 / 3);
			if (gProduct2b < 0.00001)
				gProduct2b = 0;

			stat[y][O_PROD2] = gProduct2b;


			double product3 = model.GetF(LOGAN2b, y)*model.GetF(COLD_TOLERANCE, y);
			double gProduct3 = sqrt(product3);
			if (gProduct3 < 0.00001)
				gProduct3 = 0;

			stat[y][O_PROD3] = gProduct3;
		}

		SetOutput(stat);

		return msg;
	}




	//**************************
	//this method is called to load parameters in your variables
	ERMsg CMPBModel::ProcessParameter(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		m_parameters = parameters;

		return msg;
	}


}