//********************* JR 9 Jan 1995 ***********************
//   This program takes as argument the current project path
//   and set file name from which it should read its set 
//   parameters. If not provided, current.cfs in the current
//   directory is used
//***********************************************************
#include "ModelBase/BioSIMModelBase.h"
#include "WhitemarkedTussockMoth.h"
namespace WBSF
{

	class CWhitemarkedTussockMothModel : public CBioSIMModelBase
	{

	public:

		CWhitemarkedTussockMothModel();
		virtual ~CWhitemarkedTussockMothModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject(){ return new CWhitemarkedTussockMothModel; }

		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;


	protected:

		bool m_bCumul;

		std::array< std::array<double, WTM::NB_DEV_RATE_PARAMS>, WTM::NB_STAGES> m_P;
		std::array< std::array<double, WTM::NB_RDR_PARAMS>, WTM::NB_STAGES> m_D;
		
//		double m_P[WTM::NB_STAGES][WTM::NB_DEV_RATE_PARAMS];
	//	double m_D[WTM::NB_STAGES][WTM::NB_RDR_PARAMS];
		
		std::array<double, WTM::NB_HATCH_PARAMS> m_H;
		std::array<double, 2> m_egg_factor;

		//std::set<int> m_years;
		std::array< std::array<CStatistic,5>, 2> m_Jday;
		std::array< std::array<size_t, 5>, 2> m_col_weight;
		

		
		void ExecuteDaily(const CWeatherYear& weather, CModelStatVector& stat);

		bool IsParamValid()const;
	};

}