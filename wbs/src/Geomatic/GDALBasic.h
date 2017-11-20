//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include <vector>
#include <deque>
#include <float.h>
#include <memory>
#include <array>

#include "basic/ERMsg.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilStd.h"
#include "Basic/Statistic.h"
#include "Basic/UtilMath.h"
#include "Basic/Callback.h"
#include "Basic/Registry.h"
#include "Basic/Timer.h"
#include "Basic/GeoBasic.h"

#include "Geomatic/UtilGDAL.h"
#include "Geomatic/Projection.h"

class MTParser;

namespace WBSF
{
	class CProjection;
	class CGDALDatasetExVector;
	class CGDALDatasetEx;
	class CSingleBandHolder;
	class CDataLine;
	class CDataWindow;

	typedef std::shared_ptr<CDataWindow> CDataWindowPtr;
	typedef std::map<std::string, std::string> MetaData;
	typedef std::vector<MetaData> MetaDataVector;
	typedef std::vector<std::map<std::string, std::string>> BandsMetaData;


	class CDataWindow
	{
	public:


		CDataWindow(const CGeoRectIndex& windowRect, const DataVector* pData, const CGeoRectIndex& dataRect, double noData, DataType maskDataUsed, double nsres, double ewres, bool bProjected, __int16 captor)
		{
			ASSERT(pData == NULL || (int)pData->size() == dataRect.Height()*dataRect.Width());
			ASSERT(pData == NULL || dataRect.IsRectIntersect(windowRect));
			m_pData = pData;
			m_dataRect = dataRect;
			m_windowRect = windowRect;
			m_noData = noData;
			m_maskDataUsed = maskDataUsed;
			m_nsres = nsres;
			m_ewres = ewres;
			m_bProjected = bProjected;
			m_captor = captor;
		}


		void SetMask(CDataWindowPtr pMaskWindow)
		{
			m_pMaskWindow.swap(pMaskWindow);
		}

		void GetStat(CStatistic& stat)const
		{
			if (m_pData != NULL)
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

		int XSize()const{ return m_windowRect.Width(); }
		int YSize()const{ return m_windowRect.Height(); }
		size_t size()const{ return m_windowRect.Height()*m_windowRect.Width(); }
		CGeoSize GetGeoSize()const{ return m_windowRect.GetGeoSize(); }


		bool IsInside(CGeoPointIndex pt)const{ return IsInside(pt.m_x, pt.m_y); }
		bool IsInside(int x, int y)const
		{
			x += m_windowRect.m_x;
			y += m_windowRect.m_y;
			return m_dataRect.PtInRect(x, y);
		}

		DataType at(CGeoPointIndex pt)const{ return at(pt.m_x, pt.m_y); }
		DataType at(int x, int y)const
		{
			if (m_pData == NULL)
				return (DataType)m_noData;

			x += m_windowRect.m_x;
			y += m_windowRect.m_y;

			DataType v = (DataType)m_noData;
			if (m_dataRect.PtInRect(x, y))
			{
				__int64 pos = ((__int64)y - m_dataRect.m_y)*m_dataRect.Width() + (x - m_dataRect.m_x);
				ASSERT(pos >= 0 && pos < (__int64)m_pData->size());

				if (pos >= 0 && pos < (__int64)m_pData->size())
				{
					v = m_pData->at((size_t)pos);
					if (m_pMaskWindow && (m_pMaskWindow.get() != NULL) && m_pMaskWindow->IsPixelMasked(x, y))
						v = (DataType)m_noData;
				}
			}
			return v;
		}



		DataType operator ()(int x, int y)const{ return at(x, y); }
		bool IsValid(CGeoPointIndex xy)const	{ return IsValid(at(xy)); }
		bool IsValid(int x, int y)const	{ return IsValid(at(x, y)); }

		bool IsValid(DataType v)const
		{
			return m_noData == MISSING_NO_DATA || fabs(v - (DataType)m_noData) > EPSILON_NODATA;
		}

		double GetNoData()const{ return m_noData; }
		__int16 GetCaptor()const{ return m_captor; }
	protected:

		//does the pixel is used. this layer is a mask layer
		bool IsPixelMasked(int x, int y)const
		{
			if (m_pData == NULL)
				return false;

			size_t pos = (m_windowRect.m_y - m_dataRect.m_y + y)*m_dataRect.Width() + m_windowRect.m_x - m_dataRect.m_x + x;
			if (pos >= m_pData->size())
				return false;

			DataType maskValue = m_pData->at(pos);

			return (m_maskDataUsed == DataTypeMin) ? fabs(maskValue - m_noData) < EPSILON_NODATA : maskValue != m_maskDataUsed;
		}

		const DataVector* m_pData;
		CGeoRectIndex m_dataRect;
		CGeoRectIndex m_windowRect;
		double m_noData;
		DataType m_maskDataUsed;
		double m_nsres;
		double m_ewres;
		bool m_bProjected;
		__int16 m_captor;

		CDataWindowPtr m_pMaskWindow;
	};

	class CRasterWindow : public std::vector < CDataWindowPtr >
	{
	public:

		CRasterWindow(size_t sceneSize = 0, CGeoRectIndex windowRect = CGeoRectIndex())
		{
			m_sceneSize = sceneSize;
			m_windowRect = windowRect;
		}

		size_t GetNbScenes()const{ return size() / m_sceneSize; }
		size_t GetSceneSize()const{ return m_sceneSize; }

		template <class T>
		bool IsValid(size_t i, const T& pixel)const
		{
			assert(((i + 1)*m_sceneSize - 1) < size());
			const_iterator it1 = begin() + i*m_sceneSize;

			bool bValid = true;
			for (T::const_iterator it2 = pixel.begin(); it2 != pixel.end() && bValid; it2++)
				bValid = (*it1)->IsValid(*it2);


			return bValid;
		}

		template <class T>
		bool IsValid(size_t i, size_t z, const T& pixel)const
		{
			assert((i*m_sceneSize + z) < size());
			return at(i*m_sceneSize + z)->IsValid(pixel);
		}

		CGeoSize GetGeoSize()const 	{ return m_windowRect.GetGeoSize(); }

	protected:

		size_t m_sceneSize;
		CGeoRectIndex m_windowRect;
	};


	class CBandsHolder;

	class CSingleBandHolder
	{
	public:

		bool m_bDontLoadContantBand;

		CSingleBandHolder(GDALDataset* pDataset = NULL, size_t bandNo = 0, std::string bandName = "");
		CSingleBandHolder(const CSingleBandHolder& in);
		~CSingleBandHolder();
		CSingleBandHolder* GetCopy(){ return new CSingleBandHolder(*this); }//(m_pDataset, m_bandNo, m_bandName); }


		virtual ERMsg Load(CBandsHolder* pParent)
		{
			ASSERT(pParent);
			ERMsg msg;
			m_pParent = pParent;
			return msg;
		}

		virtual void Init(int windowSize);
		void FlushCache(double yMax = -9999999999);


		bool IsValid(DataType v)const
		{
			return m_noData == (DataType)MISSING_NO_DATA || (fabs(v - (DataType)m_noData) > EPSILON_NODATA);// && v>-1.0e38
		}

		DataType GetPixel(int x, int y)const
		{
			size_t pos = GetOffset(x, y);
			return (pos >= 0 && pos < m_data.size()) ? m_data[pos] : (DataType)m_noData;
		}


		CDataWindowPtr GetWindow(const CGeoRectIndex& rectIn)const
		{
			if (m_dataRect.IsRectEmpty() || !m_dataRect.IsRectIntersect(rectIn))
				return CDataWindowPtr(new CDataWindow(CGeoRectIndex(), NULL, CGeoRectIndex(), m_noData, DataTypeMin, m_nsres, m_ewres, m_bProjected, m_captor));

			return CDataWindowPtr(new CDataWindow(rectIn, &m_data, m_dataRect, m_noData, m_maskDataUsed, m_nsres, m_ewres, m_bProjected, m_captor));
		}

		CDataWindowPtr GetWindow(int x, int y, int xSize, int ySize)const{ return GetWindow(CGeoRectIndex(x - xSize / 2, y - ySize / 2, x + (xSize - 1) / 2 + 1, y + (ySize - 1) / 2 + 1)); }
		CDataWindowPtr GetWindow(int x, int y, int size)const{ return GetWindow(x, y, size, size); }
		CDataWindowPtr GetLine(int y)const	{ return GetWindow(CGeoRectIndex(0, y, m_extents.m_xSize, y + 1)); }

		CGeoExtents GetDatasetExtents()const{ return m_datasetExtent; }
		const CGeoExtents& GetExtents()const{ return m_extents; }
		void SetExtents(const CGeoExtents& extents){ m_extents = extents; }

		const CGeoExtents& GetInternalMapExtents()const{ return m_internalMapExtents; }
		void SetInternalMapExtents(const CGeoExtents& extents){ m_internalMapExtents = extents; }

		bool ExcludeBand() const { return m_bExclude; }
		void ExcludeBand(bool bExclude) { m_bExclude = bExclude; }

		bool IsUsed(int x, int y)const
		{
			if (m_pDataset == NULL)
				return false;

			ASSERT(x >= 0 && x < m_dataRect.Width());
			ASSERT(y >= 0 && y < m_dataRect.Height());

			size_t pos = GetOffset(x, y);
			DataType value = m_data[pos];

			return fabs(value - m_noData) > EPSILON_NODATA;
		}

		bool IsMasked(int x, int y)const
		{
			if (m_pDataset == NULL)
				return false;

			ASSERT(x >= 0 && x < m_dataRect.Width());

			size_t pos = GetOffset(x, y);
			DataType value = m_data[pos];

			return (m_maskDataUsed == DataTypeMin) ? fabs(value - m_noData) <= EPSILON_NODATA : value != m_maskDataUsed;
		}

		double GetNoData()const{ return m_noData; }
		void SetMaskDataUsed(DataType maskDataUsed){ m_maskDataUsed = maskDataUsed; }
		DataType GetMaskDataUsed()const{ return m_maskDataUsed; }
		const DataVector* GetData()const { return &m_data; }
		DataVector* GetData(){ return &m_data; }

		GDALDataset* Dataset()const { return m_pDataset; }
		const CGeoRectIndex& GetDataRect()const{ return m_dataRect; }

	protected:

		size_t GetOffset(size_t x, size_t y)const
		{
			return y*m_dataRect.Width() + x;
		}


		GDALDataset* m_pDataset;
		size_t m_bandNo;
		std::string m_bandName;

		double m_noData;
		CGeoExtents m_datasetExtent;
		DataType m_maskDataUsed;
		CGeoExtents m_internalMapExtents;
		bool m_bExclude;
		bool m_bConstantBand;

		CGeoExtents m_extents;
		DataVector m_data;
		CGeoRectIndex m_dataRect;
		double m_nsres;
		double m_ewres;
		bool m_bProjected;
		__int16 m_captor;

		CBandsHolder* m_pParent;
		//CCriticalSection m_CS;
	};

	typedef std::shared_ptr<CSingleBandHolder> CSingleBandHolderPtr;
	typedef std::vector<CSingleBandHolderPtr> CSingleBandHolderPtrVector;

	//**************************************************************************************************
	typedef std::vector< size_t > CBandToData;
	typedef std::vector< std::vector< size_t> > CVariableToData;
	typedef std::vector< std::vector< size_t> > CVariablePos;


	class CMTParserVariable
	{
	public:

		std::string m_name;
		double m_value;
		double m_noData;

		CMTParserVariable(std::string name = "", double noData = MISSING_NO_DATA);
		CMTParserVariable(const CMTParserVariable& in);
		CMTParserVariable& operator = (const CMTParserVariable& in);

		bool operator == (const CMTParserVariable& in)const;
		bool operator != (const CMTParserVariable& in)const;


		bool IsValid()const{ return (m_noData != MISSING_NO_DATA) ? abs(m_value - m_noData) > EPSILON_NODATA:true; }
	};


	typedef std::vector<CMTParserVariable> CMTParserVariableVector;


	class CVirtualBandHolder : public CSingleBandHolder
	{
	public:


		std::string m_formula;

		CVirtualBandHolder(const std::string& formula = "");
		~CVirtualBandHolder();

		virtual ERMsg Load(CBandsHolder* pParent);
		virtual void Init(int windowSize);

	protected:

		double Evaluate(const std::vector<float>& vars);

		CMTParserVariableVector m_vars;
		MTParser* m_pParser;

		//virtual bands
		std::vector<size_t> m_bandPos;
	};

	typedef std::vector<CVirtualBandHolder> CVirtualBandHolderVector;



	class CGDALDatasetEx
	{
	public:

		CGDALDatasetEx();
		~CGDALDatasetEx();

		virtual ERMsg OpenInputImage(const std::string& filePath, const CBaseOptions& options = CBaseOptions());
		virtual ERMsg CreateImage(const std::string& filePath, const CBaseOptions& option);
		virtual CSingleBandHolderPtr GetSingleBandHolder(size_t bandNo = 0)const;
		virtual void GetBandsHolder(CBandsHolder& BandsHolder)const;
		virtual void UpdateOption(CBaseOptions& options)const;
		virtual void Close(const CBaseOptions& options = CBaseOptions());

		//ERMsg OpenInputImage(const std::string& filePath, double srcNodata, int IOCPU, bool bUseDefaultNoData);
		//ERMsg CreateImage(const std::string& filePath, const CGeoExtents& extents, int nbBand = 1, double dstNodata = MISSING_NO_DATA, bool bUseDefaultNoData = true, const std::string& prj = "", const char* outDriverName = "GTiff", short cellType = 3, const StringVector& createOptions = StringVector(), bool bOverwrite = true);

		void BuildOverviews(const std::vector<int>& list, bool bQuiet);
		void ComputeStats(bool bQuiet);
		void ComputeHistogram(bool bQuiet);

		int GetRasterXSize()const;
		int GetRasterYSize()const;
		size_t GetRasterCount()const;
		ERMsg BuildVRT(bool bQuiet);
		void GetGeoTransform(CGeoTransform GT)const;
		
		GDALRasterBand *GetRasterBand(size_t i);
		const GDALRasterBand *GetRasterBand(size_t i)const;

		GDALDataset* operator->(){ return m_poDataset; }
		GDALDataset* operator->()const{ return m_poDataset; }
		GDALDataset* Dataset(){ return m_poDataset; }
		GDALDataset* Dataset()const{ return m_poDataset; }
		GDALDataset* Detach(){ GDALDataset* poDataset = m_poDataset; m_poDataset = NULL; m_pProjection.reset(); m_filePath.clear();  return poDataset; }

		
		bool IsOpen()const{ return !m_filePath.empty(); }// m_poDataset != NULL;
		bool IsOpenUpdate()const{ return m_bOpenUpdate; }

		const std::string& GetFilePath()const{ return m_filePath; }

		short GetDataType(size_t i = 0)const;
		ERMsg VerifyNoData(double nodata, size_t i = 0)const;
		double GetBandLimit(size_t i, bool bLow)const;
		double LimitToBound(double v, size_t i = 0, double shiftedBy = 0, bool bRound = true)const;
		double GetDefaultNoData(size_t i = 0)const;
		double GetNoData(size_t i)const;
		std::vector<double> GetNoData()const;
		bool HaveNoData(size_t i = 0)const;
		double PostTreatment(double v, size_t i = 0, double shiftedBy = 0, bool bRound = true);


		bool IsVRT()const { return m_bVRT; }
		double ReadPixel(size_t i, const CGeoPointIndex& xy)const{ return ReadPixel(i, xy.m_x, xy.m_y); }
		double ReadPixel(size_t i, int x, int y)const;
		CGeoSize ComputeBlockSize(double memoryLimit, const CGeoExtents& extents, CTPeriod period, CTM TM)const;
		int GetMaxBlockSizeZ(CGeoExtents extents, CTPeriod period, CTM TM)const;

		const std::vector<std::string>& GetVirtualBand()const{ return m_virtualBands; }

		const std::string& GetInternalName(size_t i)const{ ASSERT(i < m_internalName.size());  return m_internalName[i]; }
		const CGeoExtents& GetInternalExtents(size_t i)const{ ASSERT(i < m_internalExtents.size()); return m_internalExtents[i]; }

		CStatistic GetWindowMean(size_t i, int x1, int y1, int x2, int y2){ return WBSF::GetWindowMean(GetRasterBand(i), x1, y1, x2, y2); }
		double GetWindowMean(size_t i, int nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d){ return WBSF::GetWindowMean(GetRasterBand(i), nbNeighbor, power, pts, d); }

		ERMsg GetRegularCoord(int nbPointX, int nbPointY, bool bExp, const CGeoRect& rect, CGridPointVector& points, CCallback& callBack = DEFAULT_CALLBACK);
		ERMsg GetRandomCoord(int nbPoint, bool bExp, bool bExtrem, double factor, const CGeoRect& rect, CGridPointVector& points, CCallback& callBack = DEFAULT_CALLBACK);

		CProjectionPtr GetPrj()const{ return m_pProjection; }
		size_t GetPrjID()const{ return m_pProjection ? m_pProjection->GetPrjID() : PRJ_NOT_INIT; }

		const CGeoExtents& GetExtents()const{ return m_extents; }
		void FlushCache(double yMax=-9999999999);

		void SetVRTBand(size_t i, GDALDataset* pDataset);
		void CloseVRTBand(size_t i);
		void GetBandsMetaData(BandsMetaData& meta_data)const;

		//temporal section
		size_t GetSceneSize()const{ return m_scenesPeriod.size()>0 ? GetRasterCount() / m_scenesPeriod.size() : GetRasterCount(); }
		size_t GetNbScenes()const{ return m_scenesPeriod.size(); }
		const std::vector<CTPeriod>& GetScenePeriod()const{ return m_scenesPeriod; }

		CTPeriod GetPeriod()const
		{
			CTPeriod p;
			for (std::vector<CTPeriod>::const_iterator it = m_scenesPeriod.begin(); it != m_scenesPeriod.end(); it++)
				p += *it;

			return p;
		}

		std::string GetCommonBandName();

	protected:

		//std::string GetMetaDataFilePath(const std::string& filePath)const;
		//ERMsg LoadMetaData(const std::string& filePath);

		bool m_bVRT;
		bool m_bOpenUpdate;
		GDALDataset* m_poDataset;
		CProjectionPtr m_pProjection;

		std::vector<CGeoExtents> m_internalExtents;
		StringVector m_internalName;
		CGeoExtents m_extents;
		std::vector<std::string> m_virtualBands;

		std::string m_filePath;

		//this vector is intended to keep each band of a VRT file open for writing
		bool m_bMultipleImages;// for VRT output only
		std::vector<GDALDataset*> m_poDatasetVector;

		//temporal section of the dataset
		std::vector<CTPeriod> m_scenesPeriod;//one period per scenes

	};

	class CGDALDatasetExVector : public std::vector < CGDALDatasetEx >
	{
	public:

		CGDALDatasetExVector(size_t t = 0) : std::vector<CGDALDatasetEx>(t)
		{}

		ERMsg VerifyInputSize(CGDALDatasetExVector& inputDS)
		{
			ERMsg msg;
			for (size_t i = 1; i < inputDS.size() && msg; i++)
				msg += WBSF::VerifyInputSize(inputDS[i].Dataset(), at(i).Dataset());

			return msg;
		}

		CGeoExtents GetExtents()const
		{
			CGeoExtents extents;
			for (size_t i = 0; i < size(); i++)
				extents.UnionExtents(at(i).GetExtents());

			return extents;
		}

	};

	ERMsg VerifyInputSize(CGDALDatasetExVector& inputDS);


	class CEquationDefinition
	{
	public:

		std::string m_equationName;
		std::string m_equation;
	};

	class CGDALDatasetCalculator : public CGDALDatasetEx
	{
	public:

		ERMsg Open(const std::string& filePath, const CBaseOptions& option);
		void GetBandsHolder(CBandsHolder& multiBand);

		//equation
	protected:

		std::vector<CEquationDefinition> m_equations;
	};



	//**************************************************************************************************

	class CBandsHolder
	{
	public:


		CBandsHolder(int maxWindowSize = 1, double memoryLimit = 0, int IOCPU = 1)
		{
			m_maxWindowSize = maxWindowSize;
			m_memoryLimit = memoryLimit;
			m_IOCPU = IOCPU;
			m_maskDataUsed = DataTypeMin;
			m_bEmpty = true;
			m_nbScenes = 0;
			m_sceneSize = 0;
		}



		void AddBand(CSingleBandHolderPtr pBandHolder){ m_bandHolder.push_back(pBandHolder); }
		size_t GetRasterCount()const{ return m_bandHolder.size(); }

		virtual ERMsg Load(const CGDALDatasetEx& inputDS, bool bQuiet = true, const CGeoExtents& extents = CGeoExtents(), CTPeriod period = CTPeriod());
		void LoadBlock(int xBlock, int yBlock, CTPeriod p = CTPeriod()){ LoadBlock(m_entireExtents.GetBlockExtents(xBlock, yBlock), p); }
		virtual void LoadBlock(CGeoExtents extents, CTPeriod p = CTPeriod());

		virtual double Evaluate(size_t virtualBandNo, const std::vector<float>& variable){ ASSERT(false); return DBL_MAX; }
		void FlushCache(double yMax = -9999999999);

		CGeoSize ComputeBlockSize(const CGeoExtents& extents, CTPeriod period)const;

		DataType GetPixel(size_t layer, int x, int y)const
		{
			DataType noData = (DataType)m_bandHolder[layer]->GetNoData();

			const CGeoRectIndex& rect = m_bandHolder[layer]->GetDataRect();
			if (rect.PtInRect(x, y))
			{
				if (m_pMaskBandHolder.get() && m_pMaskBandHolder->GetDataRect().PtInRect(x, y))
				{
					ASSERT(m_pMaskBandHolder.get());
					if (!m_pMaskBandHolder->IsMasked(x, y))
						return noData;
				}

				return m_bandHolder[layer]->GetPixel(x, y);
			}

			return noData;
		}

		CDataWindowPtr GetWindow(size_t layer, const CGeoRectIndex& rect)const
		{
			ASSERT(layer < m_bandHolder.size());
			ASSERT(m_entireExtents.GetPosRect().IsRectIntersect(rect));

			CDataWindowPtr pWindow = m_bandHolder[layer]->GetWindow(rect);

			if (m_pMaskBandHolder.get() && m_pMaskBandHolder->GetDataRect().IsRectIntersect(rect))
			{
				ASSERT(m_pMaskBandHolder.get());
				const CSingleBandHolder* pBand = m_pMaskBandHolder.get();
				pWindow->SetMask(m_pMaskBandHolder->GetWindow(rect));
			}

			return pWindow;
		}

		CDataWindowPtr GetWindow(size_t layer)const
		{
			return GetWindow(layer, m_bandHolder[layer]->GetExtents().GetPosRect());
		}

		CDataWindowPtr GetWindow(size_t layer, int x, int y, int XSize, int YSize)const
		{
			return GetWindow(layer, CGeoRectIndex(x - XSize / 2, y - YSize / 2, x + (XSize - 1) / 2 + 1, y + (YSize - 1) / 2 + 1));
		}

		CDataWindowPtr GetWindow(size_t layer, int x, int y, int size)const
		{
			return GetWindow(layer, x, y, size, size);
		}

		void GetWindow(std::vector<CDataWindowPtr>& input)const
		{
			input.resize(GetRasterCount());
			for (size_t i = 0; i < input.size(); i++)
				input[i] = GetWindow(int(i));
		}

		CRasterWindow GetWindow()const
		{
			if (m_bEmpty)
				return CRasterWindow();

			CRasterWindow input(m_sceneSize, m_bandHolder.front()->GetExtents().GetPosRect());
			input.resize(GetRasterCount());
			for (size_t i = 0; i < input.size(); i++)
				input[i] = GetWindow(int(i));

			return input;
		}


		CDataWindowPtr GetLine(size_t layer, int y)const
		{
			ASSERT(layer >= 0 && layer < GetRasterCount());
			return GetWindow(layer, CGeoRectIndex(0, y, m_entireExtents.GetPosRect().Width(), y + 1));
		}


		bool IsValid(size_t layer, DataType v)const{ return m_bandHolder[layer]->IsValid(v); }

		template <class T>
		void GetBlockLineAllLayer(int y, std::vector<std::vector<T>>& input, bool byBandFirst = true)
		{
			size_t nXSize = m_extents.m_xBlockSize;
			size_t nbBand = GetRasterCount();

			if (input.size() != (byBandFirst ? nbBand : nXSize))
			{
				input.resize(byBandFirst ? nbBand : nXSize);
				for (size_t i = 0; i < input.size(); i++)
					input[i].resize(byBandFirst ? nXSize : nbBand);
			}

			for (size_t z = 0; z < nbBand; z++)
			{
				CDataWindowPtr pLine = GetLine((int)z, y);//zero base 
				for (size_t x = 0; x < nXSize; x++)
				{
					if (byBandFirst)
						input[z][x] = (T)pLine->at((int)x, 0);
					else 
						input[x][z] = (T)pLine->at((int)x, 0);
				}
			}
		}


		template <class T>
		void GetBlockAllLayer(std::vector<std::vector<std::vector<T>>>& input)
		{
			CGeoSize size = m_extents.GetSize();
			size_t nXSize = size.m_x;
			size_t nYSize = size.m_y;
			size_t nbBand = GetRasterCount();

			if (input.size() != nbBand || input[0].size() != nYSize || input[0][0].size() != nXSize)
			{
				input.resize(nbBand);
				for (size_t i = 0; i < input.size(); i++)
				{
					input[i].resize(nYSize);
					for (size_t j = 0; j < input[i].size(); j++)
						input[i][j].resize(nXSize);
				}
			}

			for (size_t z = 0; z < nbBand; z++)
			{
				CDataWindowPtr pWindow = GetWindow((int)z);//zero base 
				for (size_t y = 0; y < nYSize; y++)
				{
					for (size_t x = 0; x < nXSize; x++)
					{
						input[z][y][x] = (T)pWindow->at((int)x, (int)y);
					}
				}
			}
		}

		const CGeoExtents& GetExtents()const{ return m_entireExtents; }
		void SetExtents(const CGeoExtents& extents){ m_entireExtents = extents; }

		CTPeriod GetPeriod()const
		{
			CTPeriod p;
			for (std::vector<CTPeriod>::const_iterator it = m_scenesPeriod.begin(); it != m_scenesPeriod.end(); it++)
				p += *it;

			return p;
		}

		void SetScenePeriod(const std::vector<CTPeriod>& scenePeriod){ m_scenesPeriod = scenePeriod; }

		size_t size()const{ return m_bandHolder.size(); }
		const CSingleBandHolderPtr& operator[](size_t v)const{ return m_bandHolder[v]; }

		int GetBlockSizeX(int i = -1, int j = -1)const{ return m_entireExtents.GetBlockSize(i, j).m_x; }
		int GetBlockSizeY(int i = -1, int j = -1)const{ return m_entireExtents.GetBlockSize(i, j).m_y; }
		int GetBlockSizeZ(int i = -1, int j = -1, CTPeriod p = CTPeriod())const;
		std::vector<bool> GetIntersectBands()const;

		int GetMaxWindowSize()const { return m_maxWindowSize; }
		void SetMaxWindowSize(int maxWindowSize){ m_maxWindowSize = maxWindowSize; }

		void SetMask(CSingleBandHolderPtr pBandHolder, DataType maskDataUsed)
		{
			m_pMaskBandHolder = pBandHolder;
			m_maskDataUsed = maskDataUsed;
		}

		bool IsEmpty()const{ return m_bEmpty; }

		size_t GetBandByName(const std::string& name)const;

		void SetIOCPU(int IOCPU){ m_IOCPU = IOCPU; }
		int GetIOCPU()const{ return m_IOCPU; }

		CGeoExtents GetMinimumDataExtents(bool bQuiet = true);
		bool GetXLimit(bool bMin, int& value, bool bQuiet = true);
		bool GetYLimit(bool bMin, int& value, bool bQuiet = true);

		size_t GetNbScenes()const{ return m_nbScenes; }
		size_t GetSceneSize()const{ return m_sceneSize; }

	protected:

		CSingleBandHolderPtrVector m_bandHolder;
		CSingleBandHolderPtr m_pMaskBandHolder;

		CGeoExtents m_entireExtents;

		size_t m_nbScenes;
		size_t m_sceneSize;
		CTPeriod m_entirePeriod;//extraction period ??
		std::vector<CTPeriod> m_scenesPeriod;//period of scenes

		int m_maxWindowSize;

		int m_IOCPU;
		double m_memoryLimit;
		DataType m_maskDataUsed;

		bool m_bEmpty;
	};

	template< class T>
	class CBandsHolderMTTemplate : public T
	{
	public:

		CBandsHolderMTTemplate(int maxWindowSize = 1, double  memoryLimit = 0, int IOCPU = 1, size_t nbThread = 1) :T(maxWindowSize, memoryLimit / nbThread, IOCPU)
		{
			ASSERT(nbThread >= 1);

			for (size_t i = 0; i < nbThread - 1; i++)
				m_thread.push_back(CMultiBlockHolderPtr(new T(maxWindowSize, memoryLimit / nbThread, IOCPU)));
		}


		const CBandsHolder& operator[](size_t i)const
		{
			ASSERT(i <= m_thread.size());

			if (i < m_thread.size())
				return *(m_thread[i]);

			return *this;
		}


		CBandsHolder& operator[](size_t i)
		{
			ASSERT(i <= m_thread.size());

			if (i < m_thread.size())
				return *(m_thread[i]);

			return *this;
		}

		//using T::Load;
		ERMsg Load(const CGDALDatasetEx& inputDS, bool bQuiet = true, const CGeoExtents& extents = CGeoExtents(), CTPeriod period = CTPeriod())
		{
			ERMsg msg;

			//lead master tread
			msg += T::Load(inputDS, bQuiet, extents, period);

			//load all other thread
			for (size_t i = 0; i < m_thread.size() && msg; i++)
			{
				msg += m_thread[i]->Load(inputDS, true, extents, period);
			}

			return msg;
		}
		size_t GetNbThread()const{ return m_thread.size() + 1; }

		void SetMask(CSingleBandHolderPtr pBandHolder, DataType maskDataUsed)
		{
			for (size_t i = 0; i < m_thread.size(); i++)
			{
				//copy the band holder
				m_thread[i]->SetMask(CSingleBandHolderPtr(pBandHolder->GetCopy()), maskDataUsed);
			}

			T::SetMask(CSingleBandHolderPtr(pBandHolder->GetCopy()), maskDataUsed);
		}


		typedef std::shared_ptr<T> CMultiBlockHolderPtr;
		typedef std::vector<CMultiBlockHolderPtr> CMultiBlockHolderPtrVector;

	protected:

		CMultiBlockHolderPtrVector m_thread;

	};

	typedef CBandsHolderMTTemplate<CBandsHolder> CBandsHolderMT;



	class CGeoCoordFile : public StringVector
	{
	public:

		CGeoCoordFile()
		{
			m_prjIDSource = PRJ_NOT_INIT;
		}

		std::string m_header;
		CGeoPointVector m_xy;

		ERMsg Load(const std::string& filePath, const std::string X = "X", const std::string Y = "Y");
		ERMsg Save(const std::string& filePath);

		vector<size_t> GetCoordinate(const StringVector& header, const std::string X = "X", const std::string Y = "Y");
		ERMsg ManageProjection(size_t dstPrjID);

	protected:

		size_t m_prjIDSource;
	};



}