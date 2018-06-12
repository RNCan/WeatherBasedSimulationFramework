//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "CloudCleanerI.h"

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
		CCloudCleanerI CloudCleanerI;
		ERMsg msg = CloudCleanerI.m_options.ParseOption(argc, argv);

		if (!msg || !CloudCleanerI.m_options.m_bQuiet)
			cout << CloudCleanerI.GetDescription() << endl;

		if (msg)
			msg = CloudCleanerI.Execute(); 

		if (!msg)
		{
			PrintMessage(msg);
			return -1;
		}

		timer.Stop();

		if (!CloudCleanerI.m_options.m_bQuiet)
			cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

		return 0;
	}
