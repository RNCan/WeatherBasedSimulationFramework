#include "Basic/ModelStat.h"
#include "ModelBase/BioSIMModelBase.h"
#include "SpruceBudworm.h"


namespace WBSF
{

	class CSpruceBudwormDispersal : public CBioSIMModelBase
	{

	public:

		CSpruceBudwormDispersal();
		virtual ~CSpruceBudwormDispersal();

		virtual ERMsg OnExecuteHourly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static WBSF::CBioSIMModelBase* CreateObject(){ return new CSpruceBudwormDispersal; }

	protected:

		size_t m_nbMoths;
	};
}

