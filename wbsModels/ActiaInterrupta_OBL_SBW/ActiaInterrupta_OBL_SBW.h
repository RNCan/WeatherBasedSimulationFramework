//*****************************************************************************
// Class: CActiaInterrupta, CActiaInterruptaHost, CActiaInterruptaStand
//
// Description:	CActiaInterrupta represent a spruce budworm insect or a group of insect with same carractéristics. 
//				CActiaInterruptaHost represent the tree that contain CActiaInterrupta. 
//				CActiaInterruptaStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
//#include "ActiaInterruptaEquations.h"
#include "ActiaInterrupta.h"
#include "ObliqueBandedLeafroller.h"
#include "SpruceBudworm.h"


namespace WBSF
{
	
	namespace ActiaInterrupta
	{
		enum TActiaInterrupta_OBL_SBW_Stats
		{
			S_NB_OBL = TActiaInterruptaStats::NB_STATS, S_NB_OBL_L3D, S_NB_SBW,
			NB_STATS_EX
		};

	}


	class CActiaInterrupta_OBL_SBW_Host;
	class CActiaInterrupta_OBL_SBW_Stand;
	class CActiaInterrupta_OBL_SBW : public CActiaInterrupta
	{
	public:

		CActiaInterrupta_OBL_SBW(CHost* pHost, CTRef creationDate = CTRef(), double age = ActiaInterrupta::MAGGOT, WBSF::TSex sex = WBSF::RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1, CIndividualPtr& pAssociateHost = CIndividualPtr());
		CActiaInterrupta_OBL_SBW(const CActiaInterrupta_OBL_SBW& in) :CActiaInterrupta(in){ operator=(in); }
		CActiaInterrupta_OBL_SBW& operator=(const CActiaInterrupta_OBL_SBW& in);

		virtual void OnNewDay(const CWeatherDay& weather);
		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		//virtual bool CanPack(const CIndividualPtr& in)const;
		//virtual void Pack(const CIndividualPtr& in);
		virtual size_t GetNbStages()const{ return ActiaInterrupta::NB_STAGES; }
		virtual CIndividualPtr CreateCopy()const{ return std::make_shared<CActiaInterrupta_OBL_SBW>(*this); }
		virtual std::string get_property(const std::string& name)const override;




		inline CActiaInterrupta_OBL_SBW_Host* GetHost();
		inline const CActiaInterrupta_OBL_SBW_Host* GetHost()const;
		inline CActiaInterrupta_OBL_SBW_Stand* GetStand();
		inline const CActiaInterrupta_OBL_SBW_Stand* GetStand()const;

		inline CActiaInterruptaEquations & Equations();
		std::weak_ptr<CIndividual> m_pAssociateHost;


		std::array< double, ActiaInterrupta::NB_HOSTS> m_broods_by_host;
	};

	//class CObliqueBandedLeafrollerEx : public CObliqueBandedLeafroller
	//{
	//	using CObliqueBandedLeafroller::CObliqueBandedLeafroller;
	//	//inline CActiaInterrupta_OBL_SBW_Stand* GetStand();
	//	//inline const CActiaInterrupta_OBL_SBW_Stand* GetStand()const;
	//	//inline CObliqueBandedLeafrollerEquations& Equations();

	//	virtual std::string get_property(const std::string& name)override;
	//};

	//class CSpruceBudwormEx : public CSpruceBudworm
	//{
	//	using CSpruceBudworm::CSpruceBudworm;
	//	//inline CActiaInterrupta_OBL_SBW_Stand* GetStand();
	//	//inline const CActiaInterrupta_OBL_SBW_Stand* GetStand()const;
	//	//inline CSpruceBudwormEquations& Equations();

	//	virtual std::string get_property(const std::string& name)override;

	//};



	class CModelStat;
	class CWeatherDay;

	class CActiaInterrupta_OBL_SBW_Host : public CActiaInterruptaHost
	{
	public:

		CActiaInterrupta_OBL_SBW_Host(WBSF::CStand* pStand):
			CActiaInterruptaHost(pStand)
		{}
		
		//virtual std::string get_property(const std::string& name)override;
		void Initialize(const CInitialPopulation& initValue);

		inline CActiaInterrupta_OBL_SBW_Stand* GetStand();
		inline const CActiaInterrupta_OBL_SBW_Stand* GetStand()const;

		//virtual void Live(const CWeatherDay& weaDay);
		//virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);

		
	};

	//*******************************************************************************************************
	// CActiaInterruptaStand
	class CActiaInterrupta_OBL_SBW_Stand : public CActiaInterruptaStand
	{
	public:

		//CActiaInterruptaStand m_ActiaInterruptaStand;
		CObliqueBandedLeafrollerStand m_OBLStand;
		CSBWStand m_SBWStand;

		//global variables of all bugs

		CActiaInterrupta_OBL_SBW_Stand(CBioSIMModelBase* pModel) :
			CActiaInterruptaStand(pModel),
			//m_ActiaInterruptaStand(pModel),
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
	inline CActiaInterrupta_OBL_SBW_Host* CActiaInterrupta_OBL_SBW::GetHost(){ return static_cast<CActiaInterrupta_OBL_SBW_Host*>(m_pHost); }
	inline const CActiaInterrupta_OBL_SBW_Host* CActiaInterrupta_OBL_SBW::GetHost()const{ return static_cast<const CActiaInterrupta_OBL_SBW_Host*>(m_pHost); }
	inline CActiaInterrupta_OBL_SBW_Stand* CActiaInterrupta_OBL_SBW::GetStand(){ ASSERT(m_pHost); return static_cast<CActiaInterrupta_OBL_SBW_Stand*>(GetHost()->GetStand()); }
	inline const CActiaInterrupta_OBL_SBW_Stand* CActiaInterrupta_OBL_SBW::GetStand()const{ ASSERT(m_pHost); return static_cast<const CActiaInterrupta_OBL_SBW_Stand*>(GetHost()->GetStand()); }
	inline CActiaInterrupta_OBL_SBW_Stand* CActiaInterrupta_OBL_SBW_Host::GetStand(){ ASSERT(m_pStand); return static_cast<CActiaInterrupta_OBL_SBW_Stand*>(m_pStand); }
	inline const CActiaInterrupta_OBL_SBW_Stand* CActiaInterrupta_OBL_SBW_Host::GetStand()const{ ASSERT(m_pStand); return static_cast<const CActiaInterrupta_OBL_SBW_Stand*>(m_pStand); }

	inline CActiaInterruptaEquations& CActiaInterrupta_OBL_SBW::Equations(){ return GetStand()->m_equations; }
}