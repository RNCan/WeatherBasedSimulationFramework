#pragma once

#include "Basic/ModelStat.h"
#include "ModelBase/BioSIMModelBase.h"
#include "BlueStainVariables.h"

namespace WBSF
{


	

	enum TStyle{ ADD, MUL, NB_STYLE };
	class CLimits : public std::array<double, NB_STYLE>
	{
	public:

		CLimits(double a = 0, double m = 1) :
			array<double, 2>({ { a, m } })
		{
		}

		friend CLimits operator-(const CLimits& in1, const CLimits& in2){ return CLimits(in1[ADD] - in2[ADD], in1[MUL] - in2[MUL]); }
		friend CLimits operator+(const CLimits& in1, const CLimits& in2){ return CLimits(in1[ADD] + in2[ADD], in1[MUL] + in2[MUL]); }
		friend CLimits operator*(double v, const CLimits& in){ return CLimits(in[ADD] * v, in[MUL] * v); }
		friend CLimits operator/(const CLimits& in, double v){ return CLimits(in[ADD] / v, in[MUL] / v); }
		friend CLimits operator/(const CLimits& in1, const CLimits& in2){ return CLimits(in1[ADD] / in2[ADD], in1[MUL] / in2[MUL]); }
		CLimits& operator-=(const CLimits& in){ at(ADD) -= in[ADD]; at(MUL) -= in[MUL]; return *this; }
		CLimits& operator+=(const CLimits& in){ at(ADD) += in[ADD]; at(MUL) += in[MUL]; return *this; }
		CLimits& operator+=(double in){ at(ADD) += in; at(MUL) += in; return *this; }
		CLimits& operator*=(double in){ at(ADD) *= in; at(MUL) *= in; return *this; }

	};

	class CBlueStainIndexModel : public CBioSIMModelBase
	{
	public:

		enum TLimit{ L_LO, L_10, L_90, L_HI, NB_LIMITS };
		static const double PERCENTILS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS];

		CBlueStainIndexModel();
		virtual ~CBlueStainIndexModel();
		virtual ERMsg OnExecuteAnnual();
		//virtual ERMsg CBlueStainIndexModel::OnExecuteAtemporal();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual void GetFValueAnnual(CStatisticXY& stat);
		virtual void AddAnnualResult(const StringVector& header, const StringVector& data);

		static CBioSIMModelBase* CreateObject(){ return new CBlueStainIndexModel; }

		//ERMsg ExecuteAtemporal(CModelStatVector& output);
		static CLimits GetF(const double LIMITS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS], size_t v, size_t nbVars, double f);
		static CLimits GetBSI(const double LIMITS[CBlueStainVariables::NB_VARIABLES][NB_LIMITS], const CSelectionVars& vars, const CModelStatVector& values);


	protected:

		CSelectionVars m_vars;
		double m_lo;
		double m_hi;
		
		
	};
}