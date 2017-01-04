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

#include "FileManager/FileManager.h"
#include "UI/Common/PropertySheetSize.h"
#include "UI/FileManagerPages.h"


namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CDBManagerDlg

	//typedef CResizablePropertySheet CDBManagerPropertySheet;
	//typedef CMFCPropertySheet CDBManagerPropertySheet;
	class CDBManagerDlg : public CResizablePropertySheet
	{
		
		DECLARE_DYNAMIC(CDBManagerDlg)
		// Construction
	public:

		//static int CALLBACK XmnPropSheetCallback(HWND hWnd, UINT message, LPARAM lParam);
		enum TPages{ NORMAL, DAILY, HOURLY, GRIB, MAP_INPUT, MODEL, WEATHER_UPDATE, SCRIPT, NB_PAGES };

		CDBManagerDlg(CWnd* pWndParent = NULL, int iSelection = 0);
		virtual ~CDBManagerDlg();
		virtual void PostNcDestroy();
		virtual BOOL OnInitDialog();
		//virtual INT_PTR DoModal();
		//virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
		//virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

		// Attributes
	public:
		CNormalMPage m_NormalsPage;
		CDailyMPage m_dailyPage;
		CHourlyMPage m_hourlyPage;
		CGribMPage m_gribPage;
		CMapInputMPage m_mapPage;
		CModelMPage m_modelPage;
		CWeatherUpdateMPage m_weatherUpdatePage;
		CScriptMPage m_scriptPage;



		// Generated message map functions
	protected:

		DECLARE_MESSAGE_MAP()
		//afx_msg void OnSize(UINT nType, int cx, int cy);
		//afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
		//afx_msg LRESULT OnResizePage(WPARAM wParam, LPARAM lParam);

		//BOOL   m_bNeedInit;
		//CRect  m_rCrt;
		////int    m_nMinCX;
		//int    m_nMinCY;

		HICON	m_hIcon;
	};

}