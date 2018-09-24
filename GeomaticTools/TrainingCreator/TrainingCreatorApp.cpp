//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
#include "stdafx.h"
#include <iostream>
#include "TrainingCreator.h"

using namespace std;
using namespace WBSF;

int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale(""));

	CTimer timer(true);

	CTrainingCreator TrainingCreator;
	ERMsg msg = TrainingCreator.m_options.ParseOptions(argc, argv);

	if( !msg || !TrainingCreator.m_options.m_bQuiet )
		cout << TrainingCreator.GetDescription() << endl;


	if( msg )  
		msg = TrainingCreator.Execute();

	if( !msg)  
	{
		PrintMessage(msg); 
		return -1;
	}

	timer.Stop();

	if( !TrainingCreator.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}



