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


#include "UI/Common/CommonCtrl.h"
#include "WeatherBasedSimulationUI.h"
#include "afxpropertygridctrl.h"
#include "ModelBase/WGInput.h"

namespace WBSF
{


	class CSearchRadiusPropertyGridCtrl : public CMFCPropertyGridCtrl
	{
	public:

		CSearchRadiusPropertyGridCtrl(CSearchRadius& searchRadius);

		void EnableProperties(BOOL bEnable);
		void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);

		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
		virtual BOOL ValidateItemData(CMFCPropertyGridProperty* /*pProp*/);
		virtual void Init();

		//const CSearchRadius& Get()const{ return m_parameters; }
		//void Set(const CSearchRadius& in);

		


	protected:

		CSearchRadius& m_searchRadius;
		DECLARE_MESSAGE_MAP()
	};



	class CSearchRadiusDlg : public CDialog
	{
	public:

		CSearchRadiusDlg(CWnd* pParentDlg = NULL);
		~CSearchRadiusDlg();
		
		CSearchRadius m_searchRadius;

	protected:

		enum { IDD = IDD_SEARCH_RADIUS };

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		DECLARE_MESSAGE_MAP()
		void OnSize(UINT nType, int cx, int cy);
		void OnDestroy();

		void AdjustLayout();


		CSearchRadiusPropertyGridCtrl m_ctrl;
		
	};

}