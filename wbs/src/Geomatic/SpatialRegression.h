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

		enum TTrend { LON = 1, LAT = 2, ELEV = 4, EXPO = 8, SHORE = 16, LON² = 32, LAT² = 64, ELEV² = 128, EXPO² = 256, SHORE² = 512, NB_TERMS = 10 };
		static const char* GetName(size_t i) { ASSERT(i >= 0 && i < 3*5); return TERMS_NAME[i]; }

		CTerm(int v = 0) { Init(v); }
		void Reset() { Init(0); }
		void Init(int v) { m_v = v; }

		std::string GetName()const;

		bool operator == (const CTerm& in)const { return m_v == in.m_v; }
		bool operator != (const CTerm& in)const { return !operator==(in); }


		bool IsInit()const { return m_v > 0; }
		int GetNbItem()const;

		double GetE(const CGridPoint& pt)const { return GetE(pt.m_x, pt.m_y, pt.m_z, pt.GetExposition(), pt.m_shore); }
		double GetE(double x, double y, double elev, double expo, double shore)const;

		bool HaveExposition()const { return HaveExposition(m_v); }
		bool HaveShore()const { return HaveShore(m_v); }
		bool IsValid(bool bUseLatLon, bool bUseElev, bool bUseExpo, bool bUseShore)const { return IsValid(m_v, bUseLatLon, bUseElev, bUseExpo, bUseShore); }

		int m_v;

		static size_t GetNbTerms() {return 1 << NB_TERMS;};
		
		static bool HaveLatLon(int v) { return v & LON || v & LON²|| v & LAT || v & LAT²; }
		static bool HaveElevation(int v) { return v & ELEV || v & ELEV²; }
		static bool HaveExposition(int v){ return v & EXPO || v & EXPO²; }
		static bool HaveShore(int v){ return v & SHORE || v & SHORE²; }
		static bool IsValid(int v, bool bUseLatLon, bool bUseElev, bool bUseExpo, bool bUseShore){ return v>0 && (bUseLatLon || !HaveLatLon(v)) && (bUseElev || !HaveElevation(v)) && (bUseExpo || !HaveExposition(v)) && (bUseShore || !HaveShore(v)); }
		
	protected:

		static const char* TERMS_NAME[3*5];
	};

	typedef std::vector<CTerm> CTermVector;

	class CGeoRegression : public CTermVector
	{
	public:

		CGeoRegression(size_t size = 0) :CTermVector(size) { m_intercept = 0; m_R² = 0; }

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

		//int size()const{ return (int)CTermVector::size(); }
		double GetF(const CGridPoint& pt)const { return GetF(pt.m_x, pt.m_y, pt.m_z, pt.GetExposition(), pt.m_shore); }
		double GetF(double x, double y, double elev, double expo, double shore)const;

		//return -999 if not able to do regression
		double Compute(const CGridPointVector& pts, const CPrePostTransfo& transfo);

		bool HaveExposition()const
		{
			bool bRep = false;
			for (size_t i = 0; i < size() && !bRep; i++)
				bRep = at(i).HaveExposition();

			return bRep;
		}

	protected:

		//parameter of term
		std::vector<double> m_param;
		double m_intercept;
		double m_R²;
	};


	class CSpatialRegression : public CGridInterpolBase
	{
	public:


		CSpatialRegression();
		virtual ~CSpatialRegression();

		void Reset();

		virtual ERMsg Initialization(CCallback& callback);
		virtual void GetParamterset(CGridInterpolParamVector& parameterset);

		virtual std::string GetFeedbackBestParam()const;
		virtual double Evaluate(const CGridPoint& pt, int iXval = -1)const;

	protected:

		ERMsg StepWise(const CGridPointVector& calibPts, std::vector<int>& regressionTerm, double criticalR2, CCallback& callback)const;

		//regression term
		CGeoRegression m_regression;
	};

}