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
	static const int MINIMUM_CALIB_POINT = 30;
	
	void CGridInterpolBase::FreeMemoryCache()
	{
	}

	const int CGridInterpolParam::DETRENDING_TERM_DEFINE[NB_DETRENDINGS][4] =
	{
		{ 0, 0, 0, 0 },
		{ 1, CTerm::LAT, 0, 0 },
		{ 1, CTerm::LON, 0, 0 },
		{ 1, CTerm::ELEV, 0, 0 },
		{ 1, CTerm::EXPO, 0, 0 },
		{ 1, CTerm::SHORE, 0, 0 },
		{ 3, CTerm::LAT, CTerm::LON, CTerm::LAT | CTerm::LON },
		{ 3, CTerm::LAT, CTerm::ELEV, CTerm::LAT | CTerm::ELEV },
		{ 3, CTerm::LAT, CTerm::EXPO, CTerm::LAT | CTerm::EXPO },
		{ 3, CTerm::LAT, CTerm::SHORE, CTerm::LAT | CTerm::SHORE },
		{ 3, CTerm::LON, CTerm::ELEV, CTerm::LON | CTerm::ELEV },
		{ 3, CTerm::LON, CTerm::EXPO, CTerm::LON | CTerm::EXPO },
		{ 3, CTerm::LON, CTerm::SHORE, CTerm::LON | CTerm::SHORE },
		{ 3, CTerm::ELEV, CTerm::EXPO, CTerm::ELEV | CTerm::EXPO },
		{ 3, CTerm::ELEV, CTerm::SHORE, CTerm::ELEV | CTerm::SHORE },
		{ 3, CTerm::EXPO, CTerm::SHORE, CTerm::EXPO | CTerm::SHORE },
		{ 1, CTerm::LAT², 0, 0 },
		{ 1, CTerm::LON², 0, 0 },
		{ 1, CTerm::ELEV², 0, 0 },
		{ 1, CTerm::EXPO², 0, 0 },
		{ 1, CTerm::SHORE², 0, 0 },
	};

	const int CGridInterpolParam::EX_DRIFT_TERM_DEFINE[NB_EXTERNAL_DRIFTS][2] =
	{
		{ 0, 0},
		{ 1, CTerm::EXPO },//we put expo and shore before elev to put elev at the same position (3) than ever
		{ 1, CTerm::SHORE },
		{ 1, CTerm::ELEV },
		{ 0, 0 },
	};

	//***************************************************************************
	const char* CGridInterpolParam::XML_FLAG = "InterpolParam";

	const char* CGridInterpolParam::MEMBER_NAME[NB_MEMBER] = { "NbPoints", "OutputNoData", "MaxDistance", "XValPoints", "OutputType", "UseElev", "UseExpo", "UseShore", "GDALOptions", "RegionalLimit", "RegionalSD", "RegionalLimitToBound", "GlobalLimit", "GlobalSD", "GlobalLimitToBound", "GlobalMinMaxLimit", "GlobalMinLimit", "GlobalMaxLimit", "GlobalMinMaxLimitToBound", "RegressionModel", "RegressCriticalR2", "VariogramModel", "NbLags", "LagDistance", "DetrendingModel", "ExternalDrift", "FillNugget", "IWDModel", "IWDPower", "TPSMaxError", "RFTreeType", "OutputVariogramInfo" };

	CGridInterpolParam::CGridInterpolParam()
	{
		Reset();
	}


	void CGridInterpolParam::Reset()
	{
		m_nbPoints = 35;
		m_noData = -999;
		m_maxDistance = 200000;//200 km
		m_XvalPoints = 0.2;//20% for validation
		m_outputType = O_CALIB_VALID;
		m_bUseLatLon = true;
		m_bUseElevation = true;
		m_bUseExposition = true;
		m_bUseShore = true;
		m_GDALOptions = "-of GTIFF -ot Float32 -co COMPRESS=LZW -Stats -Hist -Overview \"2,4,8,16\"";
		m_bGlobalLimit = false;
		m_globalLimitSD = 2;
		m_bGlobalLimitToBound = false;
		m_bGlobalMinMaxLimit = false;
		m_globalMinLimit = 0;
		m_globalMaxLimit = 0;
		m_bGlobalMinMaxLimitToBound = false;

		//Spatial Regression
		m_regressionModel.empty();
		m_regressCriticalR2 = 0.0005;

		//Kriging
		m_variogramModel = BEST_VARIOGRAM;
		m_nbLags = 0;	//best nb lag
		m_lagDist = 0; //best lag distance
		m_detrendingModel = NO_DETRENDING;
		m_externalDrift = ED_ELEV;//elevation
		m_bFillNugget = false;
		m_bOutputVariogramInfo = false;
		m_bRegionalLimit = false;
		m_regionalLimitSD = 2;
		m_bRegionalLimitToBound = false;

		//Inverse Weighted Distance
		m_IWDModel = BEST_IWD_MODEL;
		m_power = 0; //best power

		//Thin Plate Sline
		m_TPSMaxError = 1.0e-04;

		//Random Forest
		m_RFTreeType = 1;//regression by default

	}

	CGridInterpolParam::CGridInterpolParam(const CGridInterpolParam& in)
	{
		operator=(in);
	}

	CGridInterpolParam& CGridInterpolParam::operator =(const CGridInterpolParam& in)
	{
		//General
		m_nbPoints = in.m_nbPoints;
		m_noData = in.m_noData;
		m_maxDistance = in.m_maxDistance;
		m_XvalPoints = in.m_XvalPoints;
		m_outputType = in.m_outputType;
		m_bUseLatLon = in.m_bUseLatLon;
		m_bUseElevation = in.m_bUseElevation;
		m_bUseExposition = in.m_bUseExposition;
		m_bUseShore = in.m_bUseShore;
		m_GDALOptions = in.m_GDALOptions;
		m_bGlobalLimit = in.m_bGlobalLimit;
		m_globalLimitSD = in.m_globalLimitSD;
		m_bGlobalLimitToBound = in.m_bGlobalLimitToBound;
		m_bGlobalMinMaxLimit = in.m_bGlobalMinMaxLimit;
		m_globalMinLimit = in.m_globalMinLimit;
		m_globalMaxLimit = in.m_globalMaxLimit;
		m_bGlobalMinMaxLimitToBound = in.m_bGlobalMinMaxLimitToBound;

		//Spatial Regression
		m_regressionModel = in.m_regressionModel;
		m_regressCriticalR2 = in.m_regressCriticalR2;

		//Kriging
		m_variogramModel = in.m_variogramModel;
		m_nbLags = in.m_nbLags;
		m_lagDist = in.m_lagDist;
		m_detrendingModel = in.m_detrendingModel;
		m_externalDrift = in.m_externalDrift;
		m_bFillNugget = in.m_bFillNugget;
		m_bOutputVariogramInfo = in.m_bOutputVariogramInfo;
		m_bRegionalLimit = in.m_bRegionalLimit;
		m_regionalLimitSD = in.m_regionalLimitSD;
		m_bRegionalLimitToBound = in.m_bRegionalLimitToBound;

		//Inverse Weighted Distance
		m_IWDModel = in.m_IWDModel;
		m_power = in.m_power;

		//Thin Plate Sline
		m_TPSMaxError = in.m_TPSMaxError;

		//Random Forest
		m_RFTreeType = in.m_RFTreeType;


		return *this;
	}


	bool CGridInterpolParam::operator ==(const CGridInterpolParam& in)const
	{
		bool bEqual = true;

		//General
		if (m_nbPoints != in.m_nbPoints)bEqual = false;
		if ((float)m_noData != (float)in.m_noData)bEqual = false;
		if ((float)m_maxDistance != (float)in.m_maxDistance)bEqual = false;
		if ((float)m_XvalPoints != (float)in.m_XvalPoints)bEqual = false;
		if (m_outputType != in.m_outputType)bEqual = false;
		if (m_bUseLatLon != in.m_bUseLatLon)bEqual = false;
		if (m_bUseElevation != in.m_bUseElevation)bEqual = false;
		if (m_bUseExposition != in.m_bUseExposition)bEqual = false;
		if (m_bUseShore != in.m_bUseShore)bEqual = false;
		if (m_GDALOptions != in.m_GDALOptions)bEqual = false;
		if (m_bGlobalLimit != in.m_bGlobalLimit)bEqual = false;
		if (m_globalLimitSD != in.m_globalLimitSD)bEqual = false;
		if (m_bGlobalLimitToBound != in.m_bGlobalLimitToBound)bEqual = false;
		if (m_bGlobalMinMaxLimit != in.m_bGlobalMinMaxLimit)bEqual = false;
		if (m_globalMinLimit != in.m_globalMinLimit)bEqual = false;
		if (m_globalMaxLimit != in.m_globalMaxLimit)bEqual = false;
		if (m_bGlobalMinMaxLimitToBound != in.m_bGlobalMinMaxLimitToBound)bEqual = false;



		//Spatial Regression
		if (m_regressionModel != in.m_regressionModel)bEqual = false;
		if (m_regressCriticalR2 != in.m_regressCriticalR2)bEqual = false;

		//Kriging
		if (m_variogramModel != in.m_variogramModel)bEqual = false;
		if (m_nbLags != in.m_nbLags)bEqual = false;
		if (m_lagDist != in.m_lagDist)bEqual = false;
		if (m_detrendingModel != in.m_detrendingModel)bEqual = false;
		if (m_externalDrift != in.m_externalDrift)bEqual = false;
		if (m_bFillNugget != in.m_bFillNugget)bEqual = false;
		if (m_bOutputVariogramInfo != in.m_bOutputVariogramInfo)bEqual = false;
		if (m_bRegionalLimit != in.m_bRegionalLimit)bEqual = false;
		if (m_regionalLimitSD != in.m_regionalLimitSD)bEqual = false;
		if (m_bRegionalLimitToBound != in.m_bRegionalLimitToBound)bEqual = false;


		//Inverse Weighted Distance
		if (m_IWDModel != in.m_IWDModel)bEqual = false;
		if ((float)m_power != (float)in.m_power)bEqual = false;

		//Thin Plate Spline
		if ((float)m_TPSMaxError != (float)in.m_TPSMaxError)bEqual = false;

		//Random Forest
		if (m_RFTreeType != in.m_RFTreeType)bEqual = false;


		return bEqual;
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
		case OUTPUT_TYPE:			str = ToString(m_outputType); break;
		case USE_ELEV:				str = ToString(m_bUseElevation); break;
		case USE_EXPO:				str = ToString(m_bUseExposition); break;
		case USE_SHORE:				str = ToString(m_bUseShore); break;
		case GDAL_OPTIONS:			str = m_GDALOptions; break;
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
		case OUTPUT_VARIOGRAM_INFO: str = ToString(m_bOutputVariogramInfo); break;
		case REGIONAL_LIMIT:		str = ToString(m_bRegionalLimit); break;
		case REGIONAL_SD:			str = ToString(m_regionalLimitSD); break;
		case REGIONAL_LIMIT_TO_BOUND:str = ToString(m_bRegionalLimitToBound); break;

		case IWD_MODEL:				str = ToString(m_IWDModel); break;
		case IWD_POWER:				str = ToString(m_power, 2); break;
		case TPS_MAX_ERROR:			str = ToString(m_TPSMaxError); break;
		case RF_TREE_TYPE:			str = ToString(m_RFTreeType); break;

		default: ASSERT(false);
		}

		return str;
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
		m_bUseShore = false;
		m_bGeographic = false;
		m_nbDimension = 0;
		m_nSize = 0;
	}

	void CANNSearch::Init(CGridPointVectorPtr& pPts, bool bUseElevation, bool bUseShore)
	{
		_ASSERTE(pPts.get());
		Reset();

		m_bUseElevation = bUseElevation;
		m_bUseShore = bUseShore;

		CGridPointVector& pts = *pPts;
		m_bGeographic = IsGeographic(pPts->GetPrjID());
		m_nbDimension = 2;
		if (m_bGeographic)
			m_nbDimension++;
		if (m_bUseElevation)
			m_nbDimension++;
		if (m_bUseShore)
			m_nbDimension++;

		//? m_bUseElevation ? 4 : 3 : m_bUseElevation ? 3 : 2;
		m_nSize = pts.size();
		m_pDataPts = annAllocPts((int)m_nSize, (int)m_nbDimension);

		for (size_t i = 0; i < m_nSize; i++)
		{
			for (int k = 0, kk = 0; k < 6; k++)
			{

				if ((k < 2 && !m_bGeographic) || (k < 3 && m_bGeographic))
				{
					m_pDataPts[i][kk] = m_bGeographic ? pts[i](k) : pts[i][k];
					kk++;
				}
				else if (k == 3 && m_bUseElevation)
				{
					m_pDataPts[i][kk] = pts[i].m_elev * 100;
					kk++;
				}
				/*else if (k == 4 && m_bHaveExposition)
				{
					dp = pts[i].GetExposition();
				}*/
				else if (k == 5 && m_bUseShore)
				{
					m_pDataPts[i][kk] = pts[i].m_shore;
					kk++;
				}
			}
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

			for (int k = 0, kk = 0; k < 6; k++)
			{

				if ((k < 2 && !m_bGeographic) || (k < 3 && m_bGeographic))
				{
					q[kk] = m_bGeographic ? pt(k) : pt[k];
					kk++;
				}
				else if (k == 3 && m_bUseElevation)
				{
					q[kk] = pt.m_elev * 100;
					kk++;
				}
				/*else if (k == 4 && m_bHaveExposition)
				{
				dp = pts[i].GetExposition();
				}*/
				else if (k == 5 && m_bUseShore)
				{
					q[kk] = pt.m_shore;
					kk++;
				}
			}

			m_pTreeRoot->annPkSearch(q, (int)nbPointSearch, nn_idx, dd, 0);

			result.resize(nbPointSearch);
			for (size_t i = 0; i < nbPointSearch; i++)
			{
				result[i].i = nn_idx[i];

				if (m_bGeographic)
				{
					const double _2x6371x1000_ = 2 * 6371 * 1000;
					result[i].d = _2x6371x1000_ * asin(sqrt(dd[i]) / _2x6371x1000_);
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
//		m_bInit = false;
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
		
		
		/*if (InternalInit(callback))
		{
			Interpolation(lineIn, lineOut, callback);
		}*/

		unsigned __int64 size = (unsigned __int64)lineOut.size();
		//Write line index and output result
		os << xIndex;
		os << yIndex;
		os << width;
		os << height;

		//write result
		os << size;
		os.write((char*)lineOut.data(), lineOut.size() * sizeof(CGridLine::value_type));
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

		//compute projection transformation
		CGridPointVector* pPts = m_pPts.get();
		m_PT = GetReProjection(pPts->GetPrjID(), PRJ_WGS_84);

		//compute increment for Xval
		float xValPercent = max(0.0f, min(1.0f, (float)m_param.m_XvalPoints));//compute value in float to avoid difference
		//size_t nbPoints = max(1.0f, (1.0f - xValPercent)*m_pPts->size());
		//m_inc = max(1.0, (double)pPts->size() / nbPoints);
		size_t nbCalibPoints = (1.0f - xValPercent)*m_pPts->size();
		m_inc = (double)pPts->size() / nbCalibPoints;
		//m_inc = max(1.0, (double)pPts->size() / nbPoints);

		
		if (nbCalibPoints < MINIMUM_CALIB_POINT)
			msg.ajoute("Need at least "+to_string(MINIMUM_CALIB_POINT )+" points to calibrate model. Calibration points: " + to_string(nbCalibPoints) + "Verify number of valid points and validation ratio.");

		return msg;
	}

	//ERMsg CGridInterpolBase::InternalInit(CCallback& callback)const
	//{
	//	ASSERT(m_pPts.get());

	//	ERMsg msg;

	//	CGridInterpolBase& me = const_cast<CGridInterpolBase&>(*this);

	//	//to avoid to reinit multiple time on the same object, we have to protect it
	//	me.m_CS.Enter();
	//	msg = me.Initialization(callback);
	//	me.m_CS.Leave();

	//	return msg;
	//}

	double CGridInterpolBase::GetOptimizedR²(CCallback& callback)const
	{
		CXValidationVector XValidation;
		GetXValidation(CGridInterpolParam::O_CALIBRATION, XValidation, callback);
		
		//Gat statistic for this parameter set
		CStatisticXY stat = XValidation.GetStatistic(m_param.m_noData);
		double R² = stat[COEF_D];
		return R²;
	}


	//GetOptimizedR² isn't thread safe, caller must protect it
	void CGridInterpolBase::GetOptimizedR²(std::istream& is, std::ostream& os)
	{
		//IGenericStream* inStream = (IGenericStream*) inStreamIn;
		//IGenericStream* outStream = (IGenericStream*) outStreamIn;


//		m_bInit = false;//load new set of parameters, then reinitialized

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

	void CGridInterpolBase::GetXValidation(CGridInterpolParam::TOutputType type, CXValidationVector& XValidation, CCallback& callback)const
	{
		ERMsg msg;


		//XValidation.clear();


		//msg = InternalInit(callback);
		//if (msg)
		//{

		XValidation.resize(m_pPts->size());


		#pragma omp parallel for /*schedule(static, 10)*/ num_threads( m_info.m_nbCPU ) if (m_info.m_bMulti)
		for (int i = 0; i < m_pPts->size(); i++)
		{
			int l = (int)ceil((i) / m_inc);
			int ii = int(l*m_inc);

			const CGridPoint& pt = m_pPts->at(i);

			XValidation[i].m_observed = pt.m_event;
			if (type == CGridInterpolParam::O_CALIBRATION)
			{
				XValidation[i].m_predicted = (i == ii) ? Evaluate(pt, i) : m_param.m_noData;
			}
			if (type == CGridInterpolParam::O_VALIDATION)
			{
				XValidation[i].m_predicted = (i != ii) ? Evaluate(pt, i) : m_param.m_noData;
			}
			else if (type == CGridInterpolParam::O_CALIB_VALID)
			{
				XValidation[i].m_predicted = Evaluate(pt, i);
			}
			else if (type == CGridInterpolParam::O_INTERPOLATION)
			{
				XValidation[i].m_predicted = Evaluate(pt);
			}


			if (m_pAgent && m_pAgent->TestConnection(m_hxGridSessionID) == S_FALSE)
				throw(CHxGridException());
		}
	//	}


		//return msg;
	}

	bool CGridInterpolBase::GetVariogram(CVariogram& variogram)const
	{
		//do nothing by default
		return false;
	}

	void CGridInterpolBase::Interpolation(const CGridPointVector& lineIn, CGridLine& lineOut, CCallback& callback)const
	{
		//ERMsg msg;

		//msg = InternalInit(callback);
		//if (!msg)
			//return msg;

		lineOut.resize(lineIn.size());

		
		for (size_t i = 0; i < lineIn.size(); i++)
		{
			const CGridPoint& pt = lineIn[i];
			if (fabs(pt.m_z - m_info.m_noData) > EPSILON_NODATA)
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

		//return msg;
	}

	CGridPointVectorPtr CGridInterpolBase::GetCalibrationPts()const
	{
		CGridPointVectorPtr pPts(new CGridPointVector);
		size_t nbPoints = (size_t)round(m_pPts->size() / m_inc);
		//CGridPointVector points;
		pPts->resize(nbPoints);
		for (size_t i = 0, ii = 0; i < nbPoints&&ii < m_pPts->size(); ++i, ii = i * m_inc)
		{
			ASSERT(ii < m_pPts->size());
			pPts->at(i) = m_pPts->at(ii);
		}

		return pPts;
	}

	CGridPointVectorPtr CGridInterpolBase::GetValidationPts()const
	{
		CGridPointVectorPtr pPts(new CGridPointVector);
		size_t nbPoints = m_pPts->size() - (size_t)round(m_pPts->size() / m_inc);
		//CGridPointVector points;
		pPts->resize(nbPoints);
		for (size_t i = 0; i < nbPoints; ++i)
		{
			size_t l = size_t(ceil(i / m_inc));
			size_t ii = size_t(l*m_inc);

			ASSERT(ii < m_pPts->size());
			if(i!=ii)
				pPts->at(i) = m_pPts->at(ii);
		}

		return pPts;
	}

	void CGridInterpolBase::Cleanup()
	{

	}
}