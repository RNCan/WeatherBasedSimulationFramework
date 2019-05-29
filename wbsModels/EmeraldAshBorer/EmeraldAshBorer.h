#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

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