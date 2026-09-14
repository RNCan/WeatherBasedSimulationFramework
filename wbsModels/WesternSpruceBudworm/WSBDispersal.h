#include "Basic/ModelStat.h"
#include "ModelBase/BioSIMModelBase.h"
#include "WSpruceBudworm.h"


namespace WBSF
{

	class CWSBDispersal : public CBioSIMModelBase
	{

	public:

		CWSBDispersal();
		virtual ~CWSBDispersal();

		//virtual ERMsg OnExecuteHourly();
		virtual ERMsg OnExecuteAtemporal();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static WBSF::CBioSIMModelBase* CreateObject(){ return new CWSBDispersal; }

	protected:

		size_t m_nbMoths;
		double m_defoliation;
		double m_survivalRate;
		int m_adult_longivity_max;//maximum adult longevity [days]
	};
}

