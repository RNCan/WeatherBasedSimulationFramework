//**********************************************************************
//15/02/2007	Rémi Saint-Amant	create from matlab(.m) file 
//08/09/2011	Rémi Saint-Amant	update to new BioSIM 10 
//27/03/2013	Rémi Saint-Amant	New compilation
//**********************************************************************
#include "SpruceBeetleModel.h"
#include "EntryPoint.h"


//this line link this model with the EntryPoint of the DLL
static const bool bRegistred = 
	CModelFactory::RegisterModel( CSpruceBeetleModel::CreateObject );

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
	VERSION = "1.2 (2013)";

}

CSpruceBeetleModel::~CSpruceBeetleModel()
{
}

//This method is call to compute solution
ERMsg CSpruceBeetleModel::OnExecuteAnnual()
{
    ERMsg msg;

	CSpruceBeetle spruceBeetle;
//	spruceBeetle.SetParameter( m_param );

	
	enum TAnnualStat{PRI15, DAY15, PEAK, HR17, VOLTINISM, NB_STATS };
	typedef CModelStatVectorTemplate<NB_STATS> CStatVector;
	
	CStatVector output(m_weather.GetNbYear()-1, CTRef( m_weather[1].GetYear() ) );

	for(int y=0; y<m_weather.GetNbYear()-1; y++)
	{
		//double prop_tree = 
		spruceBeetle.Compute(m_weather[y], m_weather[y+1]);

		//m_outputFile << m_weather[y+1].GetYear();
		//m_outputFile << spruceBeetle.m_pri15 << spruceBeetle.m_day15 ;
		//m_outputFile << spruceBeetle.m_peak+1 << spruceBeetle.m_Hr17;
		//m_outputFile << spruceBeetle.m_propTree;
		//m_outputFile.EndLine();
		output[y][PRI15] = spruceBeetle.m_pri15; 
		output[y][DAY15] = spruceBeetle.m_day15 ;
		output[y][PEAK] =  spruceBeetle.m_peak+1;
		output[y][HR17] = spruceBeetle.m_Hr17;
		output[y][VOLTINISM] =  spruceBeetle.m_propTree;
	}

	SetOutput(output);

	return msg;
}

//this method is call to load your parameter in your variable
ERMsg CSpruceBeetleModel::ProcessParameter(const CParameterVector& parameters)
{
    ERMsg msg;

	int c=0;
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

//this method is call to get the error string msg

/*ERMsg CSpruceBeetleModel::GetErrorMessage( int errorID)
{
    //call the base class for general error
    ERMsg msg; 


    if( m_info.m_language == FRENCH)
    {
        //add your french msg here
        switch(errorID)
        {
        case NEED_2YEARS: msg.ajoute("Ce modèle a besoin d'au moins 2 ans de données météo"); break;
        default: msg = CBioSIMModelBase::GetErrorMessage(errorID);
        }
    }
    else if( m_info.m_language == ENGLISH)
    {
        //add your english msg here
        switch(errorID)
        {
        case NEED_2YEARS: msg.ajoute("This model needs at least 2 years of weather data"); break;
        default: msg = CBioSIMModelBase::GetErrorMessage(errorID);
        }
    }

    return msg;
}

*/