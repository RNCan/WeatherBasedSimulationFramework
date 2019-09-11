
#include "ModelBase/BioSIMModelBase.h"
namespace WBSF
{
	//class CWeatherYear;

	class CSiteIndexClimateModel : public CBioSIMModelBase
	{

	public:

		CSiteIndexClimateModel();
		virtual ~CSiteIndexClimateModel();

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		static CBioSIMModelBase* CreateObject() { return new CSiteIndexClimateModel; }

	protected:



		static double GetDegreeDays(const CWeatherYear& weather);
		static double GetUtilPrcp(const CWeatherYear& weather);
		static double GetAridite(const CWeatherYear& weather);
		static double GetEvapotranspiration(const CWeatherYear& weather);
		static double GetVaporPressureDeficit(const CWeatherYear& weather);


	};

}