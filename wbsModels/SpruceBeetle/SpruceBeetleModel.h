#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "SpruceBeetle.h"

namespace WBSF
{

	class CSpruceBeetleModel : public CBioSIMModelBase
	{
	public:


		CSpruceBeetleModel();
		virtual ~CSpruceBeetleModel();

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		static CBioSIMModelBase* CreateObject(){ return new CSpruceBeetleModel; }


	protected:

		size_t m_flight_peak;
	};


}