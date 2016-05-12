//*********************************************************************
// File: SafranyikModel.cpp
//
// Class: CSafranyikModel
//
//************** MODIFICATIONS  LOG ********************
// 01/09/2006   Rémi Saint-Amant    Creation
// 03/02/2009   Rémi Saint-Amant    Add gProduct3: Logan2b and cold tolerance
// 26/05/2009   Rémi Saint-Amant    Compile with the new BioSIMModelBase (Compatible with hxGrid)
//*********************************************************************
#include "MPB-SLRModel.h"
#include "SLR.h"
#include "EntryPoint.h"


//this line link this model with the EntryPoint of the DLL
static const bool bRegistred = 
	CModelFactory::RegisterModel( CSafranyikModel::CreateObject );


//**************************
//define your errorID here
//for each errorID defined here, go to the GetErrorMessage() method
//and add the appropriate error msg string
//#define NEED_2YEARS (ERROR_USER_BASE + 1)


//**************************
//these two lines define the main class 
//create the main object of your class 
//very important: don't change the name of pGlobalModel
//CSafranyikModel myModel;
//CBioSIMModelBase* pGlobalModel = &myModel;

//**************************
//set if needs MTClim computation( Radiation, Vapor Pressure Defficit)
CSafranyikModel::CSafranyikModel()
{
	//**************************
	//put the number of input parameters
	//NB_INPUT_PARAMETER is used to determine if the dll
	//uses the same number of parameters than the model interface
	NB_INPUT_PARAMETER = 7;

}

CSafranyikModel::~CSafranyikModel()
{
}

//**************************
//This method is called to compute the solution
ERMsg CSafranyikModel::OnExecuteAnnual()
{
	ERMsg msg;
	
	_ASSERTE( m_weather.GetNbYear() >= 2);
//		return GetErrorMessage( NEED_2YEARS );

	CSLR model;
	model.ProcessParameter(m_parameters); 

	model.Execute(m_weather);

	// save result to disk
	for( int y=0; y<(int)model.GetNbYear(); y++)
	{
		m_outputFile << model.GetYear(y);

		for(int i=0; i<NB_OUTPUT; i++)
			m_outputFile << model.GetF(i,y);

		double product1 = model.GetF(LOGAN,y)*model.GetF(SAFRANYIK_P4,y)*model.GetF(COLD_TOLERANCE,y);
		if( product1 < 0.00001)
			product1=0;

		m_outputFile << product1;

		double product2b = model.GetF(LOGAN2b,y)*model.GetF(SAFRANYIK_P4,y)*model.GetF(COLD_TOLERANCE,y);
		double gProduct2b = pow( product2b, 1.0/3);
		if( gProduct2b < 0.00001)
			gProduct2b=0;
		m_outputFile << gProduct2b;

		
		double product3 = model.GetF(LOGAN2b,y)*model.GetF(COLD_TOLERANCE,y);
		double gProduct3 = sqrt( product3 );
		if( gProduct3 < 0.00001)
			gProduct3=0;
		m_outputFile << gProduct3;  


		m_outputFile.EndLine();
	}

    return msg;
}




//**************************
//this method is called to load parameters in your variables
ERMsg CSafranyikModel::ProcessParameter(const CParameterVector& parameters)
{
    ERMsg msg;

    //transfer your parameter here
	m_parameters = parameters;
    
    return msg;
}


