//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Geomatic/GridInterpolBase.h"
#include "Geomatic/Variogram.h"
#include "newmat/newmat.h"


namespace WBSF
{

	class CParamUK
	{
	public:
		CParamUK();

		void Reset();

		double m_xsiz;
		double m_ysiz;
		float m_outputNoData;

		size_t m_nbPoint;
		size_t m_nxdis, m_nydis, m_nzdis;
		double m_radius;


		bool m_bSK;
		double m_SKmean;
		bool m_bTrend;

		//bool bFiltnug;
		//bool bUnbiased;
	};


	class CDiscretisation : public CGridPointVector
	{
	public:

		CDiscretisation();
		void Reset();

		//int size()const{ return CGridPointVector::size(); }
		void Initialization(const CGridPoint& pt, const CVariogram& variogram, const CParamUK& p, bool bFillNugget);

		double cbb;
	};

	typedef CGeoRegression CExternalDrift;

	class CUniversalKriging : public CGridInterpolBase
	{
	public:

		static void FreeMemoryCache();

		CUniversalKriging();
		virtual ~CUniversalKriging();
		void Reset();

		virtual ERMsg Initialization();
		virtual double GetOptimizedR²()const;
		virtual void GetParamterset(CGridInterpolParamVector& parameterset);
		virtual std::string GetFeedbackBestParam()const;
		virtual std::string GetFeedbackOnOptimisation(const CGridInterpolParamVector& parameterset, const std::vector<double>& optimisationR²)const;
		virtual double Evaluate(const CGridPoint& pt, int iXval = -1)const;
		virtual bool GetVariogram(CVariogram& variogram)const;

		CParamUK m_p;

	protected:


		int GetMDT()const;

		void Init_va(const CGridPoint& pt, int iXVal, CGridPointVector& va)const;
		void Init_a(const CGridPoint& pt, CGridPointVector& va, NEWMAT::Matrix &a)const;
		void Init_r(const CGridPoint& pt, CGridPointVector& va, NEWMAT::ColumnVector& r)const;

		CVariogram* GetVariogram()const;
		


		CVariogram* m_pVariogram;
		CExternalDrift m_externalDrift;
		std::unique_ptr<CANNSearch> m_pANNSearch;
		size_t m_lastCheckSum;

		static CVariogramCache VARIOGRAM_CACHE;

	};

}