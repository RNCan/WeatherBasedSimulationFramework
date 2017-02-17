//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "basic/ERMsg.h"
#include "Geomatic/GridInterpolBase.h"

namespace WBSF
{

	class CTerm
	{
	public:
		enum TTrend{ LON = 1, LAT = 2, ELEV = 4, EXPO = 8, LAT² = 16, LON² = 32, ELEV² = 64, EXPO² = 128, NB_TERM = 8 };
		static const char* GetName(int i){ ASSERT(i >= 0 && i < NB_TERM); return TERM_NAME[i]; }

		CTerm(int v = 0){ Init(v); }
		void Reset(){ Init(0); }
		void Init(int v){ m_v = v; }

		std::string GetName()const;

		bool operator == (const CTerm& in)const{ return m_v == in.m_v; }
		bool operator != (const CTerm& in)const{ return !operator==(in); }


		bool IsInit()const{ return m_v > 0; }
		int GetNbItem()const;

		double GetE(const CGridPoint& pt)const{ return GetE(pt.m_x, pt.m_y, pt.m_z, pt.GetExposition()); }
		double GetE(double x, double y, double elev, double expo)const;

		bool HaveExposition()const { return m_v&EXPO || m_v&EXPO²; }

		int m_v;

	protected:


		static const char* TERM_NAME[NB_TERM];
	};

	typedef std::vector<CTerm> CTermVector;

	class CGeoRegression : public CTermVector
	{
	public:

		CGeoRegression(int size = 0) :CTermVector(size){ m_intercept = 0; m_R² = 0; }

		void Reset()
		{
			clear();
			m_param.clear();
			m_intercept = 0;
			m_R² = 0;

		}

		CGeoRegression& operator=(const std::vector<int>& in)
		{
			resize(in.size());
			for (size_t i = 0; i < in.size(); i++)
				at(i) = in[i];

			
			return *this;
		}

		std::string GetEquation()const;//terms parameters like equation
		std::string GetTerms()const;//terms

		int size()const{ return (int)CTermVector::size(); }
		double GetF(const CGridPoint& pt)const{ return GetF(pt.m_x, pt.m_y, pt.m_z, pt.GetExposition()); }
		double GetF(double x, double y, double elev, double expo)const;

		//return -999 if not able to do regression
		double Compute(const CGridPointVector& pts, const CPrePostTransfo& transfo);

		bool HaveExposition()const
		{
			bool bRep = false;
			for (int i = 0; i < size() && !bRep; i++)
				bRep = at(i).HaveExposition();

			return bRep;
		}

	protected:

		int GetTrendIndex(int i)const;

		//parameter of term
		std::vector<double> m_param;
		double m_intercept;
		double m_R²;
	};


	class CSpatialRegression : public CGridInterpolBase
	{
	public:

		enum TTerm{ NB_TERM = 26 };
		CSpatialRegression();
		virtual ~CSpatialRegression();

		void Reset();



		virtual ERMsg Initialization(CCallback& callback);
		virtual void GetParamterset(CGridInterpolParamVector& parameterset);
		//virtual double GetOptimizedR²()const;

		virtual std::string GetFeedbackBestParam()const;
		virtual double Evaluate(const CGridPoint& pt, int iXval = -1)const;


	protected:

		double StepWise(std::vector<int>& regressionTerm, double criticalR2)const;

		//regression term
		CGeoRegression m_regression;

		int GetTerm(int i, bool bExpo)const{ ASSERT(i >= 0 && i < NB_TERM); return bExpo ? TERM_WITH_EXPO[i] : TERM_WITHOUT_EXPO[i]; }

		static const int TERM_WITH_EXPO[NB_TERM];
		static const int TERM_WITHOUT_EXPO[NB_TERM];
	};

}