#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/UtilTime.h"

namespace WBSF
{

	enum TOutputDaily { O_TMAX, O_TMIN, O_PPT, O_SVP_TMAX, O_SVP_TMIN, O_SVP_TDEW, O_VPD, O_TMEAN, O_K_TRF, O_PET, O_AET, O_RUNOFF, O_SMI, O_PSMI, NB_DAILY_OUTPUT };
	enum TOutput { O_PSMI_MIN, O_PSMI_MEAN, O_PSMI_MAX, NB_OUTPUT };

	//typedef CModelStatVectorTemplate<NB_DAILY_OUTPUT> CDailyOutput;
	typedef CModelStatVectorTemplate<NB_OUTPUT> CMonthlyOutput;
	typedef CModelStatVectorTemplate<NB_OUTPUT> CAnnualOutput;


	class CSMIModel : public CBioSIMModelBase
	{
	public:

		enum TModel{ BILINEAR, QUADRATIC_LINEAR };
		CSMIModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CSMIModel; }


	private:

		double GetAETFactor(double SMI)const;
		void ComputeDailyValue(CModelStatVector& output)const;

		double m_model;
		double m_SMIcrit; //Critical soil moisture (mm)
		double m_SMImax; //Maximum soil moisture (mm)


	};
}