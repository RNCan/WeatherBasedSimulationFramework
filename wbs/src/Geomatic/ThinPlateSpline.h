//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "GridInterpolBase.h"
#include <unordered_map>


namespace WBSF
{

	//class MySpline;
	//typedef std::list<int> key_tracker_type;
	//typedef std::unordered_map< int, std::pair< MySpline*, typename key_tracker_type::iterator>> key_to_value_type;

	class CThinPlateSpline2 : public CGridInterpolBase
	{
	public:

		CThinPlateSpline2();
		virtual ~CThinPlateSpline2();
		void Reset();

		virtual ERMsg Initialization(CCallback& callback);
		virtual double GetOptimizedR²(CCallback& callback)const;
		virtual void GetParamterset(CGridInterpolParamVector& parameterset);
		virtual std::string GetFeedbackBestParam()const;
		virtual double Evaluate(const CGridPoint& pt, int iXval = -1)const;

	protected:

		//static void GetClusterNode(int nbCluster, const CPrePostTransfo& transfo, CGridPointVector& pointIn, CGridPointVector& pointOut, float noData);

		std::unique_ptr<CANNSearch> m_pANNSearch;

		//key_to_value_type m_splinMap;
		//key_tracker_type m_keyTracker;

		CRITICAL_SECTION CS;

		void* m_pObject;
		size_t m_nbDim;
		bool m_bGeographic;
		bool m_bHaveExposition;
		bool m_bUseElevation;
		bool m_bUseShore;


		//a mettre dans m_param;
		//bool m_bClustered;
		double m_inc;
	};

}