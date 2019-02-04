//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "FireSeverity.h"

using namespace std;
using namespace WBSF;


	//***********************************************************************
	//									 
	//	Main                                                             
	//						 	 		 
	//***********************************************************************
	int _tmain(int argc, _TCHAR* argv[])
	{
		CTimer timer(true);

		//Create a mergeImages object
		CFireSeverity FireSeverity;
		ERMsg msg = FireSeverity.m_options.ParseOption(argc, argv);

		if (!msg || !FireSeverity.m_options.m_bQuiet)
			cout << FireSeverity.GetDescription() << endl;

		if (msg)
			msg = FireSeverity.Execute(); 

		if (!msg)
		{
			PrintMessage(msg);
			return -1;
		}

		timer.Stop();

		if (!FireSeverity.m_options.m_bQuiet)
			cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

		return 0;
	}
