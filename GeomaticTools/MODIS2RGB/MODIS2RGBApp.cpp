//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "MODIS2RGB.h"

using namespace std;

//***********************************************************************
//									 
//	Main                                                             
//						 	 		 
//***********************************************************************
int _tmain(int argc, _TCHAR* argv[])
{
	CTimer timer(true);

	//Create object
	WBSF::CMODIS2RGB MODIS2RGB;
	ERMsg msg = MODIS2RGB.m_options.ParseOption(argc, argv);

	if (!msg || !MODIS2RGB.m_options.m_bQuiet)
		cout << MODIS2RGB.GetDescription() << endl;


	if( msg )  
		msg = MODIS2RGB.Execute();

	if( !msg)   
	{
		WBSF::PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!MODIS2RGB.m_options.m_bQuiet)
		cout << endl << "Total time = " << WBSF::SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}
 