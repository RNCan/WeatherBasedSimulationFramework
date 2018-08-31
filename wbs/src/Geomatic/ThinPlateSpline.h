//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Geomatic\GridInterpolBase.h"
#include "Geomatic\ProjectionTransformation.h"
#include <unordered_map>


namespace WBSF
{

	class CThinPlateSpline : public CGridInterpolBase
	{
	public:

		CThinPlateSpline();
		virtual ~CThinPlateSpline();
		void Reset();

		virtual ERMsg Initialization(CCallback& callback);
		virtual double GetOptimizedR²(CCallback& callback)const;
		virtual void GetParamterset(CGridInterpolParamVector& parameterset);
		virtual std::string GetFeedbackBestParam()const;
		virtual double Evaluate(const CGridPoint& pt, int iXval = -1)const;

	protected:


//		std::unique_ptr<CANNSearch> m_pANNSearch;


		CRITICAL_SECTION CS;

		void* m_pObject;
		size_t m_nbDim;
		bool m_bGeographic;
		bool m_bUseElevation;
		bool m_bUseExposition;
		bool m_bUseShore;
		

		//a mettre dans m_param;
		//bool m_bClustered;
		double m_inc;
	};

}