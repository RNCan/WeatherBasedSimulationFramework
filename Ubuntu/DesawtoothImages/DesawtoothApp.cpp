//***********************************************************************
// program to merge Landsat image image over a period
//
//***********************************************************************

#include <boost/timer/timer.hpp>
#include <stdio.h>
#include <iostream>
#include "geomatic/LandTrendCore.h"


#include "Desawtooth.h"
#include "DesawtoothDirect.h"

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
	CDesawtooth desawtooth;

	ERMsg msg = desawtooth.m_options.ParseOption(argc, argv);

	if (!desawtooth.m_options.m_bQuiet)
		cout << desawtooth.GetDescription() << endl;


	if (msg)
	{
		if (!desawtooth.m_options.m_bDirect)
		{
			msg = desawtooth.Execute();
		}
		else
		{
			CDesawtoothDirect desawtoothDirect(desawtooth.m_options);
			msg = desawtoothDirect.Execute();
		}
	}

	if (!msg)
	{
		PrintMessage(msg);
		return -1;
	}


	timer.stop();

	if (!desawtooth.m_options.m_bQuiet)
	{
		desawtooth.m_options.PrintTime();
		cout << endl << "Total time = " << SecondToDHMS(timer.elapsed().wall / 1e9) << endl;
	}



	return 0;
}
