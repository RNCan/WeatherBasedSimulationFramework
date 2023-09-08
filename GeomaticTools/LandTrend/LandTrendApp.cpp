//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "LandTrend.h"
#include "LandTrendCore.h"
#include <stdio.h>
#include <tchar.h>
#include <iostream>



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
	CLandTrend LandTrend;

	ERMsg msg = LandTrend.m_options.ParseOption(argc, argv);

	if (!msg || !LandTrend.m_options.m_bQuiet)
		cout << LandTrend.GetDescription() << endl;


	if (msg)
		msg = LandTrend.Execute();

	if (!msg)
	{
		PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!LandTrend.m_options.m_bQuiet)
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}
