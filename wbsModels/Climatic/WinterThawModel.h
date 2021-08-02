//*****************************************************************************
// File: DegreeDay.h
//
// Class: CWinterThawModel
//*****************************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/DegreeDays.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	//**********************************************************
	class CWinterThawModel : public CBioSIMModelBase
	{
	public:

		

		CWinterThawModel();
		virtual ~CWinterThawModel();

		
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CWinterThawModel; }
		WBSF::CMonthDay m_begin;
		WBSF::CMonthDay m_end;


	};

}