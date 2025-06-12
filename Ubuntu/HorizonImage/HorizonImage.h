//***********************************************************************
#pragma once

#include <boost/dynamic_bitset.hpp>
#include <deque>
#include "geomatic/UtilGDAL.h"
#include "geomatic/GDALDatasetEx.h"
//#include "geomatic/LandsatDataset2.h"


namespace WBSF
{

class CHorizonOption : public CBaseOptions
{
public:

    enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };


    CHorizonOption();
    virtual ERMsg ParseOption(int argc, char* argv[]);
    virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

    double m_rings;
    double m_dist_search;
    std::string m_ray_algorithm;
    std::string m_geom_type;
    std::string m_ellps;//Earth's surface approximation (sphere, GRS80 or WGS84)
    int m_azim_num; // number of azimuth sampling directions[-]
    double m_hori_acc;  // [degree]
    double m_ray_org_elev;
    //double m_hori_fill; 
    double m_elev_ang_low_lim;
    double m_elev_ang_up_lim;


};

typedef std::deque < std::vector< float>> OutputData;
typedef std::deque < std::vector< float>> BreaksData;

typedef std::pair<double, size_t> NBRPair;
typedef std::vector<NBRPair> NBRVector;

class CHorizon
{
public:

    ERMsg Execute();

    std::string GetDescription()
    {
        return  std::string("HorizonImages version ") + VERSION + " (" + __DATE__ + ")";
    }

    ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS,  CGDALDatasetEx& outputDS);
    void ReadBlock(CGDALDatasetEx& inputDS, int xBlock, int yBlock, CRasterWindow& bandHolder);
    void ProcessBlock(int xBlock, int yBlock, const CRasterWindow& bandHolder, OutputData& outputData);
    void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData);
    void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

    CHorizonOption m_options;

    static const char* VERSION;
    static const size_t NB_THREAD_PROCESS;
};
}
