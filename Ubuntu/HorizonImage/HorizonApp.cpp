//***********************************************************************
// program to output slope, aspect and sky view factor from DEM
//
//***********************************************************************

#include <boost/timer/timer.hpp>
#include <stdio.h>
#include <iostream>
#include "HorizonImage.h"


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
    CHorizon Horizon;

    ERMsg msg = Horizon.m_options.ParseOption(argc, argv);

    if (!Horizon.m_options.m_bQuiet)
        cout << Horizon.GetDescription() << endl;


    if (msg)
        msg = Horizon.Execute();

    if (!msg)
    {
        PrintMessage(msg);
        return -1;
    }


    timer.stop();

    if (!Horizon.m_options.m_bQuiet)
    {
        Horizon.m_options.PrintTime();
        cout << endl << "Total time = " << SecondToDHMS(timer.elapsed().wall / 1e9) << endl;
    }



    return 0;
}
