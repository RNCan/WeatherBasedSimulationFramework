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
#include "MyPalette.h"
#include "UI/Common/UtilWin.h"
#include "WeatherBasedSimulationString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


const TCHAR CMyPalette::STRHEAD[] = _T("[CMyPalette]");
const short CMyPalette::CURRENT_VERSION = 1;

CMyPalette::CMyPalette():
m_version( CURRENT_VERSION ),
m_formatType(RANGE)
{}

CMyPalette::~CMyPalette()
{}

CMyPalette& CMyPalette::operator=(const CMyPalette& palette)
{
    if( this != &palette )
    {
        m_formatType = palette.m_formatType ;
        m_colorArray.Copy(palette.m_colorArray);
        m_colorProfileArray.Copy(palette.m_colorProfileArray);

        m_version = palette.m_version ;
        m_filePath = palette.m_filePath ;
    }

    return *this;
}

bool CMyPalette::operator == (const CMyPalette& palette)const
{
    bool bEqual= true;

    if( m_formatType != palette.m_formatType) bEqual= false;
    if( !UtilWin::IsExactlyEgal( m_colorArray, palette.m_colorArray) )bEqual= false;
    if( !UtilWin::IsExactlyEgal( m_colorProfileArray, palette.m_colorProfileArray) )bEqual= false;
    if( m_version != palette.m_version) bEqual= false;
    if( m_filePath != palette.m_filePath) bEqual= false;

    return bEqual;
}

bool CMyPalette::operator != (const CMyPalette& palette)const
{
    return !((*this) == palette);
}



ERMsg CMyPalette::Load(const CString& filePath)
{
    ERMsg message;
    std::ifstream io( (LPCTSTR)filePath);
    if( io.is_open() )
    {
        m_filePath = filePath;
        io >> (*this);
        io.close();
    }
    else
    {
        CString error;
        error.FormatMessage(IDS_BSC_UNABLE_OPEN_READ, (LPCTSTR)filePath);
        message.ajoute(UtilWin::ToUTF8(error));
    }
    
    return message;
}

ERMsg CMyPalette::Save(const CString& filePath)
{
    ERMsg message;

    std::ofstream io( (LPCTSTR)filePath );
    if( io.is_open() )
    {
        m_filePath = filePath;
        io << (*this);
        io.close();
    }
    else
    {
        CString error;
        error.FormatMessage(IDS_BSC_UNABLE_OPEN_WRITE, filePath);
		message.ajoute(UtilWin::ToUTF8(error));
    }

    return message;
}

std::istream& operator >> (std::istream& io, CMyPalette& palette)
{
//    ASSERT((HPALETTE) palette == NULL);
    if((HPALETTE) palette != NULL)
         palette.Detach();
        
    int bMicrosoftPal = false;
        
    if( io )
    {
		char buffer[MAX_PATH] = { 0 };
        int nSize = 0;
        //int nFormat; 
        
		io >> buffer;
		CString name = UtilWin::Convert( buffer );
		if (name.Find(CMyPalette::STRHEAD) != -1)
        {
            io >> palette.m_version; ASSERT(palette.m_version == CMyPalette::CURRENT_VERSION);
            io >> palette.m_formatType; 
            ASSERT(palette.m_formatType == CMyPalette::RANGE || palette.m_formatType == CMyPalette::FIXE);
            io >> nSize;
        }
        else if( name.Find( _T("JASC-PAL") ) != -1 )
        {
            palette.m_version = 1;//ok
            io >> palette.m_formatType; 
            ASSERT(palette.m_formatType == 100);
            palette.m_formatType = CMyPalette::FIXE;
            io >> nSize;
        }
        else if( name.Find( _T("RIFF") ) != -1 )
        {
            io.seekg(0);
            io.ignore(16);
            
            io.read( (char*)&nSize, 4);
            io.ignore(4);
            nSize = nSize/4 - 2;
            palette.m_formatType = CMyPalette::FIXE;
            bMicrosoftPal = true;
        }
        else
        {
            ASSERT(false );
        }

        
        
        
        //LOGPALETTE* pLP = (PLOGPALETTE) new BYTE[sizeof(LOGPALETTE) +  nSize*sizeof(PALETTEENTRY)];
        //pLP->palVersion = 0x300;
        //pLP->palNumEntries = nSize;
        palette.m_colorArray.SetSize(nSize);
        
        if( palette.m_formatType == CMyPalette::RANGE )
            palette.m_colorProfileArray.SetSize(nSize);

        for(int i=0; i<nSize; i++)
        {
            
            if( bMicrosoftPal )
            {
                ASSERT( palette.m_formatType == CMyPalette::FIXE );

                //COLORREF color;
                BYTE tmp;
                BYTE R,G,B;
       	        io >> R; //pLP->palPalEntry[i].peRed = (BYTE)tmp;
		        io >> G; //pLP->palPalEntry[i].peGreen = (BYTE)tmp;
		        io >> B; //pLP->palPalEntry[i].peBlue = (BYTE)tmp;
                //pLP->palPalEntry[i].peFlags = 0;
                io >> tmp;

                palette.m_colorArray[i] = RGB(R,G,B);
            }
            else
            {
                if( palette.m_formatType == CMyPalette::RANGE )
                {   
                    
                    short R,G,B;
       	            io >> R; //pLP->palPalEntry[i].peRed = (BYTE)tmp;
		            io >> G; //pLP->palPalEntry[i].peGreen = (BYTE)tmp;
		            io >> B; //pLP->palPalEntry[i].peBlue = (BYTE)tmp;
                    palette.m_colorProfileArray[i].SetBeginColor( RGB(R,G,B) );
       	            io >> R; //pLP->palPalEntry[i].peRed = (BYTE)tmp;
		            io >> G; //pLP->palPalEntry[i].peGreen = (BYTE)tmp;
		            io >> B; //pLP->palPalEntry[i].peBlue = (BYTE)tmp;
                    palette.m_colorProfileArray[i].SetEndColor( RGB(R,G,B) );
                    short pound;
                    io >> pound;
                    if( pound >= 0 )
                        palette.m_colorProfileArray[i].SetPound( pound );
                    else palette.m_colorProfileArray[i].SetPound( 1 );

                }
                else
                {
                    ASSERT( palette.m_formatType == CMyPalette::FIXE );
                    short R,G,B;
       	            io >> R; //pLP->palPalEntry[i].peRed = (BYTE)tmp;
		            io >> G; //pLP->palPalEntry[i].peGreen = (BYTE)tmp;
		            io >> B; //pLP->palPalEntry[i].peBlue = (BYTE)tmp;
                    //pLP->palPalEntry[i].peFlags = 0;
                    palette.m_colorArray.SetAt(i, RGB(R,G,B) );
                }
            }
        }

        //palette.CreateColorArray(nSize);
        //palette.CreatePalette(nSize);
        //VERIFY( palette.CreatePalette(pLP) );
        
        //delete [] pLP;
    }

    return io;
}

std::ostream& operator << (std::ostream& io, CMyPalette& palette)
{
    if( io )
    {
        INT_PTR nSize = palette.m_colorArray.GetSize();
      
        if( palette.m_formatType == CMyPalette::RANGE )
            nSize = palette.m_colorProfileArray.GetSize();

        
        
        
        io << CMyPalette::STRHEAD << ' '; 
        io << CMyPalette::CURRENT_VERSION << std::endl; 
        io << palette.m_formatType << std::endl;
        io << nSize;
        
        //PALETTEENTRY* pPE = (LPPALETTEENTRY) new BYTE[nSize*sizeof(PALETTEENTRY)];

        //palette.GetPaletteEntries( 0, nSize, pPE );
        
        for(int i=0; i<nSize; i++)
        {
            if( palette.m_formatType == CMyPalette::RANGE ) 
            {
                COLORREF color = palette.m_colorProfileArray[i].GetBeginColor();
                io << std::endl;
       	        io << (short)GetRValue(color) << "\t"; //pPE[i].peRed;
		        io << (short)GetGValue(color) << "\t"; //pPE[i].peGreen;
		        io << (short)GetBValue(color) << "\t"; //pPE[i].peBlue;
                color = palette.m_colorProfileArray[i].GetEndColor();
                io << (short)GetRValue(color) << "\t"; //pPE[i].peRed;
		        io << (short)GetGValue(color) << "\t"; //pPE[i].peGreen;
		        io << (short)GetBValue(color) << "\t"; //pPE[i].peBlue;
                io << palette.m_colorProfileArray[i].GetPound();
            }
            else
            {
                COLORREF color = palette.m_colorArray[i];
                io << std::endl;
       	        io << (short)GetRValue(color) << "\t"; //pPE[i].peRed;
		        io << (short)GetGValue(color) << "\t"; //pPE[i].peGreen;
		        io << (short)GetBValue(color) ; //pPE[i].peBlue;
            }
        }

        //delete [] pPE;
    }
    
    return io;
}

int CMyPalette::GetPaletteIndex(COLORREF color)
{
    int nRep = -1;
    
	if( color & 0xff000000)
	{
		nRep = color & 0x00ffffff;
	}
	else
	{
        int nSize = GetEntryCount();
	    ASSERT(false);       
        
        PALETTEENTRY* pPE = (LPPALETTEENTRY) new BYTE[nSize*sizeof(PALETTEENTRY)];

        GetPaletteEntries( 0, nSize, pPE );
		for(int i=0; i<nSize; i++)
		{
        
			COLORREF tmp = RGB(pPE[i].peRed, pPE[i].peGreen, pPE[i].peBlue);
			
			if( tmp == color)
			{
				nRep = i;
				break;
			}
        
       		
		}

        delete [] pPE;
	}
    

    return nRep;
}

BOOL CMyPalette::CreatePalette( int nbColor )
{
    if( m_formatType == RANGE )
        CreateColorArray(nbColor, m_colorProfileArray, m_colorArray);

    return CreatePalette( nbColor, m_colorArray);
}

BOOL CMyPalette::CreatePalette( int nbColor, const COLORREFArray& colorArray)
{
    if( m_hObject != NULL)
        Detach();

	//quoi faire???
	if( nbColor > 250 ) 
		nbColor = 250;


    LOGPALETTE* pLP = (PLOGPALETTE) new BYTE[sizeof(LOGPALETTE) +  nbColor*sizeof(PALETTEENTRY)];
    pLP->palVersion = 0x300;
    pLP->palNumEntries = nbColor;


    ASSERT(nbColor >= 0 && nbColor < 256);
    

	INT_PTR nSize = colorArray.GetSize();
    for(int i=0; i<nbColor && i<nSize; i++)
    {
        pLP->palPalEntry[i].peRed = GetRValue(colorArray[i]);
        pLP->palPalEntry[i].peGreen = GetGValue( colorArray[i]);
        pLP->palPalEntry[i].peBlue = GetBValue(colorArray[i]);
        pLP->palPalEntry[i].peFlags = 0;
    }

    
    BOOL bRep = CreatePalette(pLP);
    
    delete [] pLP;

    return bRep;
}

BOOL CMyPalette::AddExtraColor(COLORREF color)
{
    ASSERT( GetEntryCount( ) < 255 );

    BOOL bRep = FALSE;

    int count = GetEntryCount();
    if( ResizePalette( count+ 1 ) )
    {
        PALETTEENTRY entry;
        entry.peRed = GetRValue( color );
        entry.peGreen = GetGValue( color );
        entry.peBlue = GetBValue( color );
        entry.peFlags = 0;

        if( SetPaletteEntries( count, 1, &entry) )
            bRep = TRUE;
    }

    return bRep;
}

void CMyPalette::CreateColorArray(int nbColor, const CColorProfileArray& colorProfileArray, COLORREFArray& colorArray)
{
    //ASSERT( m_formatType == CMyPalette::RANGE );
    
    colorArray.SetSize(nbColor);
    int poidtotal =  GetTotalPound(colorProfileArray);
    int currentPos = 0;
    double currentNbColor = 0;
    
    for(int i=0; i<colorProfileArray.GetSize(); i++)
    {
        const CColorProfile& profile = colorProfileArray[i];



        double NbProfileColor = (double)nbColor * profile.GetPound() / poidtotal;
        currentNbColor += NbProfileColor + 0.00001;
        if( currentNbColor > nbColor)
            currentNbColor = nbColor;

        CStepColor step;
        profile.GetStepColor(NbProfileColor, step);

        //int ;
	    for(int j=currentPos, profileStep = 0; j<(int)currentNbColor; j++, profileStep++)
        {
            COLORREF currentColor = profile.GetBeginColor();
            step.StepColor(profileStep, currentColor);
		    colorArray.SetAt(j, currentColor );
            currentPos++;
        }
    }
}

BOOL CMyPalette::ResizePalette(UINT nbColor)
{
	if( m_hObject == NULL)
	{
		CreateDefaultPalette(nbColor);
	}
	
	return CPalette::ResizePalette(nbColor);
}

BOOL CMyPalette::CreateDefaultPalette(int nbColor)
{
	CColorProfileArray colorProfileArray;
	colorProfileArray.SetSize(3);
	
	colorProfileArray[0].SetBeginColor( RGB(0, 5, 250) );
	colorProfileArray[0].SetEndColor( RGB(169, 29, 55) );
	colorProfileArray[1].SetBeginColor( RGB(169, 29, 55) );
	colorProfileArray[1].SetEndColor( RGB(13, 161, 55) );
	colorProfileArray[2].SetBeginColor( RGB(13, 161, 55) );
	colorProfileArray[2].SetEndColor( RGB(255, 255, 255) );

	
	CreateColorArray(nbColor, colorProfileArray, m_colorArray);


    /*m_formatType = FIXE;
    m_colorArray.SetSize(nbColor);

	int peRed = 0;
    int peGreen = 5;
    int peBlue = 250;
	for(int i=0; i<nbColor; i++)
    {
		m_colorArray.SetAt(i, RGB(peRed, peGreen, peBlue) );

	  if(i < nbColor/3)
        {
            peRed += 190/ (int)(nbColor/3);
            peGreen += 15/ (int)(nbColor/3);
            peBlue += -195/ (int)(nbColor/3);
        }
        else if( i < nbColor*2/3)
		{
			peRed += -170/ (int)(nbColor*2/3-nbColor/3);
			peGreen += 150/ (int)(nbColor*2/3-nbColor/3);
			peBlue += 10/ (int)(nbColor*2/3-nbColor/3);
		}
		else
		{
			peRed += 230/ (int)(nbColor-nbColor*3/4);
			peGreen += 80/ (int)(nbColor-nbColor*3/4);
			peBlue += 240/ (int)(nbColor-nbColor*3/4);
		}

		if( peBlue < 0 ) peBlue = 0;
		if( peGreen < 0 ) peGreen = 0;
		if( peRed < 0 ) peRed = 0;
		if( peBlue > 255 ) peBlue = 255;
		if( peGreen > 255) peGreen = 255;
		if( peRed > 255) peRed = 255;
    }
	*/

    return CreatePalette( nbColor, m_colorArray);

}


BOOL CMyPalette::CreatePalette( LPLOGPALETTE lpLogPalette )
{
    return CPalette::CreatePalette(lpLogPalette);
}

int CMyPalette::GetTotalPound(const CColorProfileArray& colorProfileArray)
{
    int totalPound = 0;

    for(int i=0; i<colorProfileArray.GetSize(); i++)
    {
        totalPound += colorProfileArray[i].GetPound();
    }

    return totalPound;
}
