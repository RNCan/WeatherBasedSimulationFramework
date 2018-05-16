//*****************************************************************************
// Class: CSpruceBudwormDispersal
//
// Description: CSpruceBudwormDispersal is a BioSIM model for Spruce budworm dispersal
//*****************************************************************************
// 02/05/2018   1.1.0   Rémi Saint-Amant	Transfer liftoff from model to dispersal
// 04/05/2017   1.0.1   Rémi Saint-Amant	Update with new hourly generation
// 05/01/2016	1.0.0	Rémi Saint-Amant    Creation from old code
//*****************************************************************************
#include "Basic/ModelStat.h"
#include "Basic/UtilStd.h"
#include "ModelBase/EntryPoint.h"
#include "SpruceBudwormDispersal.h"
#include "SpruceBudworm.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;

namespace WBSF
{
	using namespace SBW;

	static const bool ACTIVATE_PARAMETRIZATION = false;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSpruceBudwormDispersal::CreateObject);

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

	CSpruceBudwormDispersal::CSpruceBudwormDispersal()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 2;
		VERSION = "1.1.0 (2018)";
	}

	CSpruceBudwormDispersal::~CSpruceBudwormDispersal()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CSpruceBudwormDispersal::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_nbMoths = parameters[c++].GetInt();
		m_defoliation = parameters[c++].GetFloat();

		return msg;
	}

	ERMsg CSpruceBudwormDispersal::OnExecuteAtemporal()
	{
		ERMsg msg;

		//becasue we don't have a lot of object, we randomize each time
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
			CSBWStand stand(this);
			stand.m_bFertilEgg = false;
			stand.m_bApplyAttrition = false;
			stand.m_defoliation = m_defoliation;

			CSBWTreePtr pTree(new CSBWTree(&stand));
			stand.m_host.push_front(pTree);


			//Create tree
			pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, m_nbMoths, m_nbMoths, L2o, RANDOM_SEX, false, 0));

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				for (CSBWTree::iterator it = pTree->begin(); it != pTree->end(); it++)
				{
					CSpruceBudworm& budworm = static_cast<CSpruceBudworm&>(*(*it).get());
					if (budworm.IsAlive())
					{
						const CHourlyData& w = m_weather.GetHour(TRef);

						ASSERT(w.GetParent());
						const CWeatherDay& dayº = (const CWeatherDay&)*w.GetParent();

						bool bBegin = TRef == p.Begin();
						bool bEnd = TRef == p.End();
						if (bBegin || bEnd ||
							dayº[H_TMIN2][MEAN] >= -10)
						{
							budworm.Live(w, 1);

							//compute brood and flight activity only once
							if (budworm.GetStage() == ADULT) 
							{
								size_t sex = budworm.GetSex();

								ASSERT(budworm.GetTotalBroods() == 0);
								flyers.push_back(CBugStat(TRef, sex, budworm.GetA(), budworm.GetM(), budworm.Getξ(), budworm.GetFᴰ()/ budworm.GetFº(), budworm.GetFº(), budworm.GetFᴰ()));

								budworm.SetStatus(CIndividual::DEAD);
								budworm.SetDeath(CIndividual::EXODUS);
							}//if adult

							budworm.Die(dayº);
						}//temperature is over -10 ºC
					}//if is alive
				}//for all insect 

				HxGridTestConnection();
			}//for all hours of a year
		}//for all years

		//overallPeriod.Transform(CTM(CTM::HOURLY, CTM::FOR_EACH_YEAR));
		CTPeriod byInsect(CTRef(1, 0, 0, 0, CTM(CTM::ATEMPORAL)), CTRef((int)flyers.size(), 0, 0, 0, CTM(CTM::ATEMPORAL)));
		sort(flyers.begin(), flyers.end(), cmp_by_TRef);


		m_output.Init(byInsect, NB_OUTPUTS, -999, HOURLY_HEADER);//hourly output


		//copy stat to output
		for (size_t i = 0; i < flyers.size(); i++)
		{
			CTRef TRef(int(i+1), 0, 0, 0, CTM(CTM::ATEMPORAL));

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