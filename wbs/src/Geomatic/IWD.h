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



namespace WBSF
{

	struct DataTable;
	class CGeoMapBase;
	class CCallback;
	class CXValidation;



	class CIWD : public CGridInterpolBase
	{
	public:

		CIWD();
		virtual ~CIWD();

		virtual ERMsg Initialization(CCallback& callback);
		virtual std::string GetFeedbackBestParam()const;
		virtual void GetParamterset(CGridInterpolParamVector& parameterset);
		virtual double Evaluate(const CGridPoint& pt, int iXval = -1)const;

		void Search(CGridPointResultVector& result, const CGridPoint& pt, int iXval = -1)const;
		bool IsInit() const { return (bool)m_pANNSearch; }

	protected:

		double GetIWD(const CGridPointResultVector& result)const;

		//computing variable
		size_t m_lastCheckSum;
		std::unique_ptr<CANNSearch> m_pANNSearch;

	};
}