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

#include <string>
#include "UI/Common/HtmlTree/XHtmlTree.h"
#include "UI/Common/CommonCtrl.h"
#include "Tasks/TaskBase.h"





/////////////////////////////////////////////////////////////////////////////
// CTaskTreeCtrl


class CTaskTreeCtrl : public CXHtmlTree
{

	// Construction
public:


	//enum TClass{ UNKNOWN = -1, GROUP, WEATHER_UPDATE, WEATHER_GENERATION, MODEL_EXECUTION, ANALYSIS, FUNCTION_ANALYSIS, MERGE_EXECUTABLE, MAPPING, IMPORT_FILE, INPUT_ANALYSIS, DISPERSAL, SCRIPT_R, COPY_EXPORT, MODEL_PARAMETERIZATION, NB_CLASS };


	CTaskTreeCtrl();
	virtual ~CTaskTreeCtrl();
		

	void AddRoot(size_t t, UINT iImageIndex, CString name = _T(""));
	HTREEITEM FindItem(size_t p)const;
	size_t GetPosition(HTREEITEM hItem)const;
	size_t InsertTask(WBSF::CTaskPtr pTask, UINT iImage, HTREEITEM hInsertAfter);
	//void MoveItem(HTREEITEM hItem, HTREEITEM hInsertAfter);
	//void MoveItem(HTREEITEM hItem, bool bDown);

	//void SetCheckedItem(const WBSF::StringVector& data);
	//void GetCheckedItem(WBSF::StringVector& data);


	virtual void PreSubclassWindow();
	virtual void Init();
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);


	bool CanPaste(WBSF::CTaskPtr pItem, HTREEITEM hPasteOnItem);

	//WBSF::CTaskPtr GetNewExecutable(UINT classType);
	void OnUpdateToolBar(CCmdUI *pCmdUI);
	WBSF::CTaskPtr GetSelectedTask();

	int ID2ImagesIndex(UINT ID)const;
	

	void Update(){ Init(); }
	//void SetParent(CTaskWnd* pParent){ m_pParent = pParent; }

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEndEditLabel(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClick(NMHDR*, LRESULT*);
	
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnItemMove(UINT ID);


	std::string		m_curSel;
	CImageList		m_taskTypeImage;
	CImageList		m_taskImages;
	WBSF::CTaskPtr	m_pTask;
	CToolBar		m_ID2Index;//use junk toolbar to compute image ID to index 

	static XHTMLTREEDATA GetExtraData();
	static TVINSERTSTRUCT GetInsertStruct(CString& name, int imageIndex, HTREEITEM hParent, HTREEITEM hInsertAfter);

};

//*****************************************************************************************************


//	void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, StringVector& data);

