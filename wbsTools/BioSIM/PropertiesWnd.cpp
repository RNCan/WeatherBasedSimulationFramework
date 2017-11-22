
#include "stdafx.h"

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "BioSIM.h"
#include "BioSIMDoc.h"

using namespace WBSF;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif





static const UINT ID_PROPERTIES_CTRL = 1000;



BEGIN_MESSAGE_MAP(CExecutablePropertiesCtrl, CMFCPropertyGridCtrl)
	ON_WM_CREATE()
END_MESSAGE_MAP()





int CExecutablePropertiesCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCPropertyGridCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	InitPropList();

	return 0;
}

void CExecutablePropertiesCtrl::InitPropList()
{
	SetPropListFont();

	EnableHeaderCtrl(FALSE);
	EnableDescriptionArea(false);
	SetVSDotNetLook();
	MarkModifiedProperties();
}

void CExecutablePropertiesCtrl::SetXML(const zen::XmlElement& in)
{
	//ASSERT(pRoot);
	//if (pRoot == NULL)
	//	AfxThrowInvalidArgException();

	RemoveAll();

	//create property the size of XML
	CMFCPropertyGridProperty* pGroup = NULL;

	AddProperties(&pGroup, in);
	if (pGroup)
	{
		pGroup->Expand();
		AddProperty(pGroup);
	}
	
}

void CExecutablePropertiesCtrl::AddProperties(CMFCPropertyGridProperty** pProperties, const zen::XmlElement& in)
{
	ASSERT(pProperties);


	std::string name = in.getNameAs<std::string>();
	std::string value;
	in.getValue<std::string>(value);
	
	auto children = in.getChildren();
	size_t nbChild = std::distance(children.first, children.second);

	//add current element

	CMFCPropertyGridProperty* pSubProp = (nbChild > 0) ? new CMFCPropertyGridProperty(CString(name.c_str())) : new CMFCPropertyGridProperty(CString(name.c_str()), CString(value.c_str()));
	pSubProp->AllowEdit(FALSE);

	//add child element
	//for (size_t i = 0; i<nbChild; i++)
	for (auto it = children.first; it != children.second; it++)
	{
		//const zen::XmlElemen child = pRoot->GetChild(i);
		
		//if (pChild->name != CExecutableVector::GetXMLFlag())//don't add sub items
		AddProperties(&pSubProp, (*it) );
	}


	////add property to the parant
	if (pSubProp->IsGroup())
		pSubProp->Expand(false);

	if (*pProperties == NULL)
		*pProperties = pSubProp;
	else (*pProperties)->AddSubItem(pSubProp);
}

void CExecutablePropertiesCtrl::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	m_fntPropList.CreateStockObject(DEFAULT_GUI_FONT);
	SetFont(&m_fntPropList);
}

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

CBioSIMDoc* CPropertiesWnd::GetDocument()
{
	CDocument* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = docT->GetNextDoc(pos);
		}
	}

	return static_cast<CBioSIMDoc*>(pDoc);
}

CPropertiesWnd::CPropertiesWnd()
{
}

CPropertiesWnd::~CPropertiesWnd()
{
}


int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_propertiesCtrl.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(0, 0, 0, 0), this, ID_PROPERTIES_CTRL))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	AdjustLayout();
	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}


void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rect;
	GetClientRect(rect);

	m_propertiesCtrl.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
}

void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_propertiesCtrl.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	m_propertiesCtrl.SetPropListFont();
}


BOOL CPropertiesWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the view to rout command
	if (GetFocus() == &m_propertiesCtrl && m_propertiesCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CPropertiesWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CBioSIMDoc* pDoc = GetDocument();
	if (!pDoc)
		return;

	WBSF::CExecutablePtr& project = pDoc->GetExecutable();

	zen::XmlElement xml;
	project->writeStruc(xml);

	m_propertiesCtrl.SetXML(xml);
}