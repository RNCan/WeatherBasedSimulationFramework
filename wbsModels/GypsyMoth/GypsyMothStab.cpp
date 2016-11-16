// Program accepts 1 argument: full path and name of the parameter-set file
//************** M O D I F I C A T I O N S   L O G ********************
//RSA 16/11/2016: Compile for BioSIM 11
//RSA 08/03/2012: Recompilation with new model
//RSA 17/11/2008: Recompilation with new gray egg equation
//RSA 17/03/2006: Output each viability flags
//RSA 15/07/2005: Model ajust ovipDate to simulate the good year
//RSA 18/05/2005: integretion to BioSIMModelBase + cleaning
//JR  13/05/2005: harmonized this Gypmphen with the Stability version.
//
//JR 2/3/1999: made the model output one line on day 273, if first hatch occurs 
//           on or after 273. That is to ensure output
//		   files are not empty. BioSIM does not like empty output files.
//
//JR 24/3/1999: added function Reset() in Gymphen.cpp to initialize arrays
//		   in an attempt to solve the DLL vs EXE problem
//JR 25/3/1999: added function free_arrays() to free allocated global arrays 
//			(currently only **hatching). Johnson() also allocates (and frees) 
//			*day_deg.
//JR 26/4/1999: added Eggs_left to replace eggs_hatching[][] as an output variable
//			because hatching rate is hard to interpret graphically.
//JR 23/9/1999: Started implementing Sawyer et al's model...
//**********************************************************************/
#include "GypsyMothStab.h"
#include "GypsyMoth.h"
#include "ModelBase/EntryPoint.h"

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CGypsyMothStability::CreateObject);


	enum { O_VIABILITY, NB_OUTPUT };
	typedef CModelStatVectorTemplate<NB_OUTPUT> COutputVector;


	CGypsyMothStability::CGypsyMothStability()
	{
		NB_INPUT_PARAMETER = 4;
		VERSION = "2.2.1 (2016)";

		// initialise your variable here (optionnal)

		m_hatchModelType = CGypsyMoth::GRAY_MODEL;
		m_nbGenerations = 10;
	}

	CGypsyMothStability::~CGypsyMothStability()
	{
	}

	//This method is called to compute solution
	//ERMsg CGypsyMothStability::Execute() 
	//{
	//    ERMsg message;
	//
	//
	//	CGypsyMothVector gypsyMothVector;
	//	gypsyMothVector.resize(m_nbGenerations);
	//	
	//	//bool stabilityFlag = false;
	//	CGMEggInputParam paramTmp = m_param;
	//
	//	for(int g=0; g<m_nbGenerations; g++) 
	//	{
	//		//simulate developement
	//		gypsyMothVector[g].SimulateDeveloppement(paramTmp, m_weather);
	//
	//		int newOvipDate = gypsyMothVector[g].GetNewOvipDate();
	//		int firstHatch = gypsyMothVector[g].GetFirstHatch();
	//
	//		int flags = 0;
	//		bool viabilityFlag = gypsyMothVector[g].GetViabilityFlag(paramTmp, flags);
	//	
	//		bool bStability =	viabilityFlag && ( g==m_nbGenerations-1 || newOvipDate%365 == paramTmp.GetOvipDate());
	//
	//		m_outputFile << g+1 << paramTmp.GetOvipDate()+1 << firstHatch+1 << newOvipDate+1;
	//		
	//		m_outputFile << (flags&CGypsyMoth::DIAPAUSE_BEFORE_WINTER?1:0) << (flags&CGypsyMoth::POSDIAPAUSE_BEFORE_SUMMER?1:0) << (flags&CGypsyMoth::FIRST_WINTER_EGG?1:0) << (flags&CGypsyMoth::ADULT_BEFORE_WINTER?1:0) << (viabilityFlag?1:0);
	//		m_outputFile <<( bStability?1:0);
	//		m_outputFile.EndLine();
	//
	//		if( bStability || !viabilityFlag)
	//			g=m_nbGenerations; //end output
	//		else paramTmp.SetOvipDate(newOvipDate%365);
	//	}
	//
	//	return message;
	//}

	ERMsg CGypsyMothStability::OnExecuteAnnual()
	{
		if (m_nbGenerations == 0)
			ExecuteWithoutGeneration();
		else ExecuteWithGeneration();

		return ERMsg();
	}

	void CGypsyMothStability::ExecuteWithGeneration()
	{
		//Set global class variables
		CGypsyMoth::SetApplyMortality(false);


		std::vector<bool> bViability(m_weather.GetNbYears() - 1, false);

		for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		{
			//init parameter and ovipisition for the current year
			CGMEggParam eggParamTmp = m_eggParam;
			eggParamTmp.m_ovipDate.m_year = m_weather[y].GetTRef().GetYear();

			bViability[y] = true;
			bool bStabilized = false;

			for (int g = 0; g < m_nbGenerations&& bViability[y] && !bStabilized; g++)
			{
				CTPeriod p(m_weather[y].GetEntireTPeriod(CTM::DAILY).Begin(), m_weather[y + 1].GetEntireTPeriod(CTM::DAILY).End());
				//simulate developement
				CGypsyMoth gypsyMoth(m_hatchModelType, eggParamTmp);
				gypsyMoth.SimulateDeveloppement(m_weather, p);


				if (gypsyMoth.GetViabilityFlag())
				{
					//Get stability and new oviposition date
					CTRef newOvipDate = gypsyMoth.GetNewOvipDate();
					bStabilized = newOvipDate.GetJDay() == eggParamTmp.m_ovipDate.GetJDay();
					eggParamTmp.m_ovipDate.SetJDay(newOvipDate.GetJDay());
				}
				else
				{
					//the model for this year is not viable
					bViability[y] = false;
				}
			}
		}

		//compute viability
		int nbValid = 0;
		for (size_t i = 0; i < bViability.size(); i++)
			if (bViability[i])
				nbValid++;

		COutputVector stat(1, CTRef(0, 0, 0, 0, CTM(CTRef::ANNUAL, CTRef::OVERALL_YEARS) ));
		stat[0][0] = (double)nbValid / bViability.size();
		SetOutput(stat);


	}

	void CGypsyMothStability::ExecuteWithoutGeneration()
	{
		//Set global class variables
		CGypsyMoth::SetApplyMortality(false);

		bool bViability = true;

		CGMEggParam eggParamTmp = m_eggParam;

		for (size_t y = 0; y < m_weather.GetNbYears() - 1 && bViability; y++)
		{
			//CTPeriod p(m_weather[y].GetFirstTRef(), m_weather[y + 1].GetLastTRef());
			CTPeriod p(m_weather[y].GetEntireTPeriod(CTM::DAILY).Begin(), m_weather[y + 1].GetEntireTPeriod(CTM::DAILY).End());

			//simulate developement
			CGypsyMoth gypsyMoth(m_hatchModelType, eggParamTmp);
			gypsyMoth.SimulateDeveloppement(m_weather, p);

			bViability = gypsyMoth.GetViabilityFlag();
			eggParamTmp.m_ovipDate = gypsyMoth.GetNewOvipDate();;
		}

		//Output data
		COutputVector stat(1, CTRef(0, 0, 0, 0, CTM(CTRef::ANNUAL, CTRef::OVERALL_YEARS)));
		stat[0][0] = bViability ? 1 : 0;
		SetOutput(stat);


	}



	/*
	ERMsg CGypsyMothStability::Execute()
	{
	ERMsg message;

	//extra initialisation for GetWinterMid and AdjustOvipDate
	//	m_param.InitWeatherInfo(m_weather);
	//	m_param.AjustOvipDate();

	CGypsyMothVector gypsyMothVector;
	gypsyMothVector.resize(m_nbGenerations);

	bool stabilityFlag = false;
	CGMEggInputParam paramTmp = m_param;

	for(int g=0; g<m_nbGenerations; g++)
	{
	//simulate developement
	gypsyMothVector[g].SimulateDeveloppement(paramTmp, m_weather);

	int newOvipDate = gypsyMothVector[g].GetNewOvipDate();
	int firstHatch = gypsyMothVector[g].GetFirstHatch();

	int flags = 0;
	bool viabilityFlag = gypsyMothVector[g].GetViabilityFlag(paramTmp, flags);
	if(viabilityFlag)
	{
	//Generation viable.
	//If new oviposition date same as previous this run is over,
	//because oviposition date can no longer change
	if(newOvipDate%365 != paramTmp.GetOvipDate())
	{
	//If this is the last generation and still viable, output stability
	if(g==m_nbGenerations-1)
	stabilityFlag=true;

	m_outputFile << g+1 << paramTmp.GetOvipDate()+1 << firstHatch+1 << newOvipDate+1;
	m_outputFile << (flags&CGypsyMoth::START_BEFORE_365?1:0) << (flags&CGypsyMoth::FIRST_WINTER_EGG?1:0) << (flags&CGypsyMoth::ADULT_BEFORE_WINTER?1:0) << (viabilityFlag?1:0);
	m_outputFile << (stabilityFlag?1:0) ;
	m_outputFile.EndLine();

	paramTmp.SetOvipDate(newOvipDate%365);
	//ajust oviposition date
	//paramTmp.AjustOvipDate();
	//from here, do the next generation if it is not the last
	}
	else
	{
	//output remaining generations
	stabilityFlag=true;
	for (int gg=g; gg<m_nbGenerations; gg++)
	{
	m_outputFile << gg+1 << paramTmp.GetOvipDate()+1 << firstHatch+1<< newOvipDate+1 ;
	m_outputFile << (flags&CGypsyMoth::START_BEFORE_365?1:0) << (flags&CGypsyMoth::FIRST_WINTER_EGG?1:0) << (flags&CGypsyMoth::ADULT_BEFORE_WINTER?1:0) << 1;
	m_outputFile << (stabilityFlag?1:0) ;
	m_outputFile.EndLine();
	}
	g=m_nbGenerations-1;
	}
	}
	else
	{
	//Generation NOT viable. Output variables and terminate run.
	for (int gg=g; gg<m_nbGenerations; gg++)
	{
	m_outputFile << gg+1 << paramTmp.GetOvipDate()+1 << firstHatch+1 << newOvipDate+1;
	m_outputFile << (flags&CGypsyMoth::START_BEFORE_365?1:0) << (flags&CGypsyMoth::FIRST_WINTER_EGG?1:0) << (flags&CGypsyMoth::ADULT_BEFORE_WINTER?1:0) << (viabilityFlag?1:0);
	m_outputFile << (stabilityFlag?1:0);
	m_outputFile.EndLine();
	}
	g=m_nbGenerations;
	}
	}

	return message;
	}
	*/
	/****************************************************************************************************************/

	//this method is call to load your parameter in your variable
	ERMsg CGypsyMothStability::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg message;

		CTPeriod period = m_weather.GetEntireTPeriod(CTM::DAILY);

		int c = 0;
		m_hatchModelType = parameters[c++].GetInt();
		m_eggParam.m_ovipDate = period.Begin() + parameters[c++].GetInt();
		m_eggParam.m_sawyerModel = parameters[c++].GetInt();
		m_nbGenerations = parameters[c++].GetInt();

		return message;
	}


}