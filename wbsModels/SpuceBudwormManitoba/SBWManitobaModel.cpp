//************** M O D I F I C A T I O N S   L O G ********************
//
// 30/05/2013	Rémi Saint-Amant	Revision of parameter. Add Cumulative mode and Provicial location (NE, SE)
// 08/04/2011	Rémi Saint-Amant	Create specific  SBW to Manitoba
//**********************************************************************
#include "SBWManitobaModel.h"
#include "Statistic.h"
#include <math.h>
#include <crtdbg.h>
#include "EntryPoint.h"
#include "SimulatedAnnealingVector.h"

//this line links this model with the EntryPoint of the DLL
static const bool bRegistred = 
	CModelFactory::RegisterModel( CSBWManitobaModel::CreateObject );

using namespace CFL;
using namespace std;
using namespace stdString;

enum TEvaluation { VERTICAL, HORIZONTAL, DIAGONAL};
static const TEvaluation EVALUATION = HORIZONTAL;

//South-East
//NbVal=   451	Bias=-0.94932	MAE=32.48847	RMSE=41.97562	CD= 0.71717	R²= 0.80037
//a1                  	= 231.85385  
//a2                  	= 282.87228  
//a3                  	= 323.86385  
//a4                  	= 394.70047  
//a5                  	= 473.23165  

//North-West
//NbVal=   444	Bias= 4.75682	MAE=26.01399	RMSE=34.54547	CD= 0.78561	R²= 0.81650
//a1                  	= 219.89726  
//a2                  	= 270.45967  
//a3                  	= 318.75441  
//a4                  	= 372.80927  
//a5                  	= 452.21838  
//a5                  	= 445.17697 
//341.28	612.6425963
//325.44	590.9731343
//	
//	
//493	885
//402	730

const double CSBWManitobaModel::A[NB_MODEL][NB_SUB_MODEL][NB_PARAMS] = 
{
  { //Continuing_ratio
	//{237.5, 286.2, 328.7, 398.6, 476.7},
	{231.8, 282.8, 323.9, 394.7, 473.2, 612.6},//South East
	{219.9, 270.5, 318.8, 372.8, 445.2, 591.0} //NorthWest
  },
  {//Degree-Day
	{310, 380, 475, 580, 720, 885},//South East
	{255, 320, 390, 470, 575, 730} //NorthWest
  }
};

const double CSBWManitobaModel::B[NB_MODEL][NB_SUB_MODEL][NB_PARAMS] = 
{
  {//Continuing_ratio
	{1.0, 1.25, 1.7, 1.6, 1.5, 1.5}, //South East
	//{1, 1.25, 1.6, 1.4, 1.4}, //South East
	{1.5, 1.5, 2.0, 1.5, 1.2, 1.2}, //NorthWest
	//{1, 1.25, 1.4, 1.4, 0.95} //NorthWest
  },
  {//Degree-Day
	{0.00, 0.00, 0.00, 0.00, 0.00, 0.00},//South East
	{0.00, 0.00, 0.00, 0.00, 0.00, 0.00} //NorthWest
  }
};
//	
//NbVal=    84	Bias= 0.55493	MAE= 3.34010	RMSE= 3.89913	CD= 0.99853	R²= 0.99861
//	a1                  	= 263.81189 { 256.21677, 281.35533}	VM={   2.22222,   4.44444}
//	b1                  	=   1.13245 {   0.85168,   1.21330}	VM={   0.02640,   0.08800}
//	a2                  	= 361.54442 { 357.72492, 363.11434}	VM={   0.52675,   1.05350}
//	b2                  	=   0.88574 {   0.84923,   0.96643}	VM={   0.01875,   0.05625}
//	a3                  	= 418.45272 { 417.85624, 421.18966}	VM={   0.34722,   0.69444}
//	b3                  	=   1.24006 {   0.85670,   1.24975}	VM={   0.00683,   0.01707}
//	a4                  	= 533.42134 { 531.63031, 534.03096}	VM={   0.19509,   0.78037}
//	b4                  	=   0.98720 {   0.96914,   1.01974}	VM={   0.00632,   0.01778}
//	a5                  	= 637.28373 { 609.12855, 675.19347}	VM={   8.68056,  26.04167}
//	b5                  	=   1.05417 {   0.80261,   1.22321}	VM={   0.13500,   0.45000}
//	
//static const double A[NB_STAGE] = {263.8, 361.5, 418.4, 533.4, 637.3 };
//static const double B[NB_STAGE] = {1.13, 1.88, 1.24, 0.98, 1.05};

CSBWManitobaModel::CSBWManitobaModel()
{
//NB_INPUT_PARAMETER is used to determine if the dll
//uses the same number of parameters as the model interface
	NB_INPUT_PARAMETER = -1;
	VERSION = "1.2 (2014)";


	m_firstYear=-999;
	m_lastYear=-999;
}

CSBWManitobaModel::~CSBWManitobaModel()
{}


ERMsg CSBWManitobaModel::OnExecuteDaily()
{
    ERMsg msg;

	CModelStatVector stat;
	m_continuingRatio.Execute(m_weather, stat);
	SetOutput(stat);


	//CModelStatVector stat(m_weather.GetNbDay(), m_weather.GetFirstTRef());
	//for(int y=0; y<m_weather.GetNbYear(); y++)
	//{
	//	double DD=0;
	//	//for(int m=0; m<12; m++)
	//	{
	//		for(CTRef d=m_weather[y].GetFirstTRef(); d<=m_weather[y].GetLastTRef(); d++)
	//		{
	//			DD += m_weather[y][d].GetDD(m_info.m_loc, CWeatherDay::SINGLE_SINE, 4.5, 35);

	//			stat[d][CSBWContinuingRatio::O_DD] = DD;
	//			int stage = 2;
	//			if( DD>=310 )
	//				stage = 3;
	//			else if( DD>=380 )
	//				stage = 4;
	//			else if( DD>=475 )
	//				stage = 5;
	//			else if( DD>=580 )
	//				stage = 6;
	//			else if( DD>=720 )
	//				stage = 7;
	//			else if( DD>=885 )
	//				stage = 8;

	//			if( stage<=6)
	//				stat[d][stage-2] = 100;

	//			stat[d][CSBWContinuingRatio::O_AVERAGE_INSTAR] = stage;
	//		}
	//	}
	//}
	//SetOutput(stat);

	return msg;
}

//this method is call to load your parameter in your variable
ERMsg CSBWManitobaModel::ProcessParameters(const CParameterVector& parameters)
{
    ERMsg msg;

	int c=0;
	
	m_model = parameters[c++].GetInt();
	m_subModel = parameters[c++].GetInt();
	m_continuingRatio.m_bCumul = parameters[c++].GetBool();
	m_continuingRatio.m_loc = m_info.m_loc;
	m_continuingRatio.m_startDate = 60-1;
	m_continuingRatio.m_lowerThreshold = 4.5;
	m_continuingRatio.m_bMultipleVariance = true;
	m_continuingRatio.m_bPercent=true;
	m_continuingRatio.m_bAdjustFinalProportion=true;
	m_continuingRatio.m_DDType = CSBWContinuingRatio::SINGLE_SINE;

	for(int i=0; i<NB_PARAMS; i++)
	{
		m_continuingRatio.m_a[i] = A[m_model][m_subModel][i];
		m_continuingRatio.m_b[i] = B[m_model][m_subModel][i];
	}

	if( parameters.size()>3 )
	{
	
		m_continuingRatio.m_lowerThreshold = parameters[c++].GetFloat();
	
		for(int i=0; i<NB_PARAMS; i++)
		{
			m_continuingRatio.m_a[i] = parameters[c++].GetFloat();
			m_continuingRatio.m_b[i] = parameters[c++].GetFloat();
		}
	}
	

    return msg;
}


void CSBWManitobaModel::AddDailyResult(const StringVector& header, const StringVector& data)
{
//		PLOT_ID	PLOT_NAME	PLOT_LAT	PLOT_LON	PLOT_ELEV	DATE	Year	Month	Day	jDay	SPECIES	SURVEY_ID	Spayed	LI2	LI3	LI4	LI5	LI6	PUPA	nbInd	AI
	//transform value to date/stage
	ASSERT( header[0] == "Name");
	ASSERT( header[1] == "ID");
	ASSERT( header[2] == "Group");
	ASSERT( header[3] == "Year");
	ASSERT( header[4] == "Month");
	ASSERT( header[5] == "Day");
	ASSERT( header[7] == "LI2");
	ASSERT( header[13] == "NbInds");
	ASSERT( header[14] == "AvIns");
			

	if (ToInt(data[2]) == m_subModel)
	{
		CTRef ref(ToShort(data[3]), ToShort(data[4]) - 1, ToShort(data[5]) - 1);

		std::vector<double> obs(NB_INPUT);
		for(int i=0; i<NB_INPUT; i++)
		{
			obs[i] = ToDouble(data[13 + i]);//Cumulative L2
		}
		
		m_SAResult.push_back( CSAResult(ref, obs ) );
	}
}

void CSBWManitobaModel::GetFValueDaily(CStatisticXY& stat)
{
	ERMsg msg;

	if( m_SAResult.size() > 0)
	{
		if( m_DDStat[NB_VALUE]==0 )
		{
			CModelStatVector statSim;
			m_continuingRatio.m_bCumul = true;
			m_continuingRatio.Execute(m_weather, statSim);

			const CSimulatedAnnealingVector& SAVector = GetSimulatedAnnealingVector();

			for( size_t sa=0; sa<SAVector.size(); sa++)
			{
				const CSAResultVector& SAResult = SAVector[sa]->GetSAResult();

				for(size_t i=0; i<SAResult.size(); i++)
				{
					ASSERT( SAResult[i].m_obs.size()==NB_INPUT );
					for(int p=P_L2_L3; p<NB_PARAMS; p++)
					{
						int s = p+1;
						if( SAResult[i].m_obs[I_L2+s]>-999 && statSim.IsInside(SAResult[i].m_ref) )
						{
							double obsS= SAResult[i].m_obs[I_L2+s];
							if( obsS>1 && obsS<99 )
							{
								int pp = CSBWContinuingRatio::O_FIRST_STAGE+s;
			
								short year = SAResult[i].m_ref.GetYear();
								long index = statSim.GetFirstIndex(pp, obsS, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
								if( index>= 1)
								{
									double obsDD1 = statSim[index][CSBWContinuingRatio::O_DD];
									double obsDD2 = statSim[index+1][CSBWContinuingRatio::O_DD];  

									double obsS1 = statSim[index][pp];
									double obsS2 = statSim[index+1][pp];
									double slope = (obsDD2-obsDD1)/(obsS2-obsS1);
									double obsDD = obsDD1 + (obsS-obsS1)*slope;
									ASSERT( !_isnan(obsDD) && _finite(obsDD) );
									ASSERT( !_isnan(obsS) && _finite(obsS) );

									m_DDStat+=obsDD;
									m_stageStat[p]+=obsS;
								}
							}
						}
					}
				}
			}

			ASSERT( m_DDStat[NB_VALUE]>0 );
		}




		//remove unused years
		if( m_firstYear==-999 && m_lastYear==-999)
		{
			CStatistic years;
			for(CSAResultVector::const_iterator p= m_SAResult.begin(); p<m_SAResult.end(); p++)
				years += p->m_ref.GetYear();
 
			m_firstYear = (int)years[LOWEST];
			m_lastYear = (int)years[HIGHEST];
			while( m_weather.GetNbYear()>1 && m_weather.GetFirstYear()<m_firstYear )
				m_weather.RemoveYear(0);
	
			while( m_weather.GetNbYear()>1 && m_weather.GetLastYear()>m_lastYear )
				m_weather.RemoveYear(m_weather.GetNbYear()-1);
		}


		//look to see if all ai are in growing order
		bool bValid = true;
		for(int i=1; i<NB_PARAMS&&bValid; i++)
		{
			if( m_continuingRatio.m_a[i] < m_continuingRatio.m_a[i-1] )
				bValid = false;
		}
		
		if( bValid )
		{
//******************************************************************************************************************************************************
			//CModelStatVector statSim;
			//m_continuingRatio.m_bCumul = false;
			//m_continuingRatio.Execute(m_weather, statSim);
			//
			//int L3 = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 3);
			//int L4 = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 4);
			//int L5 = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 5);
			//int L6 = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 6);
			////int Pupa = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 7);
			//bValid = L2>=0 && L4>=0 && L5>=0 && L6>=0;
			//if( bValid )
			//{
			//	stat.Add(L3>=0?statSim[L3][CSBWContinuingRatio::O_DD]:-999, 310); 
			//	stat.Add(L4>=0?statSim[L4][CSBWContinuingRatio::O_DD]:-999, 380); 
			//	stat.Add(L5>=0?statSim[L5][CSBWContinuingRatio::O_DD]:-999, 475); 
			//	stat.Add(L6>=0?statSim[L6][CSBWContinuingRatio::O_DD]:-999, 580); 
			////stat.Add(Pupa>=0?statSim[Pupa][CSBWContinuingRatio::O_DD]:-999, 720); 
			//}

//******************************************************************************************************************************************************
//Vertical lookup
			if( EVALUATION == VERTICAL)
			{
				CModelStatVector statSim;
				m_continuingRatio.m_bCumul = true;
				m_continuingRatio.Execute(m_weather, statSim);

				for(size_t i=0; i<m_SAResult.size(); i++)
				{
					ASSERT( m_SAResult[i].m_obs.size()==NB_INPUT );
					for(int p=P_L2_L3; p<=P_L6_PUPA; p++)
					{
						int s = p+1;
						if( m_SAResult[i].m_obs[I_L2+s]>-999 && statSim.IsInside(m_SAResult[i].m_ref) )
						{
						
							double obs= m_SAResult[i].m_obs[I_L2+s];
							double sim= statSim[m_SAResult[i].m_ref][CSBWContinuingRatio::O_FIRST_STAGE+s];
						
							stat.Add(sim, obs); 
						}
					}
				}
			}
//******************************************************************************************************************************************************
//Horizontal lookup
			else if( EVALUATION == HORIZONTAL)
			{
				CModelStatVector statSim;
				m_continuingRatio.m_bCumul = true;
				m_continuingRatio.Execute(m_weather, statSim);

				for(size_t i=0; i<m_SAResult.size(); i++)
				{
					ASSERT( m_SAResult[i].m_obs.size()==NB_INPUT );
					for(int p=P_L2_L3; p<=P_L6_PUPA; p++)
					{
						int s = p+1;
						if( m_SAResult[i].m_obs[I_L2+s]>-999 && statSim.IsInside(m_SAResult[i].m_ref) )
						{
							double obsS= m_SAResult[i].m_obs[I_L2+s];
							if( obsS > 1 && obsS < 99 )
							{
								int pp = CSBWContinuingRatio::O_FIRST_STAGE+s;
			
								short year = m_SAResult[i].m_ref.GetYear();
								long index = statSim.GetFirstIndex(pp, obsS, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
								if( index>= 1)
								{
									double obsDD1 = statSim[index][CSBWContinuingRatio::O_DD];
									double obsDD2 = statSim[index+1][CSBWContinuingRatio::O_DD];  

									double obsS1 = statSim[index][pp];
									double obsS2 = statSim[index+1][pp];
									double slope = (obsDD2-obsDD1)/(obsS2-obsS1);
									double obsH = obsDD1 + (obsS-obsS1)*slope;
									double simH = statSim[m_SAResult[i].m_ref][CSBWContinuingRatio::O_DD];
									ASSERT( !_isnan(obsH) && !_isnan(simH) );

									stat.Add(simH, obsH); 
								}
							}
						}
					}
				}
			}
//******************************************************************************************************************************************************			
//Diagonal lookup
			else if( EVALUATION == DIAGONAL)
			{
				CModelStatVector statSim;
				m_continuingRatio.m_bCumul = true;
				m_continuingRatio.Execute(m_weather, statSim); 

				for(size_t i=0; i<m_SAResult.size(); i++)
				{
					ASSERT( m_SAResult[i].m_obs.size()==NB_INPUT );
					for(int p=P_L2_L3; p<NB_PARAMS; p++)
					{
						int s = p+1;
						if( m_SAResult[i].m_obs[I_L2+s]>-999 && statSim.IsInside(m_SAResult[i].m_ref) )
						{
							double obsS = m_SAResult[i].m_obs[I_L2+s];
							double simS = statSim[m_SAResult[i].m_ref][CSBWContinuingRatio::O_FIRST_STAGE+s];
							if( obsS>1 && obsS<99 )
							{
								int pp = CSBWContinuingRatio::O_FIRST_STAGE+s;
			
								short year = m_SAResult[i].m_ref.GetYear();
								long index = statSim.GetFirstIndex(pp, obsS, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
								if( index>= 1)
								{
									double obsDD1 = statSim[index][CSBWContinuingRatio::O_DD];
									double obsDD2 = statSim[index+1][CSBWContinuingRatio::O_DD];  

									double obsS1 = statSim[index][pp];
									double obsS2 = statSim[index+1][pp];
									double slope = (obsDD2-obsDD1)/(obsS2-obsS1);
									double obsDD = obsDD1 + (obsS-obsS1)*slope;
									double simDD = statSim[m_SAResult[i].m_ref][CSBWContinuingRatio::O_DD];
								

									obsDD = (obsDD-m_DDStat[MEAN])/m_DDStat[STD_DEV_OVER_POP];
									simDD = (simDD-m_DDStat[MEAN])/m_DDStat[STD_DEV_OVER_POP];
									obsS = (obsS-m_stageStat[p][MEAN])/m_stageStat[p][STD_DEV_OVER_POP];
									simS = (simS-m_stageStat[p][MEAN])/m_stageStat[p][STD_DEV_OVER_POP];
									ASSERT( !_isnan(obsDD) && !_isnan(simDD) );
									ASSERT( !_isnan(obsS) && !_isnan(simS) );

									stat.Add(simDD, obsDD); 
									stat.Add(simS, obsS); 
								}
							}
						}
					}
				}
			}
//******************************************************************************************************************************************************			
			//CModelStatVector statSim;
			//m_continuingRatio.m_bCumul = true;
			//m_continuingRatio.Execute(m_weather, statSim);

			//for(int k=0; k<(int)m_SAResult.size(); k++)
			//{
			//	if( m_SAResult[k].m_obs[I_AI]>-999 && statSim.IsInside(m_SAResult[k].m_ref) )
			//	{
			//	
			//		double obs= m_SAResult[k].m_obs[I_AI];
			//		double sim= statSim[m_SAResult[k].m_ref][CSBWContinuingRatio::O_AVERAGE_INSTAR];
			//		stat.Add(sim, obs); 
			//	}
			//}
		}
	}
}


	
