//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "BreaksImage.h"

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

	//Create a BreaksImages object
	CBreaksImage BreaksImage;
	ERMsg msg = BreaksImage.m_options.ParseOption(argc, argv);

	if( !msg || !BreaksImage.m_options.m_bQuiet )
		cout << BreaksImage.GetDescription() << endl;


	if( msg )  
		msg = BreaksImage.Execute();

	if( !msg)  
	{
		PrintMessage(msg);  
		return -1;
	}

	timer.Stop();

	if( !BreaksImage.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}
 