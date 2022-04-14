#pragma once


#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CClimaticMPB : public CBioSIMModelBase
	{
	public:


		CClimaticMPB();
		virtual ~CClimaticMPB();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CClimaticMPB; }

	protected:

	
		

	};


}