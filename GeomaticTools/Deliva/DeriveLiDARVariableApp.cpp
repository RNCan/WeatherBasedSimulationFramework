//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
#include "stdafx.h"
#include <iostream>
#include "DeriveLiDARVariable.h"

using namespace std;
using namespace WBSF;

int _tmain(int argc, _TCHAR* argv[])
{
	std::locale::global(std::locale(""));

	CTimer timer(true);

	CDeriveLiDARVariable DeriveLiDARVariable;
	ERMsg msg = DeriveLiDARVariable.m_optionsIn.ParseOptions(argc, argv);

	if( !msg || !DeriveLiDARVariable.m_optionsIn.m_bQuiet )
		cout << DeriveLiDARVariable.GetDescription() << endl;


	if( msg )  
		msg = DeriveLiDARVariable.Execute();

	if( !msg)  
	{
		PrintMessage(msg); 
		return -1;
	}

	timer.Stop();

	if( !DeriveLiDARVariable.m_optionsIn.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}



