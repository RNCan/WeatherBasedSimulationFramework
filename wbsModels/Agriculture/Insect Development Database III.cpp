//**********************************************************************
//
// 06/08/2013 Rémi Saint-Amant	Create from excel file 
//**********************************************************************
#include <math.h>
#include <crtdbg.h>


#include "Basic/UtilStd.h"
#include "Basic/CSV.h"
#include "Basic/DegreeDays.h"
#include "ModelBase/EntryPoint.h"
#include "Insect Development Database III.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CIDDModel::CreateObject);



	CCriticalSection CInsectDevelopmentDatabaseIII::CS;
	std::map<string, CInsectDevelopmentIII> CInsectDevelopmentDatabaseIII::INSECTS_DEVELOPMENT_DATABASE;


	ERMsg CInsectDevelopmentDatabaseIII::Set(const std::string& data)
	{
		ERMsg msg;

		CS.Enter();
		if (INSECTS_DEVELOPMENT_DATABASE.empty())
		{
			std::stringstream stream(data);

			if (msg)
			{
				//INSECTS_DEVELOPMENT_DATABASE.reserve(50);

//				int s = 0;

				enum TInput{ I_NAME, I_SCIENTIFIC_NAME, I_COMMON_NAME, I_BASE_DEVEL, I_UPPER_DEVEL, I_EGG_DD, I_L1_DD, I_L2_DD, I_L3_DD, I_L4_DD, I_L5_DD, I_PUPAE_DD, I_ADULT_DD, I_MATURE_ADULT_DD, NB_INPUT_COLUMNS };

				//RowReader row;
				//stream >> row; //read header
				for (CSVIterator loop(stream); loop != CSVIterator(); ++loop)
				{
					//stream >> row;

					if ((*loop).size() == NB_INPUT_COLUMNS)
					{
						//INSECTS_DEVELOPMENT_DATABASE.resize(INSECTS_DEVELOPMENT_DATABASE.size() + 1);
						string name = (*loop)[I_NAME];
						INSECTS_DEVELOPMENT_DATABASE[name].m_name = (*loop)[I_NAME];

						ASSERT(!(*loop)[I_BASE_DEVEL].empty());
						INSECTS_DEVELOPMENT_DATABASE[name].m_upperThreshold = ToDouble((*loop)[I_BASE_DEVEL]);
						if (!(*loop)[I_UPPER_DEVEL].empty())
							INSECTS_DEVELOPMENT_DATABASE[name].m_upperThreshold = ToDouble((*loop)[I_UPPER_DEVEL]);

						for (size_t i = 0; i < NB_STAGES; i++)
						{
							string tmp = TrimConst((*loop)[I_EGG_DD + i]);
							if (!tmp.empty())
							{
								INSECTS_DEVELOPMENT_DATABASE[name].m_haveStage[i] = true;
								INSECTS_DEVELOPMENT_DATABASE[name].m_DDThreshold[i] = ToDouble(tmp);
							}
						}

						//s++;
					}
				}
			}
		}

		CS.Leave();

		return msg;

	}



	enum TDailyStat{ O_DD, O_EGG, O_L1, O_L2, O_L3, O_L4, O_L5, O_PUPAE, O_ADULT, O_MATURE_ADULT, NB_IDDIII_OUTPUTS };
	extern const char IDDIII_HEADER[] = "DD|Egg|L1|L2|L3|L4|L5|Pupae|Adult|MatureAdult";
	//typedef CModelStatVectorTemplate<NB_IDDIII_STATS, IDDIII_HEADER> CIDDIIIStatVector;


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


		CInsectDevelopmentDatabaseIII IDDatabase;
		IDDatabase.Set(GetFileData(0));

		int c = 0;
		string name = parameters[c++].GetString();
		m_insectInfo = IDDatabase[name];
		m_bCumulative = parameters[c++].GetBool();

		
		/*if (insectIndex >= 0 && insectIndex < IDDatabase.Get().size())
		{
			m_insectInfo = IDDatabase[insectIndex];
			m_bCumulative = parameters[c++].GetBool();
		}
		else
		{
			msg = GetErrorMessage(ERROR_INVALID_LIST_INDEX);
		}
*/

		ASSERT(m_insectInfo.m_haveStage[EGG] || m_insectInfo.m_haveStage[L1]);


		return msg;
	}

/*
	ERMsg CIDDModel::OnExecuteHourly()
	{
		ERMsg msg;

		CIDDIIIStatVector output(m_weather.GetEntireTPeriod(CTM(CTM::HOURLY)));
		Execute(output);
		SetOutput(output);

		return msg;
	}
*/
	ERMsg CIDDModel::OnExecuteDaily()
	{
		ERMsg msg;

		//CIDDIIIStatVector output(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)));
		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)), NB_IDDIII_OUTPUTS, 0.0, IDDIII_HEADER);
		Execute(m_output);
	//	SetOutput(output);

		return msg;
	}

	void CIDDModel::Execute(CModelStatVector& output)
	{
		CDegreeDays DD(CDegreeDays::DAILY_AVERAGE);

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			double DDsum = 0;

			size_t firstStage = m_insectInfo.GetStage(DDsum);
			size_t stage = firstStage;

			CTPeriod p = m_weather.GetEntireTPeriod();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				if (m_insectInfo.m_haveStage[stage])
				{
					DD.m_lowerThreshold = m_insectInfo.m_lowerThreshold;
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

}