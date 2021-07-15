#pragma once

//#pragma warning( disable : 4201)
//#pragma warning( disable : 4514)

#include "ModelBase/BioSIMModelBase.h"
//#include "ContinuingRatio.h"

namespace WBSF
{


	class CSBWLysykModel : public CBioSIMModelBase
	{
	public:

		enum TSpecies { BALSAM_FIR, WHITE_SPRUCE, NB_SPECIES };
		enum TParams { A2, A3, A4, A5, A6, A7, B, NB_PARAMS };
		enum TStages { L2, L3, L4, L5, L6, L7, L8, NB_STAGES };


		CSBWLysykModel();
		virtual ~CSBWLysykModel();


		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		static CBioSIMModelBase* CreateObject() { return new CSBWLysykModel; }

		ERMsg ExecuteModel(CModelStatVector& stat);

	protected:


		std::array<double, NB_STAGES> ContinuingRatio(double ddays)const;


		size_t m_species;

	};

}