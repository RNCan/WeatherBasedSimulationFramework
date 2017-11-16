//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "MedianImage.h"

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

	//Create a medianImages object
	CMedianImage medianImage;
	ERMsg msg = medianImage.m_options.ParseOption(argc, argv);

	if( !msg || !medianImage.m_options.m_bQuiet )
		cout << medianImage.GetDescription() << endl;


	if( msg )  
		msg = medianImage.Execute();

	if( !msg)  
	{
		PrintMessage(msg);  
		return -1;
	}

	timer.Stop();

	if( !medianImage.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}
 