#pragma once

#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{
	class CMPB_CT_model : public CBioSIMModelBase
	{
	public:


		CMPB_CT_model();
		virtual ~CMPB_CT_model();

		//    virtual ERMsg Execute();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameter(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CMPB_CT_model; }




	private:

		//double m_RhoG;
		//double m_MuG;
		//double m_SigmaG;
		//double m_KappaG;
		//double m_RhoL;
		//double m_MuL;
		//double m_SigmaL;
		//double m_KappaL;
		//double m_Lambda0;
		//double m_Lambda1;
		bool m_bMicroClimate;

	};
}