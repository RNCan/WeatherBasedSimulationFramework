//********************* JR 9 Jan 1995 ***********************
//   This program takes as argument the current project path
//   and set file name from which it should read its set 
//   parameters. If not provided, current.cfs in the current
//   directory is used
//***********************************************************
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CLaricobiusLarvaeModel : public CBioSIMModelBase
	{

	public:

		enum { ORIGINAL, REMI_PARAMETER };

		CLaricobiusLarvaeModel();
		virtual ~CLaricobiusLarvaeModel();

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLaricobiusLarvaeModel; }

		void ExecuteDaily(CModelStatVector& stat);
	};

}