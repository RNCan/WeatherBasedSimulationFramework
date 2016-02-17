#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/ContinuingRatio.h"

namespace WBSF
{

	enum TStage{ HIVER, PREMIER, DEUXIEME, NB_GENERATIONS };
	enum TOutput{ DEGRES_DAY, OUT_HIVER, OUT_PREMIER, OUT_DEUXIEME, NB_OUTPUT };
	extern const char OIGNON_MOUCHE_HEADER[];
	extern const char OIGNON_MOUCHE_HEADER_CR[];
	typedef CContinuingRatio<NB_GENERATIONS, 0, 1, OIGNON_MOUCHE_HEADER_CR> COignonMoucheRatioContinue;
	

	class CModeleOignonMouche : public CBioSIMModelBase
	{
	public:


		CModeleOignonMouche();
		virtual ~CModeleOignonMouche();

		virtual ERMsg OnExecuteHourly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void AddHourlyResult(const StringVector& header, const StringVector& data);
		virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueHourly(CStatisticXY& stat);
		virtual void GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CModeleOignonMouche; }


	protected:

		size_t m_species;
		COignonMoucheRatioContinue m_CR;

		static const int NB_SPECIES = 1;
		static const double A[NB_SPECIES][NB_GENERATIONS];
		static const double B[NB_SPECIES][NB_GENERATIONS];
	};

}