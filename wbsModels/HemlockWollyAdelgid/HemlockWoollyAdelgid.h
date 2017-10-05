#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CHemlockWoollyAdelgidCMModel : public CBioSIMModelBase
	{
	public:

		enum TEquation { EQUATION_3, EQUATION_6, EQUATION_SA, NB_COLD_EQ };


		static double Eq3(double Tmin);
		static double Eq6(double Tmin, double DD0, double DD10);
		static double Eq11(double S, double DD10, double DDpro);
		double EqRemi(double Tmin, double DD0, double DDx, double DD10, double Q3, size_t nbDayUnder);

		CHemlockWoollyAdelgidCMModel();
		virtual ~CHemlockWoollyAdelgidCMModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CHemlockWoollyAdelgidCMModel; }

		void ExecuteDaily(CModelStatVector& output);

		void AddDailyResult(const StringVector& header, const StringVector& data);
		void GetFValueDaily(CStatisticXY& stat);


	protected:

		double m_sistensFallDensity; //fall sistens insects desnity [insects/branch]
		size_t m_equation;
		

		size_t m_nbDays;
		double m_Tlow;
		double m_p[6];

		//optimisation var
		bool m_bInit;
	};
}