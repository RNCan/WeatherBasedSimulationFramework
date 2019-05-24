//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "ConvertNetCDF2GeoTIFF.h"



using namespace std;

//***********************************************************************
//									 
//	Main                                                             
//						 	 		 
//***********************************************************************
int _tmain(int argc, _TCHAR* argv[])
{
	CTimer timer(true);

	//Create a mergeImages object
	WBSF::CConvertNetCDF2GeoTIFF ConvertNetCDF2GeoTIFF;
	ERMsg msg = ConvertNetCDF2GeoTIFF.m_options.ParseOption(argc, argv);

	if (!msg || !ConvertNetCDF2GeoTIFF.m_options.m_bQuiet)
		cout << ConvertNetCDF2GeoTIFF.GetDescription() << endl;


	if( msg )  
		msg = ConvertNetCDF2GeoTIFF.Execute();

	if( !msg)   
	{
		WBSF::PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!ConvertNetCDF2GeoTIFF.m_options.m_bQuiet)
		cout << endl << "Total time = " << WBSF::SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}
 