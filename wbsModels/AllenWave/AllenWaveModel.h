#pragma once


#include "ModelBase/BioSIMModelBase.h"


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

		size_t m_hourTmax;
	};
}