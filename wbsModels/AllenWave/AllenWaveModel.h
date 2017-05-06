#pragma once


#include "ModelBase/BioSIMModelBase.h"

enum TAllenMinMaxHour { ALLEN_FIXED, ALLEN_SUN, NB_ALLEN_TYPE };


namespace WBSF
{
	class CAllenWaveModel : public CBioSIMModelBase
	{
	public:


		CAllenWaveModel();
		virtual ~CAllenWaveModel();

		virtual ERMsg OnExecuteHourly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CAllenWaveModel; }

	protected:


		size_t m_hourTmin;
		size_t m_hourTmax;
		
	};
}