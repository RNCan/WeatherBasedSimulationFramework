#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CClimaticFromTPOnlyModel : public CBioSIMModelBase
	{
	public:


		CClimaticFromTPOnlyModel();
		virtual ~CClimaticFromTPOnlyModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();

		static CBioSIMModelBase* CreateObject(){ return new CClimaticFromTPOnlyModel; }

	private:

		int GetFrostDay(int year, const double& th);
		int GetDaysBelow(int year, const double& th);
		double GetLowestMinimum();
		double GetHighestMaximum();

		//double m_threshold;
		//int m_PETType;
	};
}