//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "Landsat2RGB.h"

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
	WBSF::CLandsat2RGB landsatWarp;
	ERMsg msg = landsatWarp.m_options.ParseOption(argc, argv);

	if (!msg || !landsatWarp.m_options.m_bQuiet)
		cout << landsatWarp.GetDescription() << endl;


	if( msg )  
		msg = landsatWarp.Execute();

	if( !msg)   
	{
		WBSF::PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!landsatWarp.m_options.m_bQuiet)
		cout << endl << "Total time = " << WBSF::SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}
 