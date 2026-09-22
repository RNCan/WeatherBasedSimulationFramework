#include "ModelBase/BioSIMModelBase.h"
#include "../GypsyMoth/EggModel.h"


namespace WBSF
{

	
	class CSaintAmantModel : public CEggModel
	{

	public:


		CSaintAmantModel(const CGMEggParam& param);
		virtual ~CSaintAmantModel();


		virtual ERMsg ComputeHatch(const CWeatherStation& weather, const CTPeriod& p);
		void ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& stat);
		
		bool m_bApplyAttrition;
		

	};

}