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



    size_t m_minneeded;
    double m_desawtooth_val;
    bool m_year_by_year;
    bool m_bDirect;

    Landsat2::TIndices m_indice;
    double m_rings;
    std::string m_CloudsMask;

    int m_firstYear;
    bool m_bBackwardFill;
    bool m_bForwardFill;

};

typedef std::deque < std::vector<double>> OutputData;

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

    ERMsg OpenAll(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& validityDS, CGDALDatasetEx& outputDS);
    void ReadBlock(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& validityDS, int xBlock, int yBlock, Landsat2::CLandsatWindow& bandHolder);
    void ProcessBlock(int xBlock, int yBlock, const Landsat2::CLandsatWindow& bandHolder, OutputData& outputData);
    void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData);
    void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

    CDesawtoothOption m_options;

    static const char* VERSION;
    static const size_t NB_THREAD_PROCESS;
};
}
