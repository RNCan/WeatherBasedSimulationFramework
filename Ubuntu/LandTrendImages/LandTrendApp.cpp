//***********************************************************************
// program to merge Landsat image image over a period
//
//***********************************************************************

#include <boost/timer/timer.hpp>
#include <stdio.h>
#include <iostream>
#include "LandTrend.h"
#include "geomatic/LandTrendCore.h"


using namespace std;
using namespace WBSF;


//***********************************************************************
//
//	Main
//
//***********************************************************************
int main(int argc, char* argv[])
{
    //boost::timer::auto_cpu_timer auto_timer(1);
    boost::timer::cpu_timer timer;


    timer.start();

   
    //Create a mergeImages object
    CLandTrend LandTrend;

    ERMsg msg = LandTrend.m_options.ParseOption(argc, argv);

    if (!LandTrend.m_options.m_bQuiet)
        cout << LandTrend.GetDescription() << endl;


    if (msg)
        msg = LandTrend.Execute();

    if (!msg)
    {
        PrintMessage(msg);
        return -1;
    }


    timer.stop();

    if (!LandTrend.m_options.m_bQuiet)
    {
        LandTrend.m_options.PrintTime();
        cout << endl << "Total time = " << SecondToDHMS(timer.elapsed().wall / 1e9) << endl;
    }



    return 0;
}
