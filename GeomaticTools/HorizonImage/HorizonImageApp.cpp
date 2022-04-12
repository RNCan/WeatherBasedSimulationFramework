//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "HorizonImage.h"

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

	//Create a HorizonImages object
	CHorizonImage HorizonImage;
	ERMsg msg = HorizonImage.m_options.ParseOption(argc, argv);

	if( !msg || !HorizonImage.m_options.m_bQuiet )
		cout << HorizonImage.GetDescription() << endl;


	if( msg )  
		msg = HorizonImage.Execute();

	if( !msg)  
	{
		PrintMessage(msg);  
		return -1;
	}

	timer.Stop();

	if( !HorizonImage.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}
 