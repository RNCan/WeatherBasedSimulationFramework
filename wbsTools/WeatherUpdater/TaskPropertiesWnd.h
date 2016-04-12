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
#include "Tasks/TaskBase.h"
#include "WeatherBasedSimulationUI.h"


class CPropertiesToolBar : public CMFCToolBar
{
	DECLARE_SERIAL(CPropertiesToolBar)

	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler){CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }
};



class CWeatherUpdaterDoc;

class CTaskPropertyGridCtrl : public CMFCPropertyGridCtrl
{
	DECLARE_DYNAMIC(CTaskPropertyGridCtrl)
public:

	CTaskPropertyGridCtrl();

	WBSF::CTaskPtr m_pTask;

	void EnableProperties(BOOL bEnable);
	void EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;
	virtual void OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* pOldSel);
	virtual void Init();
	void Update();
	
	size_t GetCurAtt()const{ return m_curAttibute; }
	void SetPropertyColumnWidth(int width) { m_nLeftColumnWidth = width; AdjustLayout(); }

protected:

	std::string m_lastClassName;
	size_t m_curAttibute;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};



class CTaskPropertyWnd : public CDockablePane
{
	DECLARE_DYNCREATE(CTaskPropertyWnd)
public:

	static CWeatherUpdaterDoc* GetDocument();

	CTaskPropertyWnd();
	~CTaskPropertyWnd();

	void AdjustLayout();
	
	

	void SetVSDotNetLook(BOOL bSet)
	{
		m_propertiesCtrl.SetVSDotNetLook(bSet);
		m_propertiesCtrl.SetGroupNameFullWidth(bSet);
	}

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	
	

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnUpdateToolBar(CCmdUI *pCmdUI);
	afx_msg void OnOpenProperty();
	


	CFont m_fntPropList;
	CPropertiesToolBar		m_wndToolBar;
	CTaskPropertyGridCtrl	m_propertiesCtrl;

	std::string m_lasClassName;
};


