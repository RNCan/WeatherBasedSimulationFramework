//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <map>
#include <memory>
#include <unordered_map>
//#include <combaseapi.h>
#include "ANN/ANN.h"

#include "Basic/GeoBasic.h"
#include "Basic/Statistic.h"
#include "Geomatic/PrePostTransfo.h"

#if !defined( interface )
#define interface struct
#endif

interface IAgent;

namespace WBSF
{

	class CVariogram;

	class CGridPointResult
	{
	public:
		size_t i;	//index of point 
		double d;//distance between search and result point
	};

	typedef std::vector<CGridPointResult> CGridPointResultVector;


	class CANNSearch
	{

	public:

		CANNSearch();
		CANNSearch(const CANNSearch& in){ operator=(in); }
		~CANNSearch();
		void Reset();

		CANNSearch& operator=(const CANNSearch& in)
		{
			if (&in != this)
			{
				m_bGeographic = in.m_bGeographic;
				m_bUseElevation = in.m_bUseElevation;
				m_pDataPts = in.m_pDataPts;		// data points
				m_nbDimension = in.m_nbDimension;
				m_nSize = m_nSize;
			}

			return *this;
		}

		void Init(CGridPointVectorPtr& pts, bool bUseElevation = false);
		void Search(const CGridPoint& pt, size_t nbPoints, CGridPointResultVector& result)const;

		size_t GetSize()const{ return m_nSize; }
		size_t GetDimension()const{ return m_nbDimension; }

	protected:

		bool m_bGeographic;
		bool m_bUseElevation;
		std::unique_ptr<ANNkd_tree> m_pTreeRoot;
		ANNpointArray m_pDataPts;		// data points
		size_t m_nbDimension;
		size_t m_nSize;
	};

	class CXvalTuple
	{
	public:
		CXvalTuple(double obs = 0, double pred = 0)
		{
			m_observed = obs;
			m_predicted = pred;
		}

		double m_observed;
		double m_predicted;
	};


	class CXValidationVector : public std::vector < CXvalTuple >
	{
	public:

		size_t size()const{ return std::vector<CXvalTuple>::size(); }

		void GetStatistic(CStatisticXY& stat, double noData)
		{
			for (size_t i = 0; i<size(); i++)
			{
				if (fabs(at(i).m_observed - noData)>EPSILON_NODATA &&
					fabs(at(i).m_predicted - noData)>EPSILON_NODATA )
					stat.Add(at(i).m_predicted, at(i).m_observed);
			}
		}

	};

	class CGridInterpolParam
	{
	public:

		enum TIWDModel{ BEST_IWD_MODEL = -1, IWD_CLASIC, IWD_TENSION, NB_IWD_MODEL };
		enum TVARIOGRAM{ BEST_VARIOGRAM = -1, SPERICAL, EXPONENTIAL, GAUSSIAN, POWER, NB_MODEL };
		enum TDetrending { BEST_DETRENDING = -1, NO_DETRENDING, /*...*/ };
		enum TExternalDrift { BEST_EXTERNAL_DRIFT = -1, NO_EXTERNAL_DRIFT, /*...*/ };
		enum TRegression { BEST_REGRESSION = -1, /*...*/ };
		enum TTPSType{ TPS_REGIONAL, TPS_GLOBAL, TPS_GLOBAL_WITH_CLUSTER, NB_TPSTYPE };

		enum TMember { NB_POINTS, OUTPUT_NO_DATA, MAX_DISTANCE, GDAL_OPTIONS, REGIONAL_LIMIT, REGIONAL_SD, REGIONAL_LIMIT_TO_BOUND, GLOBAL_LIMIT, GLOBAL_SD, GLOBAL_LIMIT_TO_BOUND, GLOBAL_MINMAX_LIMIT, GLOBAL_MIN_LIMIT, GLOBAL_MAX_LIMIT, GLOBAL_MINMAX_LIMIT_TO_BOUND, REGRESSION_MODEL, REGRESS_CRITICAL_R2, VARIOGRAM_MODEL, NB_LAGS, LAG_DISTANCE, DETRENDING_MODEL, EXTERNAL_DRIFT, FILL_NUGGET, IWD_MODEL, IWD_POWER, IWD_USE_ELEV, TPS_TYPE, OUTPUT_VARIOGRAM_INFO, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

		CGridInterpolParam();
		CGridInterpolParam(const CGridInterpolParam& in);

		void Reset();
		CGridInterpolParam& operator =(const CGridInterpolParam& in);
		bool operator ==(const CGridInterpolParam& in)const;
		bool operator !=(const CGridInterpolParam& in)const{ return !operator==(in); }

		std::string GetMember(size_t i)const;

		//Common	
		size_t	m_nbPoints;
		double	m_noData;
		double	m_maxDistance;
		std::string  m_GDALOptions;

		bool m_bRegionalLimit;
		double m_regionalLimitSD;
		bool m_bRegionalLimitToBound;
		bool m_bGlobalLimit;
		double m_globalLimitSD;
		bool m_bGlobalLimitToBound;

		bool m_bGlobalMinMaxLimit;
		double m_globalMinLimit;
		double m_globalMaxLimit;
		bool m_bGlobalMinMaxLimitToBound;


		//Regression 
		std::vector<int> m_regressionModel;
		double	m_regressCriticalR2;

		//Kriging
		int		m_variogramModel;
		int		m_nbLags;
		double	m_lagDist;
		int		m_detrendingModel;
		int		m_externalDrift;
		bool	m_bFillNugget;
		bool	m_bOutputVariogramInfo;


		//inverse weighted distance
		int m_IWDModel;
		double  m_power;
		bool m_bUseElevation;


		//Thin Plate Spline
		int m_TPSType;

	protected:

		//	static CStringArrayEx METHOD_NAME;
		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};

	class CGridInterpolParamPtr : public std::unique_ptr < CGridInterpolParam >
	{
	public:

		CGridInterpolParamPtr()
		{
			reset(new CGridInterpolParam);
		}
	};



	typedef std::vector<CGridInterpolParam> CGridInterpolParamVector;

	class CGridInterpolInfo
	{
	public:

		enum TMember { CELL_SIZE_X, CELL_SIZE_Y, OUTPUT_NO_DATA, MULTI, NB_CPU, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }


		CGridInterpolInfo();

		void Reset();
		std::string GetMember(int i)const;
	

		//utile information about the DEM and other
		double m_cellSizeX;
		double m_cellSizeY;
		double m_noData;
		bool	m_bMulti;
		int		m_nbCPU;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];
	};



	typedef std::unordered_map<double, std::unique_ptr<CANNSearch> > _CANNSearchCache;


	class CANNSearchCache : public _CANNSearchCache
	{
	public:

		CANNSearchCache()
		{}


		~CANNSearchCache()
		{
			FreeMemoryCache2();
		}

		void FreeMemoryCache2();


		void Enter()
		{
			m_CS.Enter();
		}
		void Leave()
		{
			m_CS.Leave();
		}

	protected:


		CCriticalSection m_CS;

	private:
		CANNSearchCache(const CANNSearchCache& in)
		{}

		CANNSearchCache& operator=(const CANNSearchCache& in)
		{
			return *this;
		}


	};



	class CGridInterpolBase
	{
	public:

		static void FreeMemoryCache();
		CGridInterpolBase();
		virtual ~CGridInterpolBase();

		static const char* DATA_DESCRIPTOR;

		virtual ERMsg Initialization();
		virtual void GetParamterset(CGridInterpolParamVector& parameterset){}
		virtual double GetOptimizedR²()const;

		virtual std::string GetFeedbackBestParam()const{ return std::string(); }
		virtual std::string GetFeedbackOnOptimisation(const CGridInterpolParamVector& parameterset, const std::vector<double>& optimisationR²)const{ return std::string(); }

		virtual double Evaluate(const CGridPoint& pt, int iXval = -1)const{ return -999; }

		virtual ERMsg GetXValidation(CXValidationVector& XValidation)const;
		virtual bool GetVariogram(CVariogram& variogram)const;
		virtual ERMsg Interpolation(const CGridPointVector& lineIn, CGridLine& lineOut)const;

		

		void GetOptimizedR²(std::istream& inStream, std::ostream& outStream);
		void Interpolation(std::istream& inStream, std::ostream& outStream);

		void WriteStream(std::ostream& stream)const;
		void ReadStream(std::istream& stream);

		const CGridPointVectorPtr& GetDataset()const{ return m_pPts; }
		void SetDataset(const CGridPointVectorPtr& pts){ m_pPts = pts; m_bInit = false; }

		const CGridInterpolInfo& GetInfo()const{ return m_info; }
		void SetInfo(const CGridInterpolInfo& info){ m_info = info; }
		const CGridInterpolParam& GetParam()const{ return m_param; }
		void SetParam(const CGridInterpolParam& param){ m_param = param;  }
		const CPrePostTransfo& GetPrePostTransfo()const	{ return m_prePostTransfo; }
		void SetPrePostTransfo(const CPrePostTransfo& prePostTransfo){ m_prePostTransfo = prePostTransfo;  }


		CGridPointVectorPtr GetPts(double);


	protected:

		ERMsg InternalInit()const;


		CGridPointVectorPtr m_pPts;
		CGridInterpolParam m_param;
		CPrePostTransfo m_prePostTransfo;
		CGridInterpolInfo m_info;
		CCriticalSection m_CS;
		IAgent* m_pAgent;
		DWORD m_hxGridSessionID;

		bool m_bInit;
		CStatistic m_stat;
	};

	class CHxGridException
	{
	};

	typedef std::unique_ptr<CGridInterpolBase> CGridInterpolBasePtr;


}



namespace zen
{


	template <> inline
		void writeStruc(const WBSF::CGridInterpolParam& in, XmlElement& output)
	{
		XmlOut out(output);

		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::NB_POINTS)](in.m_nbPoints);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::OUTPUT_NO_DATA)](in.m_noData);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::MAX_DISTANCE)](in.m_maxDistance);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GDAL_OPTIONS)](in.m_GDALOptions);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGIONAL_LIMIT)](in.m_bRegionalLimit);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGIONAL_SD)](in.m_regionalLimitSD);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGIONAL_LIMIT_TO_BOUND)](in.m_bRegionalLimitToBound);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_LIMIT)](in.m_bGlobalLimit);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_SD)](in.m_globalLimitSD);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_LIMIT_TO_BOUND)](in.m_bGlobalLimitToBound);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_MINMAX_LIMIT)](in.m_bGlobalMinMaxLimit);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_MIN_LIMIT)](in.m_globalMinLimit);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_MAX_LIMIT)](in.m_globalMaxLimit);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_MINMAX_LIMIT_TO_BOUND)](in.m_bGlobalMinMaxLimitToBound);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGRESSION_MODEL)](WBSF::ToString(in.m_regressionModel));//a vérifier
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGRESS_CRITICAL_R2)](in.m_regressCriticalR2);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::VARIOGRAM_MODEL)](in.m_variogramModel);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::NB_LAGS)](in.m_nbLags);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::LAG_DISTANCE)](in.m_lagDist);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::DETRENDING_MODEL)](in.m_detrendingModel);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::EXTERNAL_DRIFT)](in.m_externalDrift);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::FILL_NUGGET)](in.m_bFillNugget);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::OUTPUT_VARIOGRAM_INFO)](in.m_bOutputVariogramInfo);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::IWD_MODEL)](in.m_IWDModel);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::IWD_POWER)](in.m_power);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::IWD_USE_ELEV)](in.m_bUseElevation);
		out[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::TPS_TYPE)](in.m_TPSType);

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGridInterpolParam& out)
	{
		XmlIn in(input);

		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::NB_POINTS)](out.m_nbPoints);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::OUTPUT_NO_DATA)](out.m_noData);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::MAX_DISTANCE)](out.m_maxDistance);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GDAL_OPTIONS)](out.m_GDALOptions);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGIONAL_LIMIT)](out.m_bRegionalLimit);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGIONAL_SD)](out.m_regionalLimitSD);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGIONAL_LIMIT_TO_BOUND)](out.m_bRegionalLimitToBound);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_LIMIT)](out.m_bGlobalLimit);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_SD)](out.m_globalLimitSD);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_LIMIT_TO_BOUND)](out.m_bGlobalLimitToBound);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_MINMAX_LIMIT)](out.m_bGlobalMinMaxLimit);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_MIN_LIMIT)](out.m_globalMinLimit);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_MAX_LIMIT)](out.m_globalMaxLimit);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::GLOBAL_MINMAX_LIMIT_TO_BOUND)](out.m_bGlobalMinMaxLimitToBound);
		std::string str;
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGRESSION_MODEL)](str);//a vérifier
		out.m_regressionModel = WBSF::ToVector<int>(str);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::REGRESS_CRITICAL_R2)](out.m_regressCriticalR2);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::VARIOGRAM_MODEL)](out.m_variogramModel);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::NB_LAGS)](out.m_nbLags);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::LAG_DISTANCE)](out.m_lagDist);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::DETRENDING_MODEL)](out.m_detrendingModel);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::EXTERNAL_DRIFT)](out.m_externalDrift);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::FILL_NUGGET)](out.m_bFillNugget);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::OUTPUT_VARIOGRAM_INFO)](out.m_bOutputVariogramInfo);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::IWD_MODEL)](out.m_IWDModel);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::IWD_POWER)](out.m_power);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::IWD_USE_ELEV)](out.m_bUseElevation);
		in[WBSF::CGridInterpolParam::GetMemberName(WBSF::CGridInterpolParam::TPS_TYPE)](out.m_TPSType);

		return true;
	}


	template <> inline
		void writeStruc(const WBSF::CGridInterpolInfo& in, XmlElement& output)
	{
		XmlOut out(output);

		out[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::CELL_SIZE_X)](in.m_cellSizeX);
		out[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::CELL_SIZE_Y)](in.m_cellSizeY);
		out[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::OUTPUT_NO_DATA)](in.m_noData);
		out[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::MULTI)](in.m_bMulti);
		out[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::NB_CPU)](in.m_nbCPU);
		
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CGridInterpolInfo& out)
	{
		XmlIn in(input);

		in[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::CELL_SIZE_X)](out.m_cellSizeX);
		in[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::CELL_SIZE_Y)](out.m_cellSizeY);
		in[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::OUTPUT_NO_DATA)](out.m_noData);
		in[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::MULTI)](out.m_bMulti);
		in[WBSF::CGridInterpolInfo::GetMemberName(WBSF::CGridInterpolInfo::NB_CPU)](out.m_nbCPU);

		return true;
	}
	
}