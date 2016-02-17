//**********************************************************************
//
// 22/01/2016	1.1.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 17/04/2013			Rémi Saint-Amant	Create
//**********************************************************************

#include <math.h>
#include <crtdbg.h>
#include "ModelBase/EntryPoint.h"
#include "OignonMouche.h"


using namespace std;

namespace WBSF
{


	//"Overwintering gen., 5% adults|Overwintering gen., 50% adults|1st gen., 5% adults|1st gen., 50% adults|2nd gen., 5% adults|2nd gen., 50% adults;
	const char OIGNON_MOUCHE_HEADER_CR[] = "DD|Génération0|Génération1|Génération2|AI";
	const char OIGNON_MOUCHE_HEADER[] = "DD|Génération0|Génération1|Génération2";
	const double CModeleOignonMouche::A[1][NB_GENERATIONS] = { { 572, 1368, 1947 } };
	const double CModeleOignonMouche::B[1][NB_GENERATIONS] = { { 3.82, 2.65, 1.27 } };
	static const bool SIMULATED_ANNEALING = false;


	//*************************************************************************************************************************

	//*************************************************************************************************************************

	//*************************************************************************************************************************

	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CModeleOignonMouche::CreateObject);


	//enum TDailyStat{O_DD,O_SEED,O_EMERGENCE,O_LEAF1,O_LEAF2,O_LEAF3,O_LEAF4,O_LEAF5,O_LEAF6,O_LEAF7,O_FLAG_LEAF_EMERGED,O_BOOT_SWELLING_BEINGS,O_MID_BOOT,O_HEADING_EMERGED,O_HEADING_COMPLETE,O_KERNEL_WATERY_RIPE,O_GROWTH_STAGE,O_PRCP,O_ET_REF,O_KC,O_ET,NB_DAILY_STATS};
	//typedef CModelStatVectorTemplate<NB_DAILY_STATS> CDailyStatVector;


	CModeleOignonMouche::CModeleOignonMouche()
	{
		NB_INPUT_PARAMETER = SIMULATED_ANNEALING ? 8 : 2;
		VERSION = "1.1.0 (2016)";

	}

	CModeleOignonMouche::~CModeleOignonMouche()
	{}

	//-----------------------------------------------------------------------------------------
	// Cette fonction appelle une fonction pour calculer les degrés-jours, méthode Sinus simple,
	// à chaque jour. Le résultat est des degrés-jours cumulés.
	//
	// Les données météo en input sont des données quotidiennes.
	// Les données en output sont des données quotidiennes.
	//
	// Paramètres en entrée:
	//   températureMaxAir:  Tableau de température maximum de l/air.
	//   températureMinAir:  Tableau de température minimum de l/air.
	//
	// Note:
	// Les données en input doivent commencer à la date de début du modèle.
	// Les tableaux en input doivent commencer par le même indice (0 ou 1, ça n//a pas
	// d//importance) et doivent être de la même grandeur.
	//
	// Paramètre en sortie:
	//   outputDegJourCumul: Tableau de degrés-jours cumulés.
	//-----------------------------------------------------------------------------------------

	void Compute(CModelStatVector stat, CModelStatVector& output)
	{
		static const SIZE_T OUTPUT_NO[NB_OUTPUT] = { 0, 2, 3, 4 };

		ASSERT(stat.GetTM().Type() == CTM::HOURLY);
		ASSERT(stat.GetNbStat() == 6);
		output.Init(stat.GetTPeriod(), NB_OUTPUT, 0, OIGNON_MOUCHE_HEADER);
		for (size_t i = 0; i < stat.size(); i++)
		{
			for (size_t j = 0; j < NB_OUTPUT; j++)
			{
				output[i][j] = stat[i][OUTPUT_NO[j]];
				if (j != 0)
				{
					if (output[i][j] < 0.1)
						output[i][j] = 0;
					else if (output[i][j] > 99.9)
						output[i][j] = 100;
				}

			}
		}

	}

	ERMsg CModeleOignonMouche::OnExecuteHourly()
	{
		ERMsg msg;

		CModelStatVector stat;
		m_CR.Execute(m_weather, stat);
		Compute(stat, m_output);


		return msg;
	}




	ERMsg CModeleOignonMouche::OnExecuteDaily()
	{
		ERMsg msg;

		//CModelStatVector stat;
		//	m_CR.Execute(m_weather, stat);
		//m_CR.Transform();

		//Compute(stat, m_output);
		return msg;
	}


	//this method is call to load your parameter in your variable
	ERMsg CModeleOignonMouche::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		m_species = parameters[c++].GetInt();

		CTRef TRef(0, APRIL, FIRST_DAY);

		m_CR.m_aType = COignonMoucheRatioContinue::AT_DAILY;
		if (m_weather.GetTM().Type() == CTM::HOURLY)
			m_CR.m_method = COignonMoucheRatioContinue::BIOSIM_HOURLY;
		else
			m_CR.m_method = COignonMoucheRatioContinue::SINGLE_SINE;

		m_CR.m_bAdjustFinalProportion = true;
		m_CR.m_bMultipleVariance = true;
		m_CR.m_startJday = TRef.GetJDay();
		m_CR.m_bPercent = true;
		m_CR.m_lowerThreshold = 4;
		m_CR.m_upperThreshold = 999;//optimum à 40 = pas de maximum
		m_CR.m_bCumul = parameters[c++].GetBool();

		for (int k = 0; k < NB_GENERATIONS; k++)
		{
			if (SIMULATED_ANNEALING)
			{
				ASSERT(parameters.size() == 8);
				m_CR.m_a[k] = parameters[c++].GetFloat();
				m_CR.m_b[k] = parameters[c++].GetFloat();

			}
			else
			{
				m_CR.m_a[k] = A[m_species][k];
				m_CR.m_b[k] = B[m_species][k];
			}
		}

		//for (size_t i = 1; i < NB_GENERATIONS; i++)
		//m_CR[i] = m_CR[0];

		return msg;
	}

	void CModeleOignonMouche::AddHourlyResult(const StringVector& header, const StringVector& data)
	{
		static const double LIMIT[NB_SPECIES][NB_GENERATIONS][2] =
		{
			{
				{ 269, 572 + (572 - 269) }, { 1072, 1368 + (1368 - 1072) }, { 1803, 1947 + (1947 - 1803) }
			}
		};

		std::vector<double> obs(NB_SPECIES*NB_GENERATIONS * 2);
		for (size_t i = 0; i < NB_SPECIES; i++)
			for (size_t j = 0; j < NB_GENERATIONS; j++)
				for (size_t k = 0; k < 2; k++)
					obs[i*NB_GENERATIONS * 2 + j * 2 + k] = LIMIT[i][j][k];


		m_SAResult.push_back(CSAResult(CTRef(), obs));
	}

	void CModeleOignonMouche::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		AddHourlyResult(header, data);
	}


	void CModeleOignonMouche::GetFValueHourly(CStatisticXY& stat)
	{
		if (m_SAResult.size() > 0)
		{
			//look to see if all GDD are in growing order
			CStatistic statDD[NB_GENERATIONS];
			CStatistic::SetVMiss(-999);

			CModelStatVector statSim;
			m_CR.Execute(m_weather, statSim);

			for (size_t d = 0; d < statSim.size(); d++)
			{
				for (size_t g = 0; g < NB_GENERATIONS; g++)
				{
					if (statSim[d][COignonMoucheRatioContinue::O_FIRST_STAGE + g + 1] > 5 &&
						statSim[d][COignonMoucheRatioContinue::O_FIRST_STAGE + g + 1] < 95)
					{
						double DD = statSim[d][COignonMoucheRatioContinue::O_DD];
						statDD[g] += DD;
					}
				}
			}


			for (size_t g = 0; g < NB_GENERATIONS; g++)
			{
				for (size_t k = 0; k < 2; k++)	//add min and max
				{
					double obs = m_SAResult[0].m_obs[m_species*NB_GENERATIONS * 2 + g * 2 + k];
					double sim = statDD[g][k == 0 ? LOWEST : HIGHEST];
					stat.Add(sim, obs);
				}
			}
		}
	}


	void CModeleOignonMouche::GetFValueDaily(CStatisticXY& stat)
	{
		if (m_SAResult.size() > 0)
		{
			//look to see if all GDD are in growing order
			bool bValid = true;
			/*for (int s = 1; s<NB_GENERATIONS&&bValid; s++)
			{
			if( m_CR.m_a[s] < m_CR.m_a[s-1] )
			bValid = false;
			}*/

			CStatistic statDD[NB_GENERATIONS];
			CStatistic::SetVMiss(-999);
			if (bValid)
			{
				CModelStatVector statSim;
				//for (size_t i = 0; i < NB_GENERATIONS; i++)
				m_CR.Execute(m_weather, statSim);

				for (size_t d = 0; d < statSim[0].size(); d++)
				{
					for (size_t g = 0; g < NB_GENERATIONS; g++)
					{
						if (statSim[d][COignonMoucheRatioContinue::O_FIRST_STAGE + g + 1] > 5 &&
							statSim[d][COignonMoucheRatioContinue::O_FIRST_STAGE + g + 1] < 95)
						{
							double DD = statSim[d][COignonMoucheRatioContinue::O_DD];
							statDD[g] += DD;
						}
					}
				}
			}


			for (size_t g = 0; g < NB_GENERATIONS; g++)
			{
				for (size_t k = 0; k < 2; k++)	//add min and max
				{
					double obs = m_SAResult[0].m_obs[m_species*NB_GENERATIONS * 2 + g * 2 + k];
					double sim = statDD[g][k == 0 ? LOWEST : HIGHEST];
					stat.Add(sim, obs);
				}
			}
		}
	}



}