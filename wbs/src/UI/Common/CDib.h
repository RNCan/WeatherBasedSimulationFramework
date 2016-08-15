// cdib.h declaration for Inside Visual C++ CDib class
#pragma once

#include <atlimage.h>
#include <math.h>
#include "basic/ERMsg.h"
#include "UI/Common/UtilWin.h"


class CDib : public CImage
{
public:
	CDib();
	CDib(const CDib& in);
	CDib(int width, int height, int nBitCount);
	CDib(CSize size, int nBitCount);
	void Create(int nWidth, int nHeight, int nBPP, DWORD dwFlags= 0 );
	void CreateDib(CSize size, int nBitCount);
	void CreateCompatibleDIB( CDC& dc, int width, int height );
	void Copy(const CDib& dib, UINT nBitcount);
	BOOL SetBitmap(CDC& dc);
    //BOOL SetBitmap(CBitmap* pBitmap, CPalette& palette);
	
	CDib& operator = (const CDib& in);
	//a faire : obtenir de l'information sur une image avant de la charger
	//ERMsg GetImageInformation(const CStrinf& filePath, ImageInfo& info);
	ERMsg LoadImage(const CString& filePath);
	ERMsg SaveImage(const CString& filePath, REFGUID guidFileType = GUID_NULL, int bpp=-1 )const;

	inline BOOL Draw( CDC* pDC, int xDest, int yDest, int nDestWidth, 
		int nDestHeight, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight ) const throw();
	inline BOOL Draw( CDC* pDC, const RECT& rectDest, const RECT& rectSrc ) const throw();
	inline BOOL Draw( CDC* pDC, int xDest, int yDest ) const throw();
	inline BOOL Draw( CDC* pDC, const POINT& pointDest ) const throw();
	inline BOOL Draw( CDC* pDC, int xDest, int yDest, int nDestWidth, 
		int nDestHeight ) const throw();
	inline BOOL Draw( CDC* pDC, const RECT& rectDest ) const throw();

	//inline BOOL Draw(CDC* pDC, CPoint destPt, CSize destSize);  // until we implemnt CreateDibSection
	//inline BOOL Draw(CDC* pDC, CRect& src, CRect& dest);
    
	BOOL DrawTransparent (CDC* pDC, int x, int y, COLORREF crColor);
    BOOL DrawTransparent (CDC* pDC, const CRect& origin, const CRect& dest, COLORREF crColor);
	inline int GetColorTableEntries()const;
	inline void Empty();

	BOOL SetPalette( CPalette& palette );
	BOOL GetPalette( CPalette& palette )const;
	void CreatePaletteFromImage( CPalette& palette)const;


	void CopyFromWindow(CWnd* wnd, bool bAllWindowsArea=true);
	void CopyFromScreen(const CRect& rectIn);
    BOOL CopyFromClipboard();
    BOOL CopyToClipboard();
    static HGLOBAL WINAPI CopyHandle (HGLOBAL h);
    static CString GetExportImageFilter();
	static CString GetImportImageFilter();
	

	HBITMAP CreateSection(CDC* pDC = NULL)
	{
		return (HBITMAP)this;
	};

	BOOL MakePalette()
	{
		return true;
	};

	inline void SetPixelIndexed(int x, int y, int index);
	inline int GetPixelIndexed(int x, int y)const;
	
private:

	HRESULT Load( LPCTSTR pszFileName ) throw(){ return CImage::Load( pszFileName );};
	HRESULT Load( IStream* pStream ) throw(){ return CImage::Load( pStream );};
	HRESULT Save( IStream* pStream, REFGUID guidFileType ) const throw(){ return CImage::Save( pStream, guidFileType  );};
	HRESULT Save( LPCTSTR pszFileName, REFGUID guidFileType = GUID_NULL ) const throw(){ return CImage::Save( pszFileName, guidFileType  );};

	inline BYTE Red16(WORD val)const;
    inline BYTE Green16(WORD val)const;
    inline BYTE Blue16(WORD val)const;
	inline WORD RGB16( COLORREF color)const;

	inline int StorageWidth()const;
	HGLOBAL CreateGlobalDIB()const;

	void CreateDefaultPalette();
	void CreateDefaultPalette2();
    void CreateDefaultPalette16();
    void CreateDefaultPalette256();
	
	void GetSelectionIndex(COLORREF crColor, UtilWin::CIntArray& selectionArray);
};

inline BYTE CDib::Blue16(WORD val)const
{
    ASSERT( (val & 0x001F) >= 0 && (val & 0x001F) <= 31);
    return (BYTE)((val & 0x001F)*8.22580645161290+0.5); //255/31
}

inline BYTE CDib::Green16(WORD val)const
{
    ASSERT( ((val & 0x03E0)>>5) >= 0 && ((val & 0x03E0)>>5) <= 31);
    return (BYTE)(((val & 0x03E0)>>5)*8.22580645161290+0.5);
}

inline BYTE CDib::Red16(WORD val)const
{
    ASSERT( ((val & 0x7C00)>>10) >= 0 && ((val & 0x7C00)>>10) <= 31);
    return (BYTE)(((val & 0x7C00)>>10)*8.22580645161290+0.5);
}

inline WORD CDib::RGB16( COLORREF color)const
{
	WORD R = ((BYTE)(GetRValue(color)*0.12156862745098+0.5))&0x1F;	// 31/255
	WORD G = ((BYTE)(GetGValue(color)*0.12156862745098+0.5))&0x1F;	//&1F pour etre sur de n'avoir que 5 bits
	WORD B = ((BYTE)(GetBValue(color)*0.12156862745098+0.5))&0x1F;	//+0.5 pour arrondir
	return ((R<<10) | ( G<<5 ) | B);
}

inline int CDib::StorageWidth()const
{
    return !IsNull()?((((GetWidth() * GetBPP())+ 31) & ~31) >> 3):0;
}


inline BOOL CDib::Draw( CDC* pDC, const RECT& rectDest ) const throw()
{
	HDC hDestDC = pDC->GetSafeHdc();
	::SetStretchBltMode(hDestDC, COLORONCOLOR);
	return( CImage::Draw( hDestDC, rectDest.left, rectDest.top, rectDest.right-
		rectDest.left, rectDest.bottom-rectDest.top, 0, 0, GetWidth(), 
		GetHeight()) );
}

inline BOOL CDib::Draw( CDC* pDC, int xDest, int yDest, int nDestWidth, int nDestHeight ) const throw()
{
	HDC hDestDC = pDC->GetSafeHdc();
	::SetStretchBltMode(hDestDC, COLORONCOLOR);
	return( CImage::Draw( hDestDC, xDest, yDest, nDestWidth, nDestHeight, 0, 0, GetWidth(), GetHeight() ) );
}

inline BOOL CDib::Draw( CDC* pDC, const POINT& pointDest ) const throw()
{
	HDC hDestDC = pDC->GetSafeHdc();
	::SetStretchBltMode(hDestDC, COLORONCOLOR);
	return( CImage::Draw( hDestDC, pointDest.x, pointDest.y, GetWidth(), GetHeight(), 0, 0, GetWidth(), GetHeight() ) );
}

inline BOOL CDib::Draw( CDC* pDC, int xDest, int yDest ) const throw()
{
	HDC hDestDC = pDC->GetSafeHdc();
	::SetStretchBltMode(hDestDC, COLORONCOLOR);
	return( CImage::Draw( hDestDC, xDest, yDest, GetWidth(), GetHeight(), 0, 0, GetWidth(), GetHeight() ) );
}

inline BOOL CDib::Draw( CDC* pDC, const RECT& rectDest, const RECT& rectSrc ) const throw()
{
	HDC hDestDC = pDC->GetSafeHdc();
	::SetStretchBltMode(hDestDC, COLORONCOLOR);
	return( CImage::Draw( hDestDC, rectDest.left, rectDest.top, rectDest.right-
		rectDest.left, rectDest.bottom-rectDest.top, rectSrc.left, rectSrc.top, 
		rectSrc.right-rectSrc.left, rectSrc.bottom-rectSrc.top ) );
}

inline BOOL CDib::Draw( CDC* pDC, int xDest, int yDest, int nDestWidth, 
		int nDestHeight, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight ) const throw()
{
	HDC hDestDC = pDC->GetSafeHdc();
	::SetStretchBltMode(hDestDC, COLORONCOLOR);
	return CImage::Draw( hDestDC, xDest, yDest, nDestWidth, 
		nDestHeight, xSrc, ySrc, nSrcWidth, nSrcHeight );
}

inline int CDib::GetPixelIndexed(int x, int y)const
{
	ASSERT( GetBPP() <= 8);

	int index = -1;
	if( !IsNull() )
    {
	    if( GetBPP() == 1)
	    {
			BYTE ByteMask = 1<<(7-(x%8));
			
			BYTE* pImage = (BYTE*)GetPixelAddress(x,y);
			index = (ByteMask & *pImage);
		}
		else if( GetBPP() == 4)
		{
			BYTE* pImage = (BYTE*)GetPixelAddress(x,y);
            int index = (x%2)?(*pImage&0x0F):((*pImage&0xF0)>>4);
            ASSERT(index >= 0 && index <= 15);
        }
		else
		{
			BYTE* pImage = (BYTE*)GetPixelAddress(x,y);
			int index = *pImage;
			ASSERT(index >= 0 && index <= 255);
        }
	}

	return index;
}

inline void CDib::SetPixelIndexed(int x, int y, int index)
{
	ASSERT( GetBPP() <= 8);

	if( !IsNull() )
    {

		if( GetBPP() == 1)
		{
			BYTE ByteMask = 1<<(7-(x%8));
			
			BYTE* pImage = (BYTE*)GetPixelAddress(x,y);
			(index == 0) ? *pImage &= !ByteMask: *pImage |= ByteMask;

		}
		else if( GetBPP() == 4)
		{
			BYTE mask1 = (x%2)?0xF0: 0x0F;
			BYTE mask2 = (x%2)?(0x00|index): (0x00|(index<<4));

			BYTE* pImage = (BYTE*)GetPixelAddress(x,y);
			*pImage &= mask1;
			*pImage |= mask2;
		}
		else
		{
			BYTE* pImage = (BYTE*)GetPixelAddress(x,y);
			*pImage = index;
		}
	}
}            


inline int CDib::GetColorTableEntries()const
{
	return int(GetBPP()<=8?pow( 2.0, GetBPP()):0);
}

inline void CDib::Empty()
{
	Destroy();
}

