#include "StdAfx.h"
#include ".\Stardex.h"
#include "UtilTime.h"


ERMsg CStardex::ExportFile(const CDailyStation& dailyStation)
{
	ERMsg msg;

	CStdioFile file;
	msg = UtilWin::OpenFile(file, m_filePath, CFile::modeWrite|CFile::modeCreate);

	if( !msg)
		return msg;

	CString tmp;
	tmp.Format("%lf\t%lf\n", dailyStation.GetLatDec(), dailyStation.GetLonDec() );
	file.WriteString(tmp);

	for(int y=0; y<dailyStation.GetNbYear(); y++)
	{
		const CDailyYear& data = dailyStation(y);
		

		int jd=0;
		for(int m=0; m<12; m++)
		{
			for(int d=0; d<CFL::GetNbDayPerMonth(m); d++)
			{
				int year = data.GetYear()>0?data.GetYear():1990+int(data.GetYear()/10);

				CString tmp;
				tmp.Format("%d\t%d\t%d\t%.1lf\t%.1lf\t%.1lf\t%.1lf\n", year, m+1, d+1, data[jd].m_min, data[jd].m_max, (data[jd].m_min+data[jd].m_max)/2, data[jd].m_ppt);
				file.WriteString(tmp);

				jd++;
			}
		}
	}
	
	file.Close();

	return msg;
}

void CStardex::Execute()
{
	CString command;
	command.Format( "Stardex.exe \"%s\"", m_filePath);

	UtilWin::WinExecWait(command);
}

int GetType(const CString& head)
{
	int type = -1;
	static const char* NAME[5] = {"DJF", "MAM", "JJA", "SON", "ANN"};
	for(int i=0; i<5; i++)
	{
		if( head == NAME[i])
		{
			type=i;
			break;
		}
	}

	return type;
}

ERMsg CStardex::ImportResult(float result1[4][32][55], float result2[32][58])
{
	ERMsg msg;

	CStdioFile file;
	msg = UtilWin::OpenFile(file, GetOutputFilePath(), CFile::modeRead);

	if( !msg)
		return msg;


//	for(int k=0; k<4; k++)
//		result1[k].SetSize(32*4, 56);
//	result2.SetSize(32, 58);

	CString head1;
	file.ReadString(head1);
	CString head2;
	file.ReadString(head2);


	CString line;
	for(int i=0; i<32; i++)
	{
		file.ReadString(line);

		int col1 = 0;
		int col2 = 0;
		//int type = 0;
		int nbVal=0;
		int posHeadBegin1=0;
		int posHeadEnd1=0;
		int posHeadBegin2=0;
		int posHeadEnd2=0;
		int posBegin = 0;
		int posEnd = 0;
		//int year = 0;
		for(int j=0; j<274&&posEnd!=-1; j++) 
		{
			posHeadEnd1 = head1.Find(';', posHeadBegin1); 
			CString test = posHeadEnd1==-1?head1.Mid(posHeadBegin1):head1.Mid(posHeadBegin1, posHeadEnd1-posHeadBegin1);

			posHeadEnd2 = head2.Find(';', posHeadBegin2);
			CString head = posHeadEnd2==-1?head2.Mid(posHeadBegin2):head2.Mid(posHeadBegin2, posHeadEnd2-posHeadBegin2);
			int type=GetType(head);
			if(j==273)
			{
				int i;
				i=9;
			}
			//if( head == "DJF")
			//	type = 0;
			//else type++;

			posEnd = line.Find(';', posBegin);
			CString str = posEnd==-1?line.Mid(posBegin):line.Mid(posBegin, posEnd-posBegin);
			float value = -999;
			if( !str.IsEmpty() )
				value = (float)atof(str);
			
			if(type == -1)
			{
				ASSERT(j==0);
				if( str == "trend")
					value = 1;
				else if( str == "p <") 
					value = 2;
				//year
				for(int k=0; k<4; k++)
				{
					result1[k][i][0]=value;
				}
				result2[i][0]=value;
			}
			else if( type>=0 && type<4)
			{
				result1[type][i][col1/4+1] = value;
				col1++;
			}
			else 
			{
				result2[i][col2+1] = value;
				col2++;
			}
			
			posBegin=posEnd+1;
			posHeadBegin1=posHeadEnd1+1;
			posHeadBegin2=posHeadEnd2+1;
			nbVal++;
		}
	}

	return msg;
}

void CStardex::CleanFiles()
{
	TRY
	if( UtilWin::FileExist(m_filePath) )
		CFile::Remove(m_filePath);
	if( UtilWin::FileExist(GetOutputFilePath()) )
		CFile::Remove(GetOutputFilePath());

	CATCH_ALL(e)
	END_CATCH_ALL
}