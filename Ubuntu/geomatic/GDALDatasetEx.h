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
#include "basic/GeoBasic.h"
#include "geomatic/UtilGDAL.h"
#include "geomatic/GDAL.h"


namespace WBSF
{


class CGDALDatasetEx;


class CDataWindow
{
public:


    CDataWindow()//(const CGeoRectIndex& windowRect, const CGeoRectIndex& dataRect, double noData, const CGeoExtents& extents)
    {
        //assert(pData == NULL || (int)pData->size() == dataRect.Height()*dataRect.Width());
        //assert(pData == NULL || dataRect.IsRectIntersect(windowRect));
        //m_pData = pData;
        //m_dataRect = dataRect;
        //m_windowRect = windowRect;
        //m_noData = noData;
        // m_maskDataUsed = maskDataUsed;
        //m_extents = extents;

    }

    void resize(const CGeoRectIndex& dataRect, CGeoRectIndex& windowRect, const CGeoExtents& windowExtents, DataType noData = DefaultNoData)
    {
        assert(m_windowRect.UpperLeft() >= m_dataRect.UpperLeft());
        assert(m_windowRect.LowerRight() <= m_dataRect.LowerRight());

        m_dataRect = dataRect;
        m_windowRect = windowRect;
        m_extents = windowExtents;
        m_noData = noData;
        m_data.resize(m_dataRect.size(), noData);
    }

//    void SetMask(CDataWindowPtr pMaskWindow)
//    {
//        m_pMaskWindow.swap(pMaskWindow);
//    }

    void GetStat(CStatistic& stat)const
    {
        if (!m_data.empty())
        {
            for (int x = 0; x < XSize(); x++)
            {
                for (int y = 0; y < YSize(); y++)
                {
                    DataType v = at(x, y);

                    if (IsValid(v))
                        stat += v;
                }
            }
        }
    }

    void GetSlopeAndAspect(int x, int y, double& slope, double& aspect)const;

    int XSize()const
    {
        //assert(m_windowRect.Width()*m_windowRect.Height());
        return m_windowRect.Width();
    }
    int YSize()const
    {
        return m_windowRect.Height();
    }
    size_t size()const
    {
        return m_windowRect.Height()*m_windowRect.Width();
    }
    CGeoSize GetGeoSize()const
    {
        return m_windowRect.GetGeoSize();
    }


    bool IsInside(CGeoPointIndex pt)const
    {
        return IsInside(pt.m_x, pt.m_y);
    }
    bool IsInside(int x, int y)const
    {
        x += m_windowRect.m_x;
        y += m_windowRect.m_y;
        return m_dataRect.PtInRect(x, y);
    }

    DataType at(CGeoPointIndex pt)const
    {
        return at(pt.m_x, pt.m_y);
    }
    DataType at(int x, int y)const
    {
        if (m_data.empty())
            return (DataType)m_noData;

        x += m_windowRect.m_x;
        y += m_windowRect.m_y;

        DataType v = (DataType)m_noData;
        if (m_dataRect.PtInRect(x, y))
        {
            int64_t pos = ((int64_t)y - m_dataRect.m_y)*m_dataRect.Width() + (x - m_dataRect.m_x);
            assert(pos >= 0 && pos < (int64_t)m_data.size());

            if (pos >= 0 && pos < (int64_t)m_data.size())
            {
                v = m_data.at((size_t)pos);
//                if (m_pMaskWindow && (m_pMaskWindow.get() != NULL) && m_pMaskWindow->IsPixelMasked(x, y))
                //                  v = (DataType)m_noData;
            }
        }
        return v;
    }

    void set_value(int x, int y, DataType in)
    {
        assert(!m_data.empty());

        x += m_windowRect.m_x;
        y += m_windowRect.m_y;

        assert(m_dataRect.PtInRect(x, y));

        int64_t pos = ((int64_t)y - m_dataRect.m_y)*m_dataRect.Width() + (x - m_dataRect.m_x);
        assert(pos >= 0 && pos < (int64_t)m_data.size());

        m_data.at((size_t)pos) = in;
    }

    DataType operator ()(int x, int y)const
    {
        return at(x, y);
    }
    bool IsValid(CGeoPointIndex xy)const
    {
        return IsValid(at(xy));
    }
    bool IsValid(int x, int y)const
    {
        return IsValid(at(x, y));
    }

    bool IsValid(DataType v)const
    {

        return m_noData == MISSING_NO_DATA || fabs(v - (DataType)m_noData) > EPSILON_NODATA;
    }

    void SetNoData(double noData)
    {
        m_noData=noData;
    }
    double GetNoData()const
    {
        return m_noData;
    }

    DataType* data()
    {
        return m_data.data();
    }
    const DataType* data()const
    {
        return m_data.data();
    }

    CGeoExtents GetExtents()const
    {
        return m_extents;
    }

protected:

    //does the pixel is used. this layer is a mask layer
//    bool IsPixelMasked(int x, int y)const
//    {
//        if (m_pData == NULL)
//            return false;
//
//        size_t pos = (m_windowRect.m_y - m_dataRect.m_y + y)*m_dataRect.Width() + m_windowRect.m_x - m_dataRect.m_x + x;
//        if (pos >= m_pData->size())
//            return false;
//
//        DataType maskValue = m_pData->at(pos);
//
//        return (m_maskDataUsed == DataTypeMin) ? fabs(maskValue - m_noData) < EPSILON_NODATA : maskValue != m_maskDataUsed;
//    }

    DataVector m_data;
    CGeoRectIndex m_dataRect;
    CGeoRectIndex m_windowRect;
    double m_noData;
    //DataType m_maskDataUsed;
    CGeoExtents m_extents;

    //CDataWindowPtr m_pMaskWindow;
};

//typedef std::shared_ptr<CDataWindow> CDataWindowPtr;
//typedef std::shared_ptr<CDataWindow> CDataWindowPtr;

class CRasterWindow : public std::deque < CDataWindow >
{
public:

//size_t sceneSize = 0, CGeoRectIndex windowRect = CGeoRectIndex()
    CRasterWindow(size_t sceneSize = 0)
    {
        m_sceneSize = sceneSize;
        //m_windowRect = windowRect;
    }

    void resize(size_t size, const CGeoExtents& extents, DataType noData = DefaultNoData)
    {
        m_windowRect = extents.GetPosRect();
        m_extents = extents;
        std::deque < CDataWindow >::resize(size);
        //for (size_t i = 0; i < size; i++)
        //  at(i).resize(m_extents, noData);
    }


    bool IsInit()const
    {
        return m_sceneSize!= 0;
    }
    size_t GetNbScenes()const
    {
        return size() / m_sceneSize;
    }
    size_t GetSceneSize()const
    {
        return m_sceneSize;
    }

    bool IsValid(size_t i, int x, int y)const
    {
        assert(((i + 1) * m_sceneSize - 1) < size());
        //const_iterator it1 = begin() + i * m_sceneSize;

        bool bValid = true;
        for (size_t z = 0; z < m_sceneSize && bValid; z++)
            bValid = at(i * m_sceneSize + z).IsValid(x,y);

        return bValid;
    }

//
//    template <class T>
//    bool IsValid(size_t i, const T& pixel)const
//    {
//        assert(((i + 1)*m_sceneSize - 1) < size());
////        const_iterator it1 = begin() + i*m_sceneSize;
//
//        bool bValid = true;
//        assert(false);//todo
//        //for (class T::const_iterator it2 = pixel.begin(); it2 != pixel.end() && bValid; it2++)
//        //bValid = (*it1)->IsValid(*it2);
//
//
//        return bValid;
//    }

//    template <class T>
//    bool IsValid(size_t i, size_t z, const T& pixel)const
//    {
//        assert((i*m_sceneSize + z) < size());
//        return at(i*m_sceneSize + z).IsValid(pixel);
//    }

    CGeoSize GetGeoSize()const
    {
        return m_windowRect.GetGeoSize();
    }

    CGeoExtents GetExtents()const
    {
        return m_extents;
    }

protected:

    size_t m_sceneSize;
    CGeoExtents m_extents;
    CGeoRectIndex m_windowRect;
};


//class CDataBlock
//{
//public:
//
//    CDataBlock(size_t size = 0, int dataType = GDT_Float32)
//    {
//        m_size = size;
//        m_ptr = nullptr;
//        m_dataType = dataType;
//
//        if (size > 0)
//        {
//            int dataSize = GDALGetDataTypeSize((GDALDataType)dataType) / sizeof(char);
//            m_ptr = new char[size * dataSize];
//        }
//
//    }
//
//    ~CDataBlock()
//    {
//        delete[]m_ptr;
//    }
//
//    size_t size()const
//    {
//        return m_size;
//    }
//
//    double get_value(size_t x)const
//    {
//        double value = 0;
//        switch (m_dataType)
//        {
//        case GDT_Byte:
//            value = double(((char*)m_ptr)[x]);
//            break;
//        case GDT_UInt16:
//            value = double(((uint16_t*)m_ptr)[x]);
//            break;
//        case GDT_Int16:
//            value = double(((int16_t*)m_ptr)[x]);
//            break;
//        case GDT_UInt32:
//            value = double(((uint32_t*)m_ptr)[x]);
//            break;
//        case GDT_Int32:
//            value = double(((int32_t*)m_ptr)[x]);
//            break;
//        case GDT_Float32:
//            value = double(((float*)m_ptr)[x]);
//            break;
//        case GDT_Float64:
//            value = double(((double*)m_ptr)[x]);
//            break;
//        }
//
//        return value;
//    }
//
//    void set_value(size_t x, double value)
//    {
//        switch (m_dataType)
//        {
//        case GDT_Byte:
//            ((char*)m_ptr)[x] = (char)value;
//            break;
//        case GDT_UInt16:
//            ((uint16_t*)m_ptr)[x] = (uint16_t)value;
//            break;
//        case GDT_Int16:
//            ((int16_t*)m_ptr)[x] = (int16_t)value;
//            break;
//        case GDT_UInt32:
//            ((uint32_t*)m_ptr)[x] = (uint32_t)value;
//            break;
//        case GDT_Int32:
//            ((int32_t*)m_ptr)[x] = (int32_t)value;
//            break;
//        case GDT_Float32:
//            ((float*)m_ptr)[x] = (float)value;
//            break;
//        case GDT_Float64:
//            ((double*)m_ptr)[x] = (double)value;
//            break;
//        }
//    }
//
//    void* data()
//    {
//        return m_ptr;
//    }
//    const void* data()const
//    {
//        return m_ptr;
//    }
//
//protected:
//
//    void* m_ptr;
//    size_t m_size;
//    int m_dataType;
//};





class CGDALDatasetEx
{
public:

    CGDALDatasetEx();
    ~CGDALDatasetEx();

    virtual ERMsg OpenInputImage(const std::string& filePath, const CBaseOptions& options = CBaseOptions());
    virtual ERMsg CreateImage(const std::string& filePath, const CBaseOptions& option);
    virtual void UpdateOption(CBaseOptions& options)const;
    virtual void Close(const CBaseOptions& options = CBaseOptions());
    virtual void ReadBlock(size_t i, size_t j, CRasterWindow& block)const;
    virtual void ReadBlock(const CGeoExtents& window_extents, CRasterWindow& window_data, int rings =0, int IOCPU=1, size_t first_scene = NOT_INIT, size_t last_scene = NOT_INIT)const;

    void BuildOverviews(const std::vector<int>& list, bool bQuiet, int CPU = 1);
    void ComputeStats(bool bQuiet, int CPU = 1);
    void ComputeHistogram(bool bQuiet, int CPU = 1);
    CGeoExtents ComputeLoadExtent(const CGeoExtents& extents, const CGeoExtents& iExtents, int rings=0)const;

    int GetRasterXSize()const;
    int GetRasterYSize()const;
    size_t GetRasterCount()const;
    ERMsg BuildVRT(const CBaseOptions& options);
    void GetGeoTransform(CGeoTransform GT)const;

    GDALRasterBand *GetRasterBand(size_t i);
    const GDALRasterBand *GetRasterBand(size_t i)const;

    GDALDataset* operator->()
    {
        return m_poDataset;
    }
    GDALDataset* operator->()const
    {
        return m_poDataset;
    }
    GDALDataset* Dataset()
    {
        return m_poDataset;
    }
    GDALDataset* Dataset()const
    {
        return m_poDataset;
    }
    GDALDataset* Detach()
    {
        GDALDataset* poDataset = m_poDataset;
        m_poDataset = NULL;
//        m_pProjection.reset();
        m_filePath.clear();
        return poDataset;
    }


    bool IsOpen()const
    {
        return !m_filePath.empty();    // m_poDataset != NULL;
    }
    bool IsOpenUpdate()const
    {
        return m_bOpenUpdate;
    }

    const std::string& GetFilePath()const
    {
        return m_filePath;
    }

    short GetDataType(size_t i = 0)const;
    ERMsg VerifyNoData(double nodata, size_t i = 0)const;
    double GetBandLimit(size_t i, bool bLow)const;
    double LimitToBound(double v, size_t i = 0, double shiftedBy = 0, bool bRound = true)const;
    double GetDefaultNoData(size_t i = 0)const;
    double GetNoData(size_t i)const;
    std::vector<double> GetNoData()const;
    bool HaveNoData(size_t i = 0)const;
    double PostTreatment(double v, size_t i = 0, double shiftedBy = 0, bool bRound = true);


    bool IsVRT()const
    {
        return m_bVRT;
    }
    double ReadPixel(size_t i, const CGeoPointIndex& xy)const
    {
        return ReadPixel(i, xy.m_x, xy.m_y);
    }
    double ReadPixel(size_t i, int x, int y)const;
    CGeoSize ComputeBlockSize(double memoryLimit, const CGeoExtents& extents)const;
    int GetMaxBlockSizeZ(CGeoExtents extents)const;

//		const std::vector<std::string>& GetVirtualBand()const{ return m_virtualBands; }

    const std::string& GetInternalName(size_t i)const
    {
        assert(i < m_internalName.size());
        return m_internalName[i];
    }
    const CGeoExtents& GetInternalExtents(size_t i)const
    {
        assert(i < m_internalExtents.size());
        return m_internalExtents[i];
    }

    CStatistic GetWindowMean(size_t i, int x1, int y1, int x2, int y2)
    {
        return WBSF::GetWindowMean(GetRasterBand(i), x1, y1, x2, y2);
    }
    double GetWindowMean(size_t i, int nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d)
    {
        return WBSF::GetWindowMean(GetRasterBand(i), nbNeighbor, power, pts, d);
    }

//		ERMsg GetRegularCoord(int nbPointX, int nbPointY, bool bExp, const CGeoRect& rect, CGridPointVector& points, CCallback& callBack = DEFAULT_CALLBACK);
    //ERMsg GetRandomCoord(int nbPoint, bool bExp, bool bExtrem, double factor, const CGeoRect& rect, CGridPointVector& points, CCallback& callBack = DEFAULT_CALLBACK);

//		CProjectionPtr GetPrj()const{ return m_pProjection; }
    //	size_t GetPrjID()const{ return m_pProjection ? m_pProjection->GetPrjID() : PRJ_NOT_INIT; }

    const CGeoExtents& GetExtents()const
    {
        return m_extents;
    }
    void FlushCache(double yMax=-9999999999);

    void SetVRTBand(size_t i, GDALDataset* pDataset);
    void CloseVRTBand(size_t i);
    void GetBandsMetaData(BandsMetaData& meta_data)const;

    //temporal section
    size_t GetSceneSize()const
    {
        return m_scenes_def.empty() ? 1 : m_scenes_def.size(); // m_scenes_def.size() > 0 ? GetRasterCount() / m_scenesPeriod.size() : 1;
    }
    size_t GetNbScenes()const
    {
        //return m_scenes_def.size() > 0 ? m_scenesPeriod.size(): GetRasterCount();
        return m_scenes_def.empty() ? GetRasterCount() : GetRasterCount() / m_scenes_def.size();
    }
//		const std::vector<CTPeriod>& GetScenePeriod()const{ return m_scenesPeriod; }
//
//		CTPeriod GetPeriod()const
//		{
//			CTPeriod p;
//			for (std::vector<CTPeriod>::const_iterator it = m_scenesPeriod.begin(); it != m_scenesPeriod.end(); it++)
//				p += *it;
//
//			return p;
//		}

    std::string GetCommonBandName();

protected:

    //std::string GetMetaDataFilePath(const std::string& filePath)const;
    //ERMsg LoadMetaData(const std::string& filePath);

    bool m_bVRT;
    bool m_bOpenUpdate;
    GDALDataset* m_poDataset;
    //CProjectionPtr m_pProjection;

    std::vector<CGeoExtents> m_internalExtents;
    std::vector<std::string> m_internalName;
    CGeoExtents m_extents;
    //std::vector<std::string> m_virtualBands;

    std::string m_filePath;

    //this vector is intended to keep each band of a VRT file open for writing
    bool m_bMultipleImages;// for VRT output only
    std::vector<GDALDataset*> m_poDatasetVector;

    //temporal section of the dataset
    std::vector<size_t> m_scenes_def;//one band by 

};

//	class CGDALDatasetExVector : public std::vector < CGDALDatasetEx >
//	{
//	public:
//
//		CGDALDatasetExVector(size_t t = 0) : std::vector<CGDALDatasetEx>(t)
//		{}
//
//		ERMsg VerifyInputSize(CGDALDatasetExVector& inputDS)
//		{
//			ERMsg msg;
//			for (size_t i = 1; i < inputDS.size() && msg; i++)
//				msg += WBSF::VerifyInputSize(inputDS[i].Dataset(), at(i).Dataset());
//
//			return msg;
//		}
//
//		CGeoExtents GetExtents()const
//		{
//			CGeoExtents extents;
//			for (size_t i = 0; i < size(); i++)
//				extents.UnionExtents(at(i).GetExtents());
//
//			return extents;
//		}
//
//	};
//
//	ERMsg VerifyInputSize(CGDALDatasetExVector& inputDS);
//

//**************************************************************************************************
//
//	class CBandsHolder
//	{
//	public:
//
//
//		CBandsHolder(int maxWindowSize = 1, double memoryLimit = 0, int IOCPU = 1)
//		{
//			m_maxWindowSize = maxWindowSize;
//			m_memoryLimit = memoryLimit;
//			m_IOCPU = IOCPU;
//			m_maskDataUsed = DataTypeMin;
//			m_bEmpty = true;
//			m_nbScenes = 0;
//			m_sceneSize = 0;
//		}
//
//		void clear();
//
//		void AddBand(CSingleBandHolderPtr pBandHolder){ m_bandHolder.push_back(pBandHolder); }
//		size_t GetRasterCount()const{ return m_bandHolder.size(); }
//
//		virtual ERMsg Load(const CGDALDatasetEx& inputDS, bool bQuiet = true, const CGeoExtents& extents = CGeoExtents());
//		void LoadBlock(int xBlock, int yBlock){ LoadBlock(m_entireExtents.GetBlockExtents(xBlock, yBlock)); }
//		virtual void LoadBlock(CGeoExtents extents);
//		void LoadBlock(CGeoExtents extents, boost::dynamic_bitset<size_t> selected_bands);
//
//		virtual double Evaluate(size_t virtualBandNo, const std::vector<float>& variable){ assert(false); return DBL_MAX; }
//		void FlushCache(double yMax = -9999999999);
//
//		CGeoSize ComputeBlockSize(const CGeoExtents& extents, size_t nb_scenes)const;
//
//		DataType GetPixel(size_t layer, int x, int y)const
//		{
//			DataType noData = (DataType)m_bandHolder[layer]->GetNoData();
//
//			const CGeoRectIndex& rect = m_bandHolder[layer]->GetDataRect();
//			if (rect.PtInRect(x, y))
//			{
//				if (m_pMaskBandHolder.get() && m_pMaskBandHolder->GetDataRect().PtInRect(x, y))
//				{
//					assert(m_pMaskBandHolder.get());
//					if (!m_pMaskBandHolder->IsMasked(x, y))
//						return noData;
//				}
//
//				return m_bandHolder[layer]->GetPixel(x, y);
//			}
//
//			return noData;
//		}
//
//		CDataWindowPtr GetWindow(size_t layer, const CGeoRectIndex& rect)const
//		{
//			assert(layer < m_bandHolder.size());
//			assert(m_entireExtents.GetPosRect().IsRectIntersect(rect));
//
//			CDataWindowPtr pWindow = m_bandHolder[layer]->GetWindow(rect);
//
//			if (m_pMaskBandHolder.get() && m_pMaskBandHolder->GetDataRect().IsRectIntersect(rect))
//			{
//				assert(m_pMaskBandHolder.get());
//				const CSingleBandHolder* pBand = m_pMaskBandHolder.get();
//				pWindow->SetMask(m_pMaskBandHolder->GetWindow(rect));
//			}
//
//			return pWindow;
//		}
//
//		CDataWindowPtr GetWindow(size_t layer)const
//		{
//			return GetWindow(layer, m_bandHolder[layer]->GetExtents().GetPosRect());
//		}
//
//		CDataWindowPtr GetWindow(size_t layer, int x, int y, int XSize, int YSize)const
//		{
//			//return GetWindow(layer, CGeoRectIndex(x - XSize / 2, y - YSize / 2, x + (XSize - 1) / 2 + 1, y + (YSize - 1) / 2 + 1));
//			//return GetWindow(layer, CGeoRectIndex(x-XSize / 2, y - YSize / 2, XSize, YSize));
//			return GetWindow(layer, CGeoRectIndex(x, y, XSize, YSize));
//		}
//
//		CDataWindowPtr GetWindow(size_t layer, int x, int y, int size)const
//		{
//			return GetWindow(layer, x, y, size, size);
//		}
//
//		void GetWindow(std::vector<CDataWindowPtr>& input)const
//		{
//			input.resize(GetRasterCount());
//			for (size_t i = 0; i < input.size(); i++)
//				input[i] = GetWindow(int(i));
//		}
//
//		CRasterWindow GetWindow()const
//		{
//			if (m_bEmpty)
//				return CRasterWindow();
//
//			CRasterWindow input(m_sceneSize, m_bandHolder.front()->GetExtents().GetPosRect());
//			input.resize(GetRasterCount());
//			for (size_t i = 0; i < input.size(); i++)
//				input[i] = GetWindow(int(i));
//
//			return input;
//		}
//
//
//		CDataWindowPtr GetLine(size_t layer, int y)const
//		{
//			assert(layer >= 0 && layer < GetRasterCount());
//			return GetWindow(layer, CGeoRectIndex(0, y, m_entireExtents.GetPosRect().Width(), y + 1));
//		}
//
//
//		bool IsValid(size_t layer, DataType v)const{ return m_bandHolder[layer]->IsValid(v); }
//
//		template <class T>
//		void GetBlockLineAllLayer(int y, std::vector<std::vector<T>>& input, bool byBandFirst = true)
//		{
//			size_t nXSize = m_entireExtents.m_xBlockSize;
//			size_t nbBand = GetRasterCount();
//
//			if (input.size() != (byBandFirst ? nbBand : nXSize))
//			{
//				input.resize(byBandFirst ? nbBand : nXSize);
//				for (size_t i = 0; i < input.size(); i++)
//					input[i].resize(byBandFirst ? nXSize : nbBand);
//			}
//
//			for (size_t z = 0; z < nbBand; z++)
//			{
//				CDataWindowPtr pLine = GetLine((int)z, y);//zero base
//				for (size_t x = 0; x < nXSize; x++)
//				{
//					if (byBandFirst)
//						input[z][x] = (T)pLine->at((int)x, 0);
//					else
//						input[x][z] = (T)pLine->at((int)x, 0);
//				}
//			}
//		}
//
//
//		template <class T>
//		void GetBlockAllLayer(std::vector<std::vector<std::vector<T>>>& input)
//		{
//			CGeoSize size = m_entireExtents.GetSize();
//			size_t nXSize = size.m_x;
//			size_t nYSize = size.m_y;
//			size_t nbBand = GetRasterCount();
//
//			if (input.size() != nbBand || input[0].size() != nYSize || input[0][0].size() != nXSize)
//			{
//				input.resize(nbBand);
//				for (size_t i = 0; i < input.size(); i++)
//				{
//					input[i].resize(nYSize);
//					for (size_t j = 0; j < input[i].size(); j++)
//						input[i][j].resize(nXSize);
//				}
//			}
//
//			for (size_t z = 0; z < nbBand; z++)
//			{
//				CDataWindowPtr pWindow = GetWindow((int)z);//zero base
//				for (size_t y = 0; y < nYSize; y++)
//				{
//					for (size_t x = 0; x < nXSize; x++)
//					{
//						input[z][y][x] = (T)pWindow->at((int)x, (int)y);
//					}
//				}
//			}
//		}
//
//		const CGeoExtents& GetExtents()const{ return m_entireExtents; }
//		void SetExtents(const CGeoExtents& extents){ m_entireExtents = extents; }
//
////		CTPeriod GetPeriod()const
////		{
////			CTPeriod p;
////			for (std::vector<CTPeriod>::const_iterator it = m_scenesPeriod.begin(); it != m_scenesPeriod.end(); it++)
////				p += *it;
////
////			return p;
////		}
////
////		void SetScenePeriod(const std::vector<CTPeriod>& scenePeriod){ m_scenesPeriod = scenePeriod; }
//
//		size_t size()const{ return m_bandHolder.size(); }
//		const CSingleBandHolderPtr& operator[](size_t v)const{ return m_bandHolder[v]; }
//
//		int GetBlockSizeX(int i = -1, int j = -1)const{ return m_entireExtents.GetBlockSize(i, j).m_x; }
//		int GetBlockSizeY(int i = -1, int j = -1)const{ return m_entireExtents.GetBlockSize(i, j).m_y; }
//		int GetBlockSizeZ(int i = -1, int j = -1)const;
//		std::vector<bool> GetIntersectBands()const;
//
//		int GetMaxWindowSize()const { return m_maxWindowSize; }
//		void SetMaxWindowSize(int maxWindowSize){ m_maxWindowSize = maxWindowSize; }
//
//		void SetMask(CSingleBandHolderPtr pBandHolder, DataType maskDataUsed)
//		{
//			m_pMaskBandHolder = pBandHolder;
//			m_maskDataUsed = maskDataUsed;
//		}
//
//		bool IsEmpty()const{ return m_bEmpty; }
//
//		size_t GetBandByName(const std::string& name)const;
//
//		void SetIOCPU(int IOCPU){ m_IOCPU = IOCPU; }
//		int GetIOCPU()const{ return m_IOCPU; }
//
//		CGeoExtents GetMinimumDataExtents(bool bQuiet = true);
//		bool GetXLimit(bool bMin, int& value, bool bQuiet = true);
//		bool GetYLimit(bool bMin, int& value, bool bQuiet = true);
//
//		size_t GetNbScenes()const{ return m_nbScenes; }
//		size_t GetSceneSize()const{ return m_sceneSize; }
//
//	protected:
//
//		CSingleBandHolderPtrVector m_bandHolder;
//		CSingleBandHolderPtr m_pMaskBandHolder;
//
//		CGeoExtents m_entireExtents;
//
//		size_t m_nbScenes;
//		size_t m_sceneSize;
//		//CTPeriod m_entirePeriod;//extraction period ??
//		//std::vector<CTPeriod> m_scenesPeriod;//period of scenes
//
//		int m_maxWindowSize;
//
//		int m_IOCPU;
//		double m_memoryLimit;
//		DataType m_maskDataUsed;
//
//		bool m_bEmpty;
//	};
//
//	template< class T>
//	class CBandsHolderMTTemplate : public T
//	{
//	public:
//
//		CBandsHolderMTTemplate(int maxWindowSize = 1, double  memoryLimit = 0, int IOCPU = 1, size_t nbThread = 1) :T(maxWindowSize, memoryLimit / nbThread, IOCPU)
//		{
//			assert(nbThread >= 1);
//
//			for (size_t i = 0; i < nbThread - 1; i++)
//				m_thread.push_back(CMultiBlockHolderPtr(new T(maxWindowSize, memoryLimit / nbThread, IOCPU)));
//		}
//
//
//		const CBandsHolder& operator[](size_t i)const
//		{
//			assert(i <= m_thread.size());
//
//			if (i < m_thread.size())
//				return *(m_thread[i]);
//
//			return *this;
//		}
//
//
//		CBandsHolder& operator[](size_t i)
//		{
//			assert(i <= m_thread.size());
//
//			if (i < m_thread.size())
//				return *(m_thread[i]);
//
//			return *this;
//		}
//
//		//using T::Load;
//		ERMsg Load(const CGDALDatasetEx& inputDS, bool bQuiet = true, const CGeoExtents& extents = CGeoExtents())
//		{
//			ERMsg msg;
//
//			//lead master tread
//			msg += T::Load(inputDS, bQuiet, extents);
//
//			//load all other thread
//			for (size_t i = 0; i < m_thread.size() && msg; i++)
//			{
//				msg += m_thread[i]->Load(inputDS, true, extents);
//			}
//
//			return msg;
//		}
//		size_t GetNbThread()const{ return m_thread.size() + 1; }
//
//		void SetMask(CSingleBandHolderPtr pBandHolder, DataType maskDataUsed)
//		{
//			for (size_t i = 0; i < m_thread.size(); i++)
//			{
//				//copy the band holder
//				m_thread[i]->SetMask(CSingleBandHolderPtr(pBandHolder->GetCopy()), maskDataUsed);
//			}
//
//			T::SetMask(CSingleBandHolderPtr(pBandHolder->GetCopy()), maskDataUsed);
//		}
//
//
//		typedef std::shared_ptr<T> CMultiBlockHolderPtr;
//		typedef std::vector<CMultiBlockHolderPtr> CMultiBlockHolderPtrVector;
//
//	protected:
//
//		CMultiBlockHolderPtrVector m_thread;
//
//	};
//
//	typedef CBandsHolderMTTemplate<CBandsHolder> CBandsHolderMT;
//
//
//
//	class CGeoCoordFile : public std::vector<std::string>
//	{
//	public:
//
//		CGeoCoordFile()
//		{
//			m_prjID = PRJ_NOT_INIT;
//			m_Xcol = NOT_INIT;
//			m_Ycol = NOT_INIT;
//		}
//
//		std::string m_header;
//		CGeoPointVector m_xy;
//		size_t m_Xcol;
//		size_t m_Ycol;
//
//		ERMsg Load(const std::string& filePath, const std::string X = "X", const std::string Y = "Y");
//		ERMsg Save(const std::string& filePath, bool bSaveDefinitionFile=true);
//
//		//vector<size_t> GetCoordinate(const std::vector<std::string>& header, const std::string X = "X", const std::string Y = "Y");
//		ERMsg ManageProjection(size_t dstPrjID);
//
//		size_t GetPrjID()const { return m_prjID; }
//		void SetPrjID(size_t ID){  m_prjID= ID;  }
//
//	protected:
//
//		size_t m_prjID;
//	};
//
//	class CGeoCoordTimeFile : public CGeoCoordFile
//	{
//	public:
//
//		CGeoCoordTimeFile()
//		{}
//
//		std::vector<std::string> m_time;
//
//		ERMsg Load(const std::string& filePath, const std::string X = "X", const std::string Y = "Y", const std::string Time = "Time");
//
//	};
//

}
