// xml2csv.cpp : Defines the entry point for the console application.
//

#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libxls/xls.h"

#include <iostream>
#include <fstream>
#include <iomanip>



static char  stringSeparator = '\"';
static char *lineSeparator = "\n";
static char *fieldSeparator = ","; 
static char *encoding = "UTF-8";



using namespace std;
using namespace xls;

extern "C" int xls_debug;


void ExcelSerialDateToDMY(double fSerialDate, int &nYear, int &nMonth, int &nDay, int &nHour, int &nMinute )
{

	int nSerialDate = int(fSerialDate);

	// Excel/Lotus 123 have a bug with 29-02-1900. 1900 is not a
	// leap year, but Excel/Lotus 123 think it is...
	if (nSerialDate == 60)
	{
		nDay = 29;
		nMonth = 2;
		nYear = 1900;

		return;
	}
	else if (nSerialDate < 60)
	{
		// Because of the 29-02-1900 bug, any serial date 
		// under 60 is one off... Compensate.
		nSerialDate++;
	}

	// Modified Julian to DMY calculation with an addition of 2415019
	int l = nSerialDate + 68569 + 2415019;
	int n = int((4 * l) / 146097);
	l = l - int((146097 * n + 3) / 4);
	int i = int((4000 * (l + 1)) / 1461001);
	l = l - int((1461 * i) / 4) + 31;
	int j = int((80 * l) / 2447);
	nDay = l - int((2447 * j) / 80);
	l = int(j / 11);
	nMonth = j + 2 - (12 * l);
	nYear = 100 * (n - 49) + i + l;


	fSerialDate -= nSerialDate;
	fSerialDate *= 24;
	nHour = int(fSerialDate);
	fSerialDate -= nHour;
	fSerialDate *= 60;
	nMinute = int(fSerialDate+0.5);
	if (nMinute == 60)
	{
		nHour++;
		nMinute = 0;
	}
	
}

int DMYToExcelSerialDate(int nDay, int nMonth, int nYear)
{
	// Excel/Lotus 123 have a bug with 29-02-1900. 1900 is not a
	// leap year, but Excel/Lotus 123 think it is...
	if (nDay == 29 && nMonth == 02 && nYear == 1900)
		return 60;

	// DMY to Modified Julian calculatie with an extra substraction of 2415019.
	long nSerialDate =
		int((1461 * (nYear + 4800 + int((nMonth - 14) / 12))) / 4) +
		int((367 * (nMonth - 2 - 12 * ((nMonth - 14) / 12))) / 12) -
		int((3 * (int((nYear + 4900 + int((nMonth - 14) / 12)) / 100))) / 4) +
		nDay - 2415019 - 32075;

	if (nSerialDate < 60)
	{
		// Because of the 29-02-1900 bug, any serial date 
		// under 60 is one off... Compensate.
		nSerialDate--;
	}

	return (int)nSerialDate;
}

int _tmain(int argc, _TCHAR* argv[])
{
	xls_debug = 0;

	if (argc != 3)
	{
		std::cout << "usage : xml2csv.exe file.xml csvFile.csv" << std::endl;
		return -1;
	} 

	char filePath[_MAX_PATH] = { 0 };
	strcpy(filePath, argv[1]);
	//wcstombs(filePath, argv[1], wcslen(argv[1]) + 1);
	xlsWorkBook* pWB = xls_open(filePath, encoding);
	

	if (pWB) 
	{
		if (pWB->sheets.count == 1)
		{
			ofstream outfile;
			outfile.open(argv[2]);
			if (outfile.is_open())
			{
				// open and parse the sheet
				xlsWorkSheet* pWS = xls_getWorkSheet(pWB, 0);
				xls_parseWorkSheet(pWS);

				// process all rows of the sheet
				for (size_t j = 0; j <= (size_t)pWS->rows.lastrow; ++j) 
				{
					WORD cellRow = (WORD)j;
					xlsRow* row = xls_row(pWS, cellRow);
					
					for (WORD cellCol = 0; cellCol <= pWS->rows.lastcol; cellCol++) 
					{
						xlsCell *cell = xls_cell(pWS, cellRow, cellCol);
						
						if (cell && cell->xf != 0)
						{
							if (cellCol != 0)
								outfile << fieldSeparator;

							_ASSERTE(cell->xf < pWB->xfs.count);
							WORD format = pWB->xfs.xf[cell->xf].format;
							//_ASSERTE(format < pWB->formats.count);
							//WORD index = pWB->formats.format[format].index;
							//BYTE *value = pWB->formats.format[format].value;

							
							//if (format == 165) // its a date
							//{
							//	int nYear=0;
							//	int nMonth = 0;
							//	int nDay=0;
							//	int nHour = 0;
							//	int nMinute = 0;
							//	
							//	ExcelSerialDateToDMY(cell->d, nYear, nMonth, nDay, nHour, nMinute);
							//	outfile << nYear << "-" << setfill('0') << setw(2) << nMonth << "-" << setfill('0') << setw(2) << nDay << " " << setfill('0') << setw(2) << nHour << ":" << setfill('0') << setw(2) << nMinute;
							//}
							//else 
							if (cell->str != nullptr) // its a string
							{
								outfile << cell->str;
							}
							else
							{
								outfile << cell->d;
							}
							
						}
					}

					outfile << endl;
				}

				xls_close_WS(pWS);
				outfile.close();
			}
			else
			{
				std::cout << "unable to open output file: " << argv[2] << std::endl;
			}

		}
	}
	else
	{
		std::cout << "unable to open input file: " << argv[1] << std::endl;
	}


	return 0;
}

//
//int _tmain(int argc, _TCHAR* argv[])
//{
//
//	if (argc != 3)
//	{
//		std::cout << "usage : xml2csv.exe file.xml csvFile.csv" << std::endl;
//		return -1;
//	}
//
//	BasicExcel ExecelFile;
//
//	if (ExecelFile.Load(argv[1]))
//	{
//		if (ExecelFile.GetTotalWorkSheets() == 1)
//		{
//			ofstream outfile;
//			outfile.open(argv[2]);
//			if (outfile.is_open())
//			{
//
//				//outfile.close();
//
//				//string filePathIn = 
//				// Create a new workbook by reading sample1.xlsx in the current directory.
//				//xlnt::workbook wb;
//				//wb.load(infile);
//
//				BasicExcelWorksheet* pWorksheet = ExecelFile.GetWorksheet(size_t(0));
//
//
//				pWorksheet->Print(outfile);
//				outfile.close();
//			}
//			else
//			{
//				std::cout << "unable to open output file: " << argv[2] << std::endl;
//			}
//
//		}
//	}
//	else
//	{
//		std::cout << "unable to open input file: " << argv[1] << std::endl;
//	}
//
//
//	return 0;
//}
