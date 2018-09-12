/**************************************************************
Individual-based model of Mountain pine beetle
Seasonal population dynamics (MPB-iMOD)

Jacques Régnière
Canadian Forest Service

Jim Powell
Utah State University

Barbara Bentz
USDA Forest Service, Logan Utah

Programmer: Rémi St-Amant

Initial creation Fall 2007 
Complete update Fall 2012

Work initiated after the Snowbird workshop of 15 Nov 2005, but interrupted when J Powell 
announced that a grad student of his was undertaking object-oriented modelling. 
Work restarted after a meeting at Logan, UT (Regniere-Bentz-Powell) Oct. 1-3, 2007.

Thoughts: 
	Ovipositing females live until max fec is realized. They can be killed by winter mortality according to Lester & Irwin 2012, 
	but are otherwise very long-lived and continue ovipositing as temperature allows.

**************************************************************/

//*********************************************************************
// File: MPBiModel.cpp
//
// Class: CMPBiModel
//
//************** MODIFICATIONS  LOG ********************
// 04/05/2017  2.3.1	Rémi Saint-Amant    New compile
// 27/03/2013  2.3.0	Rémi Saint-Amant    New compilation with WBSF
// 27/03/2013  2.2		Rémi Saint-Amant    New compilation
// 17/12/2012  2.1		Rémi Saint-Amant    Add the mode Year by Year adjusted
// 16/07/2012  2.0		Rémi Saint-Amant    Simplified model 
// 26/05/2009			Rémi Saint-Amant    Compile with the new BioSIMModelBase (Compatible with hxGrid)
// 12/11/2008			Rémi Saint-Amant	Use the new MPBi class
// 24/09/2008			Rémi Saint-Amant	Auto ajusting the number of object in the tree
// 14/12/2007			Rémi Saint-Amant	Integration with the new BioSIM model Base
// 12/11/2007			Jacques Régnière    Commenting/editing
// 30/10/2007			Remi Saint-Amant    Consolidation of all forms of MPB models
// 29/10/2007			Rémi Saint-Amant    Debugging 
// 23/10/2007			Rémi Saint-Amant    sets things straight
// 22/10/2007			Jacques Régnière    Modifications
// 22/11/2005			Rémi Saint-Amant    Creation
//*********************************************************************
#include "MPBiModel.h"
#include "Basic/timeStep.h"
#include "ModelBase/EntryPoint.h"

using namespace std;
//using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CMPBiModel::CreateObject);

	CMPBiModel::CMPBiModel()
	{
		//**************************
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface
		NB_INPUT_PARAMETER = 25;
		VERSION = "2.3.1 (2017)";

		//input parameters of MPBiMOD
		m_n0 = 60;             //Number of females/m² in the initial attack 
		m_attackStDev = 0;					//Peak Date of initial attack (not initialized here), and its standard deviation
		m_bFertilEgg = false;			    //Turns brood female fertility on/off
		m_nbFertilGeneration = -1;
		m_bAutoBalanceObject = true;          //Creates "super individual" objects to accelerate model
		m_survivalRate = 0;                 //Basic survival rate (applied to eggs before their creation). 
		//Sex ratio of 0.7 female also applied at egg creation. Males are not created.  
		m_A0 = 60;                            //Tree defense capacity (attacks/m²/day)
		m_Amax = 120;                         //Maximum adult beetles (successful attacks) allowed/m² 
		m_applyColdTolerance = true;          //Apply cold mortality (all stages) 
		m_applyColdToleranceOvipAdult = 1; //Apply cold mortality of ovipositing adults (Lester & Irwin 2012)
		m_emergenceThreshold = 18;             //Minimum temperature needed for adult emergence. Leads to additional mortality for adults not able to emerge before winter
		m_bMicroClimate = false;              //Calculate bark temperature from air temperature
		m_forestSize = 500;                   //Forest size (km²)

		m_forestDensity = 100000;				//Forest density trees/km²
		m_bSnowProtection = true;           //Creating a cold-mortality refuge due to snow cover
		m_bUseDefenselessTree = true;       //In low density (total bug < A), beetles can attack these moribund trees to subsist
		m_initialInfestation = 0;			//initial infestation (km²)
		m_totalInitialAttack = 0;
		m_nbObjects = 500;
		m_minObjects = 100;
		m_maxObjects = 400;
		m_heightMax = 1000;  // height max in cm
		m_pupeTeneralColdT = -18; // cold min egg in °C

	}

	CMPBiModel::~CMPBiModel()
	{
	}

	ERMsg CMPBiModel::OnExecuteAtemporal()
	{
		ERMsg msg;



		if (m_bFertilEgg)
		{
			//if eggs are fertile, run model as continous

			//create a matrix of output [number of generation estimated]x[number of days]
			CAnnualOutputVector output(m_weather.GetNbYears(), CTRef(1, 0, 0, 0, CTM(CTM::ATEMPORAL)));
			CMPBStatMatrix stat(output.size());

			//Do simulation
			DoSimulation(stat, m_attacks);

			ASSERT(stat.size() == output.size());

			//compute generation statistics
			for (size_t g = 0; g < stat.size(); g++)
			{
				ComputeGenerationValue(stat[g], output[g]);
				CTRef firstDay = stat[g].GetFirstTRef(S_NB_OBJECT_ALIVE, ">=", 0.05, 0);
				CTRef lastDay = stat[g].GetLastTRef(S_NB_OBJECT_ALIVE, ">=", 0.05, 0);

				if (firstDay.IsInit())
				{
					output[g][A_YEAR] = firstDay.GetYear();
					output[g][A_MONTH] = firstDay.GetMonth() + 1;
					output[g][A_DAY] = firstDay.GetDay() + 1;
					for (size_t gg = 0; gg <= g; gg++)
					{
						output[g][A_EGG] += stat[gg][firstDay][S_EGG];
						output[g][A_LARVAL] += stat[gg][firstDay][S_L1] + stat[gg][firstDay][S_L2] + stat[gg][firstDay][S_L3] + stat[gg][firstDay][S_L4];
						output[g][A_PUPA] += stat[gg][firstDay][S_PUPA];
						output[g][A_TENERAL] += stat[gg][firstDay][S_TENERAL_ADULT];
						output[g][A_OVIPADULT] += stat[gg][firstDay][S_OVIPOSITING_ADULT];
					}
				}
				else
				{
					output[g][A_YEAR] = -9999;
					output[g][A_MONTH] = -9999;
					output[g][A_DAY] = -9999;
					output[g][A_EGG] = -9999;
					output[g][A_LARVAL] = -9999;
					output[g][A_PUPA] = -9999;
					output[g][A_TENERAL] = -9999;
					output[g][A_OVIPADULT] = -9999;
				}

				if (lastDay.IsInit())
				{
					output[g][A_YEAR_END] = lastDay.GetYear();
					output[g][A_MONTH_END] = lastDay.GetMonth() + 1;
					output[g][A_DAY_END] = lastDay.GetDay() + 1;
				}
				else
				{
					output[g][A_YEAR_END] = -9999;
					output[g][A_MONTH_END] = -9999;
					output[g][A_DAY_END] = -9999;
				}
			}

			//Compute R and infestation (%)
			output[0][A_INFESTED_KM²_CUMUL] = (m_initialInfestation + output[0][A_INFESTED_KM²]);
			output[0][A_R] = -9999;

			for (size_t i = 1; i < output.size(); i++)
			{
				if (output[i - 1][A_SUCCESS_ATTACK] > 0)
					output[i][A_R] = output[i][A_SUCCESS_ATTACK] / output[i - 1][A_SUCCESS_ATTACK];
				else
					output[i][A_R] = 0;

				if (output[i][A_INFESTED_KM²] >= 0)
					output[i][A_INFESTED_KM²_CUMUL] = output[i - 1][A_INFESTED_KM²_CUMUL] + output[i][A_INFESTED_KM²];
				else

				{
					output[i][A_INFESTED_KM²_CUMUL] = output[i - 1][A_INFESTED_KM²_CUMUL];
					output[i][A_INFESTED_KM²] = 0;
					output[i][A_R] = 0;
				}
			}

			SetOutput(output);

		}
		else //Eggs non fertile
		{
			//run model 
			CAnnualOutputVector output(m_weather.GetNbYears() - 1, CTRef(m_weather.GetFirstYear(), 0, 0, 0, CTM(CTM::ATEMPORAL)));


			for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
			{
				CInitialPopulation attacks = m_attacks;
				attacks.UpdateYear(m_weather[y].GetTRef().GetYear());

				CMPBStatVector stat;
				DoSimulation(stat, attacks, y);

				ComputeGenerationValue(stat, output[y]);
			}

			output[0][A_INFESTED_KM²_CUMUL] = (m_initialInfestation + output[0][A_INFESTED_KM²]);

			for (size_t i = 1; i < output.size(); i++)
			{
				if (output[i][A_INFESTED_KM²] >= 0)
					output[i][A_INFESTED_KM²_CUMUL] = output[i - 1][A_INFESTED_KM²_CUMUL] + output[i][A_INFESTED_KM²];
				else output[i][A_INFESTED_KM²_CUMUL] = VMISS;
			}

			SetOutput(output);

		}


		return msg;
	}

	ERMsg CMPBiModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CAnnualOutputVector output;

		if (m_bFertilEgg)
		{
			//if eggs are fertile, run model as continous
			CMPBStatVector stat;
			DoSimulation(stat, m_attacks);
			ComputeRegularValue(stat, output);
		}
		else
		{
			//else, compute stats year, by running n years at a time

			output.Init(m_weather.GetNbYears() - 1, CTRef(m_weather[(size_t)1].GetTRef().GetYear()));


			for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
			{
				CInitialPopulation attacks = m_attacks;
				attacks.UpdateYear(m_weather[y].GetTRef().GetYear());

				CMPBStatVector stat;
				DoSimulation(stat, attacks, y);//, Min(y+2, m_weather.GetNbYears()-1) );

				ComputeGenerationValue(stat, output[y]);
			}
		}

		output[0][A_INFESTED_KM²_CUMUL] = (m_initialInfestation + output[0][A_INFESTED_KM²]);


		for (size_t i = 1; i < output.size(); i++)
		{
			if (output[i][A_INFESTED_KM²] >= 0)
			{
				output[i][A_INFESTED_KM²_CUMUL] = output[i - 1][A_INFESTED_KM²_CUMUL] + output[i][A_INFESTED_KM²];
			}
			else
			{
				output[i][A_INFESTED_KM²_CUMUL] = output[i - 1][A_INFESTED_KM²_CUMUL];
				output[i][A_INFESTED_KM²] = 0;
				output[i][A_R] = 0;
			}

			if (output[i][A_R] < 0)
				output[i][A_R] = 0;
		}



		SetOutput(output);

		return msg;
	}

	ERMsg CMPBiModel::OnExecuteDaily()
	{
		ERMsg msg;


		CDailyOutputVector output(m_weather.GetEntireTPeriod(CTM::DAILY));

		if (m_bFertilEgg)
		{
			CMPBStatVector stat;
			DoSimulation(stat, m_attacks);
			ComputeRegularValue(stat, output);
		}
		else
		{
			CInitialPopulation attacks = m_attacks;
			size_t y = 0;
			//for(short y=0; y<m_weather.GetNbYears()-1; y++)
			while (y < m_weather.GetNbYears() - 1)
			{
				CMPBStatVector stat;
				DoSimulation(stat, attacks, y);

				ComputeRegularValue(stat, output);
				//attacks = stat.GetDailyProportionStatVector(m_totalInitialAttack, E_OVIPOSITING_ADULT, m_nbObjects);
				attacks = stat.GetInitialPopulation(E_OVIPOSITING_ADULT, m_nbObjects, m_totalInitialAttack, E_OVIPOSITING_ADULT, FEMALE);
				if (attacks.empty())
				{
					ASSERT(y >= 0 && y < m_weather.GetNbYears());
					attacks = m_attacks;
					attacks.UpdateYear(m_weather[y + 1].GetTRef().GetYear());
				}
				y = attacks[0].m_creationDate.GetYear() - m_weather.GetFirstYear();
				ASSERT(y >= 0 && y < m_weather.GetNbYears());
			}
		}

		output[0][D_INFESTED_KM²_CUMUL] = (m_initialInfestation + output[0][D_INFESTED_KM²]);

		for (size_t i = 1; i < output.size(); i++)
		{
			if (output[i][D_INFESTED_KM²] >= 0)
				output[i][D_INFESTED_KM²_CUMUL] = output[i - 1][D_INFESTED_KM²_CUMUL] + output[i][D_INFESTED_KM²];
			else output[i][D_INFESTED_KM²_CUMUL] = VMISS;
		}

		SetOutput(output);


		return msg;
	}

	/*CTRef CMPBiModel::GetInitialPeakAttack(int nbRunMax, int y)
	{
	ASSERT( m_weather.GetNbYears()>=2);
	ASSERT( nbRunMax >= 1);

	if( y==m_weather.GetNbYears()-1 )
	return CTRef();

	bool bFertilEgg = m_bFertilEgg;
	vector<CTRef> originalPeak = m_peakDates;
	CTRef peak;

	//for(short y=y1; y<=y2&&!peak.IsInit(); y++)
	//{
	peak = m_peakDate;
	for(short i=0; i<nbRunMax&&peak.IsInit(); i++)
	{
	//fix seed to converge
	CFL::Randomize(1);
	InitRandomGenerator(1);

	//assigne m_peakDate as peak for this simulation. egg are not fertile
	m_bFertilEgg = false;
	m_peakDate = peak;
	CMPBStatVector stat;
	DoSimulation(stat, y, y+1);

	peak = GetInitialPeakAttack(stat);
	if( peak.IsInit() )
	peak.m_year = originalPeak.GetYear();

	if( abs(peak.GetJDay() - m_peakDate.GetJDay()) <= 7 )
	{
	//we have converged. stop here
	break;

	//we are looking top see if it's a valid  date
	//	if( y+1<m_weather.GetNbYears()-1)
	//	{
	//		m_peakDate = peak;
	//		//try with the last year
	//		peak = GetInitialPeakAttack(nbRunMax, m_weather.GetNbYears()-2);
	//		if( peak.IsInit() )
	//		{
	//			peak = m_peakDate;
	//			//we have converged. stop here
	//			break;
	//		}
	//		else
	//		{
	//			//it's not a viable peak
	//			peak.Reset();
	//		}
	//
	//		//m_peakDate = peak;
	//		//CMPBStatVector stat;
	//		//DoSimulation(stat, y+1, y+2);
	//
	//		//CTRef peak2 = GetInitialPeakAttack(stat);
	//		//if( peak2.IsInit() )
	//		//{
	//		//	//we have converged. stop here
	//		//	break;
	//		//}
	//		//else
	//		//{
	//		//	//it's not a viable peak
	//		//	peak.Reset();
	//		//}
	//	}
	//	else
	//	{
	//		//we have converged. stop here
	//		break;
	//	}
	}
	}
	//}


	//put the intial value
	m_bFertilEgg=bFertilEgg;
	m_peakDate = originalPeak;


	//initialize as initial
	bool bFixedSeed = m_info.m_TG.GetSeedType()==CMTGInput::FIXED_SEED;
	CFL::Randomize( bFixedSeed?1:0);
	InitRandomGenerator(bFixedSeed?1:0);

	//try an other year if not init
	if(!peak.IsInit())
	peak = GetInitialPeakAttack(nbRunMax, y+1);

	return peak;
	}
	*/

	void CMPBiModel::DoSimulation(CMPBStatVector& stat, const CInitialPopulation& attacks, size_t y1, size_t y2)
	{
		CMPBStatMatrix tmp(1);
		DoSimulation(tmp, attacks, y1, y2, true);
		stat = tmp[0];
	}

	void CMPBiModel::DoSimulation(CMPBStatMatrix& stat, const CInitialPopulation& attacks, size_t y1, size_t y2, bool bAllGeneration)
	{
		ASSERT(y1 == NOT_INIT || y1 < m_weather.GetNbYears() - 1);
		ASSERT(y2 == NOT_INIT || y2 <= m_weather.GetNbYears());

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		
		if (attacks.empty())
		{
			return;
		}



		if (y1 == NOT_INIT)
			y1 = 0;

		if (y2 == 0 || y2 == NOT_INIT)
			y2 = m_weather.GetNbYears() - 1;

		CTPeriod p(m_weather[y1].GetEntireTPeriod(CTM::DAILY).Begin(), m_weather[y2].GetEntireTPeriod(CTM::DAILY).End());
		//Initialize statistics 
		for (size_t g = 0; g < stat.size(); g++)
			stat[g].Init(p);
			//stat[g].Init((m_weather[y2].GetEntireTPeriod().End() - m_weather[y1].GetEntireTPeriod().Begin() + 1, m_weather[y1].GetFirstTRef());


		//CTRef peakDate(m_peakDate);
		//peakDate.m_year  = m_weather[f1].GetYear();

		//Create stand
		CMPBStand stand(this);
		stand.m_bFertilEgg = m_bFertilEgg;
		stand.m_nbFertilGeneration = m_nbFertilGeneration;
		stand.m_survivalRate = m_survivalRate;
		stand.m_applyColdTolerance = m_applyColdTolerance;
		stand.m_applyColdToleranceOvipAdult = m_applyColdToleranceOvipAdult;
		stand.m_bMicroClimate = m_bMicroClimate;
		stand.m_emergenceThreshold = m_emergenceThreshold;
		stand.m_bSnowProtection = m_bSnowProtection;
		stand.m_bUseDefenselessTree = m_bUseDefenselessTree;
		stand.m_initialInfestation = m_initialInfestation;
		stand.m_forestDensity = m_forestDensity;
		stand.m_forestSize = m_forestSize;
		stand.m_DDAlpha = m_DDAlpha;
		stand.m_pupeTeneralColdT = m_pupeTeneralColdT;
		stand.m_heightMax = m_heightMax;


		//Create the initial attack
		CMPBTree* pTree = new CMPBTree(&stand);
		pTree->m_nbMinObjects = m_minObjects;
		pTree->m_nbMaxObjects = m_maxObjects;
		pTree->m_A0 = m_A0;
		pTree->m_Amax = m_Amax;
		pTree->m_n0 = m_n0;
		//pTree->Initialise(m_nbObjects, peakDate, m_attackStDev, OVIPOSITING_ADULT, true, 0);
		//pTree->Initialize(attacks, OVIPOSITING_ADULT, true, 0);
		pTree->Initialize<CMountainPineBeetle>(attacks);


		stand.SetTree(pTree);
		stand.Init(m_weather);


		for (size_t y = y1; y <= y2; y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM::DAILY);
			for (CTRef d = p.Begin(); d <= p.End(); d++)
			{
				//tree and bugs live for the day
				stand.Live(m_weather.GetDay(d));

				if (bAllGeneration)
				{
					//get state of bugs in tree for the day
					stand.GetStat(d, stat[0][d], -1);
				}
				else
				{
					for (size_t g = 0; g < stat.size(); g++)
						stand.GetStat(d, stat[g][d], g + 1);
				}

				if (m_bAutoBalanceObject)
					stand.AdjustPopulation();

				HxGridTestConnection();
			}

			stand.HappyNewYear();

			if (stand.GetNbObjectAlive() == 0)
				break;
		}
	}

	void CMPBiModel::ComputeRegularValue(CMPBStatVector& stat, CDailyOutputVector& output)
	{
		//output.Init( stat.size(), stat.GetFirstTRef() );

		if (stat.empty())
			return;

		ASSERT(output.GetTPeriod().IsInside(stat.GetTPeriod()));
		//double cumulKm²=m_initialInfestation;

		for (CTRef d = stat.GetFirstTRef(); d <= stat.GetLastTRef(); d++)
		{
			size_t c = 0;//the current output variable

			for (size_t j = S_EGG; j <= S_DEAD_ADULT; j++)
				output[d][c++] += stat[d][j];

			output[d][c++] += stat[d][E_BROOD];
			output[d][c++] += stat[d][E_NB_ATTACKS];
			output[d][c++] += stat[d][E_NB_SUCESS_ATTACKS];
			output[d][c++] += stat[d][E_NB_INFESTED_TREE] / m_forestDensity;
			//cumulKm² += stat[d][E_NB_INFESTED_TREE]/m_forestDensity;
			output[d][c++] += 0;//cumulKm²;///m_forestSize*100;
			output[d][c++] += stat[d][S_DEAD_BY_TREE];
			output[d][c++] += stat[d][S_DEAD_DENSITY_DEPENDENCE];
			output[d][c++] += stat[d][S_DEAD_ATTRITION];
			output[d][c++] += stat[d][DF_EGG];
			output[d][c++] += stat[d][DF_L1] + stat[d][DF_L2] + stat[d][DF_L3] + stat[d][DF_L4];
			output[d][c++] += stat[d][DF_PUPA];
			output[d][c++] += stat[d][DF_TENERAL_ADULT];
			output[d][c++] += stat[d][DF_OVIPOSITING_ADULT];
			output[d][c++] += stat[d][S_BIVOLTIN];
			output[d][c++] += stat[d][S_UNIVOLTIN];
			output[d][c++] += stat[d][S_SEMIVOLTIN];
			output[d][c++] += stat[d][S_TRIENVOLTIN];
			output[d][c++] += stat[d][S_NB_OBJECT_ALIVE];
			_ASSERTE(c == NB_DAILY_OUTPUT - 2);

			if (stat[d][S_NB_OBJECT] > 0)
			{
				output[d][c++] += stat[d][S_NB_PACK] / stat[d][S_NB_OBJECT];
				output[d][c++] += stat[d][S_DD_FACTOR] / stat[d][S_NB_OBJECT];
				_ASSERTE(c == NB_DAILY_OUTPUT);
			}
		}
	}

	/*
	void CMPBiModel::ComputeCumulativeValue(CMPBStatVector& stat, CDailyOutputVector& output)
	{
	if(stat.empty() )
	return;

	ASSERT( output.GetTPeriod().IsInside( stat.GetTPeriod() ) );

	//double cumulKm²=m_initialInfestation;
	for (CTRef d=stat.GetFirstTRef(); d<=stat.GetLastTRef(); d++)
	{
	int c=0;//the current output variable

	for (int j=S_EGG; j<=S_DEAD_ADULT; j++)
	output[d][c++] += stat[d][j];

	output[d][c++] += stat[d][E_BROOD];
	output[d][c++] += stat[d][E_NB_ATTACKS];
	output[d][c++] += stat[d][E_NB_SUCESS_ATTACKS];
	output[d][c++] += stat[d][E_NB_INFESTED_TREE]/m_forestDensity;
	//cumulKm² += stat[d][E_NB_INFESTED_TREE]/m_forestDensity;
	output[d][c++] = 0;//cumulKm²/m_forestSize*100;
	output[d][c++] += stat[d][S_DEAD_BY_TREE];
	output[d][c++] += stat[d][S_DEAD_DENSITY_DEPENDENCE];
	output[d][c++] += stat[d][S_DEAD_ATTRITION];
	output[d][c++] += stat[d][DF_EGG];
	output[d][c++] += stat[d][DF_L1]+stat[d][DF_L2]+stat[d][DF_L3]+stat[d][DF_L4];
	output[d][c++] += stat[d][DF_PUPA];
	output[d][c++] += stat[d][DF_TENERAL_ADULT];
	output[d][c++] += stat[d][DF_OVIPOSITING_ADULT];
	output[d][c++] += stat[d][S_BIVOLTIN];
	output[d][c++] += stat[d][S_UNIVOLTIN];
	output[d][c++] += stat[d][S_SEMIVOLTIN];
	output[d][c++] += stat[d][S_TRIENVOLTIN];
	output[d][c++] += stat[d][S_NB_OBJECT_ALIVE];
	_ASSERTE( c == NB_DAILY_OUTPUT-2 );

	if( stat[d][S_NB_OBJECT]>0)
	{
	output[d][c++] += stat[d][S_NB_PACK]/stat[d][S_NB_OBJECT];
	output[d][c++] += stat[d][S_DD_FACTOR]/stat[d][S_NB_OBJECT];
	_ASSERTE( c == NB_DAILY_OUTPUT );
	}
	}
	}
	*/
	void CMPBiModel::ComputeRegularValue(CMPBStatVector& stat, CAnnualOutputVector& output)
	{
		output.Init(m_weather.GetNbYears() - 1, CTRef(m_weather[(size_t)1].GetTRef().GetYear()));
		if (stat.empty())
			return;

		double lastAttack = m_totalInitialAttack;
		double lastAttackKm² = m_initialInfestation;

		for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		{
			CStatistic outputStat[NB_ANNUAL_OUTPUT];
			CStatistic totalBrood;
			CStatistic totalFemale;
			CStatistic longivity;

			CTPeriod p = m_weather[y + 1].GetEntireTPeriod(CTM::DAILY);
			CTRef firstDay = p.Begin(); // m_weather[y + 1].GetFirstTRef();
			CTRef lastDay = p.End(); // m_weather[y + 1].GetLastTRef();
			for (CTRef d = firstDay; d <= lastDay; d++)
			{
				if (stat[d][S_NB_OBJECT] > 0)
				{
					if (stat[d][S_NB_OBJECT_ALIVE] > 0)
					{
						outputStat[A_NB_OBJECT_ALIVE] += stat[d][S_NB_OBJECT_ALIVE];
					}

					outputStat[A_EGG_CREATED] += stat[d][E_EGG];
					outputStat[A_BROOD] += stat[d][E_BROOD];
					totalBrood += stat[d][E_TOTAL_BROOD];
					totalFemale += stat[d][E_TOTAL_FEMALE];
					longivity += stat[d][E_LONGIVITY];
					outputStat[A_DEAD_FROZEN] += stat[d][E_DEAD_FROZEN];
					outputStat[A_DEAD_OLD_AGE] += stat[d][E_DEAD_OLD_AGE];
					outputStat[A_DEAD_BY_TREE] += stat[d][E_DEAD_BY_TREE];
					outputStat[A_DEAD_DENSITY_DEPENDENCE] += stat[d][E_DEAD_DENSITY_DEPENDENCE];
					outputStat[A_DEAD_ATTRITION] += stat[d][E_DEAD_ATTRITION];
					outputStat[A_EGG_DEAD_FROZEN] += stat[d][DF_EGG];
					outputStat[A_LARVAL_DEAD_FROZEN] += stat[d][DF_L1] + stat[d][DF_L2] + stat[d][DF_L3] + stat[d][DF_L4];
					outputStat[A_PUPA_DEAD_FROZEN] += stat[d][DF_PUPA];
					outputStat[A_TENERAL_DEAD_FROZEN] += stat[d][DF_TENERAL_ADULT];
					outputStat[A_OVIPADULT_DEAD_FROZEN] += stat[d][DF_OVIPOSITING_ADULT];
					outputStat[A_BIVOLTIN] += stat[d][S_BIVOLTIN];
					outputStat[A_UNIVOLTIN] += stat[d][S_UNIVOLTIN];
					outputStat[A_SEMIVOLTIN] += stat[d][S_SEMIVOLTIN];
					outputStat[A_TRIENVOLTIN] += stat[d][S_TRIENVOLTIN];

					outputStat[A_ATTACK] += stat[d][E_NB_ATTACKS];
					outputStat[A_SUCCESS_ATTACK] += stat[d][E_NB_SUCESS_ATTACKS];
					outputStat[A_INFESTED_KM²] += stat[d][E_NB_INFESTED_TREE] / m_forestDensity;

					outputStat[A_DD_FACTOR] += stat[d][S_DD_FACTOR] / stat[d][S_NB_OBJECT];
					outputStat[A_NB_PACK] += stat[d][S_NB_PACK] / stat[d][S_NB_OBJECT];

					if (stat[d][E_DEAD_FROZEN] > 0)
						outputStat[A_LONGIVITY] += stat[d][E_LONGIVITY] / stat[d][E_DEAD_FROZEN];

				}
			}

			//This version of the model outputs an annual "growth rate"
			//which is the ratio of effectiveBeetles(t+1)/effectiveBeetles(t)
			output[y][A_SUCCESS_ATTACK] = outputStat[A_SUCCESS_ATTACK][SUM];
			output[y][A_ATTACK] = outputStat[A_ATTACK][SUM];
			output[y][A_R] = (lastAttack > 0 && outputStat[A_SUCCESS_ATTACK][NB_VALUE] > 0) ? outputStat[A_SUCCESS_ATTACK][SUM] / lastAttack : -9999; lastAttack = outputStat[A_SUCCESS_ATTACK][SUM];

			output[y][A_EGG] = stat[firstDay][S_EGG];
			output[y][A_LARVAL] = stat[firstDay][S_L1] + stat[firstDay][S_L2] + stat[firstDay][S_L3] + stat[firstDay][S_L4];
			output[y][A_PUPA] = stat[firstDay][S_PUPA];
			output[y][A_TENERAL] = stat[firstDay][S_TENERAL_ADULT];
			output[y][A_OVIPADULT] = stat[firstDay][S_OVIPOSITING_ADULT];
			output[y][A_EGG_CREATED] = outputStat[A_EGG_CREATED][SUM];
			output[y][A_BROOD] = outputStat[A_BROOD][SUM];
			output[y][A_FECONDITY] = totalFemale[SUM] > 0 ? totalBrood[SUM] / totalFemale[SUM] : -9999;
			output[y][A_DEAD_FROZEN] = outputStat[A_DEAD_FROZEN][SUM];
			output[y][A_DEAD_OLD_AGE] = outputStat[A_DEAD_OLD_AGE][SUM];
			output[y][A_DEAD_BY_TREE] = outputStat[A_DEAD_BY_TREE][SUM];
			output[y][A_DEAD_DENSITY_DEPENDENCE] = outputStat[A_DEAD_DENSITY_DEPENDENCE][SUM];
			output[y][A_DEAD_ATTRITION] = outputStat[A_DEAD_ATTRITION][SUM];
			output[y][A_INFESTED_KM²] = outputStat[A_INFESTED_KM²][SUM];

			output[y][A_EGG_DEAD_FROZEN] = outputStat[A_EGG_DEAD_FROZEN][SUM];
			output[y][A_LARVAL_DEAD_FROZEN] = outputStat[A_LARVAL_DEAD_FROZEN][SUM];
			output[y][A_PUPA_DEAD_FROZEN] = outputStat[A_PUPA_DEAD_FROZEN][SUM];
			output[y][A_TENERAL_DEAD_FROZEN] = outputStat[A_TENERAL_DEAD_FROZEN][SUM];
			output[y][A_OVIPADULT_DEAD_FROZEN] = outputStat[A_OVIPADULT_DEAD_FROZEN][SUM];
			output[y][A_BIVOLTIN] = outputStat[A_BIVOLTIN][SUM];
			output[y][A_UNIVOLTIN] = outputStat[A_UNIVOLTIN][SUM];
			output[y][A_SEMIVOLTIN] = outputStat[A_SEMIVOLTIN][SUM];
			output[y][A_TRIENVOLTIN] = outputStat[A_TRIENVOLTIN][SUM];
			output[y][A_NB_OBJECT_ALIVE] = outputStat[A_NB_OBJECT_ALIVE][MEAN];
			output[y][A_NB_PACK] = outputStat[A_NB_PACK][MEAN];
			output[y][A_DD_FACTOR] = outputStat[A_DD_FACTOR][MEAN];
			output[y][A_LONGIVITY] = totalFemale[SUM] > 0 ? longivity[SUM] / totalFemale[SUM] : -9999;
			//		output[y][A_MONTH_PEAK] = m_peakDate.GetMonth()+1;
			//		output[y][A_DAY_PEAK] = m_peakDate.GetDay()+1;
		}
	}

	void CMPBiModel::ComputeGenerationValue(CMPBStatVector& stat, CModelStat& output)
	{
		if (stat.empty())
			return;


		CStatistic outputStat[NB_ANNUAL_OUTPUT];
		CStatistic totalBrood;
		CStatistic totalFemale;
		CStatistic longivity;


		CTRef firstDay;

		for (CTRef d = stat.GetFirstTRef(); d <= stat.GetLastTRef(); d++)
		{
			if (stat[d][S_NB_OBJECT] > 0)
			{
				if (stat[d][S_NB_OBJECT_ALIVE] > 0)
				{
					outputStat[A_NB_OBJECT_ALIVE] += stat[d][S_NB_OBJECT_ALIVE];
				}

				outputStat[A_EGG_CREATED] += stat[d][E_EGG];
				outputStat[A_BROOD] += stat[d][E_BROOD];
				totalBrood += stat[d][E_TOTAL_BROOD];
				totalFemale += stat[d][E_TOTAL_FEMALE];
				longivity += stat[d][E_LONGIVITY];
				outputStat[A_DEAD_FROZEN] += stat[d][E_DEAD_FROZEN];
				outputStat[A_DEAD_OLD_AGE] += stat[d][E_DEAD_ADULT];
				outputStat[A_DEAD_BY_TREE] += stat[d][E_DEAD_BY_TREE];
				outputStat[A_DEAD_DENSITY_DEPENDENCE] += stat[d][E_DEAD_DENSITY_DEPENDENCE];
				outputStat[A_DEAD_ATTRITION] += stat[d][E_DEAD_ATTRITION];
				outputStat[A_EGG_DEAD_FROZEN] += stat[d][DF_EGG];
				outputStat[A_LARVAL_DEAD_FROZEN] += stat[d][DF_L1] + stat[d][DF_L2] + stat[d][DF_L3] + stat[d][DF_L4];
				outputStat[A_PUPA_DEAD_FROZEN] += stat[d][DF_PUPA];
				outputStat[A_TENERAL_DEAD_FROZEN] += stat[d][DF_TENERAL_ADULT];
				outputStat[A_OVIPADULT_DEAD_FROZEN] += stat[d][DF_OVIPOSITING_ADULT];
				outputStat[A_BIVOLTIN] += stat[d][S_BIVOLTIN];
				outputStat[A_UNIVOLTIN] += stat[d][S_UNIVOLTIN];
				outputStat[A_SEMIVOLTIN] += stat[d][S_SEMIVOLTIN];
				outputStat[A_TRIENVOLTIN] += stat[d][S_TRIENVOLTIN];
				outputStat[A_SUCCESS_ATTACK] += stat[d][E_NB_SUCESS_ATTACKS];
				outputStat[A_ATTACK] += stat[d][E_NB_ATTACKS];
				outputStat[A_INFESTED_KM²] += stat[d][E_NB_INFESTED_TREE] / m_forestDensity;
				outputStat[A_DD_FACTOR] += stat[d][S_DD_FACTOR] / stat[d][S_NB_OBJECT];
				outputStat[A_NB_PACK] += stat[d][S_NB_PACK] / stat[d][S_NB_OBJECT];
			}
		}


		double lastAttack = m_totalInitialAttack;
		output[A_R] = lastAttack > 0 ? outputStat[A_SUCCESS_ATTACK][SUM] / lastAttack : -9999;
		output[A_ATTACK] = outputStat[A_ATTACK][SUM];
		output[A_SUCCESS_ATTACK] = outputStat[A_SUCCESS_ATTACK][SUM];
		output[A_EGG_CREATED] = outputStat[A_EGG_CREATED][SUM];
		output[A_BROOD] = outputStat[A_BROOD][SUM];
		output[A_FECONDITY] = totalFemale[SUM] > 0 ? totalBrood[SUM] / totalFemale[SUM] : -9999;
		output[A_DEAD_FROZEN] = outputStat[A_DEAD_FROZEN][SUM];
		output[A_DEAD_OLD_AGE] = outputStat[A_DEAD_OLD_AGE][SUM];
		output[A_DEAD_BY_TREE] = outputStat[A_DEAD_BY_TREE][SUM];
		output[A_DEAD_DENSITY_DEPENDENCE] = outputStat[A_DEAD_DENSITY_DEPENDENCE][SUM];
		output[A_DEAD_ATTRITION] = outputStat[A_DEAD_ATTRITION][SUM];
		output[A_INFESTED_KM²] = outputStat[A_INFESTED_KM²][SUM];
		output[A_EGG_DEAD_FROZEN] = outputStat[A_EGG_DEAD_FROZEN][SUM];
		output[A_LARVAL_DEAD_FROZEN] = outputStat[A_LARVAL_DEAD_FROZEN][SUM];
		output[A_PUPA_DEAD_FROZEN] = outputStat[A_PUPA_DEAD_FROZEN][SUM];
		output[A_TENERAL_DEAD_FROZEN] = outputStat[A_TENERAL_DEAD_FROZEN][SUM];
		output[A_OVIPADULT_DEAD_FROZEN] = outputStat[A_OVIPADULT_DEAD_FROZEN][SUM];
		output[A_BIVOLTIN] = outputStat[A_BIVOLTIN][SUM];
		output[A_UNIVOLTIN] = outputStat[A_UNIVOLTIN][SUM];
		output[A_SEMIVOLTIN] = outputStat[A_SEMIVOLTIN][SUM];
		output[A_TRIENVOLTIN] = outputStat[A_TRIENVOLTIN][SUM];
		output[A_NB_OBJECT_ALIVE] = outputStat[A_NB_OBJECT_ALIVE][MEAN];
		output[A_NB_PACK] = outputStat[A_NB_PACK][MEAN];
		output[A_DD_FACTOR] = outputStat[A_DD_FACTOR][MEAN];
		output[A_LONGIVITY] = totalFemale[SUM] > 0 ? longivity[SUM] / totalFemale[SUM] : -9999;
		//	output[A_MONTH_PEAK] = m_peakDate.GetMonth()+1;
		//	output[A_DAY_PEAK] = m_peakDate.GetDay()+1;
	}

	//**************************
	//this method is called to load parameters in variables
	ERMsg CMPBiModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameters here
		size_t cur = 0;
		m_n0 = parameters[cur++].GetInt();//Number of females/m² in the initial attack 
		size_t m = parameters[cur++].GetInt();// peak Date of initial attack 
		size_t d = parameters[cur++].GetInt() - 1;//and its standard deviation

		//CTRef peakDate = CTRef( m_weather.GetFirstYear(), m, d);
		m_peakDate = CTRef(m_weather.GetFirstYear(), m, d);
		m_attackStDev = parameters[cur++].GetReal();
		//	double attackStDev = parameters[cur++].GetReal();


		m_nbFertilGeneration = parameters[cur++].GetInt();                   //Turns brood female fertility on/off
		m_bFertilEgg = m_nbFertilGeneration != 0;
		//m_bFertilEgg= parameters[cur++].GetBool();                   //Turns brood female fertility on/off
		m_bAutoBalanceObject = parameters[cur++].GetBool();           //Creates "super individual" objects to accelerate model
		m_survivalRate = parameters[cur++].GetReal();                //Basic survival rate (applied to eggs before their creation). A 70% female SR is applied (no males in these simulations)
		m_A0 = parameters[cur++].GetReal();                          //Tree defense capacity (attacks/m²/day)
		m_Amax = parameters[cur++].GetReal();                        //Maximum adult beetles (successful attacks) allowed/m²
		m_applyColdTolerance = parameters[cur++].GetBool();           //Apply cold mortality (all stages)
		m_applyColdToleranceOvipAdult = parameters[cur++].GetInt();  //Apply cold mortality of ovipositing adults (Lester & Irwin 2012)
		m_emergenceThreshold = parameters[cur++].GetReal();           // Minimum temperature needed for adult emergence. Leads to additional mortality for adults not able to emerge before winter
		m_bMicroClimate = parameters[cur++].GetBool();                //Calculate bark temperature from air temperature
		m_forestSize = parameters[cur++].GetReal();                  //Forest size (km²)
		m_nbObjects = parameters[cur++].GetInt();                    //nb itial objects
		m_forestDensity = parameters[cur++].GetReal();               //Number of trees per unit land surface area (ha, km²?)
		m_bSnowProtection = parameters[cur++].GetBool();             //Creating a cold-mortality refuge due to snow cover
		m_bUseDefenselessTree = parameters[cur++].GetBool();         //In low density (total bug < A0), beetles can attack these moribund trees to subsist
		m_OptimizeOn = parameters[cur++].GetInt();                   //R, Km² or P
		m_initialInfestation = parameters[cur++].GetReal();			 //Initial infestation (km²)
		m_DDAlpha = parameters[cur++].GetReal();					 //Density dependence exponent

		m_minObjects = parameters[cur++].GetInt();                   //Minimum number of objects alive in the simulator
		m_maxObjects = parameters[cur++].GetInt();                   //Maximum number of objects alive in the simulator
		m_nbObjects = min(m_maxObjects, max(m_minObjects, m_nbObjects));
		//int nbObjects  = Min( m_maxObjects, Max( m_minObjects, m_nbObjects) );

		m_heightMax = parameters[cur++].GetReal();
		m_pupeTeneralColdT = parameters[cur++].GetReal();
		//int nbRunMax  = parameters[cur++].GetInt();



		if (m_initialInfestation*m_forestDensity < 1)
			m_initialInfestation = 1 / m_forestDensity;

		double nbTrees = m_initialInfestation*m_forestDensity;
		m_totalInitialAttack = CMPBTree::GetNbInitialAttack(nbTrees, m_Amax, m_n0);

		m_attacks.Initialize(m_peakDate, m_attackStDev, m_nbObjects, m_totalInitialAttack, OVIPOSITING_ADULT, FEMALE);


		//Peak initial attack date +- 3.5*attackVariance must be sometime in first year 
		//_ASSERTE((m_peakDate - int(3 * m_attackStDev)) >= m_weather[0].GetFirstTRef());
		//_ASSERTE((m_peakDate + int(3 * m_attackStDev)) <= m_weather[0].GetLastTRef());

		return msg;
	}

	//*****************************************************************************************************
	//Simulated Annealing
	void CMPBiModel::AddSAResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(header.size() == 5);
		ASSERT(header[0] == "STATION_ID");
		ASSERT(header[1] == "Year");
		ASSERT(header[2] == "R");
		ASSERT(header[3] == "km²");
		ASSERT(header[4] == "P");


		CTRef ref(ToInt(data[1]));

		std::vector<double> obs(3);
		for (size_t i = 0; i < 3; i++)
			obs[i] = ToDouble(data[i + 2]);

		m_SAResult.push_back(CSAResult(ref, obs));
	}

	void CMPBiModel::GetFValueAnnual(CStatisticXY& stat)
	{
		//Here the variable used for optimization can be changed
		ERMsg msg;

		if (m_SAResult.size() > 0)
		{
			OnExecuteAnnual();

			for (size_t k = 0; k < m_SAResult.size(); k++)
			{
				if (m_output.IsInside(m_SAResult[k].m_ref))
				{
					if (m_OptimizeOn == 0) //R 
					{
						double obs = m_SAResult[k].m_obs[0];
						double sim = m_output[m_SAResult[k].m_ref][A_R];
						if (obs > -999 && sim > -9999)
							stat.Add(sim, obs);
					}
					if (m_OptimizeOn == 1) //log(R)
					{
						double obs = m_SAResult[k].m_obs[0];
						double sim = m_output[m_SAResult[k].m_ref][A_R];
						if (obs > -999 && sim > -9999)
							stat.Add(log(sim + 0.001), log(obs + 0.001));
					}
					if (m_OptimizeOn == 2) //Km²
					{
						double obs = m_SAResult[k].m_obs[1];
						double sim = m_output[m_SAResult[k].m_ref][A_INFESTED_KM²];

						if (obs > -999 && sim > -9999)
							stat.Add(sim, obs);
					}
					if (m_OptimizeOn == 3) //log(Km²)
					{
						double obs = m_SAResult[k].m_obs[1];
						double sim = m_output[m_SAResult[k].m_ref][A_INFESTED_KM²];

						if (obs > -999 && sim > -9999)
							stat.Add(log(sim + 1), log(obs + 1));
					}
					if (m_OptimizeOn == 4) //P
					{
						double obs = m_SAResult[k].m_obs[2];
						double sim = m_output[m_SAResult[k].m_ref][A_INFESTED_KM²_CUMUL];
						if (obs > -999 && sim > -9999)
							stat.Add(sim, obs);
					}
					if (m_OptimizeOn == 5) //log(P)
					{
						double obs = m_SAResult[k].m_obs[2];
						double sim = m_output[m_SAResult[k].m_ref][A_INFESTED_KM²_CUMUL];
						if (obs > -999 && sim > -9999)
							stat.Add(log(sim + 0.01), log(obs + 0.01));
					}
				}
			}

			if (stat[NB_VALUE] < m_SAResult.size() - 1)
				stat.Reset();


		}
	}


	void CMPBiModel::FinalizeStat(CStatisticXY& stat)
	{
		if (stat[NB_VALUE] < m_info.m_repCounter.GetTotal()*(m_SAResult.size() - 1))
			stat.Reset();
	}

}