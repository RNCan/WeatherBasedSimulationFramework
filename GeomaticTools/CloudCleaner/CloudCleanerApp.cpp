//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "CloudCleaner.h"

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
		CCloudCleaner CloudCleaner;
		ERMsg msg = CloudCleaner.m_options.ParseOption(argc, argv);

		if (!msg || !CloudCleaner.m_options.m_bQuiet)
			cout << CloudCleaner.GetDescription() << endl;


		if (msg)
			msg = CloudCleaner.Execute(); 

		if (!msg)
		{
			PrintMessage(msg);
			return -1;
		}

		timer.Stop();

		if (!CloudCleaner.m_options.m_bQuiet)
			cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

		return 0;
	}
