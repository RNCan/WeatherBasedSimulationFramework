#include "ModelBase/BioSIMModelBase.h"
#include "MeteorusTrachynotus_OBL_SBW.h"

namespace WBSF
{
	
	class CMeteorusTrachynotus_OBL_SBW_Model : public CBioSIMModelBase
	{

	public:

		CMeteorusTrachynotus_OBL_SBW_Model();
		virtual ~CMeteorusTrachynotus_OBL_SBW_Model();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteAtemporal();

		static CBioSIMModelBase* CreateObject(){ return new CMeteorusTrachynotus_OBL_SBW_Model; }

	protected:

		void ExecuteDailyAllGenerations(std::vector<CModelStatVector>& stat);


		
		double m_generationAttrition;
		
		double m_lethalTemp;
		double m_criticalDaylength;
		double m_preOvip;
		
		bool m_bOBLAttition;
		bool m_bSBWAttition;
		double m_pSBW; //proportion of SBW relatively to OBL
	};




}