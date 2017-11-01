//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "CloudAnalyser.h"

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
		CCloudAnalyser cloudAnalyser;
		ERMsg msg = cloudAnalyser.m_options.ParseOption(argc, argv);

		if (!msg || !cloudAnalyser.m_options.m_bQuiet)
			cout << cloudAnalyser.GetDescription() << endl;


		if (msg)
			msg = cloudAnalyser.Execute(); 

		if (!msg)
		{
			PrintMessage(msg);
			return -1;
		}

		timer.Stop();

		if (!cloudAnalyser.m_options.m_bQuiet)
			cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

		return 0;
	}
