//***********************************************************************
#pragma once

#include <boost/dynamic_bitset.hpp>
#include <deque>
#include "geomatic/UtilGDAL.h"
#include "geomatic/LandsatDataset2.h"
#include "geomatic/LandTrendUtil.h"

namespace WBSF
{

class CLandTrendOption : public CBaseOptions
{
public:


    enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
    
    

    CLandTrendOption();
    virtual ERMsg ParseOption(int argc, char* argv[]);
    virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);



    double m_pval;
    double m_recovery_threshold;
    double m_distweightfactor;//0 = take recovery near edge. 2 = remove recovery near edge.
    size_t m_vertexcountovershoot;
    double m_bestmodelproportion;
    size_t m_minneeded;
    size_t m_max_segments;
    double m_desawtooth_val;
    LTR::TFitMethod m_fit_method;
    LTR::TStatistic m_stat;
    LTR::TPickBestPriority m_priority;
    int m_modifier;

    size_t max_vertices()const
    {
        return m_max_segments + 1;
    }
    //set both method;
    Landsat2::TIndices m_indice;
    bool m_bDirect;
    std::string m_indices_file_path;
    bool m_b_median_indice;
    double m_rings_indice;
    bool m_b_median_interpol;
    double m_rings_interpol;
    std::string m_CloudsMask;

    int m_firstYear;
    bool m_bBreaks;

    //bool m_b_extract_point;
    std::vector<CGeoPoint> m_extract_points;
    

    
    //bool m_bBackwardFill;
    //bool m_bForwardFill;
    bool m_bFillMissing;
    //bool m_bWithPrevious;

};

typedef std::deque < std::vector< Landsat2::LandsatDataType>> OutputData;
typedef std::deque < std::vector< Landsat2::LandsatDataType>> BreaksData;

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

    ERMsg OpenAll(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& indicesDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& validityDS, Landsat2::CLandsatDataset& outputDS, CGDALDatasetEx& breaksDS);
    void ReadBlock(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& indicesDS, CGDALDatasetEx& cloudsDS, int xBlock, int yBlock, Landsat2::CLandsatWindow& block_data, CRasterWindow& indices);
    void ProcessBlock(int xBlock, int yBlock, const Landsat2::CLandsatWindow& block_data, CRasterWindow& indices, OutputData& outputData, BreaksData& breaksData);
    void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& breaksDS, OutputData& outputData, BreaksData& breaksData);
    void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& indicesDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& breaksDS);

    CLandTrendOption m_options;
    //PE_OUTPUT_INDICE2 = PE_OUTPUT_INDICE1 + 1, PE_OUTPUT_INDICE3 = PE_OUTPUT_INDICE2 + 1,
    enum TPointExtract { PE_INPUT_BANDS, PE_SEGMENT_BREAK= PE_INPUT_BANDS + 6, PE_INPUT_INDICE = PE_SEGMENT_BREAK + 1, PE_DESAWTOOTH_INDICE = PE_INPUT_INDICE + 1, PE_FIT_INDICE = PE_DESAWTOOTH_INDICE +1, PE_OUTPUT_INDICE1= PE_FIT_INDICE + 1,  PE_REGRESS_P = PE_OUTPUT_INDICE1 +1, PE_OUTPUT_BANDS = PE_REGRESS_P + 2 * 6, NB_EXTRACTS = PE_OUTPUT_BANDS + 6 };
    std::vector < std::vector< std::array<double, NB_EXTRACTS> >> m_extract_data;
    ofStream m_export_point_file;

    static const char* VERSION;
    static const size_t NB_THREAD_PROCESS;
};
}
