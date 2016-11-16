#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "GypsyMoth.h"

namespace WBSF
{


	class CGypsyMothStability : public CBioSIMModelBase
	{
	public:

		CGypsyMothStability();
		virtual ~CGypsyMothStability();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CGypsyMothStability; }

	private:

		void ExecuteWithGeneration();
		void ExecuteWithoutGeneration();

		void Reset();

		int m_hatchModelType;
		CGMEggParam m_eggParam;
		int m_nbGenerations;


	};

}