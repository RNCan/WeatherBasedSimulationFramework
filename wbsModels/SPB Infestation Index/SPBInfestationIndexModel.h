#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	

	class CSPBInfestationIndexModel : public CBioSIMModelBase
	{

	public:

		
		CSPBInfestationIndexModel();
		virtual ~CSPBInfestationIndexModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CSPBInfestationIndexModel; }


		protected:

		void ComputeSeasonalVariable(const CWeatherYears& weather, CModelStatVector& output);
		

	};

}