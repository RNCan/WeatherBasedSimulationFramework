#include "StdAfx.h"
#include "QualityControl.h"
#include "DailyDatabase.h"
#include "Resource.h"
#include "FileManagerRes.h"
#include "oldUtilTime.h"
#include "CDib.h"

using namespace CFL;
using namespace UtilWin;

//*********************************************************************
const char* CQualityControl::ATTRIBUTE_NAME[] = { "InputFilepath", "OutputFilepath" };
const char* CQualityControl::CLASS_NAME = "QualityControl";

CQualityControl::CQualityControl(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();

}

void CQualityControl::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_QUALITY_CONTROL );

	CToolsBase::InitClass(option);

	CStringArrayEx labels(IDS_PROPERTIES_QUALITY_CONTROL);
	ASSERT( labels.GetSize() == NB_ATTRIBUTE);

	CString filter1 = GetString( IDS_CMN_FILTER_DAILY);
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[0], labels[0], filter1 ) );
	GetParameters().Add( CParamDef(CParamDef::PATH, ATTRIBUTE_NAME[1], labels[1] ) );
}

CQualityControl::~CQualityControl(void)
{
}


CQualityControl::CQualityControl(const CQualityControl& in)
{
	operator=(in);
}

void CQualityControl::Reset()
{
	m_inputFilePath.Empty();
	m_outputPath.Empty();
}

CQualityControl& CQualityControl::operator =(const CQualityControl& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_inputFilePath = in.m_inputFilePath;
		m_outputPath = in.m_outputPath;
	}

	return *this;
}

bool CQualityControl::operator ==(const CQualityControl& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if( m_inputFilePath != in.m_inputFilePath)bEqual = false;
	if( m_outputPath != in.m_outputPath)bEqual = false;
	
	return bEqual;
}

bool CQualityControl::operator !=(const CQualityControl& in)const
{
	return !operator ==(in);
}


CString CQualityControl::GetValue(short type)const
{
	CString str;
	

	ASSERT( NB_ATTRIBUTE == 2); 
	switch(type)
	{
	case I_INPUT_DB: str = m_inputFilePath;break;
	case I_OUTPUT_PATH: str = m_outputPath; break;
	default: str = CToolsBase::GetValue(type); break;
	}

	return str;
}

void CQualityControl::SetValue(short type, const CString& str)
{
	ASSERT( NB_ATTRIBUTE == 2); 
	switch(type)
	{
	case I_INPUT_DB: m_inputFilePath = str;break;
	case I_OUTPUT_PATH: m_outputPath=str; break;
	default: CToolsBase::SetValue(type, str); break;
	}

}

bool CQualityControl::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CQualityControl& info = dynamic_cast<const CQualityControl&>(in);
	return operator==(info);
}

CParameterBase& CQualityControl::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CQualityControl& info = dynamic_cast<const CQualityControl&>(in);
	return operator=(info);
}

ERMsg CQualityControl::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	CString inputFilePath(GetAbsoluteFilePath(m_inputFilePath));
	CString outputPath(GetAbsoluteFilePath(m_outputPath));
		

	UtilWin::CreateMultipleDir(m_outputPath);
	//	CStdioFile file;
	//file.Open(filePathOut, CFile::modeCreate|CFile::modeWrite);
	//file.WriteString("LOC_FILE 4 2,ID,Lat,Lon,Elev,Slope,Orientation,Nb Years,Nb SWE,Nb SnowDepth,sh METEO,sh SWE,%error,Bias,MAE,RMSE,R²\n");

	//CStdioFile fileByYear;
	//fileByYear.Open(filePathOut+"2.csv", CFile::modeCreate|CFile::modeWrite);
	//fileByYear.WriteString("Name,Years,Nb SWE,Nb SnowDepth,sh METEO,sh SWE,%error,Bias,MAE,RMSE,R²\n");

	callback.AddMessage( "Quality Control" );
	callback.AddMessage( outputPath, 1);
	callback.AddMessage("");

	
	//Get the data for each station
	CDailyDatabase inputDailyDB;
	
	msg += inputDailyDB.Open(inputFilePath);
	if( !msg)
		return msg;

	callback.SetNbStep(0, inputDailyDB.GetSize(), 1);
	

	for(int i=0; i<inputDailyDB.GetSize()&&msg; i++)
	{
		CDailyStation station;
		inputDailyDB.GetAt(i, station);
		CIntArray years;
		station.GetYearList(years, true);

		
		int firstYear = years[0];
		int endYear = years[years.GetSize()-1]+1;
		CDate firstRef(firstYear, JANUARY, FIRST_DAY);
		CDate lastRef(endYear, JANUARY, FIRST_DAY);
	
		const int SCALE_Y = 10;
		const int SCALE_X = 10;
		const int X_SIZE=366*SCALE_X;
		const int Y_SIZE=100*SCALE_Y;
		
		//CDib image;
		//image.Create((lastRef-firstRef), Y_SIZE, 8);
		//image.Create(X_SIZE, Y_SIZE, 8);
		//CImageDC dci(image);
		//CDC* pDC = CDC::FromHandle((HDC)dci);
		//pDC->FillSolidRect(0,0,(lastRef-firstRef), Y_SIZE, RGB(255,255,255));
		//pDC->FillSolidRect(0,0,X_SIZE, Y_SIZE, RGB(255,255,255));

		//CPen pen2(PS_DOT,1,RGB(300,300,300));
		//pDC->SelectObject(&pen2);

		///*for(int year=firstYear; year<endYear; year++)
		//{
		//	CDate d(year, JANUARY, FIRST_DAY);

		//	DrawScale(pDC, d - firstRef, Y_SIZE);
		//	pDC->DrawText(ToString(years[y]),,);
		//}
		//*/

		//int pos=0;
		//for(int m=0; m<12; m++)
		//{
		//	DrawScale(pDC, pos*SCALE_X, Y_SIZE);
		//	pos+=CFL::GetNbDayPerMonth(2000,m);
		//}
		//
		
		CPen penTmin(PS_SOLID,2,RGB(0,0,255));
		CPen penTmax(PS_SOLID,2,RGB(255,0,0));
		CPen penTdew(PS_SOLID,2,RGB(0,255,0));

		for(int y=0; y<years.GetSize()-1&&msg; y++)//for all years
		{
			CDib image;
			image.Create(X_SIZE, Y_SIZE, 8);
			CImageDC dci(image);
			CDC* pDC = CDC::FromHandle((HDC)dci);
		
			pDC->FillSolidRect(0,0,X_SIZE, Y_SIZE, RGB(255,255,255));

			CPen pen2(PS_DOT,1,RGB(300,300,300));
			pDC->SelectObject(&pen2);

			DrawScaleX(pDC, X_SIZE, Y_SIZE/2);
			int pos=0;
			for(int m=0; m<12; m++)
			{
				DrawScaleY(pDC, pos*SCALE_X, Y_SIZE);
				pos+=CFL::GetNbDayPerMonth(2000,m);
			}

		
			short year1 = firstYear+y;
			short year2 = year1+1;
		
			bool bValidYear = station.YearExist(year1) && (station(y).HaveCat("T")||station(y).HaveCat("H"));
			if( bValidYear )
			{
				int nbDayWithTmp=0;
				int nbDayWithPrcp=0;
				

				CDate begin(year1, JANUARY, FIRST_DAY);
				CDate end(year2, JANUARY, FIRST_DAY);
				
				pDC->SelectObject(&penTmin);
				bool bFirst=true;
				for(CDate d=begin; d<end; d++)
				{
					//we find the same station than the location
					const COneDayData& wDay = station.GetYear(d.GetYear())[d.GetJDay()];

					//int nbDay = d - firstRef;
					int nbDay = d - begin;
					

					if( wDay[DAILY_DATA::TMIN]>-999)
					{
						CPoint p(nbDay*SCALE_X, Y_SIZE/2 - int(wDay[DAILY_DATA::TMIN])*SCALE_Y);
						pDC->SetDCPenColor(RGB(255,0,0));
						bFirst?pDC->MoveTo(p):pDC->LineTo(p);
						pDC->FillSolidRect(p.x-2, p.y-2,4,4, RGB(0,0,255) );

						bFirst=false;
					}
					else bFirst=true;
				}

				pDC->SelectObject(&penTmax);
				bFirst=true;
				for(CDate d=begin; d<end; d++)
				{
					//we find the same station than the location
					const COneDayData& wDay = station.GetYear(d.GetYear())[d.GetJDay()];

					//int nbDay = d - firstRef;
					int nbDay = d - begin;

					if( wDay[DAILY_DATA::TMAX]>-999)
					{
						//CPoint p(nbDay, CFL::Max(4, Y_SIZE - int(wDay[DAILY_DATA::TMAX])*SCALE) );
						//pDC->FillSolidRect(p.x-4, p.y-4,9,9, RGB(255,0,0) );
						CPoint p(nbDay*SCALE_X, Y_SIZE/2 - int(wDay[DAILY_DATA::TMAX])*SCALE_Y);
						pDC->SetDCPenColor(RGB(255,0,0));
						bFirst?pDC->MoveTo(p):pDC->LineTo(p);
						pDC->FillSolidRect(p.x-2, p.y-2,4,4, RGB(255,0,0) );

						bFirst=false;
					}
					else bFirst=true;
				}//for all days

				pDC->SelectObject(&penTdew);
				bFirst=true;
				for(CDate d=begin; d<end; d++)
				{
					//we find the same station than the location
					const COneDayData& wDay = station.GetYear(d.GetYear())[d.GetJDay()];

					//int nbDay = d - firstRef;
					int nbDay = d - begin;

					if( wDay[DAILY_DATA::TDEW]>-999)
					{
						CPoint p(nbDay*SCALE_X, Y_SIZE/2 - int(wDay[DAILY_DATA::TDEW])*SCALE_Y);
						pDC->SetDCPenColor(RGB(0,255,0));
						bFirst?pDC->MoveTo(p):pDC->LineTo(p);
						pDC->FillSolidRect(p.x-2, p.y-2,4,4, RGB(0,255,0) );

						bFirst=false;
					}
					else bFirst=true;
				}//for all days
									
					//if( wDay[DAILY_DATA::PRCP]>-999)
					//{
					//	statDepthy+= 1;
					//	
					//	CPoint p(nbDay, Y_SIZE - int(wDay[DAILY_DATA::SNDH])*SCALE);
					//	bFirst?pDC->MoveTo(p):pDC->LineTo(p);
					//	//pDC->SetPixel(p.x,p.y,RGB(0,0,155));
					//	bFirst=false;
					//}
					//else bFirst=true;

				CString filePath = outputPath + station.GetName() + ToString(years[y]) + ".bmp";
				msg += image.SaveImage(filePath,Gdiplus::ImageFormatBMP);
			}//valid year

			msg += callback.StepIt(1.0/years.GetSize());
		}//for all years


		

			//for(int k=0; k<CStation::NB_MEMBER; k++)
			//	file.WriteString(((CStation&)dailyStation).GetMember(k, CStation::DECIMALS_DEGREES)+",");

			//CString tmp;
			//double error = statCorreletion[MAE]/statCorreletion[MEAN];
			//tmp.Format("0,0,%.0lf,%.0lf,%.0lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf\n",statSWE[NB_VALUE],statSWE[SUM],statDepth[SUM],statCorreletion.GetX()[MEAN], statCorreletion.GetY()[MEAN], error, statCorreletion[BIAS], statCorreletion[MAE], statCorreletion[RMSE], statCorreletion[COEF_C]);
			//file.WriteString(tmp);



	}//for all stations



	return msg;
}

void CQualityControl::DrawScaleX(CDC* pDC, int x, int y)
{
	pDC->MoveTo(0, y);
	pDC->LineTo(x, y);

	int nbScale = x/(10*2)-1;
	for(int i=0; i<nbScale; i++)
	{
		pDC->MoveTo(x-(i+1)*10*2, y-8);
		pDC->LineTo(x-(i+1)*10*2, y);
	}
}

void CQualityControl::DrawScaleY(CDC* pDC, int x, int y)
{
	pDC->MoveTo(x, 0);
	pDC->LineTo(x, y);
	int nbScale = y/100-1;
	for(int i=0; i<nbScale; i++)
	{
		pDC->MoveTo(x-15, y-(i+1)*100);
		pDC->LineTo(x+15, y-(i+1)*100);
	}
}

