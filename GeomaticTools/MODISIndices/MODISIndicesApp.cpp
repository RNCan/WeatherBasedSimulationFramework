//***********************************************************************
// program to merge MODIS image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "MODISIndices.h"

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
	WBSF::CMODISIndices MODISIndices;
	ERMsg msg = MODISIndices.m_options.ParseOption(argc, argv);

	if (!msg || !MODISIndices.m_options.m_bQuiet)
		cout << MODISIndices.GetDescription() << endl;


	if( msg )  
		msg = MODISIndices.Execute();

	if( !msg)   
	{
		WBSF::PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!MODISIndices.m_options.m_bQuiet)
		cout << endl << "Total time = " << WBSF::SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}
 