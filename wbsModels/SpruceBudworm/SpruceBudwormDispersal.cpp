//*****************************************************************************
// Class: CSpruceBudwormDispersal
//
// Description: CSpruceBudwormDispersal is a BioSIM model for Spruce budworm dispersal
//*****************************************************************************
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

	enum Toutput { O_YEAR, O_MONTH, O_DAY, O_HOUR, O_MINUTE, O_SECOND, O_SEX, O_A, O_M, O_G, O_F0, O_FD, O_B, O_E, O_T, O_P, O_W, O_S, NB_OUTPUTS };
	extern char HOURLY_HEADER[] = "Year,Month,Day,Hour,Minute,Second,sex,A,M,G,F°,F,B,E,T,P,W,sunset";

	class CBugStat
	{
	public:
		
		
		CBugStat(CTRef TRef, size_t sex, size_t L, double A, double M, double G, double F°, double Fᴰ, double B, double E, double T, double P, double W, double S)
		{
			m_TRef = TRef;
			m_sex=sex;
			m_L=L;
			m_A=A;
			m_M=M;
			m_G=sex==FEMALE?G:-999;
			m_F° = F°;
			m_Fᴰ = Fᴰ;
			m_B = B;
			m_E = E;
			m_T = T;
			m_P = P;
			m_W = W;
			m_S = S;
		}

		CTRef m_TRef;
		size_t m_sex;
		size_t m_L;
		double m_A;
		double m_M;
		double m_G;
		double m_F°;
		double m_Fᴰ;
		double m_B;
		double m_E;
		double m_T;
		double m_P;
		double m_W;
		double m_S;

	};

	static  bool cmp_by_TRef(const CBugStat& i, const CBugStat& j) { return i.m_TRef < j.m_TRef; }


	typedef vector<CBugStat> CBugStatVector;
	typedef map<CTRef, CBugStatVector> CBugStatVectorMap;

	CSpruceBudwormDispersal::CSpruceBudwormDispersal()
	{
		//NB_INPUT_PARAMETER is used to determine if the DLL
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 2;
		VERSION = "1.0.1 (2017)";
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
			pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, m_nbMoths, m_nbMoths, L2o, NOT_INIT, false, 0));

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				for (CSBWTree::iterator it = pTree->begin(); it != pTree->end(); it++)
				{
					CSpruceBudworm& budworm = static_cast<CSpruceBudworm&>(*(*it).get());
					if (budworm.IsAlive())
					{
						const CHourlyData& w = m_weather.GetHour(TRef);

						ASSERT(w.GetParent());
						const CWeatherDay& day° = (const CWeatherDay&)*w.GetParent();

						if (day°[H_TMIN2][MEAN] >= -10)
						{
							budworm.Live(w, 1);

							//compute brood and flight activity only once
							if (budworm.GetStage() == ADULT && w.GetTRef().GetHour() == 18)
							{

								if ((*it)->GetSex() == FEMALE)
									(*it)->Brood(day°);

								__int64 t° = 0;
								__int64 tᴹ = 0;

								if (budworm.get_t(day°, t°, tᴹ))
								{
									//calculate tᶜ
									__int64 tᶜ = (t° + tᴹ) / 2;

									//now compute tau, p and flight
									bool bExodus = false;
									static const __int64 Δt = 60;
									for (__int64 t = t°; t <= tᴹ && !bExodus; t += Δt)
									{
										double tau = double(t - tᶜ) / (tᴹ - tᶜ);

										double h = t / 3600.0;
										size_t L = size_t((h - size_t(h)) * 3600);

										const CWeatherDay& day¹ = day°.GetNext();
										const CWeatherDay& w = h < 24 ? day° : day¹;

										double T = budworm.get_Tair(w, h < 24 ? h : h - 24.0);
										double P = budworm.get_Prcp(w, h < 24 ? h : h - 24.0);
										double WS = budworm.get_WndS(w, h < 24 ? h : h - 24.0); 

										bExodus = budworm.ComputeExodus(T, P, WS, tau);
										if (bExodus)
										{
											CSun sun(day°.GetLocation().m_lat, day°.GetLocation().m_lon, day°.GetLocation().GetTimeZone());
											double sunset = (sun.GetSunset(day°.GetTRef()) + 1.0 );//+1 hour : assume to be in daylight zone  //[s]


											size_t sex = budworm.GetSex();
											CTRef TRefTmp = TRef + (size_t(h) - TRef.GetHour());
											flyers.push_back(CBugStat(TRefTmp, sex, L, budworm.GetA(), budworm.GetM(), budworm.GetG(), budworm.GetF°(), budworm.GetFᴰ(), budworm.GetTotalBroods(), budworm.GetFᴰ() - budworm.GetTotalBroods(), T, P, WS, sunset));
											
											budworm.SetStatus(CIndividual::DEAD);
											budworm.SetDeath(CIndividual::EXODUS);
										}//if exodus occurd
									}//for t in exodus period
								}//if exodus occur
							}//if adult

							budworm.Die(day°);
						}//temperature is over -10 °C
					}//if is alive
				}//for all insect 

				HxGridTestConnection();
			}//for all hours of a year
		}//for all years

		//overallPeriod.Transform(CTM(CTM::HOURLY, CTM::FOR_EACH_YEAR));
		CTPeriod byInsect(CTRef(0, 0, 0, 0, CTM(CTM::ATEMPORAL)), CTRef((int)flyers.size()-1, 0, 0, 0, CTM(CTM::ATEMPORAL)));
		sort(flyers.begin(), flyers.end(), cmp_by_TRef);


		m_output.Init(byInsect, NB_OUTPUTS, -999, HOURLY_HEADER);//hourly output

		
		//copy stat to output
		for (size_t i = 0; i < flyers.size(); i++)
		{
			CTRef TRef(int(i), 0, 0, 0, CTM(CTM::ATEMPORAL));
			
			m_output[TRef][O_YEAR] = flyers[i].m_TRef.GetYear();
			m_output[TRef][O_MONTH] = flyers[i].m_TRef.GetMonth()+1;
			m_output[TRef][O_DAY] = flyers[i].m_TRef.GetDay()+1;
			m_output[TRef][O_HOUR] = flyers[i].m_TRef.GetHour();
			m_output[TRef][O_MINUTE] = int(flyers[i].m_L/60);
			m_output[TRef][O_SECOND] = int(flyers[i].m_L - int(flyers[i].m_L / 60)*60);
			m_output[TRef][O_SEX] = flyers[i].m_sex;
			m_output[TRef][O_A] = flyers[i].m_A;
			m_output[TRef][O_M] = flyers[i].m_M;
			m_output[TRef][O_G] = flyers[i].m_G;
			m_output[TRef][O_F0] = flyers[i].m_F°;
			m_output[TRef][O_FD] = flyers[i].m_Fᴰ;
			m_output[TRef][O_B] = flyers[i].m_B;
			m_output[TRef][O_E] = flyers[i].m_E;
			m_output[TRef][O_T] = flyers[i].m_T;
			m_output[TRef][O_P] = flyers[i].m_P;
			m_output[TRef][O_W] = flyers[i].m_W;
			m_output[TRef][O_S] = flyers[i].m_S;

			
			
		} 


		return msg;
	}

}