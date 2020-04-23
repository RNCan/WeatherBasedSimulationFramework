//*****************************************************************************
// Class: CMeteorusTrachynotus, CMeteorusTrachynotusHost, CMeteorusTrachynotusStand
//
// Description:	CMeteorusTrachynotus represent a spruce budworm insect or a group of insect with same carractéristics. 
//				CMeteorusTrachynotusHost represent the tree that contain CMeteorusTrachynotus. 
//				CMeteorusTrachynotusStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
//#include "MeteorusTrachynotusEquations.h"
#include "MeteorusTrachynotus.h"
#include "ObliqueBandedLeafroller.h"
#include "SpruceBudworm.h"


namespace WBSF
{
	
	namespace MeteorusTrachynotus
	{
		enum TMeteorusTrachynotus_OBL_SBW_Stats
		{
			S_NB_OBL = TMeteorusTrachynotusStats::NB_STATS, S_NB_OBL_L3D, S_NB_SBW,
			NB_STATS_EX
		};

	}


	class CMeteorusTrachynotus_OBL_SBW_Host;
	class CMeteorusTrachynotus_OBL_SBW_Stand;
	class CMeteorusTrachynotus_OBL_SBW : public CMeteorusTrachynotus
	{
	public:

		CMeteorusTrachynotus_OBL_SBW(CHost* pHost, CTRef creationDate = CTRef(), double age = MeteorusTrachynotus::EGG, WBSF::TSex sex = WBSF::RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1, CIndividualPtr& pAssociateHost = CIndividualPtr());
		CMeteorusTrachynotus_OBL_SBW(const CMeteorusTrachynotus_OBL_SBW& in) :CMeteorusTrachynotus(in){ operator=(in); }
		CMeteorusTrachynotus_OBL_SBW& operator=(const CMeteorusTrachynotus_OBL_SBW& in);

		virtual void OnNewDay(const CWeatherDay& weather);
		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		//virtual bool CanPack(const CIndividualPtr& in)const;
		//virtual void Pack(const CIndividualPtr& in);
		virtual size_t GetNbStages()const{ return MeteorusTrachynotus::NB_STAGES; }
		virtual CIndividualPtr CreateCopy()const{ return std::make_shared<CMeteorusTrachynotus_OBL_SBW>(*this); }
		virtual std::string get_property(const std::string& name)const override;




		inline CMeteorusTrachynotus_OBL_SBW_Host* GetHost();
		inline const CMeteorusTrachynotus_OBL_SBW_Host* GetHost()const;
		inline CMeteorusTrachynotus_OBL_SBW_Stand* GetStand();
		inline const CMeteorusTrachynotus_OBL_SBW_Stand* GetStand()const;

		inline CMeteorusTrachynotusEquations & Equations();
		std::weak_ptr<CIndividual> m_pAssociateHost;


		std::array< double, MeteorusTrachynotus::NB_HOSTS> m_nb_attacks_by_host;
		std::array< double, MeteorusTrachynotus::NB_HOSTS> m_broods_by_host;
	};

	//class CObliqueBandedLeafrollerEx : public CObliqueBandedLeafroller
	//{
	//	using CObliqueBandedLeafroller::CObliqueBandedLeafroller;
	//	//inline CMeteorusTrachynotus_OBL_SBW_Stand* GetStand();
	//	//inline const CMeteorusTrachynotus_OBL_SBW_Stand* GetStand()const;
	//	//inline CObliqueBandedLeafrollerEquations& Equations();

	//	virtual std::string get_property(const std::string& name)override;
	//};

	//class CSpruceBudwormEx : public CSpruceBudworm
	//{
	//	using CSpruceBudworm::CSpruceBudworm;
	//	//inline CMeteorusTrachynotus_OBL_SBW_Stand* GetStand();
	//	//inline const CMeteorusTrachynotus_OBL_SBW_Stand* GetStand()const;
	//	//inline CSpruceBudwormEquations& Equations();

	//	virtual std::string get_property(const std::string& name)override;

	//};



	class CModelStat;
	class CWeatherDay;

	class CMeteorusTrachynotus_OBL_SBW_Host : public CMeteorusTrachynotusHost
	{
	public:

		CMeteorusTrachynotus_OBL_SBW_Host(WBSF::CStand* pStand):
			CMeteorusTrachynotusHost(pStand)
		{}
		
		//virtual std::string get_property(const std::string& name)override;
		void Initialize(const CInitialPopulation& initValue);

		inline CMeteorusTrachynotus_OBL_SBW_Stand* GetStand();
		inline const CMeteorusTrachynotus_OBL_SBW_Stand* GetStand()const;

		//virtual void Live(const CWeatherDay& weaDay);
		//virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);

		
	};

	//*******************************************************************************************************
	// CMeteorusTrachynotusStand
	class CMeteorusTrachynotus_OBL_SBW_Stand : public CMeteorusTrachynotusStand
	{
	public:

		//CMeteorusTrachynotusStand m_MeteorusTrachynotusStand;
		CObliqueBandedLeafrollerStand m_OBLStand;
		CSBWStand m_SBWStand;

		//global variables of all bugs

		CMeteorusTrachynotus_OBL_SBW_Stand(CBioSIMModelBase* pModel) :
			CMeteorusTrachynotusStand(pModel),
			//m_MeteorusTrachynotusStand(pModel),
			m_OBLStand(pModel),
			m_SBWStand(pModel)
		{
		}


		virtual void Live(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);
		virtual bool AdjustPopulation();
		virtual size_t GetNbObjectAlive()const;
		virtual void HappyNewYear();
		virtual double GetAI(bool bIncludeLast)const;

		virtual CHostPtr GetNearestHost(CHost* pHost);
		
		CIndividualPtr SelectRandomHost(bool bUseSBW);
	};

	//WARNING: cast must be defined here to avoid bug
	inline CMeteorusTrachynotus_OBL_SBW_Host* CMeteorusTrachynotus_OBL_SBW::GetHost(){ return static_cast<CMeteorusTrachynotus_OBL_SBW_Host*>(m_pHost); }
	inline const CMeteorusTrachynotus_OBL_SBW_Host* CMeteorusTrachynotus_OBL_SBW::GetHost()const{ return static_cast<const CMeteorusTrachynotus_OBL_SBW_Host*>(m_pHost); }
	inline CMeteorusTrachynotus_OBL_SBW_Stand* CMeteorusTrachynotus_OBL_SBW::GetStand(){ ASSERT(m_pHost); return static_cast<CMeteorusTrachynotus_OBL_SBW_Stand*>(GetHost()->GetStand()); }
	inline const CMeteorusTrachynotus_OBL_SBW_Stand* CMeteorusTrachynotus_OBL_SBW::GetStand()const{ ASSERT(m_pHost); return static_cast<const CMeteorusTrachynotus_OBL_SBW_Stand*>(GetHost()->GetStand()); }
	inline CMeteorusTrachynotus_OBL_SBW_Stand* CMeteorusTrachynotus_OBL_SBW_Host::GetStand(){ ASSERT(m_pStand); return static_cast<CMeteorusTrachynotus_OBL_SBW_Stand*>(m_pStand); }
	inline const CMeteorusTrachynotus_OBL_SBW_Stand* CMeteorusTrachynotus_OBL_SBW_Host::GetStand()const{ ASSERT(m_pStand); return static_cast<const CMeteorusTrachynotus_OBL_SBW_Stand*>(m_pStand); }

	inline CMeteorusTrachynotusEquations& CMeteorusTrachynotus_OBL_SBW::Equations(){ return GetStand()->m_equations; }
}