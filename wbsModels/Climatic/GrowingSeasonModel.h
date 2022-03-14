#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	class CGrowingSeasonModel : public CBioSIMModelBase
	{
	public:

		enum TInput{ BEGIN, END, NB_INPUT };

		CGrowingSeasonModel();
		virtual ~CGrowingSeasonModel();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CGrowingSeasonModel; }

	private:

		CGSInfo m_begin;
		CGSInfo m_end;
	};

}