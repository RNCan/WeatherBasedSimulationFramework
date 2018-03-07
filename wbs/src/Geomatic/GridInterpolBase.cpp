//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"

#include "Geomatic/GridInterpolBase.h"
#undef interface 


#include "Basic/Statistic.h"
#include "Basic/UtilStd.h"
#include "Basic/OpenMP.h"
#include "Geomatic/Variogram.h"
#include "hxGrid/Interface/IGenericStream.h"
#include "hxGrid/interface/IAgent.h"

using namespace std;
using namespace VITALENGINE;

namespace WBSF
{
	static CCriticalSection FREE_CS;

	//CANNSearchCache CGridInterpolBase::ANN_SEARCH_CACHE;
	void CANNSearchCache::FreeMemoryCache2()
	{
		Enter();
		if (!empty())
		{
			annClose();
			clear();
		}
		Leave();
	}

	void CGridInterpolBase::FreeMemoryCache()
	{
		FREE_CS.Enter();
		//ANN_SEARCH_CACHE.FreeMemoryCache2();
		FREE_CS.Leave();
	}

	//***************************************************************************
	const char* CGridInterpolParam::XML_FLAG = "InterpolParam";

	const char* CGridInterpolParam::MEMBER_NAME[NB_MEMBER] = { "NbPoints", "OutputNoData", "MaxDistance", "XValPoints", "GDALOptions", "RegionalLimit", "RegionalSD", "RegionalLimitToBound", "GlobalLimit", "GlobalSD", "GlobalLimitToBound", "GlobalMinMaxLimit", "GlobalMinLimit", "GlobalMaxLimit", "GlobalMinMaxLimitToBound", "RegressionModel", "RegressCriticalR2", "VariogramModel", "NbLags", "LagDistance", "DetrendingModel", "ExternalDrift", "FillNugget", "IWDModel", "IWDPower", "IWDUseElev", "TPSMaxError", "OutputVariogramInfo" };

	CGridInterpolParam::CGridInterpolParam()
	{
		Reset();
	}


	void CGridInterpolParam::Reset()
	{
		m_nbPoints = 35;
		m_noData = -999;
		m_maxDistance = 200000;//200 km
		m_XvalPoints = 0.2;//20% for X-validation
		//-co tiled=YES -co BLOCKXSIZE=512 -co BLOCKYSIZE=512 -co SPARSE_OK=YES
		m_GDALOptions = "-of GTIFF -ot Float32 -co COMPRESS=LZW -Stats -Hist -Overview \"2,4,8,16\"";
		

		m_regressionModel.empty();
		m_regressCriticalR2 = 0.0005;

		m_variogramModel = BEST_VARIOGRAM;
		m_nbLags = 0;	//best nb lag
		m_lagDist = 0; //best lag distance
		m_detrendingModel = NO_DETRENDING;
		m_externalDrift = 3;
		m_bFillNugget = false;
		m_bOutputVariogramInfo = false;

		m_IWDModel = BEST_IWD_MODEL;
		m_power = 0; //best model
		m_TPSMaxError = 1.0e-04;
		m_bUseElevation = false;


		m_bRegionalLimit = false;
		m_regionalLimitSD = 2;
		m_bRegionalLimitToBound = false;
		m_bGlobalLimit = false;
		m_globalLimitSD = 2;
		m_bGlobalLimitToBound = false;
		m_bGlobalMinMaxLimit = false;
		m_globalMinLimit = 0;
		m_globalMaxLimit = 0;
		m_bGlobalMinMaxLimitToBound = false;
	}

	CGridInterpolParam::CGridInterpolParam(const CGridInterpolParam& in)
	{
		operator=(in);
	}

	CGridInterpolParam& CGridInterpolParam::operator =(const CGridInterpolParam& in)
	{
		m_nbPoints = in.m_nbPoints;
		m_noData = in.m_noData;
		m_maxDistance = in.m_maxDistance;
		m_XvalPoints = in.m_XvalPoints;
		m_GDALOptions = in.m_GDALOptions;

		m_regressionModel = in.m_regressionModel;
		m_regressCriticalR2 = in.m_regressCriticalR2;

		m_variogramModel = in.m_variogramModel;
		m_nbLags = in.m_nbLags;
		m_lagDist = in.m_lagDist;
		m_detrendingModel = in.m_detrendingModel;
		m_externalDrift = in.m_externalDrift;
		m_bFillNugget = in.m_bFillNugget;
		m_bOutputVariogramInfo = in.m_bOutputVariogramInfo;

		m_IWDModel = in.m_IWDModel;
		m_power = in.m_power;
		m_bUseElevation = in.m_bUseElevation;

		m_TPSMaxError = in.m_TPSMaxError;


		m_bRegionalLimit = in.m_bRegionalLimit;
		m_regionalLimitSD = in.m_regionalLimitSD;
		m_bRegionalLimitToBound = in.m_bRegionalLimitToBound;
		m_bGlobalLimit = in.m_bGlobalLimit;
		m_globalLimitSD = in.m_globalLimitSD;
		m_bGlobalLimitToBound = in.m_bGlobalLimitToBound;
		m_bGlobalMinMaxLimit = in.m_bGlobalMinMaxLimit;
		m_globalMinLimit = in.m_globalMinLimit;
		m_globalMaxLimit = in.m_globalMaxLimit;
		m_bGlobalMinMaxLimitToBound = in.m_bGlobalMinMaxLimitToBound;


		return *this;
	}

	string CGridInterpolParam::GetMember(size_t i)const
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		string str;
		switch (i)
		{
		case NB_POINTS:				str = ToString(m_nbPoints); break;
		case OUTPUT_NO_DATA:		str = ToString(m_noData); break;
		case MAX_DISTANCE:			str = ToString(m_maxDistance); break;
		case XVAL_POINTS:			str = ToString(m_XvalPoints); break;
		case GDAL_OPTIONS:			str = m_GDALOptions; break;
		case REGIONAL_LIMIT:		str = ToString(m_bRegionalLimit); break;
		case REGIONAL_SD:			str = ToString(m_regionalLimitSD); break;
		case REGIONAL_LIMIT_TO_BOUND:str = ToString(m_bRegionalLimitToBound); break;
		case GLOBAL_LIMIT:			str = ToString(m_bGlobalLimit); break;
		case GLOBAL_SD:				str = ToString(m_globalLimitSD); break;
		case GLOBAL_LIMIT_TO_BOUND:str = ToString(m_bGlobalLimitToBound); break;
		case GLOBAL_MINMAX_LIMIT:	str = ToString(m_bGlobalMinMaxLimit); break;
		case GLOBAL_MIN_LIMIT:		str = ToString(m_globalMinLimit); break;
		case GLOBAL_MAX_LIMIT:		str = ToString(m_globalMaxLimit); break;
		case GLOBAL_MINMAX_LIMIT_TO_BOUND:str = ToString(m_bGlobalMinMaxLimitToBound); break;
		case REGRESSION_MODEL:		str = ToString(m_regressionModel); break;
		case REGRESS_CRITICAL_R2:	str = ToString(m_regressCriticalR2, 5); break;
		case VARIOGRAM_MODEL:		str = ToString(m_variogramModel); break;
		case NB_LAGS:				str = ToString(m_nbLags); break;
		case LAG_DISTANCE:			str = ToString(m_lagDist, 8); break;
		case DETRENDING_MODEL:		str = ToString(m_detrendingModel); break;
		case EXTERNAL_DRIFT:		str = ToString(m_externalDrift); break;
		case FILL_NUGGET:			str = ToString(m_bFillNugget); break;
		case IWD_MODEL:				str = ToString(m_IWDModel); break;
		case IWD_POWER:				str = ToString(m_power, 2); break;
		case IWD_USE_ELEV:			str = ToString(m_bUseElevation); break;
		case TPS_MAX_ERROR:			str = ToString(m_TPSMaxError); break;
		case OUTPUT_VARIOGRAM_INFO: str = ToString(m_bOutputVariogramInfo); break;
		default: ASSERT(false);
		}

		return str;
	}
	
	bool CGridInterpolParam::operator ==(const CGridInterpolParam& in)const
	{
		bool bEgual = true;

		if (m_nbPoints != in.m_nbPoints)bEgual = false;
		if (m_noData != in.m_noData)bEgual = false;
		if (m_maxDistance != in.m_maxDistance)bEgual = false;
		if (m_XvalPoints != in.m_XvalPoints)bEgual = false;
		if (m_GDALOptions != in.m_GDALOptions)bEgual = false;
		if (m_bRegionalLimit != in.m_bRegionalLimit)bEgual = false;
		if (m_regionalLimitSD != in.m_regionalLimitSD)bEgual = false;
		if (m_bRegionalLimitToBound != in.m_bRegionalLimitToBound)bEgual = false;
		if (m_bGlobalLimit != in.m_bGlobalLimit)bEgual = false;
		if (m_globalLimitSD != in.m_globalLimitSD)bEgual = false;
		if (m_bGlobalLimitToBound != in.m_bGlobalLimitToBound)bEgual = false;
		if (m_bGlobalMinMaxLimit != in.m_bGlobalMinMaxLimit)bEgual = false;
		if (m_globalMinLimit != in.m_globalMinLimit)bEgual = false;
		if (m_globalMaxLimit != in.m_globalMaxLimit)bEgual = false;
		if (m_bGlobalMinMaxLimitToBound != in.m_bGlobalMinMaxLimitToBound)bEgual = false;

		if (m_regressionModel != in.m_regressionModel)bEgual = false;
		if (m_regressCriticalR2 != in.m_regressCriticalR2)bEgual = false;
		if (m_variogramModel != in.m_variogramModel)bEgual = false;
		if (m_nbLags != in.m_nbLags)bEgual = false;
		if (m_lagDist != in.m_lagDist)bEgual = false;
		if (m_detrendingModel != in.m_detrendingModel)bEgual = false;
		if (m_IWDModel != in.m_IWDModel)bEgual = false;
		if (m_power != in.m_power)bEgual = false;
		if (m_bUseElevation != in.m_bUseElevation)bEgual = false;
		if (m_TPSMaxError != in.m_TPSMaxError)bEgual = false;
		if (m_bOutputVariogramInfo != in.m_bOutputVariogramInfo)bEgual = false;


		return bEgual;
	}

	//***************************************************************************
	const char* CGridInterpolInfo::XML_FLAG = "InterpolInfo";
	const char* CGridInterpolInfo::MEMBER_NAME[NB_MEMBER] = { "CellSizeX", "CellSizeY", "NoData", "Multi", "NbCPU" };


	CGridInterpolInfo::CGridInterpolInfo()
	{
		Reset();
	}


	void CGridInterpolInfo::Reset()
	{
		m_cellSizeX = 0;
		m_cellSizeY = 0;
		m_noData = -999;
		m_bMulti = false;
		m_nbCPU = 1;
	}


	string CGridInterpolInfo::GetMember(int i)const
	{
		ASSERT(i >= 0 && i < NB_MEMBER);

		string str;
		switch (i)
		{
		case CELL_SIZE_X:		str = ToString(m_cellSizeX); break;
		case CELL_SIZE_Y:		str = ToString(m_cellSizeY); break;
		case OUTPUT_NO_DATA:	str = ToString(m_noData); break;
		case MULTI:				str = ToString(m_bMulti); break;
		case NB_CPU:			str = ToString(m_nbCPU); break;
		default: ASSERT(false);
		}

		return str;
	}
	//
	//void CGridInterpolInfo::SetMember(int i, const string& str, const LPXNode pNode)
	//{
	//	ASSERT( i>=0 && i<NB_MEMBER);
	//	switch(i)
	//	{
	//	case CELL_SIZE_X:		m_cellSizeX = ToDouble(str);break;
	//	case CELL_SIZE_Y:		m_cellSizeY = ToDouble(str); break;
	//	case OUTPUT_NO_DATA:	m_noData = ToFloat(str); break;
	//	default: ASSERT(false);
	//	}
	//
	//}


	//util information about the DEM and other


	//***************************************************************************
	CANNSearch::CANNSearch() :
		m_pDataPts(NULL)
	{
		Reset();
	}
	CANNSearch::~CANNSearch()
	{
		Reset();
	}

	void CANNSearch::Reset()
	{
		if (m_pDataPts)
		{
			annDeallocPts(m_pDataPts);
			m_pDataPts = NULL;
		}

		m_bUseElevation = false;
		m_bGeographic = false;
		m_nbDimension = 0;
		m_nSize = 0;
	}

	void CANNSearch::Init(CGridPointVectorPtr& pPts, bool bUseElevation)
	{
		_ASSERTE(pPts.get());
		Reset();

		m_bUseElevation = bUseElevation;

		CGridPointVector& pts = *pPts;
		m_bGeographic = IsGeographic(pPts->GetPrjID());
		m_nbDimension = m_bGeographic ? m_bUseElevation ? 4 : 3 : m_bUseElevation ? 3 : 2;
		m_nSize = pts.size();
		m_pDataPts = annAllocPts((int)m_nSize, (int)m_nbDimension);

		for (size_t i = 0; i < m_nSize; i++)
		{
			for (int j = 0; j < m_nbDimension; j++)
				m_pDataPts[i][j] = m_bGeographic ? pts[i](j) : pts[i][j];

			if (m_bUseElevation)//elevation
				m_pDataPts[i][m_nbDimension - 1] *= 100;

		}

		m_pTreeRoot.reset(new ANNkd_tree(m_pDataPts, (int)m_nSize, (int)m_nbDimension));

	}

	void CANNSearch::Search(const CGridPoint& pt, size_t nbPoint, CGridPointResultVector& result)const
	{
		ERMsg message;

		//We make the search event if they are not enough points
		size_t nbPointSearch = min(nbPoint, m_nSize);
		ASSERT(nbPointSearch <= m_nSize);
		result.clear();
		if (m_nSize > 0)
		{
			ANNidxArray	nn_idx = new ANNidx[nbPointSearch];		// allocate near neigh indices
			ANNdistArray	dd = new ANNdist[nbPointSearch];	// allocate near neighbor dists
			ANNpoint	q = annAllocPt((int)m_nbDimension);		// query point

			for (int i = 0; i < m_nbDimension; i++)
				q[i] = m_bGeographic ? pt(i) : pt[i];

			if (m_bUseElevation)//elevation
				q[m_nbDimension - 1] *= 100;

			m_pTreeRoot->annPkSearch(q, (int)nbPointSearch, nn_idx, dd, 0);

			result.resize(nbPointSearch);
			for (size_t i = 0; i < nbPointSearch; i++)
			{
				result[i].i = nn_idx[i];

				if (m_bGeographic)
				{
					const double _2x6371x1000_ = 2 * 6371 * 1000;
					result[i].d = _2x6371x1000_*asin(sqrt(dd[i]) / _2x6371x1000_);
				}
				else
				{
					result[i].d = sqrt(dd[i]);
				}
			}

			annDeallocPt(q);
			delete[] nn_idx;
			delete[] dd;

			_ASSERTE(result.size() == nbPointSearch);
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	const char* CGridInterpolBase::DATA_DESCRIPTOR = "GridIterpolData";

	CGridInterpolBase::CGridInterpolBase()
	{
		m_pAgent = NULL;
		m_hxGridSessionID = 0;
		m_bInit = false;
	}

	CGridInterpolBase::~CGridInterpolBase()
	{
	}

	void CGridInterpolBase::ReadStream(std::istream&  io)
	{
		m_pPts.reset(new CGridPointVector);
		io >> (*m_pPts);

		unsigned __int64 infoSize = 0;
		io >> infoSize; 

		string buffer;
		buffer.resize((size_t)infoSize);
		io.read((char*)(buffer[0]), (DWORD)infoSize);

		zen::XmlDoc doc = zen::parse(buffer);
		zen::readStruc(doc.root(), m_param);
		zen::readStruc(doc.root(), m_prePostTransfo);
		zen::readStruc(doc.root(), m_info);
	}

	void CGridInterpolBase::WriteStream(std::ostream&  io)const
	{
		_ASSERTE(m_pPts.get());
		io << *m_pPts;

		zen::XmlDoc doc;
		doc.setEncoding("Windows-1252");
		zen::writeStruc(m_param, doc.root());
		zen::writeStruc(m_prePostTransfo, doc.root());
		zen::writeStruc(m_info, doc.root());

		std::string str = zen::serialize(doc);
		unsigned __int64 infoSize = (unsigned __int64)str.size();

		io << infoSize;
		io.write(str.c_str(), (DWORD)infoSize);
	}

	void CGridInterpolBase::Interpolation(std::istream& is, std::ostream& os)
	{
		__int32 xIndex = 0;
		__int32 yIndex = 0;
		__int32 width = 0;
		__int32 height = 0;

		is >> xIndex;
		is >> yIndex;
		is >> width;
		is >> height;

		//read points coordinate
		CGridPointVector lineIn;
		is >> lineIn;

		//Do interpolation
		CCallback callback;
		CGridLine lineOut;
		Interpolation(lineIn, lineOut, callback);

		unsigned __int64 size = (unsigned __int64)lineOut.size();
		//Write line index and output result
		os << xIndex;
		os << yIndex;
		os << width;
		os << height;

		//write result
		os << size;
		os.write((char*)lineOut.data(), lineOut.size()*sizeof(CGridLine::value_type));
	}

	ERMsg CGridInterpolBase::Initialization(CCallback& callback)
	{
		ERMsg msg;

		
		//compute global statistic for limiting range
		for (size_t i = 0; i < (size_t)m_pPts->size(); i++)
		{
			const CGridPoint& pt = m_pPts->at(i);
			m_stat += pt.m_event;
		}

		

		return msg;
	}

	ERMsg CGridInterpolBase::InternalInit(CCallback& callback)const
	{
		ASSERT(m_pPts.get());

		ERMsg msg;

		CGridInterpolBase& me = const_cast<CGridInterpolBase&>(*this);

		//to avoid to reinit multiple time on the same object, we have to protect it
		me.m_CS.Enter();

		
		msg = me.Initialization(callback);
		if (msg)
		

		me.m_CS.Leave();

		return msg;
	}

	double CGridInterpolBase::GetOptimizedR²(CCallback& callback)const
	{
		double R² = -999;

		CXValidationVector XValidation;
		if (GetXValidation(XValidation, callback))
		{
			//Gat statistic for this parameter set
			CStatisticXY stat;
			XValidation.GetStatistic(stat, m_param.m_noData);
			R² = stat[COEF_D];
		}

		return R²;
	}


	//GetOptimizedR² isn't thread safe, caller must protect it
	void CGridInterpolBase::GetOptimizedR²(std::istream& is, std::ostream& os)
	{
		//IGenericStream* inStream = (IGenericStream*) inStreamIn;
		//IGenericStream* outStream = (IGenericStream*) outStreamIn;


		m_bInit = false;//load new set of parameters, then reinitialized

		//Get the current parameter set index
		__int32 parameterIndex = 0;
		is >> parameterIndex;
		//inStream->Read(&parameterIndex, sizeof(parameterIndex));

		//Get the size of XML data
		unsigned __int64 infoSize = 0;
		is >> infoSize;

		//Load XML
		string buffer;
		buffer.resize((size_t)infoSize);
		is.read((char*)(buffer[0]), (DWORD)infoSize);

		zen::XmlDoc doc = zen::parse(buffer);
		zen::readStruc(doc.root(), m_param);
		zen::readStruc(doc.root(), m_prePostTransfo);
		zen::readStruc(doc.root(), m_info);

		//Compute R²
		CCallback callback;
		double R² = GetOptimizedR²(callback);

		//Save result
		os << parameterIndex;
		os << R²;

		//LeaveCriticalSection(&me.m_CS);
	}

	ERMsg CGridInterpolBase::GetXValidation(CXValidationVector& XValidation, CCallback& callback)const
	{
		ERMsg msg;


		XValidation.clear();


		msg = InternalInit(callback);
		if (msg)
		{

			XValidation.resize(m_pPts->size());


#pragma omp parallel for schedule(static, 10) num_threads( m_info.m_nbCPU ) if (m_info.m_bMulti)
			for (int i = 0; i < m_pPts->size(); i++)
			{
				const CGridPoint& pt = m_pPts->at(i);

				XValidation[i].m_observed = pt.m_event;
				XValidation[i].m_predicted = Evaluate(pt, i);

				if (m_pAgent && m_pAgent->TestConnection(m_hxGridSessionID) == S_FALSE)
					throw(CHxGridException());
			}
		}


		return msg;
	}
	
	bool CGridInterpolBase::GetVariogram(CVariogram& variogram)const
	{
		//do nothing by default
		return false;
	}

	ERMsg CGridInterpolBase::Interpolation(const CGridPointVector& lineIn, CGridLine& lineOut, CCallback& callback)const
	{
		ERMsg msg;

		msg = InternalInit(callback);
		if (!msg)
			return msg;

		lineOut.resize(lineIn.size());

//#pragma omp parallel for if(m_bUseOpenMP)
		for (size_t i = 0; i<lineIn.size(); i++)
		{
			const CGridPoint& pt = lineIn[i];
			if (fabs(pt.m_z-m_info.m_noData)>EPSILON_NODATA)
			{
				lineOut[i] = (float)Evaluate(pt);
			}
			else
			{
				lineOut[i] = (float)m_param.m_noData;
			}

			if (m_pAgent && m_pAgent->TestConnection(m_hxGridSessionID) == S_FALSE)
				throw(CHxGridException());
		}

		return msg;
	}


}