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
#include "StaticBitmap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CStaticBitmap, CStatic)
/////////////////////////////////////////////////////////////////////////////
// CStaticBitmap

CStaticBitmap::CStaticBitmap():
m_transparentColor( RGB(192,192,192) )
{
}

CStaticBitmap::~CStaticBitmap()
{
}


BEGIN_MESSAGE_MAP(CStaticBitmap, CStatic)
	//{{AFX_MSG_MAP(CStaticBitmap)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticBitmap message handlers

void CStaticBitmap::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	HBITMAP hBitmap = GetBitmap();
    ASSERT( hBitmap );
	// Do not call CStatic::OnPaint() for painting messages
    DrawTransparent (&dc, hBitmap, m_transparentColor);
}

void CStaticBitmap::PreSubclassWindow() 
{
	CStatic::PreSubclassWindow();

    long style = GetWindowLong( m_hWnd, GWL_STYLE);
    long CtrlID = GetWindowLong( m_hWnd, GWL_ID);

    //Le ctrl doit être de type ICON
    ASSERT( style & SS_BITMAP );

}

void CStaticBitmap::DrawTransparent (CDC* pDC, HBITMAP hBitmap, COLORREF crColor)
{
    if( hBitmap == NULL)
        return;

    BITMAP bm;
    GetObject (hBitmap, sizeof (BITMAP), &bm);
    CPoint size (bm.bmWidth, bm.bmHeight);
    pDC->DPtoLP (&size);

    CPoint org (0, 0);
    pDC->DPtoLP (&org);

    // Create a memory DC (dcImage) and select the bitmap into it
    CDC dcImage;
    dcImage.CreateCompatibleDC (pDC);
    HBITMAP hOldBitmapImage = (HBITMAP)::SelectObject (dcImage.m_hDC, hBitmap);
    dcImage.SetMapMode (pDC->GetMapMode ());

    // Create a second memory DC (dcAnd) and in it create an AND mask
    CDC dcAnd;
    dcAnd.CreateCompatibleDC (pDC);
    dcAnd.SetMapMode (pDC->GetMapMode ());

    CBitmap bitmapAnd;
    bitmapAnd.CreateBitmap (bm.bmWidth, bm.bmHeight, 1, 1, NULL);
    CBitmap* pOldBitmapAnd = dcAnd.SelectObject (&bitmapAnd);

    dcImage.SetBkColor (crColor);
    dcAnd.BitBlt (org.x, org.y, size.x, size.y, &dcImage, org.x, org.y,
        SRCCOPY);

    // Create a third memory DC (dcXor) and in it create an XOR mask
    CDC dcXor;
    dcXor.CreateCompatibleDC (pDC);
    dcXor.SetMapMode (pDC->GetMapMode ());

    CBitmap bitmapXor;
    bitmapXor.CreateCompatibleBitmap (&dcImage, bm.bmWidth, bm.bmHeight);
    CBitmap* pOldBitmapXor = dcXor.SelectObject (&bitmapXor);

    dcXor.BitBlt (org.x, org.y, size.x, size.y, &dcImage, org.x, org.y,
        SRCCOPY);

    dcXor.BitBlt (org.x, org.y, size.x, size.y, &dcAnd, org.x, org.y,
        0x220326);

    // Copy the pixels in the destination rectangle to a temporary
    // memory DC (dcTemp)
    CDC dcTemp;
    dcTemp.CreateCompatibleDC (pDC);
    dcTemp.SetMapMode (pDC->GetMapMode ());

    CBitmap bitmapTemp;
    bitmapTemp.CreateCompatibleBitmap (&dcImage, bm.bmWidth, bm.bmHeight);
    CBitmap* pOldBitmapTemp = dcTemp.SelectObject (&bitmapTemp);

    dcTemp.BitBlt (org.x, org.y, size.x, size.y, pDC, 0, 0, SRCCOPY);

    // Generate the final image by applying the AND and XOR masks to
    // the image in the temporary memory DC
    dcTemp.BitBlt (org.x, org.y, size.x, size.y, &dcAnd, org.x, org.y,
        SRCAND);

    dcTemp.BitBlt (org.x, org.y, size.x, size.y, &dcXor, org.x, org.y,
        SRCINVERT);

    // Blit the resulting image to the screen
    pDC->BitBlt (0, 0, size.x, size.y, &dcTemp, org.x, org.y, SRCCOPY);

    // Restore the default bitmaps
    dcTemp.SelectObject (pOldBitmapTemp);
    dcXor.SelectObject (pOldBitmapXor);
    dcAnd.SelectObject (pOldBitmapAnd);
    ::SelectObject(dcImage.m_hDC, hOldBitmapImage);
}
