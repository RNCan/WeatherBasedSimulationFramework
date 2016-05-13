//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************

#include "stdafx.h"
#include <iostream>
#include "MergeImages.h"

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
		CMergeImages mergeImage;
		ERMsg msg = mergeImage.m_options.ParseOption(argc, argv);

		if (!msg || !mergeImage.m_options.m_bQuiet)
			cout << mergeImage.GetDescription() << endl;


		if (msg)
			msg = mergeImage.Execute();

		if (!msg)
		{
			PrintMessage(msg);
			return -1;
		}

		timer.Stop();

		if (!mergeImage.m_options.m_bQuiet)
			cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

		return 0;
	}
