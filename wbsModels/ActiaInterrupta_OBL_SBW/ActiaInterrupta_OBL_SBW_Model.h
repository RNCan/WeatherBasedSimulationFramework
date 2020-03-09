#include "ModelBase/BioSIMModelBase.h"
#include "ActiaInterrupta_OBL_SBW.h"

namespace WBSF
{
	
	class CActiaInterrupta_OBL_SBW_Model : public CBioSIMModelBase
	{

	public:

		CActiaInterrupta_OBL_SBW_Model();
		virtual ~CActiaInterrupta_OBL_SBW_Model();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteAtemporal();

		static CBioSIMModelBase* CreateObject(){ return new CActiaInterrupta_OBL_SBW_Model; }

	protected:

		void ExecuteDailyAllGenerations(std::vector<CModelStatVector>& stat);


		bool m_bHaveAttrition;
		double m_generationAttrition;
		double m_diapauseAge;
		double m_lethalTemp;
		double m_criticalDaylength;
		size_t m_preOvip; //[days]
		//bool m_bOnGround;

	};




}