//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
//
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "external/ERMsg/ERMsg.h"
#include "geomatic/GDALDatasetEx.h"


namespace WBSF
{


namespace Landsat2
{
//enum TLandsatFormat { F_UNKNOWN = -1, F_OLD, F_NEW, NB_FORMATS };
enum TLandsatBands { B1, B2, B3, B4, B5, B7, SCENES_SIZE };
enum TIndices { I_INVALID = -1, I_B1, I_B2, I_B3, I_B4, I_B5, I_B7, I_NBR, I_NDVI, I_NDMI, I_NDWI, I_TCB, I_TCG, I_TCW, I_ZSW, I_NBR2, I_EVI, I_EVI2, I_SAVI, I_MSAVI, I_SR, I_HZ, I_LSWI, I_VIgreen, NB_INDICES };

double INDICES_FACTOR();
void INDICES_FACTOR(double f);


const char* GetBandName(size_t s);
const char* GetIndiceName(size_t i);
TIndices GetIndiceType(const std::string& str);


class CBandStat
{
public:

    CBandStat()
    {
        m_min = m_max = m_mean = m_sd = DBL_MAX;
    }

    double m_min, m_max, m_mean, m_sd;
};

typedef std::array< CBandStat, Landsat2::SCENES_SIZE> CBandStats;




typedef int16_t LandsatDataType;
typedef std::array<LandsatDataType, Landsat2::SCENES_SIZE> LandsatPixel;
class CLandsatPixel : public LandsatPixel
{
public:

    CLandsatPixel();
    void Reset();

    using LandsatPixel::operator[];
    LandsatDataType operator[](const Landsat2::TIndices& i)const;
    LandsatDataType operator[](const Landsat2::TIndices& i)
    {
        return ((const CLandsatPixel*)(this))->operator[](i);
    }

    bool IsInit()const;
    bool IsInit(Landsat2::TIndices i)const;
    bool IsValid()const;
    bool IsBlack()const
    {
        return (at(Landsat2::B1) == 0 && at(Landsat2::B2) == 0 && at(Landsat2::B3) == 0);
    }
    bool IsZero()const
    {
        return (at(Landsat2::B1) == 0 && at(Landsat2::B2) == 0 && at(Landsat2::B3) == 0 && at(Landsat2::B4) == 0 && at(Landsat2::B5) == 0 && at(Landsat2::B7) == 0);
    }


    double GetEuclideanDistance(const CLandsatPixel& pixel, CBaseOptions::TRGBTye type = CBaseOptions::NO_RGB)const;
    double NBR()const;
    double NDVI()const;
    double NDMI()const;
    double NDWI()const;
    double TCB()const;
    double TCG()const;
    double TCW()const;
    double ZSW()const;
    double NBR2()const;
    double EVI()const;
    double EVI2()const;
    double SAVI()const;
    double MSAVI()const;
    double SR() const;
    double CL()const;
    double HZ()const;
    double LSWI()const;
    double VIgreen()const;

    Color8 R(CBaseOptions::TRGBTye type = CBaseOptions::LANDWATER, const CBandStats& stats = CBandStats())const;
    Color8 G(CBaseOptions::TRGBTye type = CBaseOptions::LANDWATER, const CBandStats& stats = CBandStats())const;
    Color8 B(CBaseOptions::TRGBTye type = CBaseOptions::LANDWATER, const CBandStats& stats = CBandStats())const;
};

typedef std::vector<CLandsatPixel>CLandsatPixelVector;


class CLandsatWindow : public CRasterWindow
{
public:

    CLandsatWindow();
    CLandsatWindow(const CRasterWindow& in);
    CLandsatWindow(const CLandsatWindow& in);
    size_t size()const
    {
        assert(CRasterWindow::size() % SCENES_SIZE==0);
        return CRasterWindow::size() / SCENES_SIZE;
    }

    void resize(size_t size, const CGeoExtents& extents, DataType noData = DefaultNoData)
    {
        CRasterWindow::resize(size* SCENES_SIZE, m_extents, noData );
        //for (size_t s = 0; s < CRasterWindow::size(); s++)
        //  at(s).resize(blockSize.m_x * blockSize.m_y, DataType(m_options.m_dstNodata));
    }
    size_t GetNbBands()const
    {
        return CRasterWindow::size();
    }

    CLandsatPixel GetPixel(size_t i, int x, int y)const;
    size_t GetPrevious(size_t z, int x, int y)const;
    size_t GetNext(size_t z, int x, int y)const;

    CStatistic GetPixelIndiceI(size_t z, Landsat2::TIndices ind, int x, int y, int n_rings)const;
    LandsatDataType GetPixelIndice(size_t z, Landsat2::TIndices ind, int x, int y, double n_rings = 0)const;
    LandsatDataType GetPixelIndiceMedian(Landsat2::TIndices ind, int x, int y, double n_rings = 0)const;
    //CLandsatPixel GetPixelMean(size_t f, size_t l, int x, int y, int buffer, const std::vector<double>& weight = std::vector<double>())const;
    CLandsatPixel GetPixelMean(size_t i, int x, int y, int n_rings, const std::vector<double>& weight = std::vector<double>())const;
    CLandsatPixel GetPixelMedian(size_t f, size_t l, int x, int y, int n_rings = 0)const;
    CLandsatPixel GetPixelMedian(int x, int y, int n_rings = 0)const
    {
        return GetPixelMedian(0, GetNbScenes() - 1, x, y, n_rings);
    }
    CLandsatPixel GetPixelMedian(size_t f, size_t l, int x, int y, double n_rings)const;
    CLandsatPixel GetPixelMedian(int x, int y, double n_rings)const
    {
        return GetPixelMedian(0, GetNbScenes() - 1, x, y, n_rings);
    }
    //bool GetPixel(size_t i, int x, int y, CLandsatPixel& pixel)const;
    CLandsatPixelVector Synthetize(Landsat2::TIndices ind, int x, int y, const std::vector<LandsatDataType>& fit_ind, double n_rings = 0)const;


    //CLandsatWindow& operator[](size_t z){ return static_cast<CLandsatWindow&>(*CRasterWindow::at(z)); }
    //const CLandsatWindow& operator[](size_t z)const{ return static_cast<CLandsatWindow&>(*CRasterWindow::at(z)); }
    //Landsat2::TCorr8 m_corr8;
};



class CLandsatDataset : public CGDALDatasetEx
{
public:

    virtual ERMsg OpenInputImage(const std::string& filePath, const CBaseOptions& options = CBaseOptions());
    virtual ERMsg CreateImage(const std::string& filePath, CBaseOptions options);
    virtual void UpdateOption(CBaseOptions& options)const;
    virtual void Close(const CBaseOptions& options = CBaseOptions());

    ERMsg CreateIndices(size_t i, const std::string filePath, Landsat2::TIndices type);

    std::string GetCommonName()const;
    std::string GetCommonImageName(size_t i)const;
    std::string GetCommonBandName(size_t b)const;
    std::string GetSpecificBandName(size_t i)const;
    std::string GetSpecificBandName(size_t i, size_t j)const;
    std::string GetSubname(size_t i, size_t b = NOT_INIT)const;

protected:


};




}

}
