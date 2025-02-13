#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/ContinuingRatio.h"



namespace WBSF
{
	enum TMaleInstar{ M_NYPH2o, M_NYPH2, M_PREPUPA, M_PUPA, M_ADULT, M_DEADADULT, NB_MALE_STAGES };
	enum TFemaleInstar { F_NYPH2o, F_NYPH2, F_ADULT, F_DEAD_ADULT, NB_FEMALE_STAGES };
	enum TBabyInstar { EGG, NYPH1, NYPH2, NYPH2o, NB_BABY_STAGES };
	enum TMaleParam { M_NYPH2o_NYPH2, M_NYPH2_PREPUPA, M_PREPUPA_PUPA, M_PUPA_ADULT, M_ADULT_DEADADULT, NB_MALE_PARAMS };
	enum TFemaleParam { F_NYPH2o_NYPH2, F_NYPH2_ADULT, F_ADULT_DEADADULT, NB_FEMALE_PARAMS };
	enum TBabyParam { EGG_NYPH1, NYPH1_NYPH2, NB_BABY_PARAMS };//NYPH2_NYPH2o, 

	//extern const char EuropeanElmScale_header[];
	
	typedef CContinuingRatio<NB_MALE_PARAMS, M_NYPH2o, M_DEADADULT> CEuropeanElmScaleCRm;
	typedef CContinuingRatio<NB_FEMALE_PARAMS, F_NYPH2o, F_DEAD_ADULT> CEuropeanElmScaleCRf;
	typedef CContinuingRatio<NB_BABY_PARAMS, EGG, NYPH2o> CEuropeanElmScaleCRb;

	
	enum TDreistadtParam { D_NYPH2_ADULT, D_ADULT_DEADADULT, NB_DREISTADT_PARAMS };
	typedef CContinuingRatio<NB_DREISTADT_PARAMS, F_NYPH2, F_DEAD_ADULT> CEuropeanElmScaleCRd;


	class CEuropeanElmScaleModel : public CBioSIMModelBase
	{
	public:

		CEuropeanElmScaleModel();
		virtual ~CEuropeanElmScaleModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual bool GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CEuropeanElmScaleModel; }

	protected:

		CEuropeanElmScaleCRm m_CRm;
		CEuropeanElmScaleCRf m_CRf;
		CEuropeanElmScaleCRb m_CRb;
		CEuropeanElmScaleCRd m_CRd;
		int m_firstYear;
		int m_lastYear;



		//Simulated Annealing 
		enum TFallInput { I_M_NYPH2o, I_M_NYPH2, I_M_PREPUPA, I_M_PUPA, I_M_ADULT, I_M_DEADADULT, I_F_NYPH2o, I_F_NYPH2, I_F_ADULT, I_F_DEADADULT, I_EGG, I_NYPH1, I_NYPH2, I_NYPH2o, NB_INPUT };
		static const double Am[NB_MALE_PARAMS];
		static const double Bm[NB_MALE_PARAMS];
		static const double Af[NB_FEMALE_PARAMS];
		static const double Bf[NB_FEMALE_PARAMS];
		static const double Ab[NB_BABY_PARAMS];
		static const double Bb[NB_BABY_PARAMS];
		static const double Ad[NB_DREISTADT_PARAMS];
		static const double Bd;
		static const double THRESHOLDm;
		static const double THRESHOLDf;

	};

}