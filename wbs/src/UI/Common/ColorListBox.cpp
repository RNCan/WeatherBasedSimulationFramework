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
#include "UI/Common/ColorListBox.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
 
/////////////////////////////////////////////////////////////////////////////
// CColorListBox

CColorListBox::CColorListBox(TColorListType type):
m_type(type)
{
}

CColorListBox::~CColorListBox()
{
    
}


BEGIN_MESSAGE_MAP(CColorListBox, CListBox)
	//{{AFX_MSG_MAP(CColorListBox)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CColorListBox message handlers


void CColorListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rcItem(lpDrawItemStruct->rcItem);

    
    int index = lpDrawItemStruct->itemID;
    
    BOOL bFocus = lpDrawItemStruct->itemState & ODS_SELECTED;

    CBrush brush ( ::GetSysColor( COLOR_3DFACE ) );//bFocus? ::GetSysColor(COLOR_3DSHADOW): 
    pDC->FillRect(rcItem, &brush );

    if ( bFocus)
    {
		pDC->Draw3dRect(rcItem, ::GetSysColor(COLOR_3DHIGHLIGHT), ::GetSysColor(COLOR_3DDKSHADOW) );
        rcItem.InflateRect(-1,-1,-1,-1);
        pDC->Draw3dRect(rcItem, ::GetSysColor( COLOR_3DFACE ), ::GetSysColor(COLOR_3DSHADOW) );
    }

    if( index < 0 || index >= GetCount() )
        return;

    if( m_type == RANGE )
    {
        CColorProfile& profile = GetColorProfile( index );

        CRect beginRect = GetBeginRect(rcItem);
        CBrush beginBrush( profile.GetBeginColor() );
        CRect endRect = GetEndRect(rcItem);
        CBrush endBrush( profile.GetEndColor() );
        CRect progressRect = GetProgressRect(rcItem);
    
        pDC->FillRect( beginRect, &beginBrush);
        pDC->FillRect( endRect, &endBrush);
        
        
        CStepColor step;
        profile.GetStepColor(progressRect.Width()*profile.GetPound(), step);

	    for(int j=0; j<progressRect.Width(); j++)
        {
            COLORREF currentColor = profile.GetBeginColor();
            step.StepColor(j, currentColor);

            CPen pen(PS_SOLID, 1 ,currentColor);
            CPen* pOldPen = pDC->SelectObject(&pen);

            pDC->MoveTo(progressRect.left+j, progressRect.top);
            pDC->LineTo(progressRect.left+j, progressRect.bottom);
            
            pDC->SelectObject(pOldPen);
        }

        //CFont* pFont = CFont::FromHandle( (HFONT)GetStockObject(DEFAULT_GUI_FONT) ); // SYSTEM_FONT ANSI_VAR_FONT
        //CFont* pOldFont = (CFont*)pDC->SelectObject (pFont);
        //pDC->SetTextColor( RGB(255, 255, 255) );
        //int oldMode = pDC->SetBkMode( TRANSPARENT );
    
    
        //CString poundText;
        //poundText.Format( "%d", profile.GetPound() );
        //pDC->TextOut(progressRect.left, progressRect.top, poundText);
        //pDC->SelectObject( pOldFont );
        //pDC->SetBkMode( oldMode );
    }
    else
    {
        CRect rectColor = GetColorRect(rcItem);
        CBrush colorBrush( GetColor(index) );

        pDC->FillRect( rectColor, &colorBrush);
    }

}

void CColorListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
    lpMeasureItemStruct->itemHeight = 40;
}

void CColorListBox::OnLButtonDown(UINT nFlags, CPoint point) 
{
	
	CListBox::OnLButtonDown(nFlags, point);
}

void CColorListBox::OnLButtonUp(UINT nFlags, CPoint point) 
{
	//else CDragListBox::OnLButtonDown(nFlags, point);
	
    CListBox::OnLButtonUp(nFlags, point);	
}


/*void CColorListBox::OnExportBrowse() 
{
    CString name;
    CAppOption option;    

    exportName()->GetWindowText(name);
    CString sPath = UtilWin::GetPath(name);

    CString filter;
    filter.LoadString(IDS_EXPORT_FILTER);

    if( name.IsEmpty() || sPath.IsEmpty() )
    {
        sPath = m_defaultDirectory;
        option.GetLastOpenPath("Export path", sPath );
    }

	CFileDialog openDialog(false, "*.dat", name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, filter, this);
    openDialog.m_ofn.lpstrInitialDir = sPath;
    openDialog.m_ofn.nFilterIndex = GetFormatCtrl().GetCurSel()==3?2:1;

	if(openDialog.DoModal() == IDOK)
	{
		exportName()->SetWindowText( openDialog.GetPathName() );
        option.SetLastOpenPath("Export path", UtilWin::GetPath( openDialog.GetPathName() ) );
	}
	
}
*/

void CColorListBox::AddNew() 
{
	ASSERT( m_hWnd );

    if( m_type == RANGE )
    {
        int pos = AddString(_T(""));

        COLORREF color = RGB(0,0,0);
        if( pos > 0 )
            color = GetColorProfile(pos-1).GetEndColor();
        
        SetItemDataPtr( pos, new CColorProfile(color, color, 1) );
    }
    else
    {
        int pos = AddString(_T(""));
        SetItemData( pos, RGB(0,0,0) );
    }

}

void CColorListBox::DeleteSelected() 
{
    int nSelCount = GetSelCount( );
    int* pSel = new int[nSelCount];
    GetSelItems(nSelCount, pSel);

    
    for(int i=nSelCount-1; i>= 0; i--)
    {
        if( m_type == RANGE )
        {
            CColorProfile* pInfo = (CColorProfile*)GetItemDataPtr(pSel[i]);
            delete pInfo;
            pInfo = NULL;
        }
        DeleteString( pSel[i] );
    }

    if( nSelCount > 0)
    {
        if( pSel[0] < GetCount() )
            SetSel( pSel[0] );
        else SetSel( pSel[0] - 1);
    }

    delete pSel;

}

void CColorListBox::SelectionUp() 
{
    int nSelCount = GetSelCount( );
    ASSERT(nSelCount != 0);

    int* pSel = new int[nSelCount];
    GetSelItems(nSelCount, pSel);

    
    for(int i=0; i< nSelCount; i++)
    {
        ASSERT( pSel[i] !=  0);     
        MoveString( pSel[i], pSel[i]-1);
    }

    SetSelection(pSel, nSelCount, -1);
    

    delete pSel;
}

void CColorListBox::SelectionDown() 
{
    int nSelCount = GetSelCount( );
    ASSERT(nSelCount != 0);

    int* pSel = new int[nSelCount];
    GetSelItems(nSelCount, pSel);

    
    for(int i=nSelCount-1; i>= 0; i--)
    {
        ASSERT(pSel[i] != GetCount()-1 );
        MoveString( pSel[i], pSel[i]+1);
    }

    SetSelection(pSel, nSelCount, 1);

    delete pSel;
}

void CColorListBox::SetSelection(int* pSel, int nSize, int offset)
{
    ASSERT(nSize > 0);

    for(int i=0; i<nSize; i++)
        if( pSel[i] + offset >= 0 && pSel[i] + offset < GetCount() )
            SetSel( pSel[i]+ offset, TRUE);
}


void CColorListBox::MoveString( int oldPos, int newPos )
{
	CString text;
	GetText( oldPos, text );
	DWORD_PTR data = GetItemData(oldPos);

	DeleteString( oldPos );
	InsertString( newPos, text );
	SetItemData(newPos, data);
	SetCurSel(newPos);
}



bool CColorListBox::SelectionDownEnable()const
{
    bool bDownEnable = false;
    int nSelCount = GetSelCount( );

	if( nSelCount > 0 )
	{
        int* pSel = new int[nSelCount];
        GetSelItems(nSelCount, pSel);
        
        if( pSel[nSelCount-1] != GetCount()-1)
            bDownEnable = true;
        
        delete pSel;
        pSel = NULL;
	}

    return bDownEnable;
}

bool CColorListBox::SelectionUpEnable()const
{
    bool bUpEnable = false;
    int nSelCount = GetSelCount( );

	if( nSelCount > 0 )
	{
        int* pSel = new int[nSelCount];
        GetSelItems(nSelCount, pSel);

        
        if( pSel[0] != 0 ) 
            bUpEnable = true;
        
        delete pSel;
        pSel = NULL;
	}

    return bUpEnable;

}

void CColorListBox::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
    BOOL outside = TRUE;
	int nHitItem = ItemFromPoint(point, outside);

    BOOL bHit = FALSE;

    if( !outside && nHitItem >= 0 )
    {
        CRect rcItem;
        GetItemRect(nHitItem, rcItem);

        if( m_type == RANGE )
        {
            CRect rectBeginColor = GetBeginRect(rcItem);
            CRect rectEndColor = GetEndRect(rcItem);
            //CRect rectProgressColor = GetProgressRect(rcItem);
            CColorProfile& profile = GetColorProfile( nHitItem );

		    if ( rectBeginColor.PtInRect( point ) )
            {
                CColorDialog colorDlg(profile.GetBeginColor() , 0, this);
	            if( colorDlg.DoModal() == IDOK)
	            {
                    bHit = TRUE;
                    profile.SetBeginColor( colorDlg.GetColor() );
	            }
            }
            else if ( rectEndColor.PtInRect( point ) )
            {
                CColorDialog colorDlg(profile.GetEndColor() , 0, this);
	            if( colorDlg.DoModal() == IDOK)
	            {
                    bHit = TRUE;
                    profile.SetEndColor( colorDlg.GetColor() );
	            }

            }
            /*else if ( rectProgressColor.PtInRect( point ) )
            {
                CEnterPalettePound poundDlg;
                poundDlg.m_pound = profile.GetPound();
	            if( poundDlg.DoModal() == IDOK)
	            {
                    bHit = TRUE;
                    profile.SetPound(poundDlg.m_pound );
	            }
            }
            */
        }
        else
        {
            CRect rectColor = GetColorRect(rcItem);
            if ( rectColor.PtInRect( point ) )
            {
			    CColorDialog colorDlg( GetColor(nHitItem) , 0, this);
	            if( colorDlg.DoModal() == IDOK)
	            {
                    bHit = TRUE;
                    SetColor( nHitItem, colorDlg.GetColor() );
	            }
            }
        }
    }


    if(bHit)
    {
        
        Invalidate();
	    UpdateWindow();
    
    }
	
	CListBox::OnLButtonDblClk(nFlags, point);
}


const CMyPalette& CColorListBox::GetPalette()
{
    
    if( m_hWnd )
        RetrievePalette();

    return m_palette;
}

void CColorListBox::SetPalette(const CMyPalette& palette)
{
    m_palette = palette;
    if( m_hWnd )
    {
        FreeMemory();
        ResetContent();

        if( m_type == RANGE )
        {
            if( m_palette.GetFormatType() == CMyPalette::RANGE )
            {
                const CColorProfileArray& profileArray = m_palette.GetProfileArray();
                for(int i=0; i<profileArray.GetSize(); i++)
                {
                    int pos = AddString(_T(""));
                    SetItemDataPtr( pos, new CColorProfile(profileArray[i]) );
                }
            }
            else
            {
                const COLORREFArray& colorArray = m_palette.GetColorArray();
                INT_PTR nSize = colorArray.GetSize();
				for (INT_PTR i = 0; i<nSize; i++)
                {
                    int pos = AddString(_T(""));
                    SetItemDataPtr( pos, new CColorProfile(colorArray[i], colorArray[(i+1)%nSize], 1) );
                }
            }

        }
        else
        {
            if( m_palette.GetFormatType() == CMyPalette::RANGE )
            {
                const CColorProfileArray& profileArray = m_palette.GetProfileArray();
                for(int i=0; i<profileArray.GetSize(); i++)
                {
                    int pos = AddString(_T(""));
                    SetItemData( pos, profileArray[i].GetBeginColor() );
                }
            }
            else
            {
                const COLORREFArray& colorArray = m_palette.GetColorArray();
                for(int i=0; i<colorArray.GetSize(); i++)
                {
                    int pos = AddString(_T(""));
                    SetItemData( pos, colorArray[i] );
                }
            }
        }

        Invalidate(NULL);
        UpdateWindow();
    }
}

void CColorListBox::FreeMemory()
{
    if( m_type == RANGE )
    {
        for(int i=0; i<GetCount(); i++)
        {
            CColorProfile* pInfo = (CColorProfile*)GetItemDataPtr(i);
            delete pInfo;
        }
    }
}


void CColorListBox::OnDestroy() 
{
    RetrievePalette();

    FreeMemory();
 
	CListBox::OnDestroy();
}

void CColorListBox::RetrievePalette()
{
    if( m_type == RANGE )
    {
        CColorProfileArray profileArray;
        profileArray.SetSize(GetCount());
        for(int i=0; i<profileArray.GetSize(); i++)
        {
            profileArray[i] = GetColorProfile(i);
        }

        m_palette.SetProfileArray(profileArray);
    }
    else
    {
        COLORREFArray colorArray;
        colorArray.SetSize(GetCount());
        
        for(int i=0; i<colorArray.GetSize(); i++)
        {
            colorArray[i] = (COLORREF)GetItemData(i);
        }

        m_palette.SetColorArray(colorArray);
    }
}
