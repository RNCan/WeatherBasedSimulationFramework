//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "MODISBuildVRT.h"

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
	WBSF::CMODISBuildVRT HDF_2_GEOTIFF;
	ERMsg msg = HDF_2_GEOTIFF.m_options.ParseOption(argc, argv);

	if (!msg || !HDF_2_GEOTIFF.m_options.m_bQuiet)
		cout << HDF_2_GEOTIFF.GetDescription() << endl;


	if( msg )  
		msg = HDF_2_GEOTIFF.Execute();

	if( !msg)   
	{
		WBSF::PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!HDF_2_GEOTIFF.m_options.m_bQuiet)
		cout << endl << "Total time = " << WBSF::SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}
 