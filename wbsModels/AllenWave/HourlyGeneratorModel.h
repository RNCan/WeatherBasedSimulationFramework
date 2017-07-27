#pragma once


#include "ModelBase/BioSIMModelBase.h"

enum TAllenMinMaxHour { ALLEN_FIXED, ALLEN_SUN, NB_ALLEN_TYPE };


namespace WBSF
{
	class CHourlyGeneratorModel : public CBioSIMModelBase
	{
	public:

		CHourlyGeneratorModel();
		virtual ~CHourlyGeneratorModel();

		virtual ERMsg OnExecuteHourly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CHourlyGeneratorModel; }

		size_t m_method;
	};
}