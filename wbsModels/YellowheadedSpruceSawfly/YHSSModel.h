#pragma once
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CYHSSModel : public CBioSIMModelBase
	{
	public:

		CYHSSModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CYHSSModel; }


		void Reset();

	private:

		double ComputeStageFrequencies(double dd, double freq[9]);

		bool m_bCumulativeOutput;
		int m_adultLongevity;

	};


}