//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/GeoBasic.h"
#include "UI/Common/CommonCtrl.h"

namespace WBSF
{

#define GEN_TYPE_CHANGE			WM_USER+1
	class CGeoPointCtrl : public CMFCEditBrowseCtrlEx
	{
	public:

		CGeoPointCtrl(void);
		~CGeoPointCtrl(void);

		double GetCoord()const;
		void SetCoord(double coord);
		void UpdateText();



	protected:


		void UpdateCtrl();
		virtual void OnBrowse();

		double m_coord;
		CString m_strLabel;

		afx_msg void OnEnSetfocus();
		DECLARE_MESSAGE_MAP()

		static int GetType();
		static void SetType(int type);
		static const TCHAR* GetButtonLable();


	};

	//override DDX_Control to provide initialisation
	void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, CGeoPointCtrl& rControl);
	void AFXAPI DDX_Coord(CDataExchange* pDX, int nIDC, double& coord);

	class CGeoRectCtrl : public CAutoEnableStatic
	{
	public:

		CGeoRectCtrl(bool bShowCheckbox = true);

		virtual void PreSubclassWindow();

		WBSF::CGeoRect GetGeoRect()const;
		void SetGeoRect(const WBSF::CGeoRect& rect);

	protected:
		CGeoPointCtrl m_ctrl[4];


		DECLARE_MESSAGE_MAP()
		afx_msg void OnSetFocus(CWnd* pOldWnd);
		afx_msg void GeoCoordTypeChange(UINT id, NMHDR * pNotifyStruct, LRESULT * result);
		//afx_msg void OnEnable(BOOL bEnable);
	};


	void AFXAPI DDX_GeoRect(CDataExchange* pDX, int nIDC, WBSF::CGeoRect& rect);
	void AFXAPI DDV_GeoRect(CDataExchange* pDX, WBSF::CGeoRect& rect);

}