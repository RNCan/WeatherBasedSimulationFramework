﻿//Creation from Fabrizio Carteni MathLab code, see article : https://nph.onlinelibrary.wiley.com/doi/full/10.1111/nph.18974

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include "Basic/ModelStat.h"
#include "BudBurstSBWHost.h"

namespace WBSF
{



	class CSBWHostBudBurstModel : public CBioSIMModelBase
	{
	public:


		CSBWHostBudBurstModel();
		virtual ~CSBWHostBudBurstModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject() { return new CSBWHostBudBurstModel; }
	
	protected:


		void CalibrateSDI(CStatisticXY& stat);


		size_t m_species;
		double m_defoliation;
		size_t m_SDI_type;
		size_t m_nbSteps;
		HBB::TVersion m_version;

		CSBWHostBudBurst m_model;
		HBB::CParameters m_P;
		bool m_bUseDefoliation;
		std::map<int, double> m_defioliation_by_year;
		CStatistic m_SDI_DOY_stat;
		std::array<CStatistic, 4> m_stat;
		bool m_bCumul;

		
		std::set<int> m_years;
		CWeatherStation m_data_weather;
	};

}