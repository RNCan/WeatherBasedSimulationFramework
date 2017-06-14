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
#include "Simulation/Executable.h"
#include "UI/Common/HtmlTree/XHtmlTree.h"




namespace WBSF
{
	
	/////////////////////////////////////////////////////////////////////////////
	// CExecutableTree

	class CExecutableTree : public CXHtmlTree
	{

		// Construction
	public:


		enum TClass{ UNKNOWN = -1, GROUP, WEATHER_UPDATE, WEATHER_GENERATION, MODEL_EXECUTION, ANALYSIS, FUNCTION_ANALYSIS, MERGE_EXECUTABLE, MAPPING, IMPORT_FILE, INPUT_ANALYSIS, DISPERSAL, SCRIPT_R, COPY_EXPORT, MODEL_PARAMETERIZATION, NB_CLASS };
		static int GetClassFromName(const std::string& className);
		static int GetClassType(UINT ID);

		CExecutableTree(bool bCanEdit=true);
		virtual ~CExecutableTree();


		void SetExecutable(CExecutablePtr& pRoot, CProjectStatePtr& pProjectState);


		using CXHtmlTree::InsertItem;
		void InsertItem(CExecutablePtr pRoot,
			HTREEITEM hParent = TVI_ROOT,
			HTREEITEM hInsertAfter = TVI_FIRST,
			bool bAutoExpand = false);

		using CXHtmlTree::SetItem;
		void SetItem(CExecutable* pRoot, HTREEITEM hParent);

		std::string GetInternalName(HTREEITEM hItem)const;
		HTREEITEM FindItem(const std::string& internalName)const;

		int GetClassType(HTREEITEM hTreeItem);

		//void GetExpandedState(XNode& root);
		//void SetExpandedState(const XNode& root);

		void MoveItem(HTREEITEM hItem, bool bDown);

		void SetCheckedItem(const WBSF::StringVector& data);
		void GetCheckedItem(WBSF::StringVector& data);
		void SetExpandedItem(const CExpendedItem& data);
		void GetExpandedItem(CExpendedItem& data);



		virtual void PreSubclassWindow();
		virtual void Init();
		virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
		virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

		const std::string& GetHideItem()const{ return m_hideItem; }
		void SetHideItem(const std::string& hideItem){ m_hideItem = hideItem; }
		void SetRoot(CExecutablePtr pRoot);


		bool CanPaste(CExecutablePtr pItem, HTREEITEM hPasteOnItem);
		bool EditExecutable(UINT classType, CExecutablePtr& pItem, CExecutablePtr& pParent);
		CExecutablePtr GetNewExecutable(UINT classType);
		void OnUpdateToolBar(CCmdUI *pCmdUI);
		CExecutablePtr GetSelectedExecutable();
		

		CProjectStatePtr GetProjectState()const{ return m_pProjectState; }
		void SetProjectState(const WBSF::CProjectStatePtr& in){ m_pProjectState = in; }
		
		void Update();



		template <class T>
		bool DoModalDlg(T& wnd, WBSF::CExecutablePtr& pItem)
		{
			bool bAdd = true;

			wnd.SetExecutable(pItem);
			if (wnd.DoModal() == IDOK)
				pItem = wnd.GetExecutable();
			else
				bAdd = false;

			return bAdd;
		}

		afx_msg LRESULT OnCheckbox(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnItemExpanded(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnBeginDrag(WPARAM, LPARAM);
		afx_msg LRESULT OnEndDrag(WPARAM, LPARAM);
		afx_msg LRESULT OnDropHover(WPARAM, LPARAM);

	protected:

		DECLARE_MESSAGE_MAP()
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnEndEditLabel(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnDblClick(NMHDR*, LRESULT*);
		afx_msg void OnEdit();
		afx_msg void OnEditCopy();
		afx_msg void OnEditPaste();
		afx_msg void OnEditDuplicate();
		afx_msg void OnAdd(UINT ID);
		afx_msg void OnRemove();
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg BOOL OnItemMove(UINT ID);


		std::string		m_curSel;
		std::set<std::string> m_internalName;
		CImageList		m_elementImages;
		CExecutablePtr	m_pRoot;
		//CBioSIMProject*	m_pProject;
		CProjectStatePtr	m_pProjectState;
		std::string		m_hideItem;
		bool			m_bCanEdit;
		//void InitTree();

		static const char* CLASS_NAME[NB_CLASS];

	};


	void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, StringVector& data);

}