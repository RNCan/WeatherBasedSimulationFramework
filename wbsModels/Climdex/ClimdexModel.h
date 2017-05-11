#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	class CClimdexModel : public CBioSIMModelBase
	{
	public:


		CClimdexModel();
		virtual ~CClimdexModel();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CClimdexModel; }

		CTPeriod m_basePeriod;
		double m_nn;
		bool m_bUseBootstrap;
	};
}