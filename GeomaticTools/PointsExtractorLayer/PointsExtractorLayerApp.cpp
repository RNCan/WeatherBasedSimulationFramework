//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
#include "stdafx.h"
#include <iostream>
#include "PointsExtractorLayer.h"

using namespace std;
using namespace WBSF;

int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale(""));

	CTimer timer(true);

	CPointsExtractor pointsExtractor;
	ERMsg msg = pointsExtractor.m_options.ParseOptions(argc, argv);

	if( !msg || !pointsExtractor.m_options.m_bQuiet )
		cout << pointsExtractor.GetDescription() << endl;


	if( msg )  
		msg = pointsExtractor.Execute();

	if( !msg)  
	{
		PrintMessage(msg); 
		return -1;
	}

	timer.Stop();

	if( !pointsExtractor.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}



