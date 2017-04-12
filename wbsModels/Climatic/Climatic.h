#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CClimaticModel : public CBioSIMModelBase
	{
	public:


		CClimaticModel();
		virtual ~CClimaticModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();
		
		virtual void AddSAResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueHourly(CStatisticXY& stat);
		virtual void GetFValueDaily(CStatisticXY& stat);
		virtual void GetFValueMonthly(CStatisticXY& stat);

		static CBioSIMModelBase* CreateObject(){ return new CClimaticModel; }

	private:

		double GetSh(int n, int h)const;
		double GetH(int h)const;
		double GetVarH(const CDay& day, int hour, int var)const;
		double GetTd(const CDay& day, int hour)const;

		bool m_bInit;
		double GetSn(int n, int t);
		double GetS(int h);
		double GetHourlyWS(const CDay& day, int hour);

		//variable to optimized;
		int m_varType;

		//wind
		std::array<double, 2> m_a;
		std::array<double, 2> m_b;

		//TDew
		double m_x0;
		double m_x1;
		double m_x2;
		double m_x3;

		int GetFrostDay(int year, const double& th);
	};
}