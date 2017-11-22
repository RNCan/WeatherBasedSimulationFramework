
#pragma once

#include "basic/zenXml.h"


class CBioSIMDoc;

class CExecutablePropertiesCtrl : public CMFCPropertyGridCtrl
{
public:

	void SetXML(const zen::XmlElement& in);
	void InitPropList();
	void SetPropListFont();


protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	void AddProperties(CMFCPropertyGridProperty** pProperties, const zen::XmlElement& in);

	CFont m_fntPropList;
};



class CPropertiesWnd : public CDockablePane
{
// Construction
public:

	static CBioSIMDoc* GetDocument();


	CPropertiesWnd();
	~CPropertiesWnd();
	
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	


	void AdjustLayout();
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);



protected:

	

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);


	CExecutablePropertiesCtrl m_propertiesCtrl;
	
};

