//***********************************************************************
#pragma once

#include <boost/dynamic_bitset.hpp>
#include <deque>
#include "geomatic/UtilGDAL.h"
#include "geomatic/LandsatDataset2.h"
#include "Desawtooth.h"

namespace WBSF
{

class CDesawtoothDirect
{
public:

    CDesawtoothDirect(CDesawtoothOption& options) :
        m_options(options)
    {
    }

    ERMsg Execute();

    
    ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& validityDS, CGDALDatasetEx& outputDS);
    void ReadBlock(CGDALDatasetEx& inputDS, CGDALDatasetEx& validityDS, int xBlock, int yBlock, CRasterWindow& bandHolder);
    void ProcessBlock(int xBlock, int yBlock, const CRasterWindow& bandHolder, OutputData& outputData);
    void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData);
    void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

    CDesawtoothOption& m_options;

    static const char* VERSION;
    static const size_t NB_THREAD_PROCESS;
};
}
