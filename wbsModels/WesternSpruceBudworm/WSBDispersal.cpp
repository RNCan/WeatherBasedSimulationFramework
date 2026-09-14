//*****************************************************************************
// Class: CWSBDispersal
//
// Description: CWSBDispersal is a BioSIM model for Spruce budworm dispersal
//*****************************************************************************
// 23/07/2026	1.0.0	Rémi Saint-Amant    Creation from Eastern Spruce budworm dispersal
//*****************************************************************************
#include "Basic/ModelStat.h"
#include "Basic/UtilStd.h"
#include "ModelBase/EntryPoint.h"
#include "WSBDispersal.h"
#include "WSpruceBudworm.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;

namespace WBSF
{


	static const bool ACTIVATE_PARAMETRIZATION = false;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CWSBDispersal::CreateObject);

	enum Toutput { O_YEAR, O_MONTH, O_DAY, O_SEX, O_A, O_M, O_G, O_F0, O_FD, /*O_H,*/ NB_OUTPUTS };
	extern char HOURLY_HEADER[] = "Year,Month,Day,sex,A,M,G,F°,F";

	class CBugStat
	{
	public:


		CBugStat(CTRef TRef, size_t sex, double A, double M, double ξ, double G, double Fº, double Fᴰ)
		{
			m_TRef = TRef;
			m_sex = sex;
			m_A = A;
			m_M = M;
			m_ξ = ξ;
			m_G = sex == FEMALE ? G : -999;
			m_Fº = Fº;
			m_Fᴰ = Fᴰ;

		}

		CTRef m_TRef;
		size_t m_sex;
		double m_A;
		double m_M;
		double m_ξ;
		double m_G;
		double m_Fº;
		double m_Fᴰ;
	};

	static  bool cmp_by_TRef(const CBugStat& i, const CBugStat& j) { return i.m_TRef < j.m_TRef; }


	typedef vector<CBugStat> CBugStatVector;
	typedef map<CTRef, CBugStatVector> CBugStatVectorMap;

	CWSBDispersal::CWSBDispersal()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 3;
		VERSION = "1.0.0 (2026)";

		m_nbMoths = 10;
		m_defoliation = 0.5;
		m_adult_longivity_max = 12;
		m_survivalRate = 1.0;

	}

	CWSBDispersal::~CWSBDispersal()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CWSBDispersal::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_nbMoths = parameters[c++].GetInt();
		m_defoliation = parameters[c++].GetFloat();
		m_adult_longivity_max = parameters[c++].GetInt();

		return msg;
	}

	ERMsg CWSBDispersal::OnExecuteAtemporal()
	{
		ERMsg msg;

		//because we don't have a lot of object, we randomize each time
		InitRandomGenerator(0);
		Randomize((unsigned int)0);//init old random number just in case


		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();


		CBugStatVector flyers;

		//for all years
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod();

			//Create stand
			CWSBStand stand(this);

			stand.m_bFertilEgg = false;
			stand.m_bApplyAttrition = false;
			stand.m_bApplyAdultAttrition = false;
			stand.m_bApplyWinterMortality = false;
			stand.m_bApplyAsynchronyMortality = false;
			stand.m_bApplyWindowMortality = false;
			stand.m_survivalRate = m_survivalRate;
			stand.m_defoliation = m_defoliation / 100.0;//In Western SBW the defoliation is [0-1]
			stand.m_adult_longivity_max = m_adult_longivity_max;








			CWSBTreePtr pTree(new CWSBTree(&stand));
			stand.m_host.push_front(pTree);


			//Create tree
			pTree->Initialize<CWSpruceBudworm>(CInitialPopulation(p.Begin(), 0, m_nbMoths, m_nbMoths, L2o, RANDOM_SEX, false, 0));
			//pTree->Initialize<CWSpruceBudworm>(CInitialPopulation(p.Begin(), 0, m_nbObjects, m_initialPopulation, L2o, RANDOM_SEX, m_bFertilEgg, 0));

			size_t last_day = p.Begin().GetJDay();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				for (CWSBTree::iterator it = pTree->begin(); it != pTree->end(); it++)
				{
					CWSpruceBudworm& budworm = static_cast<CWSpruceBudworm&>(*(*it).get());
					if (budworm.IsAlive())
					{
						const CHourlyData& w = m_weather.GetHour(TRef);

						ASSERT(w.GetParent());
						const CWeatherDay& dayº = (const CWeatherDay&)*w.GetParent();

						bool bBegin = TRef == p.Begin();
						bool bEnd = TRef == p.End();
						if (bBegin || bEnd ||
							dayº[H_TMIN][MEAN] >= -10)
						{
							bool bCreated = budworm.IsCreated(TRef);
							if (TRef.GetJDay() != last_day)
								budworm.OnNewDay(dayº);

							budworm.Live(w, 1);

							//compute brood and flight activity only once
							if (budworm.GetStage() == ADULT)
							{
								size_t sex = budworm.GetSex();

								ASSERT(budworm.GetTotalBroods() == 0);
								flyers.push_back(CBugStat(TRef, sex, budworm.GetA(), budworm.GetM(), budworm.Getξ(), budworm.GetFᴰ() / budworm.GetFº(), budworm.GetFº(), budworm.GetFᴰ()));
								budworm.SetExodus(true);

							}//if adult

							budworm.Die(dayº);
						}//temperature is over -10 ºC
					}//if is alive
				}//for all insect 

				HxGridTestConnection();
				last_day = p.Begin().GetJDay();
			}//for all hours of a year
		}//for all years

		//overallPeriod.Transform(CTM(CTM::HOURLY, CTM::FOR_EACH_YEAR));
		CTPeriod byInsect(CTRef(1, 0, 0, 0, CTM(CTM::ATEMPORAL)), CTRef((int)flyers.size(), 0, 0, 0, CTM(CTM::ATEMPORAL)));
		sort(flyers.begin(), flyers.end(), cmp_by_TRef);


		m_output.Init(byInsect, NB_OUTPUTS, -999, HOURLY_HEADER);//hourly output


		//copy stat to output
		for (size_t i = 0; i < flyers.size(); i++)
		{
			CTRef TRef(int(i + 1), 0, 0, 0, CTM(CTM::ATEMPORAL));

			m_output[TRef][O_YEAR] = flyers[i].m_TRef.GetYear();
			m_output[TRef][O_MONTH] = flyers[i].m_TRef.GetMonth() + 1;
			m_output[TRef][O_DAY] = flyers[i].m_TRef.GetDay() + 1;
			m_output[TRef][O_SEX] = flyers[i].m_sex;
			m_output[TRef][O_A] = flyers[i].m_A;
			m_output[TRef][O_M] = flyers[i].m_M;
			m_output[TRef][O_G] = flyers[i].m_G;
			m_output[TRef][O_F0] = flyers[i].m_Fº;
			m_output[TRef][O_FD] = flyers[i].m_Fᴰ;
			//m_output[TRef][O_H] = flyers[i].m_h;
		}


		return msg;
	}

}