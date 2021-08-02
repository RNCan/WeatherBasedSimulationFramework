//*****************************************************************************
// File: DegreeDay.h
//
// Class: CStringFrostModel
//*****************************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/DegreeDays.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	class CFrostEventParam
	{
	public:
		
		enum TSummation { LOWER, LOWER_EQUAL,GREATHER_EQUAL, GREATHER, NB_OP_TYPE };
		CFrostEventParam(WBSF::HOURLY_DATA::TVarH var= WBSF::HOURLY_DATA::H_TMIN, size_t op= LOWER_EQUAL,
		double threshold=0, size_t prec=1 )
		{
			m_var= var;
			m_op= op;
			m_threshold= threshold;
			m_prec = prec;
		}


		WBSF::HOURLY_DATA::TVarH m_var;
		size_t m_op;
		size_t m_prec;
		double m_threshold;

		bool is_throw(const CDataInterface& data)const
		{
			bool bEvent = false;
			float value = WBSF::Round((float)data[m_var][WBSF::MEAN], m_prec);
			float threshold = WBSF::Round((float)m_threshold, m_prec);

			switch (m_op)
			{
			case LOWER:bEvent = value < threshold; break;
			case LOWER_EQUAL:bEvent = value <= threshold; break;
			case GREATHER_EQUAL:bEvent = value >= threshold; break;
			case GREATHER:bEvent = value > threshold; break;

			default: ASSERT(false);
			}

			return bEvent;
		}
		

	};

	class CFrostEventOutput
	{
	public:
		
		CFrostEventOutput(CTRef TRef=CTRef(), double CDD=0, double T=-999)
		{
			m_TRef = TRef;
			m_CDD=CDD;
			m_T=T;
		}

		CTRef m_TRef;
		double m_CDD;
		double m_T;
	};

	//**********************************************************
	class CStringFrostModel : public CBioSIMModelBase
	{
	public:

		

		CStringFrostModel();
		virtual ~CStringFrostModel();

		
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CStringFrostModel; }

	private:


		CDegreeDays m_DD;
		CFrostEventParam m_params;
		

	};

}