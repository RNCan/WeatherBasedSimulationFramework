//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"

#include "UI/Common/SYShowMessage.h"
#include "UI/WeatherUpdateDlg.h"
#include "UI/WeatherGenerationDlg.h"
#include "UI/ModelExecutionDlg.h"
#include "UI/AnalysisDlg.h"
#include "UI/FunctionAnalysisDlg.h"
#include "UI/MergeExecutableDlg.h"
#include "UI/MappingDlg.h"
#include "UI/ImportDataDlg.h"
#include "UI/WGInputAnalysisDlg.h"
#include "UI/DispersalDlg.h"
#include "UI/ScriptDlg.h"
#include "UI/CopyExportDlg.h"
#include "UI/ModelParameterizationDlg.h"
#include "UI/ExecutableTree.h"
#include "Simulation/ExecutableGroup.h"
#include "Simulation/BioSIMProject.h"
#include "WeatherBasedSimulationString.h"
#include "WeatherBasedSimulationUI.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;


namespace WBSF
{



	/////////////////////////////////////////////////////////////////////////////
	// CExecutableTree


	const char* CExecutableTree::CLASS_NAME[NB_CLASS] =
	{
		CExecutableGroup::GetXMLFlag(),
		CWeatherUpdate::GetXMLFlag(),
		CWeatherGeneration::GetXMLFlag(),
		CModelExecution::GetXMLFlag(),
		CAnalysis::GetXMLFlag(),
		CFunctionAnalysis::GetXMLFlag(),
		CMergeExecutable::GetXMLFlag(),
		CMapping::GetXMLFlag(),
		CImportData::GetXMLFlag(),
		CWGInputAnalysis::GetXMLFlag(),
		CDispersal::GetXMLFlag(),
		CScript::GetXMLFlag(),
		CCopyExport::GetXMLFlag(),
		CModelParameterization::GetXMLFlag()
	};


	BEGIN_MESSAGE_MAP(CExecutableTree, CXHtmlTree)
		ON_WM_CREATE()
		ON_UPDATE_COMMAND_UI(ID_EDIT, OnUpdateToolBar)
		ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateToolBar)
		ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateToolBar)
		ON_UPDATE_COMMAND_UI(ID_EDIT_DUPLICATE, OnUpdateToolBar)
		ON_UPDATE_COMMAND_UI(ID_REMOVE, OnUpdateToolBar)
		ON_UPDATE_COMMAND_UI(ID_ITEM_EXPAND_ALL, OnUpdateToolBar)
		ON_UPDATE_COMMAND_UI(ID_ITEM_COLLAPSE_ALL, OnUpdateToolBar)
		ON_UPDATE_COMMAND_UI_RANGE(ID_ADD_FIRST, ID_ADD_LAST, OnUpdateToolBar)

		ON_COMMAND_RANGE(ID_ADD_FIRST, ID_ADD_LAST, OnAdd)
		ON_COMMAND(ID_EDIT, &OnEdit)
		ON_COMMAND(ID_EDIT_COPY, &OnEditCopy)
		ON_COMMAND(ID_EDIT_PASTE, &OnEditPaste)
		ON_COMMAND(ID_REMOVE, &OnRemove)
		ON_COMMAND(ID_EDIT_DUPLICATE, &OnEditDuplicate)
		ON_COMMAND_EX(ID_ITEM_EXPAND_ALL, OnItemMove)
		ON_COMMAND_EX(ID_ITEM_COLLAPSE_ALL, OnItemMove)


		ON_NOTIFY_REFLECT(NM_DBLCLK, &OnDblClick)
		ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, &OnEndEditLabel)

		//ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
		//ON_REGISTERED_MESSAGE(WM_XHTMLTREE_ITEM_EXPANDED, OnItemExpanded)
		//ON_REGISTERED_MESSAGE(WM_XHTMLTREE_BEGIN_DRAG, OnBeginDrag)
		//ON_REGISTERED_MESSAGE(WM_XHTMLTREE_END_DRAG, OnEndDrag)
		//ON_REGISTERED_MESSAGE(WM_XHTMLTREE_DROP_HOVER, OnDropHover)

	END_MESSAGE_MAP()


	CExecutableTree::CExecutableTree(bool bCanEdit)
	{
		m_bCanEdit = bCanEdit;
	}

	CExecutableTree::~CExecutableTree()
	{}


	void CExecutableTree::PreSubclassWindow()
	{
		CXHtmlTree::PreSubclassWindow();

		_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
		if (pThreadState->m_pWndInit == NULL)
		{
			Init();
		}
	}

	int CExecutableTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		if (CXHtmlTree::OnCreate(lpCreateStruct) == -1)
			return -1;

		Init();
		return 0;
	}

	void CExecutableTree::SetRoot(CExecutablePtr pRoot)
	{
		m_pRoot = pRoot;
	}

	void CExecutableTree::Init()
	{
		ASSERT(GetSafeHwnd());

		CBitmap bmp;
		VERIFY(bmp.LoadBitmap(IDB_SIM_EXECUTABLE_IMAGE));

		m_elementImages.Create(16, 16, ILC_COLOR32, 0, 0);
		m_elementImages.Add(&bmp, RGB(0, 0, 0));
		SetImageList(&m_elementImages, TVSIL_NORMAL);


		Initialize(TRUE, TRUE);
		SetSmartCheckBox(FALSE);
		SetSelectFollowsCheck(FALSE);
		SetAutoCheckChildren(TRUE);
		SetHtml(FALSE);
		SetImages(TRUE);
	}



	int CExecutableTree::GetClassFromName(const string& className)
	{
		int index = -1;

		for (int i = 0; i < NB_CLASS; i++)
		{
			if (WBSF::IsEqualNoCase(className, CLASS_NAME[i]))
			{
				index = i;
				break;
			}
		}

		return index;
	}

	static XHTMLTREEDATA GetExtraData()
	{
		XHTMLTREEDATA xhtd;

		xhtd.bChecked = TRUE;
		xhtd.bEnabled = FALSE;

		return xhtd;
	}

	static TVINSERTSTRUCT GetInsertStruct(CString& name, int imageIndex, HTREEITEM hParent, HTREEITEM hInsertAfter)
	{
		TVINSERTSTRUCT tvis = { 0 };
		tvis.item.mask = TVIF_TEXT;
		tvis.item.pszText = name.LockBuffer();
		tvis.item.cchTextMax = name.GetLength();
		tvis.hParent = hParent;
		tvis.hInsertAfter = hInsertAfter;
		tvis.item.iImage = imageIndex;
		tvis.item.iSelectedImage = imageIndex;
		tvis.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		//tvis.item.mask          |= TVIF_PARAM;
		//tvis.item.lParam         = 0;
		return tvis;
	}

	//UINT_PTR GetItempData(string in)
	//{
	//	ASSERT(sizeof(UINT_PTR) == 8);//problem under win32
	//	UINT_PTR out=0;
	//	for (size_t i = 0; i < in.length(); i++)
	//	{
	//		size_t c = in[i] << i*8;
	//		out += c;
	//	}
	//		
	//	return out;
	//}

	//string GetInternalName(UINT_PTR in)
	//{
	//	ASSERT(sizeof(UINT_PTR) == 8);//problem under win32
	//	string out;

	//	for (size_t i = 0; i < in.length(); i++)
	//	{
	//		size_t c = in[i] << i * 8;
	//		out += c;
	//	}

	//	return out;
	//}

	void CExecutableTree::InsertItem(CExecutablePtr pItem, HTREEITEM hParent, HTREEITEM hInsertAfter, bool bAutoExpand)
	{
		ASSERT(pItem);
		if (pItem == NULL)
			AfxThrowInvalidArgException();

		string iName = pItem->GetInternalName();
		if (iName == m_hideItem)
			return;

		int imageIndex = GetClassFromName(pItem->GetClassName());
		if (imageIndex != -1)
		{
			CString name(pItem->GetName().c_str());
			bool bExecute = pItem->GetExecute();
			bool bExpand = m_pProjectState?m_pProjectState->m_expendedItems.find(iName) != m_pProjectState->m_expendedItems.end():false;

			//if (hParent == TVI_ROOT)
				//bExpand = true;//if it's root

			TVINSERTSTRUCT tvis = GetInsertStruct(name, imageIndex, hParent, hInsertAfter);
			XHTMLTREEDATA xhtd;
			xhtd.bExpanded = bExpand;
			xhtd.bChecked = bExecute;

			//Add this element to the tree
			HTREEITEM hItem = InsertItem(&tvis, &xhtd);

			//add internal name
			m_internalName.insert(iName);
			SetItemData(hItem, (UINT_PTR)(m_internalName.find(iName)->c_str()));

			hParent = hItem;
		}

		for (int i = 0; i < pItem->GetNbItem(); i++)
		{
			InsertItem(pItem->GetItemAt(i), hParent, NULL);
		}
	}

	void CExecutableTree::MoveItem(HTREEITEM hItem, bool bDown)
	{
		HTREEITEM hParent = GetParentItem(hItem);
		HTREEITEM hNext = GetNextItem(hItem);
		HTREEITEM hPrevious = GetPrevItem(hItem);

	}

	void CExecutableTree::SetItem(CExecutable* pItem, HTREEITEM hItem)
	{
		ASSERT(pItem);

		SetItemText(hItem, CString(pItem->GetName().c_str()));
		SetCheck(hItem, pItem->GetExecute());
	}



	int CExecutableTree::GetClassType(HTREEITEM hTreeItem)
	{
		int image1 = UNKNOWN;
		int image2 = UNKNOWN;
		if (hTreeItem)
		{
			VERIFY(GetItemImage(hTreeItem, image1, image2));
		}

		return image1;
	}

	string CExecutableTree::GetInternalName(HTREEITEM hItem)const
	{
		string iName;
		if (hItem != NULL)
		{
			const char* pIName = (const char*)GetItemData(hItem);
			iName = pIName;
			ASSERT(!iName.empty());
		}

		return iName;
	}

	HTREEITEM CExecutableTree::FindItem(const string& internalName)const
	{
		CExecutableTree& me = const_cast<CExecutableTree&>(*this);

		HTREEITEM hItem = GetRootItem();
		while (hItem)
		{
			if (((const char*)GetItemData(hItem)) == internalName)
				break;//we find it

			hItem = me.GetNextItem(hItem);
		}

		return hItem;
	}

	void CExecutableTree::SetCheckedItem(const WBSF::StringVector& data)
	{
		ASSERT(GetSafeHwnd());

		//set checked item and also uncheck all other items
		HTREEITEM hItem = GetRootItem();
		while (hItem)
		{
			string iName = GetInternalName(hItem);
			BOOL bCheck = data.Find(iName)!= NOT_INIT;
			SetCheck(hItem, bCheck);
			hItem = GetNextItem(hItem);
		}
	}

	void CExecutableTree::GetCheckedItem(WBSF::StringVector& data)
	{
		ASSERT(GetSafeHwnd());

		data.clear();

		HTREEITEM hItem = GetFirstCheckedItem();
		while (hItem)
		{
			data.push_back(GetInternalName(hItem));
			hItem = GetNextCheckedItem(hItem);
		}
	}


	void CExecutableTree::GetExpandedItem(CExpendedItem& data)
	{
		//For all element in the tree
		HTREEITEM hItem = GetRootItem();
		while (hItem)
		{
			if (GetChildrenCount(hItem) > 0)
			{
				if (IsExpanded(hItem))
				{
					string iName = GetInternalName(hItem);
					data.insert(GetInternalName(hItem));
				}
			}

			hItem = GetNextItem(hItem);
		}

	}

	void CExecutableTree::SetExpandedItem(const CExpendedItem& data)
	{
		for (CExpendedItem::const_iterator it = data.begin(); it != data.end(); it++)
		{
			HTREEITEM hItem = FindItem(*it);
			if (hItem)
			{
				HTREEITEM hParent = GetParentItem(hItem);
				if (hParent)
					Expand(hParent, TVE_EXPAND);
			}
		}
	}


	bool CExecutableTree::CanPaste(CExecutablePtr pItem, HTREEITEM hPasteOnItem)
	{
		bool bRep = false;

		string className = pItem->GetClassName();
		int type = CExecutableTree::GetClassFromName(className);

		short elemType = CExecutableTree::GetClassType(hPasteOnItem);
		short deepElemType = elemType;
		HTREEITEM hItem = hPasteOnItem;
		while (deepElemType == CExecutableTree::GROUP)
		{
			hItem = CExecutableTree::GetParentItem(hItem);
			if (hItem == NULL)
				break;

			deepElemType = CExecutableTree::GetClassType(hItem);
		}

		switch (type)
		{
		case CExecutableTree::GROUP:
		{
			//a group is valid if all element are valid
			bRep = true;
			for (int i = 0; i < pItem->GetNbItem() && bRep; i++)
				bRep = CanPaste(pItem->GetItemAt(i), hPasteOnItem);

			break;
		}
		case CExecutableTree::WEATHER_GENERATION:
		case CExecutableTree::IMPORT_FILE:
		case CExecutableTree::WEATHER_UPDATE: bRep = (deepElemType == CExecutableTree::GROUP); break;
		case CExecutableTree::MODEL_EXECUTION:
		case CExecutableTree::ANALYSIS:
		case CExecutableTree::FUNCTION_ANALYSIS:
		case CExecutableTree::MAPPING:
		case CExecutableTree::DISPERSAL:
		case CExecutableTree::SCRIPT_R:
		case CExecutableTree::COPY_EXPORT:	
		case CExecutableTree::MODEL_PARAMETERIZATION:bRep = (deepElemType != CExecutableTree::UNKNOWN) && (deepElemType != CExecutableTree::GROUP); break;
		case CExecutableTree::INPUT_ANALYSIS:	  bRep = deepElemType == CExecutableTree::WEATHER_GENERATION; break;
		case CExecutableTree::MERGE_EXECUTABLE:  bRep = (elemType == CExecutableTree::GROUP); break;

		default: ASSERT(false);
		}

		return bRep;
	}


	bool CExecutableTree::EditExecutable(UINT classType, CExecutablePtr& pItem, CExecutablePtr& pParent)
	{
		ASSERT(pItem);

		bool bAdd = true;
		switch (classType)
		{

		case CExecutableTree::GROUP:	break;

		case CExecutableTree::WEATHER_UPDATE:
		{
			CWeatherUpdateDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::WEATHER_GENERATION:
		{
			CWeatherGenerationDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::MODEL_EXECUTION:
		{
			CModelExecutionDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::ANALYSIS:
		{
			ASSERT(pParent);
			CAnalysisDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::FUNCTION_ANALYSIS:
		{
			ASSERT(pParent);
			CFunctionAnalysisDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::MERGE_EXECUTABLE:
		{
			ASSERT(pParent);
			CMergeExecutableDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::MAPPING:
		{
			ASSERT(pParent);
			CMappingDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::IMPORT_FILE:
		{
			CImportDataDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::INPUT_ANALYSIS:
		{
			ASSERT(pParent);
			CWGInputAnalysisDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::DISPERSAL:
		{
			ASSERT(pParent);
			CDispersalDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::SCRIPT_R:
		{
			ASSERT(pParent);
			CScriptDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		case CExecutableTree::COPY_EXPORT:
		{
			ASSERT(pParent);
			CCopyExportDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		//case CExecutableTree::SIMULATION:
		//{
		//	CSimulationDlg dlg(pParent, this);
		//	bAdd = DoModalDlg(dlg, pItem);
		//}break;

		case CExecutableTree::MODEL_PARAMETERIZATION:
		{
			CModelParameterizationDlg dlg(pParent, this);
			bAdd = DoModalDlg(dlg, pItem);
		}break;

		default: ASSERT(false);
		}

		return bAdd;
	}

	CExecutablePtr CExecutableTree::GetNewExecutable(UINT classType)
	{
		CExecutablePtr pItem;
		switch (classType)
		{
		case CExecutableTree::GROUP:	pItem.reset(new CExecutableGroup); break;
		case CExecutableTree::WEATHER_UPDATE:	pItem.reset(new CWeatherUpdate); break;
		case CExecutableTree::WEATHER_GENERATION:	pItem.reset(new CWeatherGeneration); break;
		case CExecutableTree::MODEL_EXECUTION:	pItem.reset(new CModelExecution); break;
		case CExecutableTree::ANALYSIS: pItem.reset(new CAnalysis); break;
		case CExecutableTree::FUNCTION_ANALYSIS: pItem.reset(new CFunctionAnalysis); break;
		case CExecutableTree::MERGE_EXECUTABLE: pItem.reset(new CMergeExecutable); break;
		case CExecutableTree::MAPPING: pItem.reset(new CMapping); break;
		case CExecutableTree::IMPORT_FILE: pItem.reset(new CImportData); break;
		case CExecutableTree::INPUT_ANALYSIS: pItem.reset(new CWGInputAnalysis); break;
		case CExecutableTree::DISPERSAL: pItem.reset(new CDispersal); break;
		case CExecutableTree::SCRIPT_R: pItem.reset(new CScript); break;
		case CExecutableTree::COPY_EXPORT: pItem.reset(new CCopyExport); break;
		case CExecutableTree::MODEL_PARAMETERIZATION:pItem.reset(new CModelParameterization); break;
		default: ASSERT(false);
		}
		
		return pItem;
	}


	int CExecutableTree::GetClassType(UINT ID)
	{
		int classType = -1;

		switch (ID)
		{
		case ID_ADD_GROUP:classType = CExecutableTree::GROUP; break;
		case ID_ADD_WEATHER_UPDATE:classType = CExecutableTree::WEATHER_UPDATE; break;
		case ID_ADD_WEATHER_GENERATION:classType = CExecutableTree::WEATHER_GENERATION; break;
		case ID_ADD_MODEL_EXECUTION:classType = CExecutableTree::MODEL_EXECUTION; break;
		case ID_ADD_ANALYSIS:classType = CExecutableTree::ANALYSIS; break;
		case ID_ADD_FUNCTION_ANALYSIS:classType = CExecutableTree::FUNCTION_ANALYSIS; break;
		case ID_ADD_MERGE:classType = CExecutableTree::MERGE_EXECUTABLE; break;
		case ID_ADD_MAP:classType = CExecutableTree::MAPPING; break;
		case ID_ADD_IMPORT_SIMULATION: classType = CExecutableTree::IMPORT_FILE; break;
		case ID_ADD_INPUT_ANALYSIS:classType = CExecutableTree::INPUT_ANALYSIS; break;
		case ID_ADD_DISPERSAL: classType = CExecutableTree::DISPERSAL; break;
		case ID_ADD_SCRIPT_R: classType = CExecutableTree::SCRIPT_R; break;
		case ID_ADD_COPY_EXPORT: classType = CExecutableTree::COPY_EXPORT; break;
		case ID_ADD_MODEL_PARAMETERIZATION:classType = CExecutableTree::MODEL_PARAMETERIZATION; break;
		default: ASSERT(false);
		}

		return classType;
	}


	void CExecutableTree::OnUpdateToolBar(CCmdUI *pCmdUI)
	{
		bool bInit = m_pRoot.get() != NULL;
		HTREEITEM hItem = GetSelectedItem();
		HTREEITEM hParent = GetParentItem(hItem);

		short elemType = GetClassType(hItem);
		short deepElemType = elemType;
		while (deepElemType == GROUP)
		{
			hItem = GetParentItem(hItem);
			if (hItem == NULL)
				break;

			deepElemType = GetClassType(hItem);
		}

		bool bDeepHaveData = 
			deepElemType != CExecutableTree::UNKNOWN && 
			deepElemType != CExecutableTree::GROUP && 
			deepElemType != CExecutableTree::WEATHER_UPDATE && 
			deepElemType != CExecutableTree::SCRIPT_R && 
			deepElemType != CExecutableTree::MODEL_PARAMETERIZATION && 
			deepElemType != CExecutableTree::COPY_EXPORT;

		switch (pCmdUI->m_nID)
		{
		case ID_ADD_GROUP:             pCmdUI->Enable(bInit); break;
		case ID_ADD_WEATHER_UPDATE:
		case ID_ADD_WEATHER_GENERATION: pCmdUI->Enable(deepElemType == CExecutableTree::GROUP); break;
		case ID_ADD_MODEL_EXECUTION:
		case ID_ADD_ANALYSIS:
		case ID_ADD_FUNCTION_ANALYSIS:
		case ID_ADD_DISPERSAL:
		case ID_ADD_SCRIPT_R:
		case ID_ADD_MAP:               
		case ID_ADD_MODEL_PARAMETERIZATION:pCmdUI->Enable(bInit && bDeepHaveData); break;
		case ID_ADD_INPUT_ANALYSIS:	   pCmdUI->Enable(bInit && (deepElemType == CExecutableTree::WEATHER_GENERATION)); break;
		case ID_ADD_MERGE:			   pCmdUI->Enable(bInit); break;
		case ID_ADD_COPY_EXPORT:	   pCmdUI->Enable(bInit && bDeepHaveData); break;
		case ID_EDIT:                  pCmdUI->Enable(bInit && (elemType != CExecutableTree::UNKNOWN && elemType != CExecutableTree::GROUP)); break;
		case ID_EDIT_COPY:			   pCmdUI->Enable(bInit); break;
		case ID_EDIT_PASTE:			   pCmdUI->Enable(bInit); break;
		case ID_EDIT_DUPLICATE:        pCmdUI->Enable(bInit); break;
		case ID_REMOVE:				   pCmdUI->Enable(bInit && elemType != CExecutableTree::UNKNOWN && hParent != NULL); break;
		case ID_ITEM_EXPAND_ALL:       pCmdUI->Enable(bInit && elemType != CExecutableTree::UNKNOWN); break;
		case ID_ITEM_COLLAPSE_ALL:     pCmdUI->Enable(bInit && elemType != CExecutableTree::UNKNOWN); break;
		}
	}



	CExecutablePtr CExecutableTree::GetSelectedExecutable()
	{
		HTREEITEM hItem = CExecutableTree::GetSelectedItem();
		string iName = CExecutableTree::GetInternalName(hItem);

		return m_pRoot->FindItem(iName);
	}


	void CExecutableTree::OnAdd(UINT ID)
	{
		ASSERT(ID >= ID_ADD_FIRST&&ID <= ID_ADD_LAST);

		short classType = GetClassType(ID);

		CBioSIMProject* pProject = dynamic_cast<CBioSIMProject*>(m_pRoot.get());
		if (pProject==NULL || pProject->GetFilePath().empty() )
		{
			if (AfxMessageBox(IDS_BSC_ASK_CREATE_PROJECT, MB_OKCANCEL) == IDOK)
				SendMessage(WM_COMMAND, ID_FILE_SAVE);

			if(pProject == NULL || pProject->GetFilePath().empty())
				return;
		}

		//CBioSIMProject& project = pDoc->GetProject();

		HTREEITEM hParent = GetSelectedItem();
		string iName = GetInternalName(hParent);
		CExecutablePtr pParent = (iName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);

		if (pParent == NULL)
			::AfxThrowInvalidArgException();


		bool bAdd = true;
		CExecutablePtr pItem = GetNewExecutable(classType);

		if (EditExecutable(classType, pItem, pParent))
		{
			InsertItem(pItem, hParent, TVI_LAST, true);
			pParent->InsertItem(pItem);

			//expand parent when we add children
			if (m_pProjectState)
			{
				m_pProjectState->m_expendedItems.insert(pItem->GetInternalName());
				Expand(hParent, TVE_EXPAND);
			}

			Invalidate();
		}
	}

	void CExecutableTree::OnDblClick(NMHDR*, LRESULT* pResult)
	{
		if (m_bCanEdit)
			OnEdit();

		*pResult = TRUE;
	}


	void CExecutableTree::OnEdit()
	{

		HTREEITEM hItem = CExecutableTree::GetSelectedItem();

		if (hItem != NULL &&
			CExecutableTree::GetClassType(hItem) != CExecutableTree::GROUP)
		{
			HTREEITEM hItem = GetSelectedItem();
			string iName = GetInternalName(hItem);
			short classType = GetClassType(hItem);
			CExecutablePtr pItem = (iName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);
			
			ASSERT(pItem);
			if (CString(pItem->GetClassName()) == CExecutableGroup::GetXMLFlag())
				return;

			CExecutablePtr pParent = pItem->GetParent();

			if (EditExecutable(classType, pItem, pParent))
			{
				m_pRoot->SetItem(iName, pItem);
				SetItem(pItem.get(), hItem);

				//pDoc->UpdateAllViews(this, CBioSIMDoc::PROJECT_CHANGE);
				Invalidate();
			}
			//*pResult = TRUE;
		}
		else
		{
			//*pResult = FALSE;
		}
	}

	void CExecutableTree::OnRemove()
	{
		HTREEITEM hItem = GetSelectedItem();
		string iName = GetInternalName(hItem);
		m_internalName.erase(iName);

		DeleteItem(hItem);
		m_pRoot->RemoveItem(iName);
	}


	
	void CExecutableTree::OnEditCopy()
	{
		HTREEITEM hItem = GetSelectedItem();
		string iName = GetInternalName(hItem);
		CExecutablePtr pItem = (iName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);

		if (pItem)
			pItem->CopyToClipBoard();
	}

	void CExecutableTree::OnEditPaste()
	{
		if (m_pRoot.get()==NULL)
			return;

		

		HTREEITEM hParent = GetSelectedItem();
		string iName = GetInternalName(hParent);
		CExecutablePtr pParent = (iName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);

		CExecutablePtr pItem = pParent->CopyFromClipBoard();
		if (pItem)
		{
			if (CanPaste(pItem, hParent))
			{
				pParent->InsertItem(pItem);
				InsertItem(pItem, hParent, TVI_LAST, true);

				//pDoc->UpdateAllViews(this, CBioSIMDoc::PROJECT_CHANGE);
				Invalidate();
			}
			//else
			//{
				//CString 
				//AfxMessageBox(IDS_CANT_PASTE_HERE, pItem->GetClassName(), pParent->GetClassName());
			//}
		}

	}

	void CExecutableTree::OnEditDuplicate()
	{
		OnEditCopy();

		//paste at the same level (on parent)
		if (m_pRoot.get()==NULL)
			return;

		

		HTREEITEM hItem = GetSelectedItem();
		HTREEITEM hParent = GetParentItem(hItem);

		string iName = CExecutableTree::GetInternalName(hParent);
		CExecutablePtr pParent = (iName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);
		if (pParent)
		{
			CExecutablePtr pItem = pParent->CopyFromClipBoard();

			if (pItem)
			{
				if (CanPaste(pItem, hParent))
				{
					pParent->InsertItem(pItem);
					InsertItem(pItem, hParent, TVI_LAST, true);
					Invalidate();
				}
			}
		}
	}


	void CExecutableTree::OnEndEditLabel(NMHDR* pNMHDR, LRESULT* pResult)
	{
		TRACE(_T("in CProjectCtrl::OnEndEditLabel\n"));

		*pResult = TRUE;			// return TRUE to accept edit

		NMTVDISPINFO* pTVDispInfo = (NMTVDISPINFO*)pNMHDR;
		HTREEITEM hItem = pTVDispInfo->item.hItem;
		ASSERT(hItem);

		LPTSTR pszText = pTVDispInfo->item.pszText;
		if (pszText)
		{
			if (*pszText != _T('\0'))
			{
				string iName = CExecutableTree::GetInternalName(hItem);

				CExecutablePtr pItem = (iName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);
				ASSERT(pItem);
				pItem->SetName(string(CStringA(pszText)));

			}
			else
			{
				*pResult = FALSE;		// don't allow empty label
			}
		}
	}

	//=============================================================================
	LRESULT CExecutableTree::OnBeginDrag(WPARAM wParam, LPARAM lParam)
		//
		// Message handler for WM_XHTMLTREE_BEGIN_DRAG:
		//    wParam = pointer to XHTMLTREEMSGDATA struct
		//    lParam = pointer to XHTMLTREEDRAGMSGDATA struct
		//=============================================================================
	{
		TRACE(_T("in CXHtmlTreeTestDlg::OnBeginDrag\n"));

		XHTMLTREEMSGDATA *pMsg = (XHTMLTREEMSGDATA *)wParam;
		ASSERT(pMsg);

		XHTMLTREEDRAGMSGDATA *pData = (XHTMLTREEDRAGMSGDATA *)lParam;
		LRESULT lResult = 0;

		if (pMsg && pData && pData->hItem)
		{
			/*
			CString strCopyMove = _T("move");
			if (pData->bCopyDrag)
			strCopyMove = _T("copy");
			CString strItem = m_Tree.GetItemText(pData->hItem);
			TRACE(_T("starting %s drag on '%s'\n"), strCopyMove, strItem);

			if (strItem == _T("Longdog"))
			lResult = 1;

			if (m_bLog && (lResult == 0))
			m_List.Printf(CXListBox::Blue, CXListBox::White, 0,
			_T("%04d  starting %s drag on '%s'"),
			m_nLineNo++, strCopyMove, strItem);
			else if (m_bLog && (lResult == 1))
			m_List.Printf(CXListBox::Red, CXListBox::White, 0,
			_T("%04d  rejecting drag of '%s'"),
			m_nLineNo++, strItem);
			*/
		}
		else
		{
			TRACE(_T("ERROR bad param\n"));
			ASSERT(FALSE);
		}

		return lResult;	// return 0 to allow drag
	}

	//=============================================================================
	LRESULT CExecutableTree::OnEndDrag(WPARAM wParam, LPARAM lParam)
		//
		// Message handler for WM_XHTMLTREE_END_DRAG:
		//    wParam = pointer to XHTMLTREEMSGDATA struct
		//    lParam = pointer to XHTMLTREEDRAGMSGDATA struct; 0 = drag terminated
		//=============================================================================
	{
		TRACE(_T("in CXHtmlTreeTestDlg::OnEndDrag\n"));

		XHTMLTREEMSGDATA *pMsg = (XHTMLTREEMSGDATA *)wParam;
		ASSERT(pMsg);

		XHTMLTREEDRAGMSGDATA *pData = (XHTMLTREEDRAGMSGDATA *)lParam;
		//CBioSIMDoc* pDoc = GetDocument();

		LRESULT lResult = 0;

		if (pMsg && pData && pData->hItem /*&& pDoc->IsInit()*/)
		{
			//if ()
			//{
				HTREEITEM hAfter = pData->hAfter;
				HTREEITEM hParent = CExecutableTree::GetParentItem(pData->hItem);
				HTREEITEM hParentPasteOn = NULL;
				if (((UINT_PTR)pData->hAfter & 0xFFFF0000) == 0xFFFF0000)
				{
					hParentPasteOn = pData->hNewParent;
					hAfter = NULL;
				}
				else
				{
					hParentPasteOn = CExecutableTree::GetParentItem(pData->hAfter);
					bool bExpended = CExecutableTree::IsExpanded(pData->hAfter);
					if (bExpended)
						hParentPasteOn = NULL;
				}

				//if(((UINT_PTR)pData->hAfter & 0xFFFF0000) == 0xFFFF0000) )
				//{
				//insert element first
				//}
				//ASSERT(CExecutableTree::GetParentItem(pData->hAfter) == hParent);
				//HTREEITEM hParentPasteOn = CExecutableTree::GetParentItem(pData->hAfter);

				//HTREEITEM hParentPasteOn = CExecutableTree::GetParentItem(hPasteOnItem);
				//if( !CanPaste(pItem, hPasteOnItem ) )
				if (hParentPasteOn == hParent)
				{
					//CBioSIMProject& project = pDoc->GetProject();


					string iName = GetInternalName(pData->hItem);
					CExecutablePtr pItem = (iName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);
					

					string iAfterName = GetInternalName(hAfter);
					CExecutablePtr pAfterItem = (iAfterName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iAfterName);
					//CExecutablePtr pAfterItem = m_pRoot->FindItem(iAfterName);

					m_pRoot->MoveItem(iName, iAfterName, pData->bCopyDrag);


					//HTREEITEM hPasteOnItem = NULL;
					//if (((UINT_PTR)pData->hAfter & 0xFFFF0000) == 0xFFFF0000)
					//hPasteOnItem = pData->hNewParent;
					//else if (pData->hAfter)
					//hPasteOnItem = pData->hAfter;
				}
				else
				{
					lResult = 1;
				}

				/*if( CanPaste(pItem, hPasteOnItem ) )
				{
				}
				else
				{
				lResult = 1;
				}*/
			//}
			//else
			//{
				// lParam = 0 ==> drag was terminated by user (left button up
				// when not on item, ESC key, right mouse button down)
				//if (m_bLog)
				//m_List.Printf(CXListBox::Red, CXListBox::White, 0, 
				//_T("%04d  drag terminated by user"), m_nLineNo++);
			//}
		}

		return lResult;	// return 0 to allow drop
	}

	//=============================================================================
	LRESULT CExecutableTree::OnDropHover(WPARAM wParam, LPARAM lParam)
		//
		// Message handler for WM_XHTMLTREE_DROP_HOVER:
		//    wParam = pointer to XHTMLTREEMSGDATA struct
		//    lParam = pointer to XHTMLTREEDRAGMSGDATA struct
		//=============================================================================
	{
		TRACE(_T("in CXHtmlTreeTestDlg::OnDropHover\n"));

		XHTMLTREEMSGDATA *pMsg = (XHTMLTREEMSGDATA *)wParam;
		ASSERT(pMsg);

		XHTMLTREEDRAGMSGDATA *pData = (XHTMLTREEDRAGMSGDATA *)lParam;
		//CBioSIMDoc* pDoc = GetDocument();


		LRESULT lResult = 0;

		if (pMsg && pData /*&& pDoc->IsInit()*/)
		{
			//CBioSIMProject& project = pDoc->GetProject();

			//string iName = CExecutableTree::GetInternalName(pData->hItem);
			//CExecutablePtr pItem = project.FindItem(iName);
			HTREEITEM hParent = GetParentItem(pData->hItem);
			//HTREEITEM hPasteOnItem = pData->hAfter;
			//if (((UINT_PTR)pData->hAfter & 0xFFFF0000) == 0xFFFF0000)
			//hPasteOnItem = pData->hNewParent;
			//else if (pData->hAfter)
			//hPasteOnItem = pData->hAfter;
			HTREEITEM hParentPasteOn = GetParentItem(pData->hAfter);
			bool bExpended = IsExpanded(pData->hAfter);
			if (bExpended)
				hParentPasteOn = pData->hAfter;
			//{
			//	//look to se if it the last item
			//	HTREEITEM hNextItem = CExecutableTree::GetNextItem(pData->hAfter);
			//	HTREEITEM hParentNextItem = CExecutableTree::GetParentItem(hNextItem);
			//	hParentPasteOn = hParentNextItem;
			//}


			//if( !CanPaste(pItem, hPasteOnItem ) )
			if (hParent != hParentPasteOn)
				lResult = 1;

			CString strItem = GetItemText(hParent);
			CString strTextHover = GetItemText(hParentPasteOn);
			CString proposedNewParent = GetItemText(pData->hNewParent);
			TRACE(_T("*********old parent '%s' new parent '%s' proposed new parent '%s'\n"), strItem, strTextHover, pData->hNewParent ? proposedNewParent : _T("NULL"));

			//TRACE(_T("dragging '%s' over '%s'\n"), strItem, strTextHover);

			//if (strTextHover == _T("Longdog"))
			//lResult = 1;
		}
		else
		{
			TRACE(_T("ERROR bad param\n"));
			ASSERT(FALSE);
		}

		return lResult;
	}


	LRESULT CExecutableTree::OnItemExpanded(WPARAM wParam, LPARAM lParam)
	{
		XHTMLTREEMSGDATA *pData = (XHTMLTREEMSGDATA *)wParam;
		ASSERT(pData);

		if (pData)
		{
			HTREEITEM hItem = pData->hItem;
			ASSERT(hItem);

			string iName = CExecutableTree::GetInternalName(hItem);
			bool bExpanded = CExecutableTree::IsExpanded(hItem);

			if (m_pProjectState)
			{
				if (bExpanded)
					m_pProjectState->m_expendedItems.insert(iName);
				else
					m_pProjectState->m_expendedItems.erase(iName);
			}
			
		}

		return 0;

	}


	void CExecutableTree::Update()
	{
		DeleteAllItems();
		m_internalName.clear();
		InsertItem(m_pRoot);

		HTREEITEM hItem = GetSelectedItem();
		HTREEITEM hNewItem = FindItem(m_curSel);
		if (hNewItem != hItem)
		{
			Select(hNewItem, TVGN_CARET);
		}

	}

	BOOL CExecutableTree::OnItemMove(UINT ID)
	{
		HTREEITEM hItem = CExecutableTree::GetSelectedItem();
		
		if( hItem && ID==ID_ITEM_EXPAND_ALL)
			CExecutableTree::Expand(hItem, TVE_EXPAND);
	
		if( hItem )
			hItem = CExecutableTree::GetChildItem(hItem);
	
		if( hItem )
		{
			do 
			{
				switch(ID)
				{
				case ID_ITEM_EXPAND_ALL: CExecutableTree::ExpandBranch(hItem); break;
				case ID_ITEM_COLLAPSE_ALL: CExecutableTree::CollapseBranch(hItem); break;
				default: ASSERT( false);
				}
	
			} while ((hItem = CExecutableTree::GetNextSiblingItem(hItem)) != NULL);
		}
	
		return TRUE;
	}
		
	void CExecutableTree::OnContextMenu(CWnd* pWnd, CPoint point)
	{

		CExecutableTree* pWndTree = (CExecutableTree*)(this);
		ASSERT_VALID(pWndTree);

		int classType = -1;
		if (point != CPoint(-1, -1))
		{
			// Select clicked item:
			CPoint ptTree = point;
			pWndTree->ScreenToClient(&ptTree);

			UINT flags = 0;
			HTREEITEM hTreeItem = pWndTree->HitTest(ptTree, &flags);
			if (hTreeItem != NULL)
			{
				classType = pWndTree->GetClassType(hTreeItem);
			}

			pWndTree->SelectItem(hTreeItem);
		}

		pWndTree->SetFocus();
		// a faire??? pas ici surement
//		int ID = GetMenuIDFromClass(classType);

		//theApp.GetContextMenuManager()->ShowPopupMenu(ID, point.x, point.y, this, TRUE);
	}


	LRESULT CExecutableTree::OnCheckbox(WPARAM wParam, LPARAM lParam)
	{
		XHTMLTREEMSGDATA *pData = (XHTMLTREEMSGDATA *)wParam;
		ASSERT(pData);
	
		BOOL bChecked = lParam;
	
		if (pData)
		{
			HTREEITEM hItem = pData->hItem;
	
			if (hItem)
			{
				//CBioSIMDoc* pDoc = GetDocument();
				//CBioSIMProject& project = pDoc->GetProject();
				string iName = CExecutableTree::GetInternalName(hItem);
	
				CExecutablePtr pItem = (iName == m_pRoot->GetInternalName()) ? m_pRoot : m_pRoot->FindItem(iName);
				ASSERT(pItem);
				if(pItem)
					pItem->SetExecute(bChecked);
				
			}
		}
	
		return 0;
	}

	
	void CExecutableTree::SetExecutable(CExecutablePtr& pRoot, CProjectStatePtr& pProjectState)
	{
		m_pRoot = pRoot;
		m_pProjectState = pProjectState;

		DeleteAllItems();
		m_internalName.clear();
		InsertItem(m_pRoot);
	}

	LRESULT CExecutableTree::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
	{
		return CXHtmlTree::WindowProc(message, wParam, lParam);
	}

	BOOL CExecutableTree::OnCommand(WPARAM wParam, LPARAM lParam)
	{
		return CXHtmlTree::OnCommand(wParam, lParam);
	}

	void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, WBSF::StringVector& data)
	{
		CExecutableTree* pCtrl = dynamic_cast<CExecutableTree*>(pDX->m_pDlgWnd->GetDlgItem(ID));
		ASSERT(pCtrl);

		if (pDX->m_bSaveAndValidate)
		{
			pCtrl->GetCheckedItem(data);
		}
		else
		{
			pCtrl->SetCheckedItem(data);
		}
	}

	
}