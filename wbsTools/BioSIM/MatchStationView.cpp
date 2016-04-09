
// BioSIMView.cpp : implementation of the CMatchStationView class
//

#include "stdafx.h"
#include "BioSIM.h"

#include "BioSIMDoc.h"
#include "MatchStationView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace UtilWin;
// CMatchStationView

IMPLEMENT_DYNCREATE(CMatchStationView, CListView)

BEGIN_MESSAGE_MAP(CMatchStationView, CListView)
	ON_WM_CREATE()
	ON_WM_SIZE()
//	ON_COMMAND(ID_EXECUTE, &OnExecute)
//	ON_UPDATE_COMMAND_UI(ID_EXECUTE, &OnUpdateExecute)
END_MESSAGE_MAP()

// CMatchStationView construction/destruction

CMatchStationView::CMatchStationView()
{
	
}

CMatchStationView::~CMatchStationView()
{
}

BOOL CMatchStationView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style |= LVS_REPORT;
	return CListView::PreCreateWindow(cs);
}

// CMatchStationView drawing

void CMatchStationView::OnDraw(CDC* /*pDC*/)
{
	CBioSIMDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}

void CMatchStationView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CMatchStationView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
}


// CMatchStationView diagnostics



// CMatchStationView message handlers

int CMatchStationView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListView::OnCreate(lpCreateStruct) == -1)
		return -1;

	//m_grid.CreateGrid( WS_CHILD|WS_VISIBLE, CRect(0,0,0,0), this, 1234 );    // arbitrary ID of 1234 - season to taste
	CListCtrl& wndList = GetListCtrl ();

	wndList.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	const int nColumns = 15;
	int iColumn = 0;

	// Insert columns:
	for (iColumn = 0; iColumn < nColumns; iColumn++)
	{
		CString strColumn;
		strColumn.Format (_T("Column %d"), iColumn + 1);

		wndList.InsertColumn (iColumn, strColumn, LVCFMT_LEFT, 70);
	}

	// Insert items:
	for (int i = 0; i < 100; i++)
	{
		const CString strItemFmt = _T("Item (%d, %d)");
		
		CString strItem;
		strItem.Format (strItemFmt, 1, i + 1);

		int iItem = wndList.InsertItem (i, strItem, 0);

		for (iColumn = 1; iColumn < nColumns; iColumn++)
		{
			strItem.Format (strItemFmt, iColumn + 1, i + 1);
			wndList.SetItemText (iItem, iColumn, strItem);
		}
	}

	return 0;
}

void CMatchStationView::OnSize(UINT nType, int cx, int cy)
{
	CListView::OnSize(nType, cx, cy);

	//m_grid.MoveWindow( 0, 0, cx, cy );       // size the grid control to 

}

//void CMatchStationView::OnSetup()
//{
//    
//     int rows = 20;
//     int cols = 20;
//     int i,j;
//     CString  temp;
//     CUGCell  cell;
//
//     // setup rows and colums
//
//     m_grid.SetNumberRows(rows);
//     m_grid.SetNumberCols(cols);
//
//     // fill cells with data
//
//     m_grid.GetCell(0,1,&cell);
//     for (i = 0; i < cols; i++) {
//          for (j = 0; j < rows; j++) {
//               temp.Format("%d",(i+1)*(j+1));
//               cell.SetText(temp);
//               m_grid.SetCell(i,j,&cell);
//         }
//     }
//     // add column headngs
//
//     for (i = 0; i < cols; i++) {
//          temp.Format("%d",(i+1));
//          cell.SetText(temp);
//          m_grid.SetCell(i,-1,&cell);
//     }
//     // add row headings
//
//     for (j = 0; j < rows; j++) {
//          temp.Format("%d",(j+1));
//          cell.SetText(temp);
//          m_grid.SetCell(-1,j,&cell);
//     }
//}

void CMatchStationView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	//CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
	//CBioSIMProject& project = pDoc->GetProject();

	//CString iName = pDoc->GetCurSel();
	//CExecutable* pItem = project.FindItem(iName);
	//if( pItem )
	//{
	//	CResultPtr result = pItem->GetResult(GetFileManager());
	//	m_grid.SetData(result);
	//	m_grid.Invalidate();
	//}	
	
}

/*void CMatchStationView::OnExecute()
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
	
	CSCCallBack callback;
	CProgressStepDlg dlg(callback, AfxGetMainWnd() );
	dlg.Create( AfxGetMainWnd() );

	
	ERMsg msg = pDoc->Execute(callback);

	dlg.DestroyWindow();

	
	if( !msg)
		SYShowMessage( msg, this );

}

void CMatchStationView::OnUpdateExecute(CCmdUI *pCmdUI)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetDocument();
	pCmdUI->Enable(true);
}
*/
#ifdef _DEBUG
void CMatchStationView::AssertValid() const
{
	CListView::AssertValid();
}

void CMatchStationView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CBioSIMDoc* CMatchStationView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBioSIMDoc)));
	return (CBioSIMDoc*)m_pDocument;
}
#endif //_DEBUG
