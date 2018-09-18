#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	//enum TParam{ P_EGG_L1, P_L1_L2, P_L2_L3, P_L3_L4, P_L4_PUPA, P_PUPA_ADULT, NB_PARAMS };


	class CEmeraldAshBorerModel : public CBioSIMModelBase
	{
	public:

		CEmeraldAshBorerModel();
		virtual ~CEmeraldAshBorerModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CEmeraldAshBorerModel; }

		void ExecuteDaily(CModelStatVector& output);
	};
}