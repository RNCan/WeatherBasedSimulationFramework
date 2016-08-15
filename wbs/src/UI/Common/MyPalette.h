//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#pragma warning( disable : 4786)
#include <fstream>
#include "basic/ERMsg.h"


class CStepColor
{
public:
    inline CStepColor();
    inline void StepColor( int nbStep, COLORREF& color);
    inline void Init(double stepRed, double stepGreen, double stepBlue);
private:
    double m_stepRed;
    double m_stepGreen;
    double m_stepBlue;
};

inline CStepColor::CStepColor()
{
    m_stepRed = m_stepGreen = m_stepBlue = 0;
}
inline void CStepColor::StepColor( int nbStep, COLORREF& color)
{
    color = RGB( int(GetRValue(color)+ m_stepRed*nbStep) , int(GetGValue(color)+ m_stepGreen*nbStep), int(GetBValue(color)+ m_stepBlue*nbStep) );
}

inline void CStepColor::Init(double stepRed, double stepGreen, double stepBlue)
{
    m_stepRed = stepRed;
    m_stepGreen = stepGreen;
    m_stepBlue = stepBlue;
}

class CColorProfile
{
public:
    inline CColorProfile();
    inline CColorProfile(const CColorProfile& profile);
    inline CColorProfile(COLORREF beginColor, COLORREF endColor, int pound);

    inline bool operator == (const CColorProfile& palette)const;
    inline bool operator != (const CColorProfile& palette)const;


    inline COLORREF GetBeginColor()const;
    inline void SetBeginColor(COLORREF color);
    inline COLORREF GetEndColor()const;
    inline void SetEndColor(COLORREF color);
    inline int GetPound()const;
    inline void SetPound(int pound);

    inline void GetStepColor(double NbProfileColor, CStepColor& stepColor )const;

private:

    COLORREF m_beginColor;
    COLORREF m_endColor;
    int m_pound;
};

inline bool CColorProfile::operator == (const CColorProfile& profile)const
{
    return (m_beginColor == profile.m_beginColor && 
        m_endColor == profile.m_endColor && 
        m_pound == profile.m_pound);
    
}

inline bool CColorProfile::operator != (const CColorProfile& profile)const
{
    return !((*this) == profile);
}

inline CColorProfile::CColorProfile(const CColorProfile& profile)
{
    m_beginColor = profile.m_beginColor;
    m_endColor = profile.m_endColor;
    m_pound = profile.m_pound;
}

inline CColorProfile::CColorProfile(COLORREF beginColor, COLORREF endColor, int pound)
{
    m_beginColor = beginColor;
    m_endColor = endColor;
    m_pound = pound;
}

inline CColorProfile::CColorProfile()
{
    m_beginColor = RGB(0,0,0);
    m_endColor = RGB(255,255,255);
    m_pound = 1;
}

inline COLORREF CColorProfile::GetBeginColor()const
{
    return m_beginColor;
}
inline void CColorProfile::SetBeginColor(COLORREF beginColor)
{
    m_beginColor = beginColor;
}
inline COLORREF CColorProfile::GetEndColor()const
{
    return m_endColor;
}

inline void CColorProfile::SetEndColor(COLORREF endColor)
{
    m_endColor = endColor;
}

inline int CColorProfile::GetPound()const
{
    return m_pound;
}
inline void CColorProfile::SetPound(int pound)
{
    ASSERT( pound >= 0);
    m_pound = pound;
}

 
inline void CColorProfile::GetStepColor(double NbProfileColor, CStepColor& stepColor )const
{
    ASSERT( NbProfileColor != 0 );
    //double step = (double)m_pound/totalPound;
    
    stepColor.Init( ( (double)GetRValue(m_endColor) - GetRValue(m_beginColor))/NbProfileColor,
        ( (double)GetGValue(m_endColor) - GetGValue(m_beginColor))/NbProfileColor,
        ( (double)GetBValue(m_endColor) - GetBValue(m_beginColor))/NbProfileColor );
}

typedef CArray< CColorProfile, CColorProfile& > CColorProfileArray;
typedef CArray <COLORREF, const COLORREF &> COLORREFArray;
 
class CMyPalette : public CPalette  
{
public:
	CMyPalette();
	virtual ~CMyPalette();

    CMyPalette& operator=(const CMyPalette& palette);
    bool operator == (const CMyPalette& palette)const;
    bool operator != (const CMyPalette& palette)const;

    enum TFormatType {RANGE, FIXE };
    
	static const TCHAR STRHEAD[];
    static const short CURRENT_VERSION;

    int GetPaletteIndex(COLORREF color);

    ERMsg Load(const CString& filePath);
    ERMsg Save(const CString& filePath);
    friend std::istream& operator >> (std::istream& io, CMyPalette& palette);
    friend std::ostream& operator << (std::ostream& io, CMyPalette& palette);

    inline const CString& GetFilePath()const;
    inline void SetFilePath(const CString& filePath);


    inline int GetFormatType()const;
    inline void SetFormatType(int formatType);
    
    //for range palette
    inline const CColorProfileArray& GetProfileArray()const;
    inline void SetProfileArray(const CColorProfileArray& profile);
    
    //for fixe palette
    inline const COLORREFArray& GetColorArray()const;
    inline void SetColorArray(const COLORREFArray& colorArray);
    
    inline COLORREF GetColor(int index)const;
    
    BOOL CreatePalette( LPLOGPALETTE lpLogPalette );
    BOOL CreatePalette( int nbColor );
    BOOL CreatePalette( int nbColor, const COLORREFArray& colorArray);
    BOOL AddExtraColor(COLORREF color);
    BOOL CreateDefaultPalette(int nbColor);
    static void CreateColorArray(int nbColor, const CColorProfileArray& colorProfileArray, COLORREFArray& colorArray);

	BOOL ResizePalette(UINT nbColor);
	int GetEntryCount(){return m_hObject!=NULL?CPalette::GetEntryCount():0;};

private:

    
    static inline int GetTotalPound(const CColorProfileArray& colorProfileArray);

    int m_formatType;

	
    COLORREFArray m_colorArray;
    CColorProfileArray m_colorProfileArray;


    int m_version;
    CString m_filePath;
};

inline int CMyPalette::GetFormatType()const
{
    return m_formatType;
}

inline void CMyPalette::SetFormatType(int formatType)
{
    m_formatType = formatType;
}

inline const CString& CMyPalette::GetFilePath()const
{
    return m_filePath;
}

inline void CMyPalette::SetFilePath(const CString& filePath)
{
    m_filePath = filePath;
}

inline const CColorProfileArray& CMyPalette::GetProfileArray()const
{
    ASSERT( m_formatType == RANGE );
    return m_colorProfileArray;
}

inline void CMyPalette::SetProfileArray(const CColorProfileArray& profile)
{
    m_colorProfileArray.Copy(profile);
    m_formatType = RANGE;
}

inline const COLORREFArray& CMyPalette::GetColorArray()const
{
    ASSERT( m_formatType == FIXE );
    return m_colorArray;
}

inline void CMyPalette::SetColorArray(const COLORREFArray& colorArray)
{
    m_colorArray.Copy(colorArray);
    m_formatType = FIXE;
}

inline COLORREF CMyPalette::GetColor(int index)const
{
	ASSERT( index >= 0 && index < m_colorArray.GetSize() );
    COLORREF color = RGB(0,0,0);
    if( index >= 0 && index < m_colorArray.GetSize() )
        color = m_colorArray[index];

    return color ;
}
