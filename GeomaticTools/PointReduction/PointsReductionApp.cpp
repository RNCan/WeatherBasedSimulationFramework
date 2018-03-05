//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
#include "stdafx.h"
#include <iostream>
#include "PointsReduction.h"

using namespace std;
using namespace WBSF;

int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale(""));

	CTimer timer(true);

	CPointsReduction PointsReduction;
	ERMsg msg = PointsReduction.m_options.ParseOptions(argc, argv);

	if( !msg || !PointsReduction.m_options.m_bQuiet )
		cout << PointsReduction.GetDescription() << endl;


	if( msg )  
		msg = PointsReduction.Execute();

	if( !msg)  
	{
		PrintMessage(msg); 
		return -1;
	}

	timer.Stop();

	if( !PointsReduction.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}



