#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	//Growth Stage

	//Growth Stage
	enum TStage{ EGG, LARVAE_NYMPH, PUPAE, ADULT, MATURE_ADULT, NB_STAGES };

	class CInsectDevelopment
	{
	public:

		CInsectDevelopment()
		{
			m_upperThreshold = 999;
			m_haveStage.fill(false);
			m_lowerThreshold.fill(0);
			m_DDThreshold.fill(0);
		}


		std::string m_name;

		double m_upperThreshold;
		std::array<bool, NB_STAGES> m_haveStage;
		std::array<double, NB_STAGES> m_lowerThreshold;
		std::array<double, NB_STAGES> m_DDThreshold;


		size_t GetStage(double DD)const
		{
			size_t s = NOT_INIT;
			size_t ss = 0;
			for (size_t i = EGG; i < NB_STAGES && s == NOT_INIT; i++)
			{
				if (m_haveStage[i])
				{
					//s=int(i)+1;
					DD -= m_DDThreshold[i];
					if (DD < 0)
						s = i;

					ss = i;
				}
			}

			if (s == NOT_INIT)
				s = ss+1;

			ASSERT(s < NB_STAGES);
			return s;
		}

	};


	class CInsectDevelopmentDatabase
	{
	public:

		ERMsg Set(const std::string& data);
		//const std::vector<CInsectDevelopment>& Get()const{ return INSECTS_DEVELOPMENT_DATABASE; }

		//const CInsectDevelopment& operator[](size_t i)const{ return INSECTS_DEVELOPMENT_DATABASE[i]; }
		const CInsectDevelopment& operator[](const std::string& i)const{ return INSECTS_DEVELOPMENT_DATABASE[i]; }

	protected:

		static CCriticalSection CS;
		static std::map<std::string, CInsectDevelopment> INSECTS_DEVELOPMENT_DATABASE;
	};


	
	//extern const char IDDII_HEADER[];
	//typedef CModelStatVectorTemplate<NB_STATS_BASE, IDDII_HEADER> CIDDStatVector;


	class CIDDModel : public CBioSIMModelBase
	{
	public:

		CIDDModel();
		virtual ~CIDDModel();

		//virtual ERMsg OnExecuteHourly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CIDDModel; }

		void Execute(CModelStatVector& stats);
	protected:

		CInsectDevelopment m_insectInfo;
		bool m_bCumulative;
	};
}