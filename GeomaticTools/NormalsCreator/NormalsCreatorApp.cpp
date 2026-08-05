//***********************************************************************
// program to extract points from image 
//***********************************************************************
#include "stdafx.h"
#include <iostream>
#include "NormalsCreator.h"

using namespace std;
using namespace WBSF;

int _tmain(int argc, _TCHAR* argv[])
{
	//std::locale::global(std::locale(""));

	//init gdal
	WBSF::RegisterGDAL();

	CTimer timer(true);

	CNormalsCreator NormalsCreator;
	ERMsg msg = NormalsCreator.m_options.ParseOptions(argc, argv);

	if( !msg || !NormalsCreator.m_options.m_bQuiet )
		cout << NormalsCreator.GetDescription() << endl;


	if( msg )  
		msg = NormalsCreator.Execute();

	if( !msg)  
	{
		PrintMessage(msg); 
		return -1;
	}

	timer.Stop();

	if( !NormalsCreator.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	return 0;
}



