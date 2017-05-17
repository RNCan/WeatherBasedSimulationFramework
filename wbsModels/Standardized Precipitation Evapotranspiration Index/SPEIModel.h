#pragma once


#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	class CSPEIModel : public CBioSIMModelBase
	{
	public:


		CSPEIModel();
		virtual ~CSPEIModel();

		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CSPEIModel; }

	protected:

		size_t m_k;
	};
}