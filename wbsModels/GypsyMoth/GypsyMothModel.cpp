//************************************************************************************
//16/09/2026	2.3.3	RSA	Add Saint-Amant egg hatch model
//13/12/2019	2.3.2	RSA	Code cleaning
//16/11/2016	2.3.1	RSA	Compile for BioSIM 11
//RSA 25/01/2012:	New model with multi generation
//					Add deadAdult
//					use double instead of float
//RSA 09/03/2011:	new compilation with new BioSIMModelBase, new interface
//					Add switch to activate or not new gray equation
//					The model for south hemisphere don't work anymore
//RSA 17/11/2008: Recompilation with new gray egg equation
//RSA 14/07/2005: Model adjust ovipDate to simulate the good year
//RSA 18/05/2005: integration to BioSIMModelBase + cleaning
//JR  13/05/2005: harmonized this Gymphen.cpp with the Stability version.
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
		VERSION = "2.3.3 (2026)";

		m_hatchModelType = CGypsyMoth::GRAY_MODEL;
		m_bHaveAttrition = false;
		m_outputStyle = REGULAR;
		m_takePreviousOvipDate = false;

	}

	CGypsyMothModel::~CGypsyMothModel()
	{
	}

	//this method is called to load your parameters in your variables
	ERMsg CGypsyMothModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		CTPeriod period = m_weather.GetEntireTPeriod(CTM::DAILY);

		//transfer your parameters here
		int c = 0;
		m_hatchModelType = parameters[c++].GetInt();
		m_eggParam.m_pModel = this;
		m_eggParam.m_ovipDate = period.Begin() + parameters[c++].GetInt();
		m_eggParam.m_sawyerModel = parameters[c++].GetInt();
		m_bHaveAttrition = parameters[c++].GetBool();
		m_outputStyle = parameters[c++].GetInt();
		m_takePreviousOvipDate = parameters[c++].GetBool();


		return msg;
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
		//Do simulation only if the oviposition date of the first generation 
		for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		{
			CGypsyMoth gypsyMoth(m_hatchModelType, eggParamTmp);

			CTPeriod p(period.GetFirstAnnualTRef(y), period.GetLastAnnualTRef(y+1));

			//simulate development
			gypsyMoth.SimulateDeveloppement(m_weather, p);

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
		}
	}

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
			output[d][O_MALE_MOTH] = stat[d][MALE_ADULT];
			output[d][O_FEMALE_MOTH] = stat[d][FEMALE_ADULT];
		}

	}

	void CGypsyMothModel::ComputeCumulativeValue(const CGMOutputVector& stat, COutputVector& output)
	{
		output.Init(stat.size(), stat.GetFirstTRef());


		CTPeriod p = stat.GetTPeriod();
		for (size_t y = 0; y < p.GetNbYears(); y++)
		{
			CTPeriod p2 = p.GetAnnualPeriodByIndex(y);
			double sumMale = stat.GetStat(MALE_ADULT, p2)[SUM];
			double sumFemale = stat.GetStat(FEMALE_ADULT, p2)[SUM];

			for (CTRef d = p2.Begin(); d <= p2.End(); d++)
			{
				bool firstDay = d == p2.Begin();

				//cumulative frequencies (as a %)
				double cumFreq[NB_STAGE] = { 0 };
				cumFreq[0] = stat[d][EGG];
				double totPop = stat[d][EGG];

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
						output[d][O_MALE_MOTH] = output[d - 1][O_MALE_MOTH] + 100 * stat[d][MALE_ADULT] / sumMale;

					if (sumFemale > 0.0001)
						output[d][O_FEMALE_MOTH] = output[d - 1][O_FEMALE_MOTH] + 100 * stat[d][FEMALE_ADULT] / sumFemale;

				}
			}
		}

	}

}