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
class Forest;
class Data;

namespace WBSF
{


	class CRandomForest : public CGridInterpolBase
	{
	public:

		enum TreeType { TREE_CLASSIFICATION, TREE_REGRESSION, TREE_SURVIVAL, TREE_PROBABILITY, NB_TREE_TYPES };

		CRandomForest();
		virtual ~CRandomForest();

		void Reset();

		virtual ERMsg Initialization(CCallback& callback);
		virtual void GetParamterset(CGridInterpolParamVector& parameterset);

		virtual std::string GetFeedbackBestParam()const;
		virtual double Evaluate(const CGridPoint& pt, int iXval = -1)const;

	protected:

		ERMsg CreateForest(size_t type, size_t nbTrees, size_t importance_mode);

		Forest* m_pForest;
		double m_inc;
	};

}