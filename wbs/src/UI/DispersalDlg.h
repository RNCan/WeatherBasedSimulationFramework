//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once


#include "Simulation/Dispersal.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{


	class CDispersalPropertyGridCtrl : public CMFCPropertyGridCtrl
	{
	public:




		CDispersalPropertyGridCtrl();


		void EnableProperties(BOOL bEnable);
		void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);

		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual BOOL ValidateItemData(CMFCPropertyGridProperty* /*pProp*/);
		virtual void Init();

		const CDispersalParamters& Get()const{ return m_parameters; }
		void Set(const CDispersalParamters& in);

	protected:

		CDispersalParamters m_parameters;
		DECLARE_MESSAGE_MAP()
	};



	class CDispersalDlg : public CDialog
	{
	public:

		CDispersalDlg(CExecutablePtr pParent, CWnd* pParentDlg = NULL);
		~CDispersalDlg();


		virtual void SetExecutable(CExecutablePtr pExecutable){ m_dispersal = GetDispersal(pExecutable); }
		virtual CExecutablePtr GetExecutable()const{ return CExecutablePtr(new CDispersal(m_dispersal)); }
		void SetData();

	protected:

		enum { IDD = IDD_SIM_DISPERSAL };

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		DECLARE_MESSAGE_MAP()
		void OnSize(UINT nType, int cx, int cy);
		void OnDestroy();


		void AdjustLayout();



		CDispersal m_dispersal;
		CDispersal& GetDispersal(const CExecutablePtr& pItem){ ASSERT(pItem); return dynamic_cast<CDispersal&>(*pItem); }


		CCFLEdit m_nameCtrl;
		CCFLEdit m_descriptionCtrl;
		CDispersalPropertyGridCtrl m_propertiesCtrl;
		CCFLEdit m_internalNameCtrl;
	};

}