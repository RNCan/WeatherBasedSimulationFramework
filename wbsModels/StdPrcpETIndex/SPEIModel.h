#pragma once


#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	class CSPEIModel : public CBioSIMModelBase
	{
	public:

		enum TET { THORNTHWAITE, HARGREAVES_SAMANI, PENMAN_MONTEITH };
		CSPEIModel();
		virtual ~CSPEIModel();

		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CSPEIModel; }

	protected:


		void HargreavesSamani(const CWeatherStation& weather, double etpSeries[]);
		void PenmanMonteith(const CWeatherStation& weather, double etpSeries[]);
		

		size_t m_k;
		size_t m_ETType;
	};
}