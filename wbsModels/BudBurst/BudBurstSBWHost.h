﻿//Creation from Fabrizio Carteni MathLab code, see article : https://nph.onlinelibrary.wiley.com/doi/full/10.1111/nph.18974

#pragma once


#include "Basic/ModelStat.h"
#include "Basic/WeatherStation.h"
#include "PHENO_eqs.h"

namespace WBSF
{
//
	enum THBBOutput { O_S_CONC, O_ST_CONC, O_BRANCH_LENGTH, O_BUDS_MASS, O_BRANCH_MASS, O_NEEDLE_MASS, O_S0, O_S1, O_S2, O_S3, O_S4, O_S5, O_SDI, NB_HBB_OUTPUTS };
	enum THBBOutputEx { O_C = NB_HBB_OUTPUTS, O_INHIBITOR, O_SUGAR , O_STARCH, O_PS, O_MOBILIZATION, O_MOBILIZATION_STOCK, O_ACCUMULATION, O_SWELLING, O_TRANSLOCATION, O_GROWTH_BDW_NDW, O_FROST_HARDENING1, O_FROST_HARDENING2, O_FROST_DEHARDENING1, O_FROST_DEHARDENING2, O_C_SINK, O_PROD_I, O_REMOVAL_I, O_SWELL_SWITCH, O_TAIR, O_TSOIL, O_PN, O_RC_G_TAIR, O_RC_F_TAIR, O_RC_M_TAIR, O_RC_G_TSOIL, O_BUDBURST, NB_HBB_OUTPUTS_EX };
	enum TSDI { SDI_DHONT, SDI_AUGER, NB_SDI_TYPE };
	enum TW2L { W2L_LORENA, W2L_REMI, NB_W2L };





	enum { μ, ѕ, Τᴴ¹, Τᴴ², ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb, NB_SDI_PARAMS };

	


	class CSBWHostBudBurst
	{
	public:





		CSBWHostBudBurst();
		~CSBWHostBudBurst();


		size_t m_species;
		TSDI m_SDI_type;
		size_t m_nbSteps;
		HBB::TVersion m_version;

		std::map<int, double> m_defioliation;
		HBB::COldParam m_P_last;
		HBB::CParameters m_P;
		//std::array<double, NB_SDI_PARAMS > m_SDI;
		std::deque<HBB::CInput> m_mean_T_day;
		bool m_bCumul;
		
		ERMsg Execute(CWeatherStation& weather, CModelStatVector& output, bool bModelEx);


		//static double Weibull1(double  SDI, const std::array < double, 2>& p, double first_stage = 0);
		//static double Weibull2(double  SDI, const std::array < double, 2>& p, double last_stage=5);
		static double Weibull(size_t stage, double  SDI, const std::array < double, 2>& p, size_t first_stage = 0, size_t last_stage = 5);
		std::array<double, 6> SDI_2_Sx(double SDI, bool bCumul=false);
			
		
		static double Weight2Length(size_t s, double w, TW2L type);
	};

}

