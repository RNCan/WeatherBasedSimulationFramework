//**********************************************************************
//
// 06/08/2013 Rémi Saint-Amant	Create from excel file 
//**********************************************************************
#include <math.h>
#include <crtdbg.h>


#include "Basic/CSV.h"
#include "Basic/UtilStd.h"
#include "Basic/DegreeDays.h"
#include "ModelBase/EntryPoint.h"
#include "Insect Development Database II.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CIDDModel::CreateObject);


	CCriticalSection CInsectDevelopmentDatabase::CS;
	std::map<string, CInsectDevelopment> CInsectDevelopmentDatabase::INSECTS_DEVELOPMENT_DATABASE;

	ERMsg CInsectDevelopmentDatabase::Set(const std::string& data)
	{
		ERMsg msg;

		CS.Enter();
		if (INSECTS_DEVELOPMENT_DATABASE.empty())
		{
			std::stringstream stream(data);

			if (msg)
			{
				//INSECTS_DEVELOPMENT_DATABASE.reserve(186);

				int s = 0;

				for (CSVIterator loop(stream); loop != CSVIterator(); ++loop, s++)
				{

					//INSECTS_DEVELOPMENT_DATABASE.resize(INSECTS_DEVELOPMENT_DATABASE.size() + 1);

					enum TInput{ I_NAME, I_SCIENTIFIC_NAME, I_COMMON_NAME, I_UPPER_DEVEL, I_EGG_THRESHOLD, I_LARVAE_NYMPH_THRESHOLD, I_PUPAE_THRESHOLD, I_ADULT_THRESHOLD, I_MATURE_ADULT_THRESHOLD, I_EGG_DD, I_LARVAE_NYMPH_DD, I_PUPAE_DD, I_ADULT_DD, I_MATURE_ADULT_DD, I_COMMENTS, NB_INPUT_COLUMNS };

					string name = (*loop)[I_NAME];
					INSECTS_DEVELOPMENT_DATABASE[name].m_name = (*loop)[I_NAME];
					if (!(*loop)[I_UPPER_DEVEL].empty())
						INSECTS_DEVELOPMENT_DATABASE[name].m_upperThreshold = ToDouble((*loop)[I_UPPER_DEVEL]);

					for (int i = 0; i < NB_STAGES; i++)
					{
						string tmp1 = TrimConst((*loop)[I_EGG_THRESHOLD + i]);
						string tmp2 = TrimConst((*loop)[I_EGG_DD + i]);
						if (!tmp1.empty() && !tmp2.empty())
						{
							INSECTS_DEVELOPMENT_DATABASE[name].m_haveStage[i] = true;
							INSECTS_DEVELOPMENT_DATABASE[name].m_lowerThreshold[i] = ToDouble(tmp1);
							INSECTS_DEVELOPMENT_DATABASE[name].m_DDThreshold[i] = ToDouble(tmp2);
						}
					}
				}
			}
		}

		CS.Leave();

		return msg;

	}

	//******************************************************************************************************************
	enum TIDDStat{ O_DD, O_EGG, O_LARVAE_NYMPH, O_PUPAE, O_ADULT, O_MATURE_ADULT, NB_IDD_OUTPUTS };
	const char IDDII_HEADER[] = "DD|Egg|Larvae/Nymph|Pupae|Adult|MatureAdult";

	CIDDModel::CIDDModel()
	{
		NB_INPUT_PARAMETER = 3;
		VERSION = "1.1.2 (2018)";

	}

	CIDDModel::~CIDDModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CIDDModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;


		CInsectDevelopmentDatabase IDDatabase;
		IDDatabase.Set(GetFileData(0));

		int c = 0;
		string insectIndex = parameters[c++].GetString();
		m_insectInfo = IDDatabase[insectIndex];
		m_bCumulative = parameters[c++].GetBool();


		ASSERT(m_insectInfo.m_haveStage[EGG] || m_insectInfo.m_haveStage[LARVAE_NYMPH]);

		return msg;
	}


	//ERMsg CIDDModel::OnExecuteHourly()
	//{
	//	ERMsg msg;

	//	//CIDDStatVector output(m_weather.GetEntireTPeriod(CTM(CTM::HOURLY)));
	//	m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::HOURLY)));
	//	Execute(m_output);
	//	//SetOutput(output);

	//	return msg;
	//}

	ERMsg CIDDModel::OnExecuteDaily()
	{
		ERMsg msg;

		//CIDDStatVector output(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)));
		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)), NB_IDD_OUTPUTS, 0.0, IDDII_HEADER);
		Execute(m_output);
		//SetOutput(output);

		return msg;
	}

	void CIDDModel::Execute(CModelStatVector& output)
	{
		ASSERT(output.GetTM().IsDaily());

		CDegreeDays DD;

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			double DDsum = 0;

			size_t firstStage = m_insectInfo.GetStage(DDsum);
			size_t stage = firstStage;

			CTPeriod p = m_weather.GetEntireTPeriod(CTM::DAILY);
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				if (m_insectInfo.m_haveStage[stage])
				{
					DD.m_lowerThreshold = m_insectInfo.m_lowerThreshold[stage];
					DD.m_upperThreshold = m_insectInfo.m_upperThreshold;
					DDsum += DD.GetDD(m_weather.GetDay(TRef));
				}

				//CTRef d = TRef;
				//d.Transform(output.GetTM());
				output[TRef][O_DD] = DDsum;


				stage = m_insectInfo.GetStage(DDsum);
				size_t f = m_bCumulative ? firstStage : stage;
				for (size_t i = f; i <= stage; i++)
					output[TRef][i+1] = 100;

			}
		}
	}

	/*
	void CIDDModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{

	//Note {1447-1545} is estimated
	static const double LIMIT[NB_SPECIES][NB_PARAMS][2] =
	{
	{{179,243},{262,337},{ 346,431},{429,525}}
	};

	std::vector<double> obs(NB_SPECIES*NB_PARAMS*2);
	for(int i=0; i<NB_SPECIES; i++)
	for(int j=0; j<NB_PARAMS; j++)
	for(int k=0; k<2; k++)
	obs[i*NB_PARAMS*2+j*2+k] = LIMIT[i][j][k];


	m_SAResult.push_back( CSAResult(CTRef(), obs ) );
	}

	void CIDDModel::GetFValueDaily(CStatisticXY& stat)
	{
	if( m_SAResult.size() > 0)
	{
	//look to see if all ai are in growing order
	bool bValid=true;
	for(int s=1; s<NB_PARAMS&&bValid; s++)
	{
	if( m_CR.m_a[s] < m_CR.m_a[s-1] )
	bValid = false;
	}

	CStatistic statDD[NB_PARAMS];
	CStatistic::SetVMiss(-999);
	if( bValid )
	{
	CModelStatVector statSim;
	m_CR.Execute(m_weather, statSim);
	for(int d=0; d<statSim.size(); d++)
	{
	for(int s=0; s<NB_PARAMS; s++)
	{
	if( statSim[d][CIDDContinuingRatio::O_FIRST_STAGE+s+1] > 1 &&
	statSim[d][CIDDContinuingRatio::O_FIRST_STAGE+s+1] < 99 )
	{
	double DD =statSim[d][CIDDContinuingRatio::O_DD];
	statDD[s]+=DD;
	}
	}
	}
	}


	for(int j=0; j<NB_PARAMS; j++)
	{
	for(int k=0; k<2; k++)
	{
	double obs= m_SAResult[0].m_obs[m_species*NB_PARAMS*2 + j*2 + k];
	double sim= statDD[j][k==0?LOWEST:HIGHEST];
	stat.Add(sim, obs);
	}
	}
	}
	}



	*/

}