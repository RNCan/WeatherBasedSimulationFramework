#pragma once


#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	//Stages
	enum TStage{ EGG, L1, L2, L3, L4, L5, PUPAE, ADULT, MATURE_ADULT, NB_STAGES };

	class CInsectDevelopmentIII
	{
	public:

		CInsectDevelopmentIII()
		{
			m_lowerThreshold = 0;
			m_upperThreshold = 999;
			m_haveStage.fill(false);
			m_DDThreshold.fill(0);
		}


		std::string m_name;

		double m_lowerThreshold;
		double m_upperThreshold;

		std::array<bool, NB_STAGES> m_haveStage;
		std::array<double, NB_STAGES> m_DDThreshold;


		size_t GetStage(double DD)const
		{
			size_t s = NOT_INIT;
			size_t ss = 0;
			for (size_t i = EGG; i < NB_STAGES && s == NOT_INIT; i++)
			{
				if (m_haveStage[i])
				{
					//s=i+1;
					DD -= m_DDThreshold[i];
					if (DD < 0)
						s = i;

					ss = i;
				}
			}

			if (s==NOT_INIT)
				s=ss+1;

			ASSERT(s < NB_STAGES);
			return s;
		}

	};


	class CInsectDevelopmentDatabaseIII
	{
	public:

		ERMsg Set(const std::string& data);
		const CInsectDevelopmentIII& operator[](const std::string& i)const{ return INSECTS_DEVELOPMENT_DATABASE[i]; }

	protected:

		static CCriticalSection CS;
		static std::map<std::string, CInsectDevelopmentIII> INSECTS_DEVELOPMENT_DATABASE;
	};




	class CIDDModel : public CBioSIMModelBase
	{
	public:

		CIDDModel();
		virtual ~CIDDModel();

		//virtual ERMsg OnExecuteHourly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CIDDModel; }

		void Execute(CModelStatVector& output);
	protected:

		CInsectDevelopmentIII m_insectInfo;
		bool m_bCumulative;
	};
}