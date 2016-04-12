
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

CPropertiesWnd::CPropertiesWnd()
{
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

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

/*void CPropertiesWnd::InitPropList()
{
	SetPropListFont();

	m_wndPropList.EnableHeaderCtrl(FALSE);
	m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();
	m_wndPropList.MarkModifiedProperties();

	CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Appearance"));

	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("3D Look"), (_variant_t) false, _T("Specifies the window's font will be non-bold and controls will have a 3D border")));

	CMFCPropertyGridProperty* pProp = new CMFCPropertyGridProperty(_T("Border"), _T("Dialog Frame"), _T("One of: None, Thin, Resizable, or Dialog Frame"));
	pProp->AddOption(_T("None"));
	pProp->AddOption(_T("Thin"));
	pProp->AddOption(_T("Resizable"));
	pProp->AddOption(_T("Dialog Frame"));
	pProp->AllowEdit(FALSE);

	pGroup1->AddSubItem(pProp);
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("Caption"), (_variant_t) _T("About"), _T("Specifies the text that will be displayed in the window's title bar")));

	m_wndPropList.AddProperty(pGroup1);

	CMFCPropertyGridProperty* pSize = new CMFCPropertyGridProperty(_T("Window Size"), 0, TRUE);

	pProp = new CMFCPropertyGridProperty(_T("Height"), (_variant_t) 250l, _T("Specifies the window's height"));
	pProp->EnableSpinControl(TRUE, 0, 1000);
	pSize->AddSubItem(pProp);

	pProp = new CMFCPropertyGridProperty( _T("Width"), (_variant_t) 150l, _T("Specifies the window's width"));
	pProp->EnableSpinControl();
	pSize->AddSubItem(pProp);

	m_wndPropList.AddProperty(pSize);

	CMFCPropertyGridProperty* pGroup2 = new CMFCPropertyGridProperty(_T("Font"));

	LOGFONT lf;
	CFont* font = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	font->GetLogFont(&lf);

	lstrcpy(lf.lfFaceName, _T("Arial"));

	pGroup2->AddSubItem(new CMFCPropertyGridFontProperty(_T("Font"), lf, CF_EFFECTS | CF_SCREENFONTS, _T("Specifies the default font for the window")));
	pGroup2->AddSubItem(new CMFCPropertyGridProperty(_T("Use System Font"), (_variant_t) true, _T("Specifies that the window uses MS Shell Dlg font")));

	m_wndPropList.AddProperty(pGroup2);

	CMFCPropertyGridProperty* pGroup3 = new CMFCPropertyGridProperty(_T("Misc"));
	pProp = new CMFCPropertyGridProperty(_T("(Name)"), _T("Application"));
	pProp->Enable(FALSE);
	pGroup3->AddSubItem(pProp);

	CMFCPropertyGridColorProperty* pColorProp = new CMFCPropertyGridColorProperty(_T("Window Color"), RGB(210, 192, 254), NULL, _T("Specifies the default window color"));
	pColorProp->EnableOtherButton(_T("Other..."));
	pColorProp->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));
	pGroup3->AddSubItem(pColorProp);

	static TCHAR BASED_CODE szFilter[] = _T("Icon Files(*.ico)|*.ico|All Files(*.*)|*.*||");
	pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("Icon"), TRUE, _T(""), _T("ico"), 0, szFilter, _T("Specifies the window icon")));

	pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("Folder"), _T("c:\\")));

	m_wndPropList.AddProperty(pGroup3);

	CMFCPropertyGridProperty* pGroup4 = new CMFCPropertyGridProperty(_T("Hierarchy"));

	CMFCPropertyGridProperty* pGroup41 = new CMFCPropertyGridProperty(_T("First sub-level"));
	pGroup4->AddSubItem(pGroup41);

	CMFCPropertyGridProperty* pGroup411 = new CMFCPropertyGridProperty(_T("Second sub-level"));
	pGroup41->AddSubItem(pGroup411);

	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 1"), (_variant_t) _T("Value 1"), _T("This is a description")));
	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 2"), (_variant_t) _T("Value 2"), _T("This is a description")));
	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 3"), (_variant_t) _T("Value 3"), _T("This is a description")));

	pGroup4->Expand(FALSE);
	m_wndPropList.AddProperty(pGroup4);
}
*/
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