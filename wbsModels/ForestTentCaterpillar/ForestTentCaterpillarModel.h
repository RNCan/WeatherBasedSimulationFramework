//********************* JR 9 Jan 1995 ***********************
//   This program takes as argument the current project path
//   and set file name from which it should read its set 
//   parameters. If not provided, current.cfs in the current
//   directory is used
//***********************************************************
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CForestTentCaterpillarModel : public CBioSIMModelBase
	{

	public:

		enum { ORIGINAL, REMI_PARAMETER };

		CForestTentCaterpillarModel();
		virtual ~CForestTentCaterpillarModel();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CForestTentCaterpillarModel; }

		int m_type;
	};

}