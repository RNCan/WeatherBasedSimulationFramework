//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "ExtractGribs.h"



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
	WBSF::CExtractGribs ExtractGribs;
	ERMsg msg = ExtractGribs.m_options.ParseOption(argc, argv);

	if (!msg || !ExtractGribs.m_options.m_bQuiet)
		cout << ExtractGribs.GetDescription() << endl;


	if( msg )  
		msg = ExtractGribs.Execute();

	if( !msg)   
	{
		WBSF::PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!ExtractGribs.m_options.m_bQuiet)
		cout << endl << "Total time = " << WBSF::SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}
 