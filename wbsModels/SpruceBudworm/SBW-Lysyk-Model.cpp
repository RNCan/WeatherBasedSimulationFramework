//**********************************************************************
//
// 02/06/2021 Rémi Saint-Amant	Update to BioSIM 11
// 02/03/2011 Rémi Saint-Amant	New build 
// 08/04/2011 Rémi Saint-Amant	Create a specific class for spruce budworm
// 01/07/2010 Rémi St-Amant and J. Régnière	Generalization to Continuing Ratio Model
//
//Stochastic Model of Eastern Spruce Budworm (Lepidoptera: Tortricidae) Phenology on White Spruce and Balsam Fir
//Timothy J.Lysyki
//Forestry Canada, Great Lakes Forestry Centre,
//Sault Ste.Marie, Ontario P6A 5M7 Canada
//**********************************************************************

#include "SBW-Lysyk-Model.h"
#include <math.h>
#include <crtdbg.h>
#include "ModelBase/EntryPoint.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSBWLysykModel::CreateObject);

	


	enum TOutput { O_DD, O_L2, O_L3, O_L4, O_L5, O_L6, O_L7, O_L8, O_AI, NB_OUTPUTS};


	CSBWLysykModel::CSBWLysykModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface
		NB_INPUT_PARAMETER = 1;
		VERSION = "3.0.0 (2021)";
	}

	CSBWLysykModel::~CSBWLysykModel()
	{}

	//this method is call to load your parameter in your variable
	ERMsg CSBWLysykModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_species = parameters[c++].GetInt();

		return msg;
	}


	ERMsg CSBWLysykModel::OnExecuteDaily()
	{
		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();



		return ExecuteModel(m_output);
	}



	//typedef CModelStatVectorTemplate<NB_OUTPUT> CSBWVector;

	ERMsg CSBWLysykModel::ExecuteModel(CModelStatVector& stat)
	{
		ERMsg msg;


		static const double BASE_TEMP = 8.0;
		 

		stat.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_OUTPUTS);

		for(size_t y=0; y<m_weather.GetNbYears(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM::DAILY);
			CTRef start_date(p.Begin().GetYear(), MARCH, DAY_01);
			double DD_sum = 0.0;
			
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& day = m_weather.GetDay(TRef);
				if (TRef >= start_date)
				{
					for (size_t h = 0; h < 24; h++)
					{
						double T = day[h][H_TAIR];
						DD_sum += max(0.0, (T - BASE_TEMP) / 24);
					}
				}


				array<double, NB_STAGES> p = ContinuingRatio(DD_sum);

				double AI = 0;
				for (size_t i = L2; i < NB_STAGES; i++)
				{
					AI += p[i] * (i + 2);
				}
	
				size_t c=0;
				stat[TRef][c++] = WBSF::Round(DD_sum,1);
				for(int i=0; i< NB_STAGES; i++)
					stat[TRef][c++] = WBSF::Round(p[i]*100,1);
			
				stat[TRef][c++] = WBSF::Round(AI,1);
			}
		}
	
		return msg;
	}
	
	std::array<double, CSBWLysykModel::NB_STAGES> CSBWLysykModel::ContinuingRatio(double ddays)const
	{
		static const std::array < std::array<double, NB_PARAMS>, NB_SPECIES> P =
		{ {
			{{89.298,126.728,180.464,232.011,361.692,466.654,1.343}},//Balsam Fir
			{{91.729,117.764,162.916,208.404,307.301,436.709,1.477}}//White Spruce
		} };


		size_t s = m_species;
		
		std::array<double, NB_STAGES> p = { 0 };
		p[L2] = 1;//100% in L2 stage
		
		if(ddays > 0) 
		{
			std::array<double, NB_PARAMS> v = { 0 };
			//Exiting first (initial) stage
			p[L2]=1./(1.+exp(-((P[s][A2]-ddays)/sqrt(Square(P[s][B])*ddays))));
	
			//Intermediate stages
			for(size_t L=L3; L < L8; L++)
			{
				ASSERT(P[s][L] >= P[s][L -1] );
				size_t A = L;
				double Pi2 = 1./(1.+exp(-((P[s][A]-ddays)/sqrt(Square(P[s][B])*ddays))));
				double Pi1 = 1./(1.+exp(-((P[s][A-1]-ddays)/sqrt(Square(P[s][B])*ddays))));
				
				//if( (Pi2<0.98 || Pi1<0.98) && (Pi2>0.02 || Pi1>0.02) && (Pi2-Pi1) < 0 )
					//bValid = false;
	
				//p[i] = Pi2-Pi1;
				p[A] = max(0.0, Pi2-Pi1 );
			}
			
			//Entering final stage (could be death of last stage, for example, if last parameters are a longevity)
			p[L8]=1./(1.+exp((P[s][A7]-ddays)/sqrt(Square(P[s][B])*ddays)));
		}
	
		return p;
	}
	




}