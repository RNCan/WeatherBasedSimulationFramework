//***********************************************************************
#pragma once

#include <boost/dynamic_bitset.hpp>
#include <deque>
#include "geomatic/UtilGDAL.h"
#include "geomatic/LandsatDataset2.h"


namespace WBSF
{

class CDesawtoothOption : public CBaseOptions
{
public:

    enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };


    CDesawtoothOption();
    virtual ERMsg ParseOption(int argc, char* argv[]);
    virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);



    //double m_pval;
    //double m_recovery_threshold;
    //double m_distweightfactor;
    //size_t m_vertexcountovershoot;
    //double m_bestmodelproportion;
    size_t m_minneeded;
    //size_t m_max_segments;
    double m_desawtooth_val;
    //size_t m_fit_method;
    int m_modifier;

    //size_t max_vertices()const
    //{
    //    return m_max_segments + 1;
    //}
    //set both method;
   // size_t fit_method;
    Landsat2::TIndices m_indice;
    double m_rings;
    std::string m_CloudsMask;

    int m_firstYear;
    //bool m_bBreaks;
//    bool m_bDesawtooth;
    bool m_bBackwardFill;
    bool m_bForwardFill;

};

typedef std::deque < std::vector< Landsat2::LandsatDataType>> OutputData;
typedef std::deque < std::vector< Landsat2::LandsatDataType>> BreaksData;

typedef std::pair<double, size_t> NBRPair;
typedef std::vector<NBRPair> NBRVector;

class CDesawtooth
{
public:

    ERMsg Execute();

    std::string GetDescription()
    {
        return  std::string("DesawtoothImages version ") + VERSION + " (" + __DATE__ + ")";
    }

    ERMsg OpenAll(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& validityDS, Landsat2::CLandsatDataset& outputDS);
    void ReadBlock(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& validityDS, int xBlock, int yBlock, Landsat2::CLandsatWindow& bandHolder);
    void ProcessBlock(int xBlock, int yBlock, const Landsat2::CLandsatWindow& bandHolder, OutputData& outputData);
    void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData);
    void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

    CDesawtoothOption m_options;

    static const char* VERSION;
    static const size_t NB_THREAD_PROCESS;
};
}
