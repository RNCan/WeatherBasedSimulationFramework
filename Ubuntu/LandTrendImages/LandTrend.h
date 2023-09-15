//***********************************************************************
#pragma once

#include <boost/dynamic_bitset.hpp>
#include <deque>
#include "geomatic/UtilGDAL.h"
#include "geomatic/LandsatDataset2.h"


namespace WBSF
{

class CLandTrendOption : public CBaseOptions
{
public:

    enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
    //enum TDebug {D_NB_NBR, D_};

    CLandTrendOption();
    virtual ERMsg ParseOption(int argc, char* argv[]);
    virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);



    double m_pval;
    double m_recovery_threshold;
    double m_distweightfactor;
    size_t m_vertexcountovershoot;
    double m_bestmodelproportion;
    size_t m_minneeded;
    size_t m_max_segments;
    //int m_background;
    //int m_modifier;//-1 for NBR
    double m_desawtooth_val;
    size_t m_fit_method;
    int m_modifier;

    size_t max_vertices()const
    {
        return m_max_segments + 1;
    }
    //set both method;
    size_t fit_method;
    Landsat2::TIndices m_indice;
    double m_rings;
    std::string m_ValidityMask;

    //double m_RMSEThreshold;
    //size_t m_maxLayers;
    int m_firstYear;
    //bool m_bExportBreaks;
    bool m_bDebug;
};

typedef std::deque < std::vector< Landsat2::LandsatDataType>> OutputData;
typedef std::deque < std::vector< Landsat2::LandsatDataType>> DebugData;

typedef std::pair<double, size_t> NBRPair;
typedef std::vector<NBRPair> NBRVector;

class CLandTrend
{
public:

    ERMsg Execute();

    std::string GetDescription()
    {
        return  std::string("LandTrendImages version ") + VERSION + " (" + __DATE__ + ")";
    }

    ERMsg OpenAll(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& validityDS, Landsat2::CLandsatDataset& outputDS, CGDALDatasetEx& debugDS);
    void ReadBlock(Landsat2::CLandsatDataset& inputDS, int xBlock, int yBlock, Landsat2::CLandsatWindow& bandHolder);
    void ProcessBlock(int xBlock, int yBlock, const Landsat2::CLandsatWindow& bandHolder, const std::deque<boost::dynamic_bitset<>>& validity, OutputData& outputData, DebugData& debugData);
    void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, OutputData& outputData, DebugData& debugData);
    void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);

    CLandTrendOption m_options;

    static const char* VERSION;
    static const size_t NB_THREAD_PROCESS;
};
}
