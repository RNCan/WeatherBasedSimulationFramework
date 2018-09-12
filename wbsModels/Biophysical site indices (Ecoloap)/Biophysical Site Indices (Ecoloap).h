// SiteClimateIndice.h: interface for the CSiteClimateIndice class.
//
//////////////////////////////////////////////////////////////////////
#pragma once


#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/ContinuingRatio.h"
namespace WBSF
{
	class CWeatherYear;
	
	class CBiophysicalSiteIndices : public CBioSIMModelBase
	{
	public:

		CBiophysicalSiteIndices();
		virtual ~CBiophysicalSiteIndices();


		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		
		
		static CBioSIMModelBase* CreateObject() { return new CBiophysicalSiteIndices; }

	protected:

		static double DegreeDays(const CWeatherYear& weather, double threshold);
		static void Aridite(const CWeatherYear& weather, double& A, double& E, double& PU);
		static double DeficitVaporPressure(const CWeatherYear& weather);

	};
}