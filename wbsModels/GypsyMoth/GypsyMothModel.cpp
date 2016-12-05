//************************************************************************************
//RSA 16/11/2016:	Compile for BioSIM 11
//RSA 25/01/2012:	New model with multi generation
//					Add deadAdult
//					use double instead of float
//RSA 09/03/2011:	new compilation with new BioSIMModelBase, new interface
//					Add switch to activate or not new gray equation
//					The model for south hemisphere don't work anymore
//RSA 17/11/2008: Recompilation with new gray egg equation
//RSA 14/07/2005: Model ajust ovipDate to simulate the good year
//RSA 18/05/2005: integretion to BioSIMModelBase + cleaning
//JR  13/05/2005: harmonized this Gypmphen with the Stability version.
//
//JR  02/03/1999: made the model output one line on day 273, if first hatch occurs 
//           on or after 273. That is to ensure output
//		   files are not empty. BioSIM does not like empty output files.
//
//JR  24/03/1999: added function Reset() in Gymphen.cpp to initialize arrays
//		   in an attempt to solve the DLL vs EXE problem
//JR  25/03/1999: added function free_arrays() to free allocated global arrays 
//			(currently only **hatching). Johnson() also allocates (and frees) 
//			*day_deg.
//JR  26/04/1999: added Eggs_left to replace eggs_hatching[][] as an output variable
//			because hatching rate is hard to interpret graphically.
//JR  23/09/1999: Started implementing Sawyer et al's model...
//************************************************************************************

#include "GypsyMothModel.h"
#include "GypsyMoth.h"
#include "ModelBase/EntryPoint.h"

using namespace std;

namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CGypsyMothModel::CreateObject);


	CGypsyMothModel::CGypsyMothModel()
	{
		// initialize your variables here (optional)
		NB_INPUT_PARAMETER = 6;
		VERSION = "2.3.1 (2016)";

		m_hatchModelType = CGypsyMoth::GRAY_MODEL;
		m_bHaveAttrition = false;
		m_outputStyle = REGULAR;
		m_takePreviousOvipDate = false;

	}

	CGypsyMothModel::~CGypsyMothModel()
	{
	}

	//This method is called to compute solution
	ERMsg CGypsyMothModel::OnExecuteDaily()
	{
		ERMsg msg;

		CGMOutputVector stat;
		ExecuteDaily(stat);

		COutputVector output;

		//fill output matrix
		if (m_outputStyle == REGULAR)
			ComputeRegularValue(stat, output);
		else 
			ComputeCumulativeValue(stat, output);

		//Set output to the framework
		SetOutput(output);

		return msg;
	}

	void CGypsyMothModel::ExecuteDaily(CGMOutputVector& stat)
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//Set global class variables
		CGypsyMoth::SetApplyMortality(m_bHaveAttrition);

		CTPeriod period = m_weather.GetEntireTPeriod(CTM::DAILY);
		period.Begin().m_year++;

		stat.Init(period);

		period = m_weather.GetEntireTPeriod(CTM::DAILY);
		

		CGMEggParam eggParamTmp = m_eggParam;
		//first: do the generation over the first two years
		//if( m_bConditionOvipDate )
		//eggParamTmp.m_ovipDate = GetInitialOvipDate(); 

		//Do simulation only if the ovip date of the first generation 
		//if( eggParamTmp.m_ovipDate.IsInit() )
		//{
		for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		{
			CGypsyMoth gypsyMoth(m_hatchModelType, eggParamTmp);

			CTPeriod p(period.GetFirstAnnualTRef(y), period.GetLastAnnualTRef(y+1));

			//simulate development
			gypsyMoth.SimulateDeveloppement(m_weather, p);

			//Attention, ici on prend les output même s'il ne sont pas viable...???
			//Get output of the second year
			gypsyMoth.GetOutputStat(stat);


			if (!m_takePreviousOvipDate)
			{
				eggParamTmp.m_ovipDate.m_year = m_weather[y + 1].GetTRef().GetYear();//update year
			}
			else
			{
				//if the new date is OK, we take it, else we take the last valid date
				if (gypsyMoth.GetViabilityFlag())
				{
					eggParamTmp.m_ovipDate = gypsyMoth.GetNewOvipDate();
				}
				else
				{
					//take the last valid date and continue with this date
					eggParamTmp.m_ovipDate.m_year = m_weather[y + 1].GetTRef().GetYear();//update year
				}
			}

			//update intial date to the current year
			//m_eggParam.m_ovipDate.m_year++;
		}

	}

	//void CGypsyMothModel::ExecuteDaily(CGMOutputVector& stat)
	//{
	//	//Set global class variables
	//	CGypsyMoth::SetApplyMortality(m_bHaveAttrition);
	//	
	//	stat.Init(m_weather.GetNbDay()-m_weather[0].GetNbDay(), m_weather[1].GetFirstTRef());
	//
	//	CGMEggParam eggParamTmp = m_eggParam;
	//	
	//	int nbGenerations = 10;
	//	CTRef lastOvipDate;
	//	//CStatistic ovipJD;
	//	for(int g=0; g<nbGenerations&&eggParamTmp.m_ovipDate.GetJDay()!=lastOvipDate.GetJDay(); g++ ) 
	//	{
	//		//int y = yy%(m_weather.GetNbYear()-1);
	//
	//		CTPeriod p( m_weather[0].GetFirstTRef(), m_weather[1].GetLastTRef() );
	//		//simulate developement
	//		CGypsyMoth gypsyMoth(m_hatchModelType, eggParamTmp);
	//		gypsyMoth.SimulateDeveloppement(m_weather, p);
	//		gypsyMoth.GetOutputStat(stat);
	//
	//		if(gypsyMoth.GetViabilityFlag())
	//		{
	//			//take new computed value
	//			//ovipJD += gypsyMoth.GetNewOvipDate().GetJDay();
	//			lastOvipDate = eggParamTmp.m_ovipDate;
	//			eggParamTmp.m_ovipDate.SetJDay( gypsyMoth.GetNewOvipDate().GetJDay() );
	//		}
	//		else
	//		{
	//			//take original value		
	//			//int nextY = (yy+1)%(m_weather.GetNbYear()-1);
	//			//eggParamTmp.m_ovipDate = m_eggParam.m_ovipDate;
	//			//eggParamTmp.m_ovipDate.m_year = m_weather[nextY].GetYear();//update year to the next year
	////			for(CTRef d=m_weather[y+1].GetFirstTRef(); d<=stat.GetLastTRef(); d++)
	//	//			stat[d][EGG] = 100;
	//			//for(int d=0; d<stat.size(); d++)
	//				//stat[d][EGG] = 100;
	//
	//			g = 10; 
	//		}
	//		
	//		//gypsyMoth.SetEggParam(eggParamTmp);
	//		//
	//		
	//	}
	//
	//	//CTRef newOvipDate;
	//	//if(ovipJD[NB_VALUE] )
	//		//newOvipDate = m_weather.GetFirstTRef() + Round(ovipJD[MEAN]);
	//
	//	
	//	//return newOvipDate;
	//}

	//CTRef CGypsyMothModel::GetInitialOvipDate()
	//{
	//	CGMEggParam eggParamTmp = m_eggParam;
	//	
	//	int nbGenerations = 10;
	//	CStatistic ovipJD;
	//	for(int g=0, yy=0; g<nbGenerations&&ovipJD[NB_VALUE]<5; g++, yy++)//newOvipDate.GetJDay()!=eggParamTmp.m_ovipDate.GetJDay(); g++ ) 
	//	{
	//		int y = yy%(m_weather.GetNbYear()-1);
	//
	//		CTPeriod p( m_weather[y].GetFirstTRef(), m_weather[y+1].GetLastTRef() );
	//		//simulate developement
	//		CGypsyMoth gypsyMoth(m_hatchModelType, eggParamTmp);
	//		gypsyMoth.SimulateDeveloppement(m_weather, p);
	//
	//		if(gypsyMoth.GetViabilityFlag())
	//		{
	//			//take new computed value
	//			ovipJD += gypsyMoth.GetNewOvipDate().GetJDay();
	//			eggParamTmp.m_ovipDate = gypsyMoth.GetNewOvipDate();
	//		}
	//		else
	//		{
	//			//take original value		
	//			int nextY = (yy+1)%(m_weather.GetNbYear()-1);
	//			eggParamTmp.m_ovipDate = m_eggParam.m_ovipDate;
	//			eggParamTmp.m_ovipDate.m_year = m_weather[nextY].GetYear();//update year to the next year
	//		}
	//		
	//		//gypsyMoth.SetEggParam(eggParamTmp);
	//		//
	//		
	//	}
	//
	//	CTRef newOvipDate;
	//	if(ovipJD[NB_VALUE] )
	//		newOvipDate = m_weather.GetFirstTRef() + Round(ovipJD[MEAN]);
	//
	//	
	//	return newOvipDate;
	//}


	void CGypsyMothModel::ComputeRegularValue(const CGMOutputVector& stat, COutputVector& output)
	{
		output.Init(stat.size(), stat.GetFirstTRef());

		for (CTRef d = output.GetFirstTRef(); d <= stat.GetLastTRef(); d++)
		{
			output[d][O_EGG] = stat[d][EGG];
			output[d][O_L1] = stat[d][L1];
			output[d][O_L2] = stat[d][L2];
			output[d][O_L3] = stat[d][L3];
			output[d][O_L4] = stat[d][L4];
			output[d][O_L5] = stat[d][L5];
			output[d][O_L6] = stat[d][L6];
			output[d][O_PUPAE] = stat[d][PUPAE];
			output[d][O_ADULT] = stat[d][ADULT];
			output[d][O_DEAD_ADULT] = stat[d][DEAD_ADULT];
			output[d][O_MALE_MOTH] = stat[d][MALE];
			output[d][O_FEMALE_MOTH] = stat[d][FEMALE];
			//		output[d][O_MALE_EMERGED]=stat[d][MALE_EMERGED];
			//	output[d][O_FEMALE_EMERGED]=stat[d][FEMALE_EMERGED];
			//if(d.m_month == DECEMBER && d.m_day == 30)
			//{
			//	//Test: force the last line to be termiated
			//	for(int i=EGG; i<TOT_POP; i++)
			//		output[d][i] = 0;
			//	
			//	output[d][DEAD_ADULT] = 100;
			//}
		}

	}

	void CGypsyMothModel::ComputeCumulativeValue(const CGMOutputVector& stat, COutputVector& output)
	{
		output.Init(stat.size(), stat.GetFirstTRef());


		CTPeriod p = stat.GetTPeriod();
		for (size_t y = 0; y < p.GetNbYears(); y++)
		{
			CTPeriod p2 = p.GetAnnualPeriodByIndex(y);
			double sumMale = stat.GetStat(MALE, p2)[SUM];
			double sumFemale = stat.GetStat(FEMALE, p2)[SUM];
			//double sumMale2 = stat.GetStat(MALE_EMERGED, p2)[SUM];
			//double sumFemale2 = stat.GetStat(FEMALE_EMERGED, p2)[SUM];

			for (CTRef d = p2.Begin(); d <= p2.End(); d++)
			{
				bool firstDay = d == p2.Begin();

				//double totPop=stat[d][EGG]+stat[d][TOT_POP]; 


				//cumulative frequencies (as a %)
				double cumFreq[NB_STAGE] = { 0 };
				cumFreq[0] = stat[d][EGG];

				double totPop = stat[d][EGG];
				//double sumMale = 0;
				//double sumFemale = 0;
				//sumMale+=stat[d][MALE];
				//sumFemale+=stat[d][FEMALE];

				for (int j = L1; j <= DEAD_ADULT; j++)
				{
					totPop += stat[d][j];

					for (int jj = j; jj <= DEAD_ADULT; jj++)
						cumFreq[j] += stat[d][jj];
				}

				for (int j = EGG; j <= DEAD_ADULT; j++)
				{
					if (totPop > 0)
						output[d][j] = 100 * cumFreq[j] / totPop;
					else 
						output[d][j] = firstDay ? 0 : output[d - 1][j];
				}

				////% cumulative catch of male/female moths
				if (!firstDay)//assume we don't have male and female the first day
				{
					if (sumMale > 0.0001)
						output[d][O_MALE_MOTH] = output[d - 1][O_MALE_MOTH] + 100 * stat[d][MALE] / sumMale;

					if (sumFemale > 0.0001)
						output[d][O_FEMALE_MOTH] = output[d - 1][O_FEMALE_MOTH] + 100 * stat[d][FEMALE] / sumFemale;

					//if(sumMale2>0) 
					//output[d][O_MALE_EMERGED]=output[d-1][O_MALE_EMERGED] + (100*stat[d][MALE_EMERGED])/sumMale2;

					//if(sumFemale2>0) 
					//output[d][O_FEMALE_EMERGED]=output[d-1][O_FEMALE_EMERGED] + (100*stat[d][FEMALE_EMERGED])/sumFemale2;
				}
			}

			//Test: force the last line to be termiated
			//for(CTRef d=p2.End()-46; d<=p2.End(); d++)
			//{
			//	output[d][EGG] = 0;
			//	for(int i=L1; i<TOT_POP; i++)
			//		output[d][i] = 100;
			//}
		}



	}

	/*

	void CGypsyMothModel::ComputeRegularValue(const CGypsyMoth& gypsyMoth, COutputVector& output)
	{
	const CEggModel& hatch = gypsyMoth.GetHatch();
	const CEggStateVector& totalEggs = hatch.GetEggs();
	const CStageVector& stageFreq = gypsyMoth.GetStage();

	output.SetFirstTRef(m_weather[1].GetFirstTRef());
	output.resize(m_weather.GetNbDay()-m_weather[0].GetNbDay());

	double eggs_left=(double)100;

	for (int i=0; i<m_weather[1].GetNbDay(); i++)
	{
	int day = m_weather[0].GetNbDay()+i;
	CTRef d = m_weather[1].GetFirstTRef() + i;

	double ee=hatch.GetHatching()[day];
	eggs_left=__max(0, eggs_left-ee);

	if(eggs_left<0.005*100)
	eggs_left=0;

	for (int j=0; j<NB_OUTPUT; j++)
	{
	if( j==0 )
	{
	output[d][j] = eggs_left;
	}
	else
	{
	if( gypsyMoth.IsSimulated() )
	output[d][j] = stageFreq[day][j-1];
	else output[d][j] = CBioSIMModelBase::VMISS;
	}
	}

	}
	}

	void CGypsyMothModel::ComputeCumulativeValue(const CGypsyMoth& gypsyMoth, COutputVector& output)
	{
	const CEggModel& hatch = gypsyMoth.GetHatch();
	const CEggStateVector& totalEggs = hatch.GetEggs();
	const CStageVector& stageFreq = gypsyMoth.GetStage();

	output.SetFirstTRef(m_weather[1].GetFirstTRef());
	output.resize(m_weather.GetNbDay()-m_weather[0].GetNbDay());

	double eggs_left=(double)100;
	double tot_pop=0;
	double tot_males = gypsyMoth.GetTotal(CStage::MALE);
	double tot_females = gypsyMoth.GetTotal(CStage::FEMALE);

	double cum_freq[10]={0};
	for (int i=0; i<m_weather[1].GetNbDay(); i++)
	{
	int day=m_weather[0].GetNbDay()+i;
	eggs_left=__max(0,eggs_left-hatch.GetHatching()[day]);

	if(eggs_left<0.005*100)
	eggs_left=0;

	tot_pop=eggs_left+stageFreq[day][10];

	//cumulative frequencies (as a %)
	for(int j=0; j<8; j++)
	{
	cum_freq[j]=0;
	for(int jj=j; jj<8; jj++)
	cum_freq[j]+=stageFreq[day][jj];
	}

	double pc_eggs=0;
	if(tot_pop>0)
	pc_eggs=100*eggs_left/tot_pop;

	for(int j=0;j<8;++j)
	{
	if(tot_pop>0)
	cum_freq[j]=100*cum_freq[j]/tot_pop;
	}

	//% cumulative catch of male/female moths
	if(tot_males>0)
	cum_freq[8]+=100*stageFreq[day][8]/tot_males;
	if(tot_females>0)
	cum_freq[9]+=100*stageFreq[day][9]/tot_females;

	CTRef d = m_weather[1].GetFirstTRef() + i;
	output[d][0] = pc_eggs;

	for (int j=0; j<NB_OUTPUT; j++)
	output[d][j] = j==0?pc_eggs:cum_freq[j-1];
	}
	}
	*/

	//output all generations
	/*void CGypsyMothModel::ComputeRegularValue2(const CGypsyMothVector& gypsyMothVector, COutputVector& output)
	{
	//no consideration of leap years in any generation
	output.resize( (gypsyMothVector.size()+2)*365 );
	for(int g=0; g<gypsyMothVector.size(); g++)
	{
	const CGypsyMoth& gypsyMoth = gypsyMothVector[g];
	const CEggModel& hatch = gypsyMoth.GetHatch();
	const CEggStateVector& totalEggs = hatch.GetEggs();
	const CStageVector& stageFreq = gypsyMoth.GetStage();

	int firstDay = gypsyMoth.GetFirstDay();
	int firstHatch = gypsyMoth.GetFirstHatch();
	int lastDay = gypsyMoth.GetLastDay();
	int NoPeak=0;
	if(lastDay<0) {
	NoPeak=1;
	lastDay=m_weather.GetNbDay()-1;
	}
	if( firstDay < 0)
	{
	firstDay = m_param.GetOvipDate();
	output.resize(output.size()+1);
	int realYear = m_weather.GetFirstYear() + m_weather.GetYearIndex(m_param.GetOvipDate());
	int ii = output.size()-1;
	output[ii][0] = realYear;
	output[ii][1] = m_param.GetOvipDate()+1;
	output[ii][2] = 100;

	for(int j=3; j<COutput::NB_OUTPUT; j++)
	output[ii][j]=0;
	return;
	}


	double eggs_left=100;

	int nbDay = lastDay-firstDay+1;
	for (int i=0; i<nbDay; i++)
	{
	int day=firstDay+i;
	int realDay = (day%365)+1;
	int realYear = (day/365)-2-gypsyMothVector.size()+g+1; // -2 because GetOvipDate() might be two years previous

	double ee=hatch.GetHatching()[day];

	eggs_left=__max(0, eggs_left-ee);

	if(eggs_left<0.5)
	eggs_left=0;

	int ii = firstDay+i+g*365;
	output[ii][0] = realYear;
	output[ii][1] = realDay;
	output[ii][2] = eggs_left;
	if( day >= firstHatch )
	{
	if(NoPeak)
	{
	for (int j=0; j<10; j++)
	output[ii][j+3] = 0;
	output[ii][13] = 0;
	}
	else
	{
	for (int j=0; j<10; j++)
	output[ii][j+3] = stageFreq[day][j];
	output[ii][13] = stageFreq[day][11];
	}
	}
	for (int j=0; j<4; j++)
	output[ii][14+j] = hatch.GetEggsPourcent(day, j);
	}
	}
	}
	*/

	/*
	void CGypsyMothModel::ExportAllGenerations(const CGypsyMothVector& gypsyMothVector)
	{
	int firstDay = m_weather.GetNbDay()-1;
	int lastDay = 0;

	for(int g=0; g<gypsyMothVector.size(); g++)
	{
	if( gypsyMothVector[g].IsSimulated() )
	{
	firstDay = min(firstDay, gypsyMothVector[g].GetFirstDay());
	lastDay = max( lastDay, gypsyMothVector[g].GetLastDay());
	}
	}

	std::ofstream file;

	CCFLString name;
	name.Format( "%s_%d", (LPCTSTR)m_outputFilePath, 1 );
	file.open(name);

	for(int d=firstDay; d<lastDay; d++)
	{
	int realDay = m_weather.GetJulianDay(d)+ 1;
	int realYear = m_weather.GetFirstYear() + m_weather.GetYearIndex(d);

	file << realYear << "\t" << realDay << "\t";
	file.precision(3);

	for(int g=0; g<gypsyMothVector.size(); g++)
	{
	if( gypsyMothVector[g].IsSimulated() )
	{
	const CEggModel& hatch = gypsyMothVector[g].GetHatch();
	//const CEggStateVector& totalEggs = hatch.GetEggs();
	const CStageVector& stageFreq = gypsyMothVector[g].GetStage();

	for (int j=0; j<4; j++)
	file << hatch.GetEggsPourcent(d, j) << "\t";


	file << stageFreq[d][CStage::FEMALE] << "\t";
	}
	else
	{
	for (int j=0; j<4; j++)
	file << -1 << "\t";

	file << -1 << "\t";
	}
	}

	file << std::endl;

	}

	file.close();
	}
	*/

	//this method is called to load your parameters in your variables
	ERMsg CGypsyMothModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		CTPeriod period = m_weather.GetEntireTPeriod(CTM::DAILY);
		


		//transfer your parameters here
		int c = 0;
		m_hatchModelType = parameters[c++].GetInt();
		m_eggParam.m_ovipDate = period.Begin() + parameters[c++].GetInt();
		m_eggParam.m_sawyerModel = parameters[c++].GetInt();
		m_bHaveAttrition = parameters[c++].GetBool();
		m_outputStyle = parameters[c++].GetInt();
		m_takePreviousOvipDate = parameters[c++].GetBool();


		return msg;
	}

}