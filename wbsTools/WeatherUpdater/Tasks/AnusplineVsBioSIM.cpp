#include "StdAfx.h"
#include "AnusplineVsBioSIM.h"
//#include "WeatherGeneratorStat.h"
#include "NormalStation.h"
#include <direct.h>
#include "NormalDatabase.h"

#include "SYShowMessage.h"
#include "CommonRes.h"
#include "FileManagerRes.h"
#include "Task.h"
#include "Resource.h"

using namespace NORMAL_DATA;
using namespace WEATHER;
using namespace UtilWin;

//enum { SIM=CXvalStationStat::SIM, OBS=CXvalStationStat::OBS };

//enum { NB_YEARS=30 };//mettre 300

//*********************************************************************
/*CFrequency CXvalStationStat::m_freqProb[2][NB_VAR];
CFrequency CXvalStationStat::m_extremFrequency[2][12][2];
CFL::CStatistic2 CXvalStationStat::m_monthlyStat[12+1][NORMAL_DATA::NB_FIELDS];


CXvalStationStat::CXvalStationStat()
{
	for(int i=0; i<2; i++)
	{
		m_frequency[i][TMIN].Reset(-40, 40, 1);
		m_frequency[i][DAILY_DATA::TMAX].Reset(-40, 40, 1);
		//m_frequency[i][TRANGE].Reset(0, 30, 1);
		m_frequency[i][DAILY_DATA::PRCP].Reset(0, 150, 2);
		m_frequency[i][DAILY_DATA::TDEW].Reset(-40, 40, 1);
		m_frequency[i][DAILY_DATA::RELH].Reset(0, 100, 1);
		m_frequency[i][DAILY_DATA::WNDS].Reset(0, 75, 1);

		m_frostPeriod[i].Reset(0, 365, 1);
		m_droughtFreq[i].Reset(1, 150, 1);
		m_hotSpellFreq[i].Reset(1, 150, 1);
	}


}

void CXvalStationStat::InitGlobal()
{
	for(int i=0; i<2; i++)
	{
		//for(int j=AN_MIN; j<=AN_MEAN; j++)
		m_freqProb[i][TMIN].Reset(-4, 4, 0.1);
		m_freqProb[i][TMAX].Reset(-4, 4, 0.1);
		m_freqProb[i][PRCP].Reset(0, 5, 0.1);
		m_freqProb[i][TDEW].Reset(-4, 4, 0.1);
		m_freqProb[i][RELH].Reset(0, 5, 0.1);
		m_freqProb[i][WNDS].Reset(0, 5, 0.1);

		for(int m=0; m<12; m++)
		{
			m_extremFrequency[i][m][0].Reset(-70, 30, 1);
			m_extremFrequency[i][m][1].Reset(-30, 70, 1);
		}
	}

	for(int m=0; m<13; m++)
		for(int k=0; k<NORMAL_DATA::NB_FIELDS; k++)
			m_monthlyStat[m][k].Reset();
}

CString CXvalStationStat::GetHeader(int type)
{
	CString text;	
	
	

	
	switch(type)
	{
	case T1_1: 
		text = "Sta,Month";
		for(int i=0; i<2; i++)
		{
			for(int j=0; j<NB_FIELDS; j++)
			{
				text += i==0?",S_":",O_";
				text += CWeatherDefine::GetNameN(j);
			}
		}
		break;
		//,S_Xmin,S_Mmin,S_Mmax,S_Xmax,S_Rxn,S_DEL,S_EPS,S_A1,S_A2,S_B1,S_B2,S_PPT,S_Sp,O_Xmin,O_Mmin,O_Mmax,O_Xmax,O_Rxn,O_DEL,O_EPS,O_A1,O_A2,O_B1,O_B2,O_PPT,O_Sp"; break;
	case T1_2: text = "Sta,Month,S_P(ppt),S_MeanPPT,O_P(ppt),O_MeanPPT"; break;
	case T1_3: text = "Sta,Month,S_Xminmin,S_Mmin,S_Xminmax,S_Smin,S_Xmaxmin,S_Mmax,S_Xmaxmax,S_Smax,S_Xpptmin,S_ppt,S_Xpptmax,S_Sppt,O_Xminmin,O_Mmin,O_Xminmax,O_Smin,O_Xmaxmin,0_Mmax,O_Xmaxmax,O_Smax,O_Xpptmin,O_ppt,O_Xpptmax,O_Sppt"; break;
//	case T1_4: text = "Sta	Month	S_Xmin	S_Mmin	S_Mmax	S_Xmax	S_Sm	S_Rxn	S_DEL	S_EPS	S_A1	S_A2	S_B1	S_B2	S_PPT	S_Sp	S_Gamma	S_Zeta	O_Xmin	O_Mmin	O_Mmax	O_Xmax	O_Sm	O_Rxn	O_DEL	O_EPS	O_A1	O_A2	O_B1	O_B2	O_PPT	O_Sp	O_Gamma	O_Zeta"; break;
	case T2_1: 
	case T2_2: 
	case T2_3: 
	case T2_4: 
	case T3:   text = "Sta	Class	Fs	Fo"; break;
	case T4_1: text = "Sta 	Month	Sim_Drt  	Obs_Drt 	S_HotSpl	O_HotSpl"; break;
	case T4_2: text = "Sta	Class	Ds	Do	Hs	Ho"; break;
	case T5_1: text += (" No  \t an \tmo\t  x  \t  N  \t  S  \t  x  \t  N  \t  S  \t  x  \t  N  \t  S  \t tot \t  N  \t  S  "); break;
	case T5_2: text = "Class	Fs	Fo	Fs	Fo	Fs	Fo"; break;
	case T5_3: text = "Class	Fs	Fo"; break;
	case T5_4: text = "Month	Class	minFs	minFo	Class	maxFs	maxFo"; break;
	case T5_5: text = "Variable,Month,Bias,MAE,RMSE,R²"; break;
//	case T6_1: text = "sta\tyear\tseason\tS_txav\tS_tnav\tS_tav\tS_trav\tS_trq10\tS_trq90\tS_txq10\tS_txq90\tS_tnq10\tS_tnq90\tS_tnfd\tS_txice\tS_tiaetr\tS_txhwd\tS_txhw90\tS_tncwd\tS_tncw10\tS_txf10\tS_txf90\tS_tnf10\tS_tnf90\tS_pav\tS_pq20\tS_pq40\tS_pq50\tS_pq60\tS_pq80\tS_pq90\tS_pq95\tS_pf20\tS_pf40\tS_pf50\tS_pf60\tS_pf80\tS_pf90\tS_pf95\tS_pn10mm\tS_pxcdd\tS_pxcwd\tS_ppww\tS_ppdd\tS_ppcr\tS_pwsav\tS_pwsmed\tS_pwssdv\tS_pdsav\tS_pdsmed\tS_pdssdv\tS_px3d\tS_px5d\tS_px10d\tS_pint\tS_pfl90\tS_pnl90\tO_txav\tO_tnav\tO_tav\tO_trav\tO_trq10\tO_trq90\tO_txq10\tO_txq90\tO_tnq10\tO_tnq90\tO_tnfd\tO_txice\tO_tiaetr\tO_txhwd\tO_txhw90\tO_tncwd\tO_tncw10\tO_txf10\tO_txf90\tO_tnf10\tO_tnf90\tO_pav\tO_pq20\tO_pq40\tO_pq50\tO_pq60\tO_pq80\tO_pq90\tO_pq95\tO_pf20\tO_pf40\tO_pf50\tO_pf60\tO_pf80\tO_pf90\tO_pf95\tO_pn10mm\tO_pxcdd\tO_pxcwd\tO_ppww\tO_ppdd\tO_ppcr\tO_pwsav\tO_pwsmed\tO_pwssdv\tO_pdsav\tO_pdsmed\tO_pdssdv\tO_px3d\tO_px5d\tO_px10d\tO_pint\tO_pfl90\tO_pnl90"; break;
//	case T6_2: text = "sta\tyear\tS_txav\tS_tnav\tS_tav\tS_trav\tS_trq10\tS_trq90\tS_txq10\tS_txq90\tS_tnq10\tS_tnq90\tS_tnfd\tS_txice\tS_tgdd\tS_tiaetr\tS_tgsl\tS_txhwd\tS_txhw90\tS_tncwd\tS_tncw10\tS_tnfsl\tS_txf10\tS_txf90\tS_tnf10\tS_tnf90\tS_pav\tS_pq20\tS_pq40\tS_pq50\tS_pq60\tS_pq80\tS_pq90\tS_pq95\tS_pf20\tS_pf40\tS_pf50\tS_pf60\tS_pf80\tS_pf90\tS_pf95\tS_pn10mm\tS_pxcdd\tS_pxcwd\tS_ppww\tS_ppdd\tS_ppcr\tS_pwsav\tS_pwsmed\tS_pwssdv\tS_pdsav\tS_pdsmed\tS_pdssdv\tS_px3d\tS_px5d\tS_px10d\tS_pint\tS_pfl90\tS_pnl90\tO_txav\tO_tnav\tO_tav\tO_trav\tO_trq10\tO_trq90\tO_txq10\tO_txq90\tO_tnq10\tO_tnq90\tO_tnfd\tO_txice\tO_tgdd\tO_tiaetr\tO_tgsl\tO_txhwd\tO_txhw90\tO_tncwd\tO_tncw10\tO_tnfsl\tO_txf10\tO_txf90\tO_tnf10\tO_tnf90\tO_pav\tO_pq20\tO_pq40\tO_pq50\tO_pq60\tO_pq80\tO_pq90\tO_pq95\tO_pf20\tO_pf40\tO_pf50\tO_pf60\tO_pf80\tO_pf90\tO_pf95\tO_pn10mm\tO_pxcdd\tO_pxcwd\tO_ppww\tO_ppdd\tO_ppcr\tO_pwsav\tO_pwsmed\tO_pwssdv\tO_pdsav\tO_pdsmed\tO_pdssdv\tO_px3d\tO_px5d\tO_px10d\tO_pint\tO_pfl90\tO_pnl90"; break;

	default: ASSERT(false);
	}

	return text;
}



CString CXvalStationStat::GetString(int type, int stationID)const
{
	CString text;	

	
	switch(type)
	{
	case T1_1:
		{
			for(int m=0; m<12; m++)
			{
				CString line;
				line.Format( "%d,%d", stationID, m+1);

				for(int i=0; i<2; i++)
				{
					for(int j=0; j<NB_FIELDS; j++)
					{
						CString format = GetRecordFormat(j);
						CString record;
						record.Format(format, m_normals[i][m][j]);
						
						line += "," + record;

					}

//					if( type == T1_4)
//					{
						//double P1 = 0;
						//double P2 = 0;
						
						//m_normals[i].GetP(m, P1, P2);
						
//						CString record;
//						record.Format(" %7.4lf\t %7.4lf\t", m_Sm[i][m][0], m_Sm[i][m][1]);
					
//						line += record;
//					}
				}
				
				text += line + "\n";
			}
			break;
		}
	case T1_2:
		{
			for(int m=0; m<12; m++)
			{
				CString line;
				line.Format( "%d,%d", stationID, m+1);

				for(int i=0; i<2; i++)
				{
					CString tmp;
					tmp.Format(",%lf,%lf\t", 
						m_probOfPpt[i][m][CFL::MEAN],
						m_meanPpt[i][m][CFL::MEAN]);
					
					line += tmp;
				}

				text += line + "\n";
			}

			break;
		}
	case T1_3:
		{
			for(int m=0; m<12; m++)
			{
				CString line;
				line.Format( "%d,%d", stationID, m+1);

				for(int i=0; i<2; i++)
				{
					for(int k=0; k<3; k++)
					{
						//int EX_VAR = (k!=0)?CFL::LOWEST:CFL::HIGHEST;
						int SD_VAR = (k!=2)?CFL::STD_DEV: CFL::COEF_VAR;
						CString tmp;
						tmp.Format(",%.4lf,%.4lf,%.4lf,%.4lf", m_extremLow[i][m][k][MEAN], m_extremMean[i][m][k][MEAN], m_extremHigh[i][m][k][MEAN], m_extremMean[i][m][k][SD_VAR]);
						
						line += tmp;
					}
				}
				text += line + "\n";
			}

			break;
		}
		
	case T2_1:
	case T2_2:
	case T2_3:
	case T2_4:
		{
			int sunType = type-T2_1;
			int nbValue = m_frequency[0][sunType].GetSize();
			for(int i=0; i< nbValue; i++)
			{
				CString line;
				line.Format( "%d\t", stationID);
//				for(int j=0; j<4; j++)
				{
					CString tmp;
					tmp.Format( "%.0lf\t", m_frequency[0][sunType].GetClassValue(i));
					line += tmp;

					for(int k=0; k<2; k++)
					{
						tmp.Format( "%.4lf\t", m_frequency[k][sunType][i]);
						line += tmp;
					}
				}
				text += line + "\n";			
			}
			break;
		}
	case T3:
		{
			int nbValue = m_frostPeriod[0].GetSize();
			for(int i=0; i< nbValue; i++)
			{
				CString line;
				line.Format( "%d\t%.0lf\t", stationID, m_frostPeriod[0].GetClassValue(i));
				for(int j=0; j<2; j++)
				{
					CString tmp;
					tmp.Format( "%.4lf\t", m_frostPeriod[j][i]);
					line += tmp;
				}
				text += line + "\n";			
			}
			break;
		}
	case T4_1:
		{
			for(int m=0; m<12; m++)
			{
				CString line;
				line.Format( "%d\t%d\t", stationID, m+1);

				for(int i=0; i<2; i++)
				{
					CString tmp;
					double value = (m_drought[i][m][CFL::NB_VALUE]>0)?m_drought[i][m][CFL::MEAN]:0;
					tmp.Format("%lf\t", value);
					line += tmp;
				}
				for(int i=0; i<2; i++)
				{
					CString tmp;
					double value = (m_hotSpell[i][m][CFL::NB_VALUE]>0)?m_hotSpell[i][m][CFL::MEAN]:0;
					tmp.Format("%lf\t", value);
					line += tmp;
				}


				text += line + "\n";
			}

			break;
		}
	
	case T4_2:
		{
			int nbValue = m_droughtFreq[0].GetSize();
			for(int i=0; i< nbValue; i++)
			{
				ASSERT( m_droughtFreq[0].GetClassValue(i) == m_hotSpellFreq[1].GetClassValue(i));

				CString line;
				line.Format( "%d\t%.0lf\t", stationID, m_droughtFreq[0].GetClassValue(i));
				for(int j=0; j<2; j++)
				{
					CString tmp;
					tmp.Format( "%.4lf\t", m_droughtFreq[j][i]);
					line += tmp;
				}
				for(int j=0; j<2; j++)
				{
					CString tmp;
					tmp.Format( "%.4lf\t", m_hotSpellFreq[j][i]);
					line += tmp;
				}

				text += line + "\n";			
			}
			break;
		}
	case T5_1:
		{
			const CYearStatArray& stat = type==T5_1?m_unknow[OBS]:m_unknow[SIM];
			int size = min(stat.GetSize(), 30); //limit to 30 years
			for(int y=0; y<size; y++)
			{
				for(int m=0; m<12; m++)
				{
					//if they have observation for this year/month
					if( stat[y][m][0].x > -999)
					{
						CString line;
						line.Format( "%5d\t%4d\t%2d\t", stationID, y+1, m+1);

						for(int i=0; i<4; i++)
						{
							//for(int j=0; j<3; j++)
							{
								CString record;
								record.Format("%5.1lf\t%5.1lf\t%5.3lf\t", stat[y][m][i][0], stat[y][m][i][1], stat[y][m][i][2] );
								line += record;
							}
						}
						
						text += line + "\n";
					}
				}
			}
	
			break;
		}

	case T5_2:
	case T5_3:
		{
			int subType = type-T5_2;
			
			int fVar = (subType==0)?0:3;
			int nbVar = (subType==0)?3:1;

			int nbValue = m_freqProb[0][fVar].GetSize();
			for(int i=0; i< nbValue; i++)
			{
				CString line;
				line.Format( "%.1lf\t", m_freqProb[0][fVar].GetClassValue(i));

				for(int v=0; v<nbVar; v++)
				{
					
					for(int k=0; k<2; k++)
					{
						CString tmp;
						tmp.Format( "%.4lf\t", m_freqProb[k][fVar+v][i]);
						line += tmp;
					}
				}

				text += line + "\n";			
			}
			break;
		}
	case T5_4:
		{
			
			for(int m=0; m<12; m++)
			{
				int nbValue = m_extremFrequency[0][0][0].GetSize();
				for(int i=0; i< nbValue; i++)
				{
					CString line;
					line.Format( "%d\t", m+1);
					for(int v=0; v<2; v++)
					{
						CString tmp;
						tmp.Format( "%.1lf\t", m_extremFrequency[0][0][v].GetClassValue(i));
						line += tmp;
						for(int k=0; k<2; k++)
						{
							tmp.Format( "%.4lf\t", m_extremFrequency[k][m][v][i]);
							line += tmp;
						}
					}

					text += line + "\n";			
				}
			}
			break;
		}
	
	case T5_5:
		{
			for(int k=0; k<NORMAL_DATA::NB_FIELDS; k++)
			{
				for(int m=0; m<13; m++)
				{
					CString line;
					line.Format( "%15.15s,%2d,%lf,%lf,%lf,%lf,", CWeatherDefine::GetNameN(k), m, m_monthlyStat[m][k][CFL::BIAS], m_monthlyStat[m][k][CFL::MAE], m_monthlyStat[m][k][CFL::RMSE], m_monthlyStat[m][k][CFL::COEF_CORR]);
					text += line + "\n";
				}
			}
			
			
			break;
		}
	default: ASSERT(false);
	}

	return text;
}

//*********************************************************************

ERMsg CXvalStationStatArray::Save(const CString& filePath)
{
	ERMsg msg;

	ASSERT( CXvalStationStat::NB_STAT ==15);

	const char* FILE_NAME[CXvalStationStat::NB_STAT]= {"1_1", "1_2", "1_3", "2_Tmin", "2_Tmax", "2_Trange", "2_ppt", "3", "4_1", "4_2", "5_1", "5_2", "5_3", "5_4", "5_5"};
	for(int i=0; i<CXvalStationStat::NB_STAT; i++)
	{
		CStdioFile file;

		CString outFilePath(filePath);
		UtilWin::SetFileTitle( outFilePath, UtilWin::GetFileTitle(filePath) + FILE_NAME[i]);

		msg = UtilWin::OpenFile( file, outFilePath, CFile::modeWrite|CFile::modeCreate );
		if( msg)
		{
			file.WriteString( CXvalStationStat::GetHeader(i) +"\n" );
			
			int size = (i<CXvalStationStat::T5_2||i>CXvalStationStat::T5_5)?GetSize():1;
			for(int j=0; j<size; j++)
			{
				CString tmp = GetAt(j).GetString(i, j+1);
				file.WriteString( tmp);
			}
			file.Close();
		}
		
	}

	return msg;
}

void CXvalStationStatArray::GetLoc(CLocArray& loc)
{
	loc.SetSize(0, GetSize());
	for(int i=0; i<GetSize(); i++)
	{
		CLocStation st;
		((CStation&)st) = GetAt(i);
		loc.Add(st);
	}

}

void CXvalStationStatArray::SetLoc(CLocArray& loc)
{
	SetSize(loc.GetSize());
	for(int i=0; i<loc.GetSize(); i++)
	{
		((CStation&)GetAt(i)) = loc[i];
	}

}
*/
//*********************************************************************

const char* CAnusplineVsBioSIM::ATTRIBUTE_NAME[NB_ATTRIBUTE]={ "NormalDB", "Location", "OutputPath"};
const char* CAnusplineVsBioSIM::CLASS_NAME = "ANUSPLINE-BIOSIM";


CAnusplineVsBioSIM::CAnusplineVsBioSIM(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CAnusplineVsBioSIM::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className = "Anuspline vs BioSIM";

	CToolsBase::InitClass(option);
	//init static 
	ASSERT( GetParameters().GetSize() < I_NB_ATTRIBUTE);

	CStringArray array;
	array.Add("Normal file path");
	array.Add("Loc file path");
	array.Add("Output file path");
	//LoadStringArray( array, IDS_PROPERTIES_EXTRACT_FROM_BIOSIMDB);
	//ASSERT( array.GetSize() == NB_ATTRIBUTE+1);

	
	CString filter2 = GetString( IDS_FM_NORMAL_FILTER);
	CString filter3 = GetString( IDS_CMN_FILTER_LOC);
	
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[0], array[0], filter2) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], array[1], filter3) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[2], array[2], "txt|*.txt||") );
	
}

CAnusplineVsBioSIM::~CAnusplineVsBioSIM(void)
{
}


CAnusplineVsBioSIM::CAnusplineVsBioSIM(const CAnusplineVsBioSIM& in)
{
	operator=(in);
}

void CAnusplineVsBioSIM::Reset()
{
	CToolsBase::Reset();

	m_normalFilePath.Empty();
	m_locFilePath.Empty();
	m_outputFilePath.Empty();
	
}

CAnusplineVsBioSIM& CAnusplineVsBioSIM::operator =(const CAnusplineVsBioSIM& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_normalFilePath = in.m_normalFilePath;
		m_locFilePath = in.m_locFilePath;
		m_outputFilePath = in.m_outputFilePath;
	}

	return *this;
}

bool CAnusplineVsBioSIM::operator ==(const CAnusplineVsBioSIM& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in) )bEqual = false;
	if( m_normalFilePath != in.m_normalFilePath)bEqual = false;
	if( m_locFilePath != in.m_locFilePath)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	
	return bEqual;
}

bool CAnusplineVsBioSIM::operator !=(const CAnusplineVsBioSIM& in)const
{
	return !operator ==(in);
}


CString CAnusplineVsBioSIM::GetValue(short type)const
{
	CString value;
	
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_NORMAL_DB:value = m_normalFilePath; break;
	case I_LOC_INPUT: value = m_locFilePath; break;
	case I_OUTPUT: value = m_outputFilePath; break;
	default: value = CToolsBase::GetValue(type); break;
	}
 
	return value;
}

void CAnusplineVsBioSIM::SetValue(short type, const CString& value)
{
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_NORMAL_DB:m_normalFilePath=value; break;
	case I_LOC_INPUT: m_locFilePath= value; break;
	case I_OUTPUT: m_outputFilePath=value; break;
	default: CToolsBase::SetValue(type, value ); break;
	}

}

int FindID(const CLocationVector& normals, const CString& ID)
{
	int index=-1;
	for(int i=0; i<normals.size(); i++)
	{
		if( normals[i].m_ID.c_str() == ID)
		{
			index=i;
			break;
		}
	}

	return index;
}

/*
void ReadMexicoData()
{
	CString path = "D:\\travail\\anusplin\\Cuauhtmoc\\";
	static const short VARIABLE[3] = { TMIN_MN, TMAX_MN, PRCP_TT};
	static const char* FILE_NAME[3] = {"TempMin\\MMonth.dat", "TempMax\\XMonth.dat", "precip\\PMonth.dat"};

	CLocationVector normals;
	CNormalDataVector dataArray;

	for(int f=0; f<3; f++)
	{
		//int s=0;

		CStdioFile file(path+FILE_NAME[f], CFile::modeRead);
		CString line;
		while( file.ReadString(line) )
		{
			
			int pos =0;
			CString contry = line.Tokenize(" \t", pos);
			CString ID = line.Tokenize(" \t", pos);
			CString lon = line.Tokenize(" \t", pos);
			CString lat = line.Tokenize(" \t", pos);
			CString elev = line.Tokenize(" \t", pos);

			int s = FindID(normals, ID);
			if( f==1 && s==-1)
				continue;

			if( s==-1)
			{
				s = normals.Add(CNormalStation());
				normals[s].SetMember(CStation::NAME, ID+" ("+contry+")");
				normals[s].SetMember(CStation::ID, ID);
				normals[s].SetMember(CStation::LAT, lat);
				normals[s].SetMember(CStation::LON, lon);
				normals[s].SetMember(CStation::ELEV, elev);

				dataArray.push_back(CNormalData());
				//CNormalData data;
				//data.Init(0);
				//normals[s].SetData(data);
			}
			else
			{
				ASSERT( normals[s].GetID() == ID);
			}

			//CNormalData data = normals[s].GetData();
			for(int m=0; m<12; m++)
			{
				CString tmp = line.Tokenize(" \t", pos);
				dataArray[s][m][VARIABLE[f]] = (float)atof(tmp);

				if( f==0 )
				{
					dataArray[s][m][TMNMX_R] = 1;
					dataArray[s][m][TMNMX_R] = 1;
					dataArray[s][m][DEL_STD] = 1;
					dataArray[s][m][EPS_STD] = 1;
					dataArray[s][m][TACF_A1] = 1;
					dataArray[s][m][TACF_A2] = 1;
					dataArray[s][m][TACF_B1] = 1;
					dataArray[s][m][TACF_B2] = 1;
				}
				else if (f==2)
				{
					dataArray[s][m][PRCP_SD] = 1;
				}
			}

			//normals[s].SetData(data);
			//s++;
		}
	}

	CNormalDatabase::DeleteDatabase( path + "Cuauhtmoc.normals");
	CNormalDatabase normal;
	VERIFY(normal.Open( path + "Cuauhtmoc.normals", CNormalDatabase::modeEdit ));

	int invalid=0;
	ASSERT( normals.GetSize() == (int)dataArray.size());
	for(int i=0; i<normals.GetSize(); i++)
	{
		//CNormalDataPlus data;
		//((CNormalData&)data) = dataArray[i];

		//if( data.IsValid() )
		//{
			normals[i].SetData(dataArray[i]);
			ASSERT( normals[i].IsValid() );
			normal.Add(normals[i]);
		//}
		//else
		//{
		//	invalid++;
		//}
	}

	normal.Close();

	VERIFY(normal.Open( path + "Cuauhtmoc.normals" ));
	
	CLocArray loc;
	
	for(int i=0; i<normal.GetSize(); i++)
		loc.Add( CLocStation(normal.GetStationHead(i)) );

	normal.Close();
	loc.Save(path + "Cuauhtmoc.loc");

	
}
*/
ERMsg CAnusplineVsBioSIM::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(GetGoodFilePath(m_outputFilePath), 1);

	//ReadMexicoData();
	//return msg;

	if( CNormalDatabase::GetVersion((LPCTSTR)m_normalFilePath)==5)
	{
		msg = CNormalDatabase::Version5ToCurrent((LPCTSTR)m_normalFilePath);
		if( !msg)
			return msg;
	}

	//create normal if not up to date
	/*CNormalDatabase normalDB;
	normalDB.Open( m_normalFilePath );

	CLocArray locArray;
	msg += locArray.Load(m_locFilePath);
	
	CSearchResultVector result;
	for(int i=0; i<locArray.GetSize(); i++)
	{
		CSearchResultVector resultTmp;
		normalDB.Match(locArray[i], 8, resultTmp, TEMPERATURE);
		result.Append( resultTmp);
		normalDB.Match(locArray[i], 8, resultTmp, PRECIPITATION);
		result.Append( resultTmp);
	}

	CLocationVector stations;
	normalDB.GetStations(result, stations);

	CString tmp(m_locFilePath);
	UtilWin::SetFileExtension( tmp, ".normals");
	CNormalDatabase newNormal;
	VERIFY(newNormal.Open( tmp, CNormalDatabase::modeEdit ));
	
	//CLocArray loc;
	//loc.Add( CLocStation(station[i]) );
	for(int i=0; i<stations.GetSize(); i++)
		newNormal.Add(stations[i]);
		

	CLocArray loc;
	for(int i=0; i<stations.GetSize(); i++)
		loc.Add( CLocStation(stations[i]) );
	
	tmp+=".loc";
	loc.Save(tmp);
	*/

	/*CLocationVector stations;
	CStdioFile file(m_normalFilePath, CFile::modeRead);

	while( file.ReadString(line) )
	{
		CNormalStation station;
		line.to
	}

	CString tmp(m_locFilePath);
	UtilWin::SetFileExtension( tmp, ".normals");
	CNormalDatabase newNormal;
	VERIFY(newNormal.Open( tmp, CNormalDatabase::modeEdit ));
	
	//CLocArray loc;
	//loc.Add( CLocStation(station[i]) );
	for(int i=0; i<stations.GetSize(); i++)
		newNormal.Add(stations[i]);
		

	CLocArray loc;
	for(int i=0; i<stations.GetSize(); i++)
		loc.Add( CLocStation(stations[i]) );
	
	tmp+=".loc";
	loc.Save(tmp);
	*/


	if( msg)
		msg = ExecuteAnuspline(callback);

	if( msg)
		msg = ExecuteBioSIM(callback);

	return msg;
}

//Do anuspline part
ERMsg CAnusplineVsBioSIM::ExecuteAnuspline(CFL::CCallback& callback)
{
	
	ERMsg msg;
/*
	
	//if they are a loc we use it, auterwise we use the daily DB
	CSearchResultVector resultLOC;
	//if( !m_locFilePath.IsEmpty() )
	//{
	CTempGenStat tempStat;
	tempStat.SetNormalFilePath(m_normalFilePath);
	tempStat.SetLocFilePath( m_locFilePath);
	msg = tempStat.GetCurrentStationList(resultLOC);
	if(!msg)
		return msg;

	//}


	CNormalDatabase normalDB;
	VERIFY( normalDB.Open(m_normalFilePath) );
	

	callback.SetNbStep(3);
	callback.SetCurrentDescription("Create Tmin and Tmax array");
	callback.SetNbStep(0, normalDB.GetSize(), 1);
	


	CSearchResultVector resultT;
	CSearchResultVector resultP;

	msg += normalDB.GetStationList(resultT, TEMPERATURE);
	msg += normalDB.GetStationList(resultP, PRECIPITATION);

	CString path = UtilWin::GetPath( GetGoodFilePath(m_normalFilePath) );
	//CString filepathTmin = path + "Tmin.dat";
	//CString filepathTmax = path + "Tmax.dat";
	//CString filepathPrcp = path + "Prcp.dat";
	//CStdioFileEx fileTmin;
	//CStdioFileEx fileTmax;
	//CStdioFileEx filePrcp;
	//msg += fileTmin.Open(filepathTmin, CFile::modeWrite|CFile::modeCreate);
	//msg += fileTmax.Open(filepathTmax, CFile::modeWrite|CFile::modeCreate);
	//msg += filePrcp.Open(filepathPrcp, CFile::modeWrite|CFile::modeCreate);

	CStringArray strData[3];
	for(int i=0; i<3; i++)
		strData[i].SetSize(normalDB.GetSize());

	if( msg) 
	{

		//create master file
		for(int i=0; i<resultT.GetSize()&&msg; i++)
		{
			CNormalStation station;
			normalDB.GetAt(resultT[i].m_index, station);
			ASSERT( station.HaveCat(TEMPERATURE));

			CString name = station.GetName().Left(20);
			name.Replace(' ','_');
			CString line[2];
			line[0].Format("%-20.20s%10.3lf%10.3lf%8.1f\n", name, station.GetLon(), station.GetLat(), (float)station.GetElev() );
			line[1].Format("%-20.20s%10.3lf%10.3lf%8.1f\n", name, station.GetLon(), station.GetLat(), (float)station.GetElev() );

			for(int m=0; m<12; m++)
			{
				CString tmp;
				tmp.Format("%7.2f", station[m][TMIN_MN]);
				line[0] += tmp;
			
				
				tmp.Format("%7.2f", station[m][TMAX_MN]);
				line[1] += tmp;
			}
			
			line[0]+="\n";
			line[1]+="\n";
			//fileTmin.WriteString(line[0]);
			//fileTmax.WriteString(line[1]);

			strData[0][resultT[i].m_index] = line[0];
			strData[1][resultT[i].m_index] = line[1];

			msg += callback.StepIt();
		}

		//fileTmin.Close();
		//fileTmax.Close();

		callback.SetCurrentDescription("Create Prcp array");
		callback.SetNbStep(0, normalDB.GetSize(), 1);
		
		for(int i=0; i<resultP.GetSize()&&msg; i++)
		{
			CNormalStation station;
			VERIFY(normalDB.GetAt(resultP[i].m_index, station));
			ASSERT( station.HaveCat(PRECIPITATION));

			CString name = station.GetName().Left(20);
			name.Replace(" ","_");

			CString line;
			line.Format("%-20.20s%10.3lf%10.3lf%8.1f\n", name, station.GetLon(), station.GetLat(), (float)station.GetElev() );

			for(int m=0; m<12; m++)
			{
				CString tmp;
				tmp.Format("%7.2f", station[m][PRCP_TT]);
				line+=tmp;
			}

			line+="\n";
			//filePrcp.WriteString(line);

			strData[2][resultP[i].m_index] = line;
			msg += callback.StepIt();
		}
	}
		//filePrcp.Close();
	
		ASSERT( strData[0].GetSize() == normalDB.GetSize() && strData[1].GetSize() == normalDB.GetSize() && strData[2].GetSize() == normalDB.GetSize());
	

	CGlobalStat stat;
	
	CString dataFilePath = path + "Input.dat";
	CString validationFilePathIn = path + "Validation.dat";
	CString validationFilePathOut = path + "Validation.SimOut";
	
	CString outputFilePath[3];
	CString outputPath = UtilWin::GetPath( m_outputFilePath);
	CString outputTitle = UtilWin::GetFileTitle( m_outputFilePath);
	outputFilePath[0] = outputPath + outputTitle + "AnusplineTmin.txt";
	outputFilePath[1] = outputPath + outputTitle + "AnusplineTmax.txt";
	outputFilePath[2] = outputPath + outputTitle + "AnusplinePrcp.txt"; 

	
	CStdioFileEx outputFile[3];
	msg += outputFile[0].Open(outputFilePath[0], CFile::modeWrite|CFile::modeCreate);
	msg += outputFile[1].Open(outputFilePath[1], CFile::modeWrite|CFile::modeCreate);
	msg += outputFile[2].Open(outputFilePath[2], CFile::modeWrite|CFile::modeCreate);
	

	if( msg)
	{
		callback.SetCurrentDescription("Run AnuSpline");
		callback.SetNbStep(0, resultLOC.GetSize()*3, 1);
		
		for(int i=0; i<resultLOC.GetSize()&&msg; i++)
		{
			//create data file 		//create validation file
			for(int j=0; j<3&&msg; j++)
			{
				CStdioFileEx fileData;
				CStdioFileEx fileValidationIn;

				msg += fileData.Open(dataFilePath, CFile::modeCreate|CFile::modeWrite);
				msg += fileValidationIn.Open(validationFilePathIn, CFile::modeCreate|CFile::modeWrite);

				for(int k=0; k<strData[j].GetSize(); k++)
				{
					ASSERT( resultLOC[i].m_index>=0 && resultLOC[i].m_index<strData[j].GetSize());

					if( !strData[j][k].IsEmpty() )
					{
					//	int locPos = resultLOC.Lookup( CSearchResult(k) );
					//	if( locPos == -1 )
						if( k != resultLOC[i].m_index)
							fileData.WriteString( strData[j][k] );
						else 
							fileValidationIn.WriteString( strData[j][k] );
					}
				}

				fileData.Close();
				fileValidationIn.Close();

				//remove old file
				CStringArray fileList;
				UtilWin::GetFileList( fileList, path + "*.SimOut", true);
				for(int k=0; k<fileList.GetSize(); k++)
					remove( fileList[k]);
				
				//run anuspline
				//CString run = j<2?"runaT.bat":"runaP.bat";
				CString run = j<2?"runbT.bat":"runbP.bat";
				CString cmdExec = "cmd.exe /c " + path + run;
				UtilWin::WinExecWait(cmdExec, path);
				
				//http://support.microsoft.com/?kbid=190351	
				//open result
				CStdioFileEx fileValidationOut;
				ERMsg msgTmp = fileValidationOut.Open(validationFilePathOut, CFile::modeRead);
				if( msgTmp)
				{
					CString line;
					while( fileValidationOut.ReadString(line) && !line.IsEmpty())
					{
						CString tmp;
						tmp.Format("%6d%s\n", i+1, line.Mid(6) );
						outputFile[j].WriteString(tmp); 

						int pos =0;
						line.Tokenize(" ", pos);//no
						line.Tokenize(" ", pos);//name
						double data[12][2] = {0};
						for(int i=0; i<2; i++)
						{
							for(int m=0; m<12; m++)
							{
								data[m][i] = atof(line.Tokenize(" ", pos));
							}
						}

						for(int m=0; m<12; m++) 
						{
							stat.m_stat[j].Add(data[m][1], data[m][0] );
							
							stat.m_monthlyStat[0][j<2?j:9].Add(data[m][1], data[m][0] );
							stat.m_monthlyStat[m+1][j<2?j:9].Add(data[m][1], data[m][0] );
						}

					}
					fileValidationOut.Close();

				}
				else
				{
					callback.AddMessage(msgTmp,1);
					callback.AddMessage(strData[j][resultLOC[i].m_index].Left(20), 2);

				}

				msg += callback.StepIt();
			}//for 3 variable
		}//for all station
			
			
		CString outputFilePath2 = m_outputFilePath;
		UtilWin::SetFileTitle( outputFilePath2, GetFileTitle(outputFilePath2)+"AnuSpline");
		stat.Save(outputFilePath2);
	
		outputFile[0].Close();
		outputFile[1].Close();
		outputFile[2].Close();
	}//if msg

	*/

	return msg;
}

//BioSIM part
ERMsg CAnusplineVsBioSIM::ExecuteBioSIM(CFL::CCallback& callback)
{
	ERMsg msg;
	/*
	CTempGenStat tempStat;

	tempStat.SetNormalFilePath(m_normalFilePath);
	tempStat.SetLocFilePath( m_locFilePath);
	tempStat.SetGradientType(CTempGenStat::LOCAL);
	tempStat.SetNbStationSearch(8);
	tempStat.SetNbStGrLocal(23);
	
	msg = tempStat.ComputeStationStat(callback);
	
	CString outputFilePath2 = m_outputFilePath;
	UtilWin::SetFileTitle( outputFilePath2, GetFileTitle(outputFilePath2)+"BioSIM");
	if( msg)
		msg = tempStat.GetGlobalStat().Save( outputFilePath2);


	
	CString outputFilePath[3];
	CString outputPath = UtilWin::GetPath( m_outputFilePath);
	CString outputTitle = UtilWin::GetFileTitle( m_outputFilePath);
	outputFilePath[0] = outputPath + outputTitle + "BioSIMTmin.txt";
	outputFilePath[1] = outputPath + outputTitle + "BioSIMTmax.txt";
	outputFilePath[2] = outputPath + outputTitle + "BioSIMPrcp.txt";

	
	CStdioFileEx outputFile[3];
	msg += outputFile[0].Open(outputFilePath[0], CFile::modeWrite|CFile::modeCreate);
	msg += outputFile[1].Open(outputFilePath[1], CFile::modeWrite|CFile::modeCreate);
	msg += outputFile[2].Open(outputFilePath[2], CFile::modeWrite|CFile::modeCreate);
	

	
	if( msg)
	{
		const CStationStatArray& stationArray = tempStat.GetStationStat();
		
		//CMapMakerInput mapInput[12][3];
		for(int i=0; i<stationArray.GetSize(); i++)
		{
			const CStationStat& station = stationArray[i];
			CString name = station.GetName().Left(20);
			name.Replace(" ","_");

			CString line[3];
			line[0].Format("%6d  %-20.20s", i+1, name );
			line[1].Format("%6d  %-20.20s", i+1, name );
			line[2].Format("%6d  %-20.20s", i+1, name );
		
			for(int j=0; j<2; j++)
			{
				for(int m=0; m<12; m++)
				{
					static const short VAR[3] = {TMIN_MN, TMAX_MN, PRCP_TT};
					for(int v=0; v<3; v++)
					{
						CString tmp;
						tmp.Format("%8.2f", station.m_normalData[j==0?1:0][m][VAR[v]]);
						line[v]+=tmp;

						//Xvalid
		//				if( 
		//				CGeoEventPoint pt;
		//				(CLocStation&)pt) = station;
		//				pt.SetEvent(
					}

					//tmp.Format("%8.2f", station.m_normalData[j==0?1:0][m][TMAX_MN]);
					//line[1]+=tmp;

					//tmp.Format("%8.2f", station.m_normalData[j==0?1:0][m][PRCP_TT]);
					//line[2]+=tmp;
			
					
				}
			}

			outputFile[0].WriteString(line[0]+"\n");
			outputFile[1].WriteString(line[1]+"\n");
			outputFile[2].WriteString(line[2]+"\n");

			
		}

		outputFile[0].Close();
		outputFile[1].Close();
		outputFile[2].Close();


		//create Xvalidation map
		
		//mapInput.m_dataTable.Add(

		//CMapMaker mapMaker;
		//mapMaker.Se
		//mapMaker.GenerateXValidation(mapInput, NULL, callback);
	}

	//if( msg)
	//	MakeXValidation();
*/
	return msg;
}

/*ERmsg CAnusplineVsBioSIM::MakeXValidation()
{
	for(int m=0; m<12; m++)

	CString inputFilePath[3];
	CString outputFilePath[3];
	CString path = UtilWin::GetPath( m_outputFilePath);
	CString title = UtilWin::GetFileTitle( m_outputFilePath);
	inputFilePath[0] = path + title + "BioSIMTmin.txt";
	inputFilePath[1] = path + title + "BioSIMTmax.txt";
	inputFilePath[2] = path + title + "BioSIMPrcp.txt";
	outputFilePath[0]= path + title + "BioSIMTminXVal.txt";
	outputFilePath[1]= path + title + "BioSIMTminXVal.txt";
	outputFilePath[2]= path + title + "BioSIMTminXVal.txt";

	
	CStdioFileEx inputFile[3];
	CStdioFileEx outputFile[3];
	msg += inputFile[0].Open(inputFilePath[0], CFile::modeRead);
	msg += inputFile[1].Open(inputFilePath[1], CFile::modeRead);
	msg += inputFile[2].Open(inputFilePath[2], CFile::modeRead);
	msg += outputFile[0].Open(outputFilePath[0], CFile::modeWrite|CFile::modeCreate);
	msg += outputFile[1].Open(outputFilePath[1], CFile::modeWrite|CFile::modeCreate);
	msg += outputFile[2].Open(outputFilePath[2], CFile::modeWrite|CFile::modeCreate);
	
	if( msg)
	{
		inputFile
		while( 
			const CStationStat& station = stationArray[i];
			CString name = station.GetName().Left(20);
			name.Replace(" ","_");

			CString line[3];
			line[0].Format("%6d  %-20.20s", i+1, name );
			line[1].Format("%6d  %-20.20s", i+1, name );
			line[2].Format("%6d  %-20.20s", i+1, name );
		
			for(int j=0; j<2; j++)
			{
				for(int m=0; m<12; m++)
				{
					CString tmp;
					tmp.Format("%8.2f", station.m_normalData[j==0?1:0][m][TMIN_MN]);
					line[0]+=tmp;

					tmp.Format("%8.2f", station.m_normalData[j==0?1:0][m][TMAX_MN]);
					line[1]+=tmp;

					tmp.Format("%8.2f", station.m_normalData[j==0?1:0][m][PRCP_TT]);
					line[2]+=tmp;
					
				}
			}

			outputFile[0].WriteString(line[0]+"\n");
			outputFile[1].WriteString(line[1]+"\n");
			outputFile[2].WriteString(line[2]+"\n");
		}

		inputFile[0].Close();
		inputFile[1].Close();
		inputFile[2].Close();

		outputFile[0].Close();
		outputFile[1].Close();
		outputFile[2].Close();
	}

}
*/
bool CAnusplineVsBioSIM::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CAnusplineVsBioSIM& info = dynamic_cast<const CAnusplineVsBioSIM&>(in);
	return operator==(info);
}

CParameterBase& CAnusplineVsBioSIM::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CAnusplineVsBioSIM& info = dynamic_cast<const CAnusplineVsBioSIM&>(in);
	return operator=(info);
}

//*******************************************************


