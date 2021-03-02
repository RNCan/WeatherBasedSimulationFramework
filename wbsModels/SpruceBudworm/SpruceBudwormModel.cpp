//*****************************************************************************
// Class: CSpruceBudwormModel
//
// Description: CSpruceBudwormModel is a BioSIM model
//*****************************************************************************
// 02/03/2021	3.2.1	Rémi Saint-Amant	Bug correction in overwintering energy mortality
// 06/12/2019	3.2.0	Rémi Saint-Amant	Bug correction when adult reach max adult longevity 
// 12/07/2019	3.1.9	Rémi Saint-Amant	Add sex as enum and not size_t
// 28/02/2019	3.1.8	Rémi Saint-Amant	Add adult longevity max parameters
// 19/12/2018   3.1.7   Rémi Saint-Amant	Add option of adult attrition
// 02/05/2018   3.1.6   Rémi Saint-Amant	Compile with VS 2017
// 04/05/2017   3.1.5   Rémi Saint-Amant	Update with new hourly generation
// 03/03/2017   3.1.4   Rémi Saint-Amant	Add defoliation parameter
// 28/02/2017   3.1.3   Rémi Saint-Amant	Bug correction in Fertil mode
// 08/01/2017	3.1.2	Rémi Saint-Amant	Add hourly live
// 23/12/2016	3.1.1	Rémi Saint-Amant    Change in overheating and flight activity
// 10/10/2016	3.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 10/05/2016	3.0.4	Rémi Saint-Amant    Minor change
// 01/01/2016	3.0.3	Rémi Saint-Amant    Include in WBSF
// 12/12/2015   3.0.2	Rémi Saint-Amant    New Specimen framework based on smart pointer
// 05/03/2015   3.0.0	Rémi Saint-Amant    Update for BioSIM11
// 30/01/2014   2.6.0	Rémi Saint-Amant    New compilation with boost
// 26/06/2013	2.4.0	Rémi Saint-Amant    Update with new framework and bug correction in fix AI
// 25/09/2011			Rémi Saint-Amant    Remove extra parameters. 
// 23/03/2010			Rémi Saint-Amant    Creation from old code
// 09/01/1995			Jacques Regniere    Creation
//*****************************************************************************
#include "Basic/ModelStat.h"
#include "Basic/UtilStd.h"
#include "ModelBase/EntryPoint.h"
#include "SpruceBudwormModel.h"
#include "SpruceBudworm.h"


namespace WBSF
{
	using namespace SBW;

	static const bool ACTIVATE_PARAMETRIZATION = false;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSpruceBudwormModel::CreateObject);


	extern char DAILY_HEADER[] = "L2o,L2,L3,L4,L5,L6,Pupa,Adult,DeadAdult,OvipositingAdult,Brood,Egg,L1,L2o2,L22,AverageInstar,MaleEmergence,FemaleEmergence,MaleFlight,FemaleFlight";
	typedef CModelStatVectorTemplate<NB_STATS, DAILY_HEADER> CDailyOutput;

	enum TOutputAnnual{ O_GROWTH_RATE, NB_ANNUAL_OUTPUT };
	extern char ANNUAL_HEADER[] = "GROWTH_RATE";
	typedef CModelStatVectorTemplate<NB_ANNUAL_OUTPUT, ANNUAL_HEADER> CAnnualOutput;

	CSpruceBudwormModel::CSpruceBudwormModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 8;
		VERSION = "3.2.1 (2021)";

		// initialize your variables here (optional)
		m_bApplyAttrition = true;
		m_bApplyAdultAttrition = true;
		m_adult_longivity_max = SBW::NO_MAX_ADULT_LONGEVITY;
		m_fixAI = 0;

		m_treeKind = 0;
		m_bFertility = false;
	}

	CSpruceBudwormModel::~CSpruceBudwormModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CSpruceBudwormModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;

		m_bApplyAttrition = parameters[c++].GetBool();
		m_bFertility = parameters[c++].GetBool();
		m_treeKind = parameters[c++].GetInt();

		m_fixDate = parameters[c++].GetTRef();
		m_fixAI = parameters[c++].GetReal();
		m_defoliation = parameters[c++].GetReal();
		m_bApplyAdultAttrition = parameters[c++].GetBool();
		m_adult_longivity_max = parameters[c++].GetInt();

		//overwrite fixe if availble in the locations file
		std::string fixeAI = m_info.m_loc.GetSSI("FixeAI");
		std::string fixeDate = m_info.m_loc.GetSSI("FixeDate");
		if (!fixeAI.empty() ||
			!fixeDate.empty())
		{
			if (!fixeAI.empty() &&
				!fixeDate.empty())
			{
				
				CTRef TRef;
				TRef.FromFormatedString(fixeDate, "", "-/\\ ");
				
				//try day first
				if (TRef.GetYear() != m_weather.GetFirstYear())
				{
					CTRef TRef2;
					TRef2.FromFormatedString(fixeDate, "%d-%m-%Y", "-/\\ ");
					if (TRef2.GetYear() == m_weather.GetFirstYear())
						TRef = TRef2;
					else
						TRef.clear();
				}

				if (TRef.IsValid() )
				{
					m_fixDate = TRef;
					m_fixAI = as<double>(fixeAI);
				}
				else
				{
					msg.ajoute("Invalid date. Format is YYYY-MM-DD.");
				}
				
			}
			else
			{
				//if only one is present, need to send an error
				msg.ajoute("Both field (FixeDate, FixeAI) in location file need to be present to set a valid AI fixe");
				msg.ajoute(std::string(fixeAI.empty()?"FixeAI":"FixeDate") + " is missing");
			}

		}

		if (msg&&m_fixDate.IsInit())
		{
			if ( m_weather.GetNbYears() == 1)
			{
				if (m_fixDate.GetYear() != m_weather.GetFirstYear())
				{
					msg.ajoute("Invalid AI fixe date. Fixe year ("+to_string(m_fixDate.GetYear() )+") must be the same as simulation year(" + to_string(m_weather.GetFirstYear()) + ")");
				}

			}
			else
			{
				msg.ajoute("AI fixe can only be use with one year simulation");
			}

			/*if (m_fixAI < 2 || m_fixAI>9)
			{
				msg.ajoute("Invalid AI"+to_string(m_fixAI)+". AI must be between 2 and 9");
			}*/
		}

		return msg;
	}

	ERMsg CSpruceBudwormModel::OnExecuteAnnual()
	{
		_ASSERTE(m_weather.size() > 1);

		ERMsg msg; 

		//In annual model stop developing of the L22 to get cumulative L22
		CModelStatVector stat;
		ExecuteDaily(stat, true);


		CAnnualOutput stateA(m_weather.size() - 1, CTRef(m_weather.GetFirstYear()+1), CBioSIMModelBase::VMISS);

		for (size_t y = 0; y < m_weather.size() - 1; y++)
		{
			CStatistic statL22;
			CTPeriod p = m_weather[y + 1].GetEntireTPeriod(CTM(CTM::DAILY));
			for (CTRef d = p.Begin(); d <= p.End(); d++)
				statL22 += stat[d][S_L22];

			
			if (statL22.IsInit())
			{
				double gr = statL22[HIGHEST];
				ASSERT(gr >= 0 && gr<3000);
				stateA[y][O_GROWTH_RATE] = gr / 100; //initial population is 100 insect
			}
		}

		SetOutput(stateA);

		return msg;
	}

	//This method is called to compute the solution
	ERMsg CSpruceBudwormModel::OnExecuteDaily()
	{
		ERMsg msg;

		ExecuteDaily(m_output);

		return msg;
	}

	void CSpruceBudwormModel::ExecuteDaily(CModelStatVector& stat, bool bStopL22)
	{
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		stat.Init(p.GetNbRef(), p.Begin(), NB_STATS, 0, DAILY_HEADER);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y1 = 0; y1 < m_weather.size(); y1++)
		{
			CTPeriod p = m_weather[y1].GetEntireTPeriod(CTM(CTM::DAILY));

			CSBWStand stand(this);
			//Create stand
			//warning, to not change stand and tree order because insect create used stand defoliation
			stand.m_bFertilEgg = m_bFertility;
			stand.m_bApplyAttrition = m_bApplyAttrition;
			stand.m_bApplyAdultAttrition = m_bApplyAdultAttrition;
			stand.m_defoliation = m_defoliation;
			stand.m_bStopL22 = bStopL22;
			stand.m_adult_longivity_max = m_adult_longivity_max;

			//Create tree
			CSBWTreePtr pTree(new CSBWTree(&stand));
			
			pTree->m_kind = m_treeKind;
			pTree->m_nbMinObjects = 100;
			pTree->m_nbMaxObjects = 1000;
			pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, 400, 100, L2o, RANDOM_SEX, m_bFertility, 0));
			//pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, 1, 100, L2o, RANDOM_SEX, m_bFertility, 0));

			//add tree to stand			
			stand.m_host.push_front(pTree);

			//if Simulated Annealing, set 
			if (ACTIVATE_PARAMETRIZATION)
			{
				//stand.m_rates.SetRho25(m_rho25Factor);
				//stand.m_rates.Save("D:\\Rates.csv");
			}


			size_t nbYear = m_bFertility ? 2 : 1;
			for (size_t y = 0; y < nbYear && y1 + y < m_weather.size(); y++)
			{
				size_t yy = y1 + y;

				CTPeriod p = m_weather[yy].GetEntireTPeriod(CTM(CTM::DAILY));

				for (CTRef d = p.Begin(); d <= p.End(); d++)
				{
					stand.Live(m_weather.GetDay(d));

					if (d == m_fixDate && m_fixAI >= 2)
						stand.FixAI(m_fixAI);

					stand.GetStat(d, stat[d]);

					/*if( y==1 && stat[d][CSBStat::L2o2]==0 && stat[d][CSBStat::L22]==0 )
					{
					//end the simulation here
					d=m_weather[yy].GetLastTRef();
					}*/

					stand.AdjustPopulation();
					HxGridTestConnection();
				}

				stand.HappyNewYear();
			}
		}
	}




	//simulated annaling 
	void CSpruceBudwormModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		//transform value to date/stage
		ASSERT(header[3] == "Year");
		ASSERT(header[4] == "Month");
		ASSERT(header[5] == "Day");
		ASSERT(header[12] == "NbInds");
		ASSERT(header[13] == "AI");

		CTRef ref(ToShort(data[3]), ToShort(data[4]) - 1, ToShort(data[5]) - 1);
		std::vector<double> obs;
		obs.push_back(ToDouble(data[13]));
		obs.push_back(ToDouble(data[12]));
		m_SAResult.push_back(CSAResult(ref, obs));
	}

	void CSpruceBudwormModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;
		CModelStatVector statSim;

		if (m_SAResult.size() > 0)
		{
			//int m_firstYear = m_SAResult.begin()->m_ref.GetYear();
			//int m_lastYear = m_SAResult.rbegin()->m_ref.GetYear();

			//while( m_weather.GetFirstYear() < m_firstYear )
			//	m_weather.RemoveYear(0);

			//while( m_weather.GetLastYear() > m_lastYear )
			//	m_weather.RemoveYear(m_weather.GetNbYear()-1);

			CStatistic years;
			for (CSAResultVector::const_iterator p = m_SAResult.begin(); p < m_SAResult.end(); p++)
				years += p->m_ref.GetYear();

			int m_firstYear = (int)years[LOWEST];
			int m_lastYear = (int)years[HIGHEST];
			/*while( m_weather.GetNbYear() > 1 && m_weather.GetFirstYear() < m_firstYear )
				m_weather.RemoveYear(0);

				while( m_weather.GetNbYear() > 1 && m_weather.GetLastYear() > m_lastYear )
				m_weather.RemoveYear(m_weather.GetNbYear()-1);
				*/

			ASSERT(m_weather.GetFirstYear() == m_firstYear);
			ASSERT(m_weather.GetLastYear() == m_lastYear);
			ExecuteDaily(statSim);


			for (int i = 0; i < (int)m_SAResult.size(); i++)
			{
				if (statSim.IsInside(m_SAResult[i].m_ref))
				{
					double AISim = statSim[m_SAResult[i].m_ref][S_AVERAGE_INSTAR];
					//_ASSERTE( AISim >= 2&&AISim<=8);
					//when all bug die, a value of -9999 can be compute
					if (AISim >= 2 && AISim <= 8)
					{
						for (int j = 0; j < m_SAResult[i].m_obs[1]; j++)
							stat.Add(AISim, m_SAResult[i].m_obs[0]);
					}
				}
			}
		}
	}
}