//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
//
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include <boost/timer/timer.hpp>
#include "external/ERMsg/ERMsg.h"
#include "basic/UtilStd.h"
#include "basic/Statistic.h"
#include "basic/GeoBasic.h"


class GDALDataset;
class GDALRasterBand;
class GDALDriver;

namespace WBSF
{

class CGDALDatasetEx;
class CGDALDatasetExVector;

//General function
void RegisterGDAL();
ERMsg OpenInputImage(const std::string& filePath, GDALDataset** poInputDS, double srcNodata = MISSING_NO_DATA, bool bUseDefaultNoData = true, bool bReadOnly= true);
ERMsg OpenOutputImage(const std::string& filePath, GDALDataset* poInputDS, GDALDataset** poOutputDS, const char* outDriverName = "GTiff", short cellType = 0, int nbBand = -1, double dstNodata = MISSING_NO_DATA, const std::vector<std::string>& createOptions = std::vector<std::string>(), bool bOverwrite = true, const CGeoExtents& extentsIn = CGeoExtents(), bool useDefaultNoData = true);
bool GoodGeotiff(const std::string& filePath);

ERMsg GetFilePathList(const char* fileName, int filePathCol, std::vector<std::string>& filePathList);
ERMsg VerifyInputSize(GDALDataset* poInputDS1, GDALDataset* poInputDS2);
CGeoExtents GetExtents(GDALDataset* poDataset);

ERMsg VerifyNoData(double nodata, short eType);
double GetNoData(GDALRasterBand* pBand);
double GetDefaultNoData(short eType);
double GetTypeLimit(short eType, bool bLow);
double LimitToBound(double v, short cellType, double shiftedBy = 0, bool bRound = true);
void PosToCoord(double* GT, int Xpixel, int Yline, double& Xgeo, double& Ygeo);
void CoordToPos(double* GT, double Xgeo, double Ygeo, int& Xpixel, int& Yline);
CStatistic GetWindowMean(GDALRasterBand* pBand, int x1, int y1, int x2, int y2);
double GetWindowMean(GDALRasterBand* pBand, int nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d);

std::string GetGDALFilter(bool bMustSupportCreate);
void GetGDALDriverList(std::vector<std::string>& GDALList, bool bMustSupportCreate);
GDALDriver* GetDriverFromExtension(const std::string& extIn, bool bMustSupportCreate);
std::string GetDriverExtension(const std::string& formatName);
//ERMsg GetGDALInfo(const std::string& filePath, CNewGeoFileInfo& info);

void Close(GDALDataset* poDS);
void PrintMessage(ERMsg msg);

typedef int16_t DataType;
static const int16_t DefaultNoData = -32768;
//static const float DataTypeMin = -FLT_MAX;
typedef std::vector<DataType> DataVector;
typedef int MaskDataType;
typedef std::vector<MaskDataType> MaskDataVector;
typedef DataVector::const_iterator DataIteratorConst;


typedef std::map<std::string, std::string> MetaData;
typedef std::vector<MetaData> MetaDataVector;
typedef std::vector<std::map<std::string, std::string>> BandsMetaData;

typedef unsigned char Color8;

//******************************************************************************************************
class COptionDef
{
public:

    const char* m_name;
    int m_nbArgs;
    const char* m_args;
    bool m_bMulti;
    const char* m_help;

    std::string GetHelp()const
    {
        std::string help;

        std::string cmdLine = std::string("  ") + m_name + (m_bMulti ? "*" : "") + (m_nbArgs > 0 ? " " : "") + m_args + ":";

        std::string strHelp = m_help;
        std::string::size_type pos = 0;
        while (pos != std::string::npos)
        {
            std::string word = Tokenize(strHelp, " ", pos);

            if (cmdLine.length() + word.length() + 1 < 80)
            {
                cmdLine += " " + word;
            }
            else
            {
                help += cmdLine + "\n";
                cmdLine = "      " + word;//begin a new line
            }
        }

        help += cmdLine;

        return help;
    }


};
//****************************************************************************************************

typedef std::vector< COptionDef > COptionDefVector;
typedef std::map< std::string, COptionDef > COptionDefMap;

class CIOFileInfoDef
{
public:

    const char* m_description;
    const char* m_name;
    const char* m_nFiles;
    const char* m_nbBands;
    const char* m_help;//help of bands separate by |
    const char* m_note;//

    std::string GetHelp()const
    {
        std::string help;


        help = std::string("  ") + m_description + ": " + m_name + "\n";

        if (m_nFiles && strlen(m_nFiles) > 0)
        {
            std::string cmdLine = std::string("    Number of files: ") + m_nFiles + "\n";
            help += cmdLine;
        }

        if (m_nbBands && strlen(m_nbBands) > 0)
        {
            std::string cmdLine = std::string("    Number of bands (columns): ") + m_nbBands + "\n";
            help += cmdLine;
        }


        if (m_help && strlen(m_help) > 0)
        {
            std::string cmdLine;
            std::string strHelp = m_help;



            int bandNo = 0;
            std::string::size_type bandInfoPos = 0;
            while (bandInfoPos != std::string::npos)
            {
                std::string bandInfo = Tokenize(strHelp, "|", bandInfoPos);
                Trim(bandInfo);

                if (!bandInfo.empty())
                {
                    bandNo++;
                    cmdLine = std::string("    ") + ToString(bandNo) + "-";


                    std::string::size_type pos = 0;
                    while (pos != std::string::npos)
                    {
                        std::string word = Tokenize(bandInfo, " ", pos);

                        if (cmdLine.length() + word.length() + 1 < 80)
                        {
                            cmdLine += " " + word;
                        }
                        else
                        {
                            help += cmdLine + "\n";
                            cmdLine = "          " + word;//begin a new line
                        }
                    }

                    help += cmdLine + "\n";
                }
            }
        }

        if (m_note && strlen(m_note) > 0)
        {
            std::string cmdLine;
            std::string note = m_note;
            if (!note.empty())
            {
                cmdLine += "    Note:";
                std::string::size_type pos = 0;
                while (pos != std::string::npos)
                {
                    std::string word = Tokenize(note, " ", pos);

                    if (cmdLine.length() + word.length() + 1 < 80)
                    {
                        cmdLine += " " + word;
                    }
                    else
                    {
                        help += cmdLine + "\n";
                        cmdLine = "          " + word;//begin a new line
                    }
                }

                help += cmdLine + "\n";
            }
        }

        return help;
    }


};

typedef std::vector< CIOFileInfoDef > CIOFileInfoDefVector;

//*****************************************************************************************************
//CBaseOptions : parse base applications option
class CBaseOptions
{
public:
    
    enum TRGBTye { NO_RGB=-1, NATURAL, LANDWATER, TRUE_COLOR, NB_RGB };
    static const char* RGB_NAME[NB_RGB];

    static const COptionDef OPTIONS_DEF[];
    static const char* DEFAULT_OPTIONS[];
    static int GetOptionIndex(const char* name);

    CBaseOptions(bool bAddDefaultOption = true);

    void Reset();
    void AddOption(const char* name);
    void AddOption(const COptionDef& optionsDef);
    void AddIOFileInfo(const CIOFileInfoDef& fileDef);
    void RemoveOption(const char* name);

    ERMsg ParseOptions(int argc, char* argv[]);
    ERMsg ParseOptions(const std::string& str);

    virtual ERMsg ParseOption(int argc, char* argv[]);
    virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
    virtual std::string GetUsage()const;
    virtual std::string GetHelp()const;
    virtual std::string GetIOFileInfo()const;

    bool IsVRT()const
    {
        return IsEqualNoCase(m_format, "VRT");
    }
    int BLOCK_CPU()const
    {
        return std::max(1, int(m_CPU / m_BLOCK_THREADS) );
    }

    bool m_bReadOnly;
    std::string m_format;
    short m_outputType;
    std::vector<std::string> m_createOptions;
    std::vector<std::string> m_workOptions;
    bool m_bUseDefaultNoData;
    double m_srcNodata;
    double m_dstNodata;
    double m_dstNodataEx;
    double m_memoryLimit;
    CGeoExtents m_extents;
    std::array<size_t, 2> m_scene_extents;
    double m_xRes;
    double m_yRes;
    TRGBTye m_RGBType;
    double m_iFactor;

    std::vector<std::string> m_filesPath;
    //std::vector<int> m_bandsToUsed;
    size_t m_nbBands;
    std::string m_prj;
    std::string m_maskName;
    float m_maskDataUsed;
    int m_CPU;
    int m_IOCPU;
    int m_BLOCK_THREADS;

    bool m_bMulti;
    bool m_bOverwrite;
    bool m_bQuiet;
    bool m_bNeedHelp;
    bool m_bNeedUsage;
    bool m_bNeedIOFileInfo;
    bool m_bVersion;
    bool m_bExtents;
    bool m_bRes;
    bool m_bTap;
    bool m_bResetJobLog;
    std::string m_jobLogName;
    bool m_bCreateImage;
    std::vector<int> m_overviewLevels;
    bool m_bComputeStats;
    bool m_bComputeHistogram;
    bool m_bRemoveEmptyBand;
    std::string m_rename;
    std::vector<size_t> m_scenes_def;//define the number of band by scene;

    //int m_TTF; //temporal type format for temporal dataset
    //int m_scenesSize;
    bool m_bOpenBandAtCreation;
    std::string m_VRTBandsName;


    CGeoExtents GetExtents()const
    {
        return m_extents;
    }
    void SetExtents(const CGeoExtents& extents)
    {
        m_extents = extents;
    }

    

    size_t GetSceneSize()const 
    {
        return m_scenes_def.empty() ? 1 : m_scenes_def.size();
    }
    //common timer used
    boost::timer::cpu_timer m_timerRead;
    boost::timer::cpu_timer m_timerProcess;
    boost::timer::cpu_timer m_timerWrite;

    static size_t m_xxFinal;
    static size_t m_xx;
    static size_t m_xxx;



    void PrintTime();

    void ResetBar(size_t xxFinal)
    {
        m_xxFinal = xxFinal;
        m_xx = 0;
        m_xxx = 0;

    }

    void UpdateBar();



protected:

    std::string m_appDescription;
    COptionDefVector m_mendatoryDef;
    COptionDefMap m_optionsDef;
    CIOFileInfoDefVector m_IOFileInfo;
};

}
