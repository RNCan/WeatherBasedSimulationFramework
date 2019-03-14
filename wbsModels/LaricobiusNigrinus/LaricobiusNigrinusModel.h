//********************* JR 9 Jan 1995 ***********************
//   This program takes as argument the current project path
//   and set file name from which it should read its set 
//   parameters. If not provided, current.cfs in the current
//   directory is used
//***********************************************************
#include "ModelBase/BioSIMModelBase.h"
#include "LaricobiusNigrinus.h"
namespace WBSF
{

	class CLaricobiusNigrinusModel : public CBioSIMModelBase
	{

	public:

		CLaricobiusNigrinusModel();
		virtual ~CLaricobiusNigrinusModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CLaricobiusNigrinusModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueDaily(CStatisticXY& stat)override;

		virtual void AddAnnualResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueAnnual(CStatisticXY& stat)override;

		protected:

		CTRef m_start;
		double m_threshold;
		double m_sumDD;

		bool m_bCumul;
		double m_P[2][LNF::NB_RDR_PARAMS];
		double m_F[4];
		double m_peak;
		double m_s;


		std::set<int> m_years;

		
		void ExecuteDaily(const CWeatherYear& weather, CModelStatVector& stat);

	};

}