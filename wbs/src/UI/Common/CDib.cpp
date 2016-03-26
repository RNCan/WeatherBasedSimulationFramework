// cdib.cpp
// new version for WIN32
#include "stdafx.h"
#include "cdib.h"

#include "WeatherBasedSimulationString.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CDib::CDib()
{
}

CDib::CDib(const CDib& in)
{
	Copy(in, in.GetBPP());
}

CDib::CDib(int width, int height, int nBitCount)
{
	CreateDib(CSize( width, height), nBitCount);
}

CDib::CDib(CSize size, int nBitCount)
{
	CreateDib(size, nBitCount);
}

void CDib::Create(int nWidth, int nHeight, int nBPP, DWORD dwFlags )
{
	CImage::Create(nWidth, nHeight, nBPP, dwFlags );
	if( IsIndexed() )
	{
		CreateDefaultPalette();
	//	GetPalette(m_palette);
	}

}

void CDib::CreateDib(CSize size, int nBitCount)
{
	Empty();
	Create(size.cx, size.cy, nBitCount);
}

void CDib::CreateCompatibleDIB( CDC& dc, int width, int height )
{
	Empty();
	CreateDib( CSize (width,height), dc.GetDeviceCaps( BITSPIXEL ) );
	if( IsIndexed() )
	{
		if( dc.GetCurrentPalette() )
			SetPalette( *(dc.GetCurrentPalette()) );
		else CreateDefaultPalette();
	}
	
}

CDib& CDib::operator = (const CDib& in)
{
	
	if( &in != this)
	{
		Empty();
		if( !in.IsNull() )
			Copy(in, in.GetBPP());
	}

	return *this;
}

void CDib::Copy(const CDib& dib, UINT nBitCount)
{
	if( &dib != this)
	{
		Empty();
		if( !dib.IsNull() )
		{
		
			int height = dib.GetHeight();
			int width = dib.GetWidth();
			Create(width, height, nBitCount);

			if( nBitCount <= 8)
			{
				CPalette palette;
				dib.CreatePaletteFromImage(palette);
				SetPalette( palette );
				//MakePalette();
			}

			for(int i=0; i<width; i++)
			{
				for(int j=0; j<height; j++)
				{
					SetPixel(i,j, dib.GetPixel(i, j) );
				}
			}
		}
	}
}


/*BOOL CDib::SetBitmap(CBitmap* pBitmap, CPalette& palette)
{
    BITMAP bm;
	pBitmap->GetBitmap(&bm);
    CSize size(bm.bmWidth, bm.bmHeight);
    
    CreateDib(size, bm.bmBitsPixel);
	
	SetPalette(palette);
	CreateSection();

	//SetPalette(*pDC->GetCurrentPalette());
	//CreateSection(pDC);
	
    
    ASSERT(m_lpImage); 
    return GetDIBits(  NULL,           // handle to device context
          (HBITMAP)(*pBitmap),      // handle to bitmap
          0,   // first scan line to set in destination bitmap
          bm.bmHeight,   // number of scan lines to copy
          m_lpImage,    // address of array for bitmap bits
          (BITMAPINFO*)m_lpBMIH, // address of structure with bitmap data
          DIB_RGB_COLORS) == bm.bmHeight;        // RGB or palette index

    //return pBitmap->GetBitmapBits(m_dwSizeImage, m_lpImage) != 0;
}
*/
BOOL CDib::SetBitmap(CDC& dc)
{
	BOOL bRep = false;
	if( dc.GetCurrentBitmap() )
	{
		CDC memDC;
		memDC.CreateCompatibleDC(&dc);

		bRep = true;
		CBitmap& bitmap = *dc.GetCurrentBitmap();
		//bitmap.FromHandle(hBitmap);
		BITMAP bm;
		bitmap.GetBitmap(&bm);
	    
		CSize size(bm.bmWidth, bm.bmHeight);
	    
		
		CreateDib(size, bm.bmBitsPixel);
		if( IsIndexed() )
		{
			if( dc.GetCurrentPalette() )
			{
				SetPalette(*dc.GetCurrentPalette());
				memDC.SelectPalette(dc.GetCurrentPalette(), false);
			}
		}

		

		memDC.SelectObject( (HBITMAP)*this);

		memDC.BitBlt( 0,0, size.cx, size.cy, &dc, 0,0, SRCCOPY); 
		//HBITMAP hBitmap = dc.SelectObject( (HBITMAP)*this );
		
		//bitmap.GetBitmapBits()

		/*for(int i=0; i<size.cx; i++)
		{
			for(int j=0; j<size.cy; j++)
			{
				SetPixel(i,j, dc.GetPixel(i, j) );
			}
		} 
		*/
	}    

	return bRep;
}


CString CDib::GetImportImageFilter()
{
	CString fileFilter;
	CSimpleArray<GUID> aguidFileTypes;
	HRESULT hResult = CImage::GetImporterFilterString(fileFilter, aguidFileTypes);
	if (FAILED(hResult)) 
	{
		CString fmt;
		fmt.Format( _T("GetExporterFilter failed:\n%x - %s"), hResult, _com_error(hResult).ErrorMessage());
		::AfxMessageBox(fmt);
		
		return _T("bimtap (*.bmp)|*.bmp||");
	}

	return fileFilter;
}

CString CDib::GetExportImageFilter()
{
	CString fileFilter;
	CSimpleArray<GUID> aguidFileTypes;
	HRESULT hResult = CImage::GetExporterFilterString(fileFilter, aguidFileTypes);
	if (FAILED(hResult)) 
	{
		CString fmt;
		fmt.Format(_T("GetExporterFilter failed:\n%x - %s"), hResult, _com_error(hResult).ErrorMessage());
		::AfxMessageBox(fmt);
		
		return _T("bimtap (*.bmp)|*.bmp||");
	}

	return fileFilter;
}

ERMsg CDib::LoadImage(const CString& filePath)
{
	ERMsg message;

	Empty(); 
	if( FAILED(Load(filePath)))
	{
		CString error;
		error.FormatMessage(IDS_BSC_READ_ERROR, filePath);
		message.ajoute( UtilWin::ToUTF8(error) );
	}

	//if( IsIndexed() )
	//{
	//	GetPalette(m_palette);
	//}
	return message;
}

ERMsg CDib::SaveImage(const CString& filePath, REFGUID guidFileType, int bpp)const
{
	ASSERT( guidFileType != GUID_NULL || !UtilWin::GetFileExtension(filePath).IsEmpty() );
	ERMsg message;

	if( bpp == -1)
	{
		bpp = GetBPP();
	}
	else if( bpp != GetBPP())
	{
		CDib dib;
		dib.Copy( *this, bpp);
		return dib.SaveImage(filePath, guidFileType);
	}

	

	if( FAILED(Save(filePath, guidFileType)))
	{
		CString error;
		error.FormatMessage(IDS_BSC_UNABLE_OPEN_WRITE, filePath );
		message.ajoute( UtilWin::ToUTF8(error) );
	}

	return message;
}

BOOL CDib::SetPalette( CPalette& palette )
{
	if( IsIndexed() )
	{
		int iColors = 0;
		if (!palette.GetObject(sizeof(iColors), &iColors)) 
		{
			TRACE("Failed to get num palette colors");
			return FALSE;
		}
		
		ASSERT(iColors>= 0 && iColors<=256);
//		iColors = min(iColors, GetColorTableEntries());
		PALETTEENTRY pe[256];
		palette.GetPaletteEntries(0, iColors, pe);

		RGBQUAD quad[256] = {0};
		for (int i = 0; i < iColors; i++) 
		{
			quad[i].rgbRed = pe[i].peRed;    
			quad[i].rgbGreen = pe[i].peGreen;    
			quad[i].rgbBlue = pe[i].peBlue;
		}

		SetColorTable(0, iColors, &(quad[0]));

		//update copy of the palette
		//GetPalette(m_palette);
	}
	

    return TRUE;
}

BOOL CDib::GetPalette(CPalette& palette)const
{
	if( (HPALETTE)palette)
		palette.DeleteObject();

	/*HDC hDC = GetDC();
	CPalette* pPalette = CPalette::FromHandle( (HPALETTE)GetCurrentObject(hDC, OBJ_PAL));
	int iColors = 0;
	if (!pPalette->GetObject(sizeof(int), &iColors)) 
	{
		TRACE("Failed to get num palette colors");
		return FALSE;
	}
	ReleaseDC();
	*/

	int iColors = GetMaxColorTableEntries();
    if(iColors==0)
		return FALSE;

	RGBQUAD quad[256] = {0};
    //LPRGBQUAD pctThis = (LPRGBQUAD) &quad;
	GetColorTable(0, iColors, &quad[0]);

	LPLOGPALETTE pLogPal = (LPLOGPALETTE) new char[2 * sizeof(WORD) +
		iColors * sizeof(PALETTEENTRY)];
	pLogPal->palVersion = 0x300;
	pLogPal->palNumEntries = iColors;
	
	for (int i = 0; i < iColors; i++) 
    {
        pLogPal->palPalEntry[i].peRed = quad[i].rgbRed;
        pLogPal->palPalEntry[i].peGreen = quad[i].rgbGreen;
        pLogPal->palPalEntry[i].peBlue = quad[i].rgbBlue;
    }

	palette.CreatePalette(pLogPal);
	delete pLogPal;

	return true;
}

void CDib::CreatePaletteFromImage( CPalette& palette)const
{
	if( palette.m_hObject )
		palette.DeleteObject();


	ASSERT( IsIndexed() == GetBPP() <= 8);
	if( GetBPP() <= 8)
	{
		GetPalette(palette);
		//CPalette* pal =  CPalette::FromHandle(m_hPalette);
		//PALETTEENTRY colors[256] = {0};
		//pal->GetPaletteEntries(0, 256, colors);
		
		//palette.SetPaletteEntries( 0, 256, colors );
	}
	else
	{
		LPLOGPALETTE pLogPal = (LPLOGPALETTE) new char[2 * sizeof(WORD) +
		256 * sizeof(PALETTEENTRY)];

		pLogPal->palVersion = 0x300;
		pLogPal->palNumEntries = 256;

		palette.CreatePalette(pLogPal);
		delete pLogPal;	
		pLogPal = NULL;

	
		
		CArray<COLORREF, COLORREF> colorArray;

		int height= GetHeight();
		int width = GetWidth();
		for(int i=0; i<width; i++)
		{
			for(int j=0; j<height; j++)
			{
				COLORREF color = GetPixel(i, j);
				if( UtilWin::FindInArray( colorArray, color) == -1)
					colorArray.Add(color);
				if( colorArray.GetSize() == 256)
					break;
			}
			
			if( colorArray.GetSize() == 256)
					break;
		}

		PALETTEENTRY pe[256]={0};
    
		
		for (int i = 0; i < colorArray.GetSize(); i++) 
		{
			COLORREF color = colorArray[i];
			pe[i].peRed = GetRValue(color);
			pe[i].peGreen = GetGValue(color);
			pe[i].peBlue = GetBValue(color);
		}

		palette.SetPaletteEntries(0, 256, pe);
	}
}

void CDib::CreateDefaultPalette()
{
	ASSERT( IsIndexed() );

    switch(GetBPP())
    {
    case 1: CreateDefaultPalette2(); break;
    case 4: CreateDefaultPalette16(); break;
    case 8: CreateDefaultPalette256(); break;
    case 16:
    case 24: 
    case 32: break;
    default: ASSERT(false);
    }
}

void CDib::CreateDefaultPalette2()
{
	RGBQUAD rgbColor[2] = {0};
    
    rgbColor[1].rgbRed = rgbColor[1].rgbGreen = rgbColor[1].rgbBlue = 255;

	SetColorTable(0,2, &(rgbColor[0]));
}

void CDib::CreateDefaultPalette16()
{
    RGBQUAD rgbColor[16] = {0};

    rgbColor[1].rgbRed = 128;
    rgbColor[2].rgbGreen = 128;
    rgbColor[3].rgbRed = rgbColor[3].rgbGreen = 128;
    rgbColor[4].rgbBlue = 128;
    rgbColor[5].rgbRed = rgbColor[5].rgbBlue = 128;
    rgbColor[6].rgbGreen = rgbColor[6].rgbBlue = 128;
    rgbColor[7].rgbRed = rgbColor[7].rgbGreen = rgbColor[7].rgbBlue = 192;
    rgbColor[8].rgbRed = rgbColor[8].rgbGreen = rgbColor[8].rgbBlue = 128;
    rgbColor[9].rgbRed = 255;
    rgbColor[10].rgbGreen = 255;
    rgbColor[11].rgbRed = rgbColor[11].rgbGreen = 255;
    rgbColor[12].rgbBlue = 255;
    rgbColor[13].rgbRed = rgbColor[13].rgbBlue = 255;
    rgbColor[14].rgbGreen = rgbColor[14].rgbBlue = 255;
    rgbColor[15].rgbRed = rgbColor[15].rgbGreen = rgbColor[15].rgbBlue = 255;

	SetColorTable(0,16, &(rgbColor[0]));
}

void CDib::CreateDefaultPalette256()
{
       
    CreateDefaultPalette16();
    
	
    RGBQUAD rgbColor[256-16] = {0};
	LPRGBQUAD pDibQuad = &rgbColor[0];

    // Create a color cube 6x6x6
    for (int r = 1; r <= 6; r++) {
        for (int g = 1; g <= 6; g++) {
            for (int b = 1; b <= 6; b++) {
                pDibQuad->rgbRed =  r * 255 / 7;
                pDibQuad->rgbGreen = g * 255 / 7;
                pDibQuad->rgbBlue = b * 255 / 7;
                pDibQuad++;
            }
        }
    }
    // Create a grey scale
    for (int i = 1; i <= 24; i++) 
    {
        pDibQuad->rgbRed = i * 255 / 25;
        pDibQuad->rgbGreen = i * 255 / 25;
        pDibQuad->rgbBlue = i * 255 / 25;
        pDibQuad++;
    }

	SetColorTable(16,256-16, &(rgbColor[0]));
}

BOOL CDib::DrawTransparent (CDC* pDC, int x, int y, COLORREF crColor)
{
	ASSERT( IsIndexed() );

	CRect rect(y,x,y+GetWidth(), x-GetHeight());

	UtilWin::CIntArray selectionArray;
	GetSelectionIndex(crColor, selectionArray);


	HDC hDestDC = pDC->GetSafeHdc();

	if( selectionArray.GetSize()==1 )
	{
		return CImage::TransparentBlt(pDC->GetSafeHdc(), rect, crColor);
	}
	else if( selectionArray.GetSize()>=1 )
	{
		CDib dib(GetWidth(), GetHeight(), 32);

		/*for(int i=0; i<GetWidth(); i++)
		{
			for(int j=0; j<GetHeight(); j++)
			{
				if( 
				SetPixel(i,j,  );
			}
		}

		CImage::AlphaBlend(dib.GetDC(), rect, crColor);
		*/

		//return 
	}

	return CDib::Draw(pDC, x, y);
}

BOOL CDib::DrawTransparent (CDC* pDC, const CRect& dest, const CRect& src, COLORREF crColor)
{
	ASSERT( CImage::IsTransparencySupported() );
	//int oldMode = pDC->SetStretchBltMode(COLORONCOLOR);
	//return CImage::TransparentBlt(pDC->GetSafeHdc(), dest,src, crColor);
	//return DrawTransparent (pDC, dest.left, dest.top, crColor);
	/*UtilWin::CIntArray selectionArray;
	GetSelectionIndex(crColor, selectionArray);


	HDC hDestDC = pDC->GetSafeHdc();

	if( selectionArray.GetSize()<=1 )
	{
		return CImage::TransparentBlt(pDC->GetSafeHdc(), dest,src, crColor);
	}
	else if( selectionArray.GetSize()>1 )
	{
		CDib dib(GetWidth(), GetHeight(), 8);
		dib.SetPalette(GetPalette);
		
		for(int i=0; i<GetWidth();i++)
			for(int j=0; j<GetHeight(); j++)
			{
				int index = GetPixelIndexed(i,j);
				if( UtilWin::FindInArray( selectionArray, index) == -1)
                    dib.SetPixelIndexed(i,j, index);
				else dib.SetPixelIndexed(i,j, selectionArray[0]);
			}
		
		dib.TransparentBlt(pDC->GetSafeHdc(), dest,src, crColor);
	}
*/
	return CImage::TransparentBlt(pDC->GetSafeHdc(), dest,src, crColor);
}

void CDib::GetSelectionIndex(COLORREF crColor, UtilWin::CIntArray& selectionArray)
{
	selectionArray.RemoveAll();
	
	CPalette palette;
	GetPalette(palette);
	int nbColor = palette.GetEntryCount();
	ASSERT(nbColor >= 0 && nbColor <=256);

	PALETTEENTRY pe[256];
	palette.GetPaletteEntries(0, nbColor, pe);

	for(int i=0; i<nbColor;i++)
	{
		if( RGB( pe[i].peRed, pe[i].peGreen, pe[i].peBlue) == crColor)
			selectionArray.Add(i);
	}
}

/************************************************************************* 
 * 
 * CopyWindowToDIB() 
 * 
 * Parameters: 
 * 
 * HWND hWnd        - specifies the window 
 * 
 * WORD fPrintArea  - specifies the window area to copy into the device- 
 *                    independent bitmap 
 * 
 * Return Value: 
 * 
 * HDIB             - identifies the device-independent bitmap 
 * 
 * Description: 
 * 
 * This function copies the specified part(s) of the window to a device- 
 * independent bitmap. 
 * 
 ************************************************************************/ 
void CDib::CopyFromWindow(CWnd* wnd, bool bFullWnd) 
{
    CDC *dc;
    if(bFullWnd)
    { /* full window */
		dc = new CWindowDC(wnd);
        //HDC hdc = ::GetWindowDC(wnd->m_hWnd);
        //dc -> Attach(hdc);
    } /* full window */
    else
    { /* client area only */
		dc = new CClientDC(wnd);
        //HDC hdc = ::GetDC(wnd->m_hWnd);
        //dc -> Attach(hdc);
    } /* client area only */

    CDC memDC;
    memDC.CreateCompatibleDC(dc);

    CBitmap bm;
    CRect r;
    if(bFullWnd)
		wnd->GetWindowRect(&r);
    else
        wnd->GetClientRect(&r);

    CString s;
    wnd->GetWindowText(s);
    CSize sz(r.Width(), r.Height());
    bm.CreateCompatibleBitmap(dc, sz.cx, sz.cy);
    CBitmap * oldbm = memDC.SelectObject(&bm);
    memDC.BitBlt(0, 0, sz.cx, sz.cy, dc, 0, 0, SRCCOPY);

	SetBitmap(memDC);
    // Enhancement - Bio/Ahmed. Call OpenClipboard() directly instead of getting parent to do it.
    // wnd->GetParent()->OpenClipboard();
//	wnd->OpenClipboard();

  //  ::EmptyClipboard();
    //::SetClipboardData(CF_BITMAP, bm.m_hObject);
    //CloseClipboard();

    memDC.SelectObject(oldbm);
    bm.Detach();  // make sure bitmap not deleted with CBitmap object

	delete dc;
}
/*
{ 
	if (!wnd.GetSafeHwnd()) 
        return; 
 
    if(bAllWindowsArea) 
    { 
        CRect rectWnd; 

        // get the window rectangle 

        wnd.GetWindowRect(&rectWnd); 

        // get the DIB of the window by calling 
        // CopyScreenToDIB and passing it the window rect 
            
        CopyFromScreen(rectWnd); 
    } 
	else
    { 
        CRect rectClient; 
        CPoint pt1, pt2; 

		
        // get the client area dimensions 
        wnd.GetClientRect(&rectClient); 

        // convert client coords to screen coords 
        pt1.x = rectClient.left; 
        pt1.y = rectClient.top; 
        pt2.x = rectClient.right; 
        pt2.y = rectClient.bottom; 
        wnd.ClientToScreen(&pt1); 
        wnd.ClientToScreen(&pt2); 
        rectClient.left = pt1.x; 
        rectClient.top = pt1.y; 
        rectClient.right = pt2.x; 
        rectClient.bottom = pt2.y; 

        // get the DIB of the client area by calling 
        // CopyScreenToDIB and passing it the client rect 

        CopyFromScreen(rectClient); 
    } 
 
} 
*/

/************************************************************************* 
 * 
 * CopyScreenToBitmap() 
 * 
 * Parameter: 
 * 
 * LPRECT lpRect    - specifies the window 
 * 
 * Return Value: 
 * 
 * HDIB             - identifies the device-dependent bitmap 
 * 
 * Description: 
 * 
 * This function copies the specified part of the screen to a device- 
 * dependent bitmap. 
 * 
 * 
 ************************************************************************/ 
void CDib::CopyFromScreen(const CRect& rectIn) 
{ 
    // check for an empty rectangle 
	if (rectIn.IsRectEmpty()) 
	{
		Empty();
		return;
	}

	CDC ScrDC;
	CDC MemDC;
	// create a DC for the screen and create 
    // a memory DC compatible to screen DC 
    ScrDC.CreateDCW(L"DISPLAY", NULL, NULL, NULL); 
    MemDC.CreateCompatibleDC(&ScrDC); 
 
	

    // get screen resolution 
    int xScrn = ScrDC.GetDeviceCaps(HORZRES); 
    int yScrn = ScrDC.GetDeviceCaps(VERTRES); 

	CRect rect(rectIn);
	rect.NormalizeRect();

	//make sure bitmap rectangle is visible 
	if (rect.left < 0) 
        rect.left = 0; 
    if (rect.top < 0) 
        rect.top = 0; 
    if (rect.right > xScrn) 
        rect.right = xScrn; 
    if (rect.bottom> yScrn) 
        rect.bottom = yScrn; 
 
    // create a bitmap compatible with the screen DC 

	
	CreateDib( CSize(rect.Width(), rect.Height()), 32);
	CBitmap* pBitmap = CBitmap::FromHandle( (HBITMAP)(*this));
	//pBitmap->GetBitmap(&bmp);

	//ASSERT(false);
	//HBITMAP hBitmap = NULL;//C reateCompatibleDIB( ScrDC, rect.Width(), rect.Height());
 
    // select new bitmap into memory DC 
	CBitmap* pOldBitmap = MemDC.SelectObject(pBitmap); 
	//HBITMAP hOldBitmap = (HBITMAP)MemDC.SelectObject((hBitmap); 
 
	
    // bitblt screen DC to memory DC 
	MemDC.BitBlt(0, 0, rect.Width(), rect.Height(), &ScrDC, rect.left, rect.top, SRCCOPY); 

	//CreatePaletteFromImage(m_palette);
 
    // select old bitmap back into memory DC and get handle to 
    // bitmap of the screen 
    //hBitmap = (HBITMAP)MemDC.SelectObject(hOldBitmap); 
	MemDC.SelectObject(pOldBitmap); 
	
 
    // clean up 
    ScrDC.DeleteDC(); 
    MemDC.DeleteDC(); 
} 

//****************************************************************************
// Sommaire:    Copy l'image du contrôle dans le clipbord.
//
// Description: 
//
// Entrée:      
//
// Sortie:      Bool: TRUE si succès, FALSE autrement.
//
// Note:        
//****************************************************************************
BOOL CDib::CopyFromClipboard() 
{
	BOOL bRep = FALSE;
    Empty();
    
    if ( ::OpenClipboard(NULL) )
    { 
        CWaitCursor wait;
        HANDLE h;
        if ( (h = GetClipboardData (CF_DIB)) != NULL) 
        { 
            /* Delete current DIB and get CF_DIB and 
             * CF_PALETTE format data from the clipboard 
             */ 
//            HANDLE hpal = GetClipboardData (CF_PALETTE); 

            HGLOBAL hDib = CopyHandle (h); 
            if (hDib) 
            {
				Attach( (HBITMAP)hDib );//??est-ce que ça fonctionne
                //bRep = true;
                //LPVOID pData = ::GlobalLock(hDib); 
                //AttachMemory(pData, TRUE, hDib);
            }
        } 
        ::CloseClipboard(); 
    } 


	return bRep;
}


//****************************************************************************
// Sommaire:    Copy l'image du contrôle dans le clipbord.
//
// Description: 
//
// Entrée:      
//
// Sortie:      Bool: TRUE si succès, FALSE autrement.
//
// Note:        
//****************************************************************************
BOOL CDib::CopyToClipboard() 
{
	BOOL bRep = FALSE;

	
	if ( !IsNull() ) 
	{
		// Clean clipboard of contents, and copy the DIB.

        if ( ::OpenClipboard(NULL) )
		{
			CWaitCursor wait;
            ::EmptyClipboard();

            //DWORD dwLenHead = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;
            
        	//HGLOBAL hCopy = ::GlobalAlloc(GHND, dwLenHead + m_dwSizeImage); 
            //LPBYTE pData = (LPBYTE)::GlobalLock(hCopy); 
      //      memcpy( pData, m_lpBMIH, dwLenHead);
        //    pData += dwLenHead;
          //  memcpy( pData, m_lpImage, m_dwSizeImage);
            //::GlobalUnlock(hCopy); 

			HGLOBAL hCopy = CreateGlobalDIB();//CopyHandle( (HBITMAP)(*this));

            ::SetClipboardData (CF_DIB, hCopy );
            ::CloseClipboard();
			bRep = TRUE;

		}
	}


	return bRep;
}

HGLOBAL WINAPI CDib::CopyHandle (HGLOBAL h)
{
	if (h == NULL)
		return NULL;

	SIZE_T dwLen = ::GlobalSize((HGLOBAL) h);
	HGLOBAL hCopy = ::GlobalAlloc(GHND, dwLen);

	if (hCopy != NULL)
	{
		void* lpCopy = ::GlobalLock((HGLOBAL) hCopy);
		void* lp     = ::GlobalLock((HGLOBAL) h);
		memcpy(lpCopy, lp, dwLen);
		::GlobalUnlock(hCopy);
		::GlobalUnlock(h);
	}

	return hCopy;
}

HGLOBAL CDib::CreateGlobalDIB()const
{
	int nColorTableEntries = this->GetColorTableEntries();
	DWORD dwLenHead = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * nColorTableEntries;
	int dwSizeImage = ((GetWidth() * GetBPP()+31) & ~31) /8 * GetHeight(); 
    
    HGLOBAL hCopy = ::GlobalAlloc(GHND, dwLenHead + dwSizeImage); 
    LPBYTE pData = (LPBYTE)::GlobalLock(hCopy); 

    BITMAP bmp; 
    PBITMAPINFO pbmi = (PBITMAPINFO )pData; 
    WORD    cClrBits; 

    // Retrieve the bitmap color format, width, and height. 
//    if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) 
  //      return false;
	CBitmap* pBitmap = CBitmap::FromHandle( (HBITMAP)(*this));
	pBitmap->GetBitmap(&bmp);

    // Convert the color format to a count of bits. 
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
    if (cClrBits == 1) 
        cClrBits = 1; 
    else if (cClrBits <= 4) 
        cClrBits = 4; 
    else if (cClrBits <= 8) 
        cClrBits = 8; 
    else if (cClrBits <= 16) 
        cClrBits = 16; 
    else if (cClrBits <= 24) 
        cClrBits = 24; 
    else cClrBits = 32; 

	ASSERT( cClrBits == GetBPP() );
    // Allocate memory for the BITMAPINFO structure. (This structure 
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
    // data structures.) 

     //if (cClrBits != 24) 
	/*if (cClrBits != 24) 
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER) + 
                    sizeof(RGBQUAD) * (1<< cClrBits)); 

     // There is no RGBQUAD array for the 24-bit-per-pixel format. 

     else 
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
                    sizeof(BITMAPINFOHEADER)); 
*/
    // Initialize the fields in the BITMAPINFO structure. 

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
    pbmi->bmiHeader.biWidth = bmp.bmWidth; 
    pbmi->bmiHeader.biHeight = bmp.bmHeight; 
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
    if (cClrBits < 24) 
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

    // If the bitmap is not compressed, set the BI_RGB flag. 
    pbmi->bmiHeader.biCompression = BI_RGB; 

    // Compute the number of bytes in the array of color 
    // indices and store the result in biSizeImage. 
    // For Windows NT, the width must be DWORD aligned unless 
    // the bitmap is RLE compressed. This example shows this. 
    // For Windows 95/98/Me, the width must be WORD aligned unless the 
    // bitmap is RLE compressed.
    pbmi->bmiHeader.biSizeImage = dwSizeImage;//((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
                                  //* pbmi->bmiHeader.biHeight; 
    // Set biClrImportant to 0, indicating that all of the 
    // device colors are important. 
     pbmi->bmiHeader.biClrImportant = 0; 
     
	 if( IsIndexed() )
	 {
		 GetColorTable(0, nColorTableEntries, pbmi->bmiColors);
	 }

	 pData += dwLenHead;
	 if( GetPitch() > 0 )
		memcpy( pData, GetPixelAddress(0,0), dwSizeImage);
	 else memcpy( pData, GetPixelAddress(0,GetHeight()-1), dwSizeImage);
     ::GlobalUnlock(hCopy); 


	 return hCopy;
 } 


