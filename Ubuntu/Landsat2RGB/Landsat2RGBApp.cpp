//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************


#include <iostream>
#include <boost/timer/timer.hpp>

#include "Landsat2RGB.h"



using namespace std;
using namespace WBSF;

//***********************************************************************
//									 
//	Main                                                             
//						 	 		 
//***********************************************************************
int main(int argc, char* argv[])
{
	boost::timer::cpu_timer timer;
	timer.start();

	//Create a mergeImages object
	WBSF::CLandsat2RGB landsat2RGB;
	ERMsg msg = landsat2RGB.m_options.ParseOption(argc, argv);

	if (!msg || !landsat2RGB.m_options.m_bQuiet)
		cout << landsat2RGB.GetDescription() << endl;


	if( msg )  
		msg = landsat2RGB.Execute();

	if( !msg)   
	{
		WBSF::PrintMessage(msg);
		return -1;
	}

	timer.stop();

	

	if (!landsat2RGB.m_options.m_bQuiet)
	{
		landsat2RGB.m_options.PrintTime();
		cout << endl << "Total time = " << SecondToDHMS(timer.elapsed().wall / 1e9) << endl;
	}

	return 0;
}
 