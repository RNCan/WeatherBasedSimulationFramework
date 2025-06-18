//***********************************************************************
// program to merge Landsat image image over a period
//
//***********************************************************************

#include <boost/timer/timer.hpp>
#include <stdio.h>
#include <iostream>
#include "Desawtooth.h"
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

    //static const char* test_SRS = "PROJCS[\"NAD_1983_Canada_Lambert\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101004,AUTHORITY[\"EPSG\",\"7019\"]],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4269\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",-95],PARAMETER[\"standard_parallel_1\",49],PARAMETER[\"standard_parallel_2\",77],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH]]";


    //SRS.SetWellKnownGeogCS("WGS84");
    //OGRSpatialReference SRS2("EPSG:1188");

    //OGRSpatialReference SRS;
    //OGRErr err = SRS.importFromWkt(test_SRS);


    //char test[255] = { 0 };
    //const char* pTest = CPLGetConfigOption("GDAL_DRIVER_PATH", test);
    //const char* pTest2 = CPLGetConfigOption("GDAL_DATA", test);


    //Create a mergeImages object
    CDesawtooth Desawtooth;

    ERMsg msg = Desawtooth.m_options.ParseOption(argc, argv);

    if (!Desawtooth.m_options.m_bQuiet)
        cout << Desawtooth.GetDescription() << endl;


    if (msg)
        msg = Desawtooth.Execute();

    if (!msg)
    {
        PrintMessage(msg);
        return -1;
    }


    timer.stop();

    if (!Desawtooth.m_options.m_bQuiet)
    {
        Desawtooth.m_options.PrintTime();
        cout << endl << "Total time = " << SecondToDHMS(timer.elapsed().wall / 1e9) << endl;
    }



    return 0;
}
