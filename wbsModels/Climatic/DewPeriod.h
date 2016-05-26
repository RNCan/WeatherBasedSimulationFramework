#pragma once
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{


	enum TWDMonthlyStat{ O_TMIN, O_TMAX, O_RH15, O_TDEW15_FROM_RH15, O_DEW_PERIOD_FROM_RH15, NB_MONTHLY_STATS };
	typedef CModelStatVectorTemplate<NB_MONTHLY_STATS> CMonthlyStatVector;

	class CDewDuration;
	class CSWEB;

	class CDewPeriodModel : public CBioSIMModelBase
	{
	public:

		CDewPeriodModel();
		virtual ~CDewPeriodModel();

		//virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteMonthly();
		static CBioSIMModelBase* CreateObject(){ return new CDewPeriodModel; }

	protected:

		static double GetHourlyNormal(size_t m, size_t h, size_t v, const CWeatherYears& hourlyData);
		static double GetTDew(double Tmax, double RH15);
		static double GetDewPeriod(double Tmin, double Tmax, double RH3);
	};

}