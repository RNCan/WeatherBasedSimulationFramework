#include "Basic/ModelStat.h"
#include "ModelBase/BioSIMModelBase.h"
#include "SpruceBudworm.h"


namespace WBSF
{

	class CSpruceBudwormModel : public CBioSIMModelBase
	{

	public:

		CSpruceBudwormModel();
		virtual ~CSpruceBudwormModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static WBSF::CBioSIMModelBase* CreateObject(){ return new CSpruceBudwormModel; }

		//function for simulated annealing
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueDaily(CStatisticXY& stat);

	protected:

		void ExecuteDaily(CModelStatVector& stat, bool bStopL22 = false);

		//parameter
		CTRef m_fixDate;
		double m_fixAI;

		bool m_bApplyAttrition;
		bool m_bApplyAdultAttrition;
		bool m_bFertility;
		short m_treeKind;
		double m_defoliation;
		int m_adult_longivity_max;//maximum adult longevity [days]
	};
}

