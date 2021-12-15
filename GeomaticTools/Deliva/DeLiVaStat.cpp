#include "StdAfx.h"
#include "DeLiVaStat.h"
//#include "UtilGDAL.h"
//#include "Resource.h"
//#include "CommunRes.h"
//#include "ogr_srs_api.h"
//#include "MapBilFile.h"

//#include "ShapeFileBase.h"

#include <float.h>


using namespace std;

namespace WBSF
{



	//***********************************************************

	const std::array<std::array<const char*, 2>, CLiDARStat::NB_VARIABLES> CLiDARStat::VARIABLES_NAME =
	{ {
		{"MEAN", "Heights mean"},
		{"MIN", "Heights minimum"},
		{"MAX", "Heights maximum"},
		{"RGE", "Heights range"},
		{"CAND", "Lidar % canopy density (2-25.5m). Nb pix of 2-25.5"},
		{"QDMH", "Quadratic mean height"},
		{"STDB", "Heights standard deviation"},
		{"MBSTD", "Heights mean absolute deviation"},
		{"SKEW", "Heights skewness"},
		{"KURT", "Heights kurtosis"},
		{"HPR05", "Height 5th percentile"},
		{"HPR10", "Height 10th percentile"},
		{"HPR25", "Height 25th percentile"},
		{"HPR50", "Height 50th percentile"},
		{"HPR75", "Height 75th percentile"},
		{"HPR90", "Height 90th percentile"},
		{"HPR95", "Height 95th percentile"},
		{"STRA0", "% of ground return (=0)"},
		{"STRA1","% of vegetation return < 0m and <= 2m in Height"},
		{"STRA2","% of vegetation return < 2m and <= 4m in Height"},
		{"STRA3","% of vegetation return < 4m and <= 7m in Height"},
		{"STRA4","% of vegetation return < 7m and <= 10m in Height"},
		{"STRA5","% of vegetation return < 10m and <= 15m in Height"},
		{"STRA6","% of vegetation return < 15m and <= 20m in Height"},
		{"STRA7","% of vegetation return < 20m and <= 25.5m in Height"},
		//	"LNUM", "Number of cells"
	} };


	size_t CLiDARStat::GetStat(const std::string& in)
	{
		size_t pos = UNKNOWN_POS;
		for (size_t i = 0; i < VARIABLES_NAME.size()&& pos == UNKNOWN_POS; i++)
		{
			if (IsEqualNoCase(in, VARIABLES_NAME[i][0]))
				pos = i;
		}

		return pos;
	}


	ERMsg CLiDARStatSelection::FromString(const std::string& in)
	{
		ERMsg msg;

		reset();

		StringVector tmp(in, "|, +");
		for (size_t i = 0; i < tmp.size(); i++)
		{
			size_t pos = CLiDARStat::GetStat(tmp[i]);
			if (pos != UNKNOWN_POS)
			{
				set(pos);
			}
			else
			{
				msg.ajoute("Unknown stats: " + tmp[i]);
			}
		}

		return msg;
	}



	CLiDARStat::CLiDARStat()
	{
		Reset();
	}

	void CLiDARStat::Reset()
	{
		CStatisticEx::Reset();
		m_bSorted = false;
	}

	double CLiDARStat::GetPourcentil(double x)const
	{
		//ASSERT( m_values.size()>1);
		ASSERT(x >= 0 && x <= 1);

		if (m_values.size() == 0)
			return GetVMiss();

		int pos = int(x * (m_values.size() - 1));

		return m_values[pos];
	}

	double CLiDARStat::GetRatio(double low, double hight)const
	{
		if (m_values.size() == 0)
			return GetVMiss();

		int nbValue = 0;
		for (int i = 0; i < (int)m_values.size(); i++)
			if (m_values[i] > low && m_values[i] <= hight)
				nbValue++;

		return (double)nbValue / m_values.size() * 100;
	}

	double CLiDARStat::operator[](size_t type)
	{
		ASSERT(type < NB_VARIABLES);

		double value = 0;

		CLiDARStat& me = *this;

		if (!m_bSorted)
		{
			std::sort(m_values.begin(), m_values.end());
			m_bSorted = true;
		}

		switch (type)
		{
		case LMEAN: value = CStatisticEx::operator [](MEAN); break;
		case LMIN:	value = CStatisticEx::operator [](LOWEST); break;
		case LMAX:	value = CStatisticEx::operator [](HIGHEST); break;
		case LRGE:	value = me[LMAX] - me[LMIN]; break;
		case LCAND:	value = GetRatio(2, 25.5); break;
		case LQDMH:	value = CStatisticEx::operator [](QUADRATIC_MEAN); break;
		case LSTDB:	value = CStatisticEx::operator [](STD_DEV); break;
		case LMBSTD:value = CStatisticEx::operator [](MAD); break;
		case LSKEW:	value = CStatisticEx::operator [](SKEWNESS); break;
		case LKURT:	value = CStatisticEx::operator [](KURTOSIS); break;
		case LHPR05:value = GetPourcentil(0.05); break;
		case LHPR10:value = GetPourcentil(0.10); break;
		case LHPR25:value = GetPourcentil(0.25); break;
		case LHPR50:value = GetPourcentil(0.50); break;
		case LHPR75:value = GetPourcentil(0.75); break;
		case LHPR90:value = GetPourcentil(0.90); break;
		case LHPR95:value = GetPourcentil(0.95); break;
		case LSTRA0:value = GetRatio(-1, 0); break;
		case LSTRA1:value = GetRatio(00, 02); break;
		case LSTRA2:value = GetRatio(02, 04); break;
		case LSTRA3:value = GetRatio(04, 07); break;
		case LSTRA4:value = GetRatio(07, 10); break;
		case LSTRA5:value = GetRatio(10, 15); break;
		case LSTRA6:value = GetRatio(15, 20); break;
		case LSTRA7:value = GetRatio(20, 25.5); break;
			//	case NB_CELLS:value=CStatisticEx::operator [](NB_VALUE); break;
		default: ASSERT(false);
		}

		ASSERT(!_isnan(value));

		return value;
	}


	//*****************************************************
	//const char* CDeriveLiDARVariable::XML_FLAG = "DeriveLiDARVariables";
	//const char* CDeriveLiDARVariable::MEMBER_NAME[NB_MEMBER] = {"InputGridFilePath","ScaleFactorIn","Resolution", "Variables", "ScaleFactorOut", "CreateDataset", "InputDatasetFilePath", "OutputDatasetFilePath", "CreateGrid", "OutputGridFilePath", "CellType" };
	//
	//
	//CDeriveLiDARVariable::CDeriveLiDARVariable(void)
	//{
	//	Reset();
	//}
	//
	//CDeriveLiDARVariable::~CDeriveLiDARVariable(void)
	//{}
	//
	//void CDeriveLiDARVariable::Reset()
	//{
	//	m_projectFilePath.Empty();
	//	m_inputImageFilePath.Empty();
	//	m_outputImageFilePath.Empty();
	//	m_inputDatasetFilePath.Empty();
	//	m_outputDatasetFilePath.Empty();
	//	m_resolution=30;
	//	m_variables.RemoveAll();
	//	m_bCreateGrid=false;
	//	m_bCreateDataset=true;
	//	m_scaleFactorIn=1;
	//	m_scaleFactorOut=1;
	//	m_cellType=GDT_Float32;
	//}
	//
	//
	//std::string CDeriveLiDARVariable::GetMember(int i, LPXNode&)const
	//{
	//	std::string str;
	//	std::string path=UtilWin::GetPath(m_projectFilePath);
	//
	//	switch(i)
	//	{
	//	case INPUT_GRID_FILEPATH: str = GetRelativePath(path, m_inputImageFilePath);  break;
	//	case CREATE_DATASET: str = ToString(m_bCreateDataset); break;
	//	case INPUT_DATASET_FILEPATH: str = GetRelativePath(path, m_inputDatasetFilePath); break;
	//	case OUTPUT_DATASET_FILEPATH: str = GetRelativePath(path, m_outputDatasetFilePath); break;
	//	case CREATE_GRID: str = ToString(m_bCreateGrid); break;
	//	case OUTPUT_GRID_FILEPATH: str = GetRelativePath(path, m_outputImageFilePath); break;
	//	case RESOLUTION: str = ToString(m_resolution); break;
	//	case VARIABLES: str = m_variables.ToString(); break;
	//	case SCALE_FACTOR_IN: str = ToString(m_scaleFactorIn); break;
	//	case SCALE_FACTOR_OUT: str = ToString(m_scaleFactorOut); break;
	//	case CELL_TYPE: str = ToString(m_cellType); break;
	//	default: ASSERT(false);
	//	}
	//
	//	return str;
	//}
	//
	//void CDeriveLiDARVariable::SetMember(int i, const std::string& str, const LPXNode&)
	//{
	//	std::string path=UtilWin::GetPath(m_projectFilePath);
	//	switch(i)
	//	{
	//	case INPUT_GRID_FILEPATH: m_inputImageFilePath = GetAbsolutePath(path, str); break;
	//	case CREATE_DATASET: m_bCreateDataset = ToBool(str); break;
	//	case INPUT_DATASET_FILEPATH: m_inputDatasetFilePath = GetAbsolutePath(path, str); break;
	//	case OUTPUT_DATASET_FILEPATH: m_outputDatasetFilePath = GetAbsolutePath(path, str); break;
	//	case CREATE_GRID: m_bCreateGrid = ToBool(str); break;
	//	case OUTPUT_GRID_FILEPATH: m_outputImageFilePath = GetAbsolutePath(path, str); break;
	//	case RESOLUTION: m_resolution = ToDouble(str); break;
	//	case VARIABLES: m_variables.FromString(str); break;
	//	case SCALE_FACTOR_IN: m_scaleFactorIn = ToDouble(str); break;
	//	case SCALE_FACTOR_OUT: m_scaleFactorOut = ToDouble(str); break;
	//	case CELL_TYPE: m_cellType = ToInt(str); break;
	//	default: ASSERT(false);
	//	}
	//}
	//
	//
	//ERMsg CDeriveLiDARVariable::Execute( CFL::CCallback& callback )
	//{
	//	ERMsg msg;
	//
	//	if( m_bCreateDataset )
	//	{
	//		msg += ExtractDataset(callback );
	//	}
	//
	//	if( msg && m_bCreateGrid)
	//	{
	//		msg += ExtractGrid( callback );
	//	}
	//
	//	return msg;
	//}
	//
	///*void CDeriveLiDARVariable::FillMapInfo(const CMapBilFile& inputBIL, CMapBilFile& outputBIL, int nbVariables, double resolution)
	//{
	//	CGeoRect bx = inputBIL.GetBoundingBox();
	//
	//	outputBIL = inputBIL;
	//
	//	int nbCols = int(inputBIL.GetNbCols()*inputBIL.GetCellSizeX()/resolution);
	//	int nbRows = int(inputBIL.GetNbRows()*inputBIL.GetCellSizeY()/resolution);
	//	
	//	outputBIL.SetNbCols(nbCols);
	//	outputBIL.SetNbRows(nbRows);
	//
	//	bx.m_xMax = bx.m_xMin + nbCols*resolution;
	//	bx.m_yMin = bx.m_yMax - nbRows*resolution;
	//	outputBIL.SetBoundingBox(bx);
	//
	//	outputBIL.SetCellSizeX(resolution);//bx.Width()/nbCols);
	//	outputBIL.SetCellSizeY(resolution);//bx.Height()/nbRows);
	//
	//	outputBIL.SetCellType(m_cellType);
	//	outputBIL.SetNbBand(nbVariables);
	//	
	//}
	//*/
	//
	//
	//
	//GDALDataset* CDeriveLiDARVariable::CreateOutputDataset(GDALDataset* poDataset, GDALDriver* poDriverOut, const std::string& outputImageFilePath, int nbVariables, double resolution, GDALDataType cellType)
	//{
	//	double GT[6] = {0};
	//	poDataset->GetGeoTransform(GT);
	//	
	//	int XSize = int(abs(poDataset->GetRasterXSize()*GT[GT_CELLSIZE_X]/resolution));
	//	int YSize = int(abs(poDataset->GetRasterYSize()*GT[GT_CELLSIZE_Y]/resolution));
	//
	//	double adfGeoTransform[6] = {GT[0], resolution, GT[2], GT[3], GT[4], -resolution};
	//	char **papszOptions = NULL;
	//
	//	GDALDataset* poDstDS = poDriverOut->Create( outputImageFilePath, XSize, YSize, nbVariables+1, cellType, papszOptions );
	//	ASSERT(poDstDS);
	//
	//	if( poDstDS)
	//	{
	//		poDstDS->SetGeoTransform( adfGeoTransform );
	//		poDstDS->SetProjection( poDataset->GetProjectionRef() );
	//	}
	//	
	//	return  poDstDS;
	//}
	//
	//void CDeriveLiDARVariable::ComputeStatistic( const CArray<CFloatArray>& profileArray, int firstCol, int nextCol, CLiDARStat& stat)
	//{
	//	for(int i=0; i<profileArray.GetSize(); i++)
	//	{
	//		for(int j=firstCol; j<nextCol; j++)
	//		{
	//			unsigned char value = unsigned char(profileArray[i][j]);
	//			double fValue = value*m_scaleFactorIn;
	//			stat += fValue;
	//		}
	//	}
	//}
	//
	//
	//ERMsg CDeriveLiDARVariable::ExtractGrid(CFL::CCallback& callback)
	//{
	//	callback.SetNbStep(1);
	//
	//	ERMsg msg;
	//
	//	GDALDataset* poDataset = (GDALDataset *) GDALOpen( m_inputImageFilePath, GA_ReadOnly );
	//
	//	if( poDataset == NULL)
	//	{
	//		msg.ajoute(CPLGetLastErrorMsg());
	//		return msg;
	//	}
	//		 
	//	GDALDriver* poDriverIn = poDataset->GetDriver();
	//	std::string test = poDriverIn->GetDescription();
	//	GDALDriver* poDriverOut = GetDriverFromExtension(GetFileExtension(m_outputImageFilePath), true);
	//	test = poDriverIn->GetDescription();
	//	
	//	if( poDriverOut == NULL)
	//	{
	//		std::string error;
	//		error.FormatMessage( IDS_UNKNOWN_OUTPUT_FORMAT, GetFileName(m_outputImageFilePath) );
	//		msg.ajoute( error );
	//		return msg;
	//	}
	//	
	//	GDALDataType cellType = m_cellType==0?GDT_Byte:GDALDataType(m_cellType);
	//	GDALDataset* poDstDS = CreateOutputDataset(poDataset, poDriverOut, m_outputImageFilePath, m_variables.GetSize(), m_resolution, cellType);
	//
	//	if( poDstDS == NULL)
	//	{
	//		msg.ajoute(CPLGetLastErrorMsg());
	//		return msg;
	//	}
	//	callback.SetCurrentDescription(GetString(IDS_EXTRACT_GRID));
	//	callback.SetCurrentStepRange(0, poDstDS->GetRasterXSize()*poDstDS->GetRasterYSize(), 1);
	//	callback.SetStartNewStep();
	//	
	//	int nXSize = poDstDS->GetRasterXSize();
	//	int nYSize = poDstDS->GetRasterYSize();
	//	int nbCells = nXSize*nYSize;
	//	double GT[6] = {6};
	//	
	//	poDataset->GetGeoTransform(GT);
	//	
	//	double cellSizeX = fabs(GT[GT_CELLSIZE_X]);
	//	double cellSizeY = fabs(GT[GT_CELLSIZE_Y]);
	//
	//	std::stringArrayEx outputTypeName( IDS_OUTPUT_TYPE_NAME);
	//	ASSERT( outputTypeName.GetSize() == 7);
	//
	//	//callback.AddMessage(GetString(IDS_INPUT_DATASET) + m_trainingSetFilePath, 1);
	//	callback.AddMessage(GetString(IDS_INPUT_MAP) + m_inputImageFilePath, 1);
	//	callback.AddMessage(GetString(IDS_OUTPUT_MAP) + m_outputImageFilePath, 1);
	//	callback.AddMessage(GetString(IDS_OUTPUT_IMAGE_FORMAT) + poDriverOut->GetMetadataItem( GDAL_DMD_LONGNAME ), 1 );
	//	
	////	callback.AddMessage(GetString(IDS_NB_PLOT) + ToString(dataTable.GetSize()), 1);
	//	callback.AddMessage(GetString(IDS_NB_CELL) + ToString(nXSize) + " x " + ToString(nYSize) + " = " + ToString(nbCells), 1);
	//	callback.AddMessage(GetString(IDS_NB_BAND) + ToString(m_variables.GetSize()), 1);
	////	callback.AddMessage(GetString(IDS_NO_DATA) + ToString(m_noData), 1);
	//	callback.AddMessage(GetString(IDS_OUTPUT_TYPE) + outputTypeName[m_cellType] + " (" + ToString(m_cellType) + ")", 1);
	//	callback.AddMessage("");
	//
	//
	//	GDALRasterBand* poBand = poDataset->GetRasterBand(1);
	//	
	//	CArrayEx<GDALRasterBand* >	poOutputBand;
	//	CArray<CFloatArray> bandData;
	//	
	//	poOutputBand.SetSize( poDstDS->GetRasterCount() );
	//	bandData.SetSize( poDstDS->GetRasterCount() );
	//	for(int k=0; k<poDstDS->GetRasterCount(); k++)
	//	{
	//		poOutputBand[k] = poDstDS->GetRasterBand(k+1);
	//		bandData[k].SetSize(poDstDS->GetRasterXSize());
	//	}
	//
	//	
	//
	//	bool bValueExceedLimit=false;
	//	int nbCols = int(m_resolution/cellSizeX);
	//	int nbRows = int(m_resolution/cellSizeY);
	//
	//	CArray<CFloatArray> profiles;
	//	profiles.SetSize(nbRows);
	//	for(int i=0; i<nbRows; i++)
	//		profiles[i].SetSize(poBand->GetXSize());
	//	
	//
	//	for(int i=0; i<poDstDS->GetRasterYSize()&&msg; i++)
	//	{
	//		int firstRow = nbRows*i;
	//		int nextRow = nbRows*(i+1);
	//		
	//		ASSERT( firstRow>=0 && firstRow<poBand->GetYSize() );
	//		ASSERT( nextRow>=0 && nextRow<=poBand->GetYSize() );
	//
	//		
	//		
	//		for(int j=0; j<nbRows&&msg; j++)
	//		{
	//			CPLErr err = poBand->RasterIO( GF_Read, 0, firstRow+j, poBand->GetXSize(), 1, profiles[j].GetData(), poBand->GetXSize(), 1, GDT_Float32, 0, 0 );
	//			
	//			msg+=callback.StepIt(0);
	//			if( err != CE_None )
	//				msg.ajoute(CPLGetLastErrorMsg());
	//		}
	//
	//		//compute statistic for each col
	//		for(int j=0; j<poDstDS->GetRasterXSize()&&msg; j++)
	//		{
	//			int firstCol = nbCols*j;
	//			int nextCol = nbCols*(j+1);
	//		
	//			ASSERT( firstCol>=0 && firstCol<poBand->GetXSize() );
	//			ASSERT( nextCol>=0 && nextCol<=poBand->GetXSize() );
	//
	//		
	//			CLiDARStat stat;
	//			ComputeStatistic( profiles, firstCol, nextCol, stat);
	//			for(int k=0; k<m_variables.GetSize(); k++)
	//			{
	//				double value = stat[m_variables[k]]*m_scaleFactorOut;
	//				if( value < GDAL_CELL_LIMIT[m_cellType][0] || value > GDAL_CELL_LIMIT[m_cellType][1] )
	//				{
	//					bValueExceedLimit=true;
	//					value = Min( GDAL_CELL_LIMIT[m_cellType][1], Max( value, GDAL_CELL_LIMIT[m_cellType][0])  );
	//				}
	//
	//				bandData[k][j] = float(value);
	//			}
	//			
	//			bandData[m_variables.GetSize()][j] = 0;//mask
	//
	//			msg+=callback.StepIt();
	//		}
	//
	//		for(int k=0; k<poDstDS->GetRasterCount(); k++)
	//			poOutputBand[k]->RasterIO( GF_Write, 0, i, poDstDS->GetRasterXSize(), 1, bandData[k].GetData(), poDstDS->GetRasterXSize(), 1, GDT_Float32, 0, 0  );    
	//	}
	//
	//	// Once we're done, close properly the dataset 
	//    GDALClose( (GDALDatasetH) poDataset );
	//
	//	// Once we're done, close properly the dataset 
	//    GDALClose( (GDALDatasetH) poDstDS );
	//
	//	//If some value exceed limit, we put an warning
	//	if( bValueExceedLimit )
	//	{
	//		std::string str1 = GetString(IDS_STR_WARNING) + ": " ;
	//		std::string str2;
	//		str2.FormatMessage( IDS_VALUE_EXCEED_LIMIT, ToString( GDAL_CELL_LIMIT[m_cellType][0] ), ToString( GDAL_CELL_LIMIT[m_cellType][1] )  );
	//		
	//		callback.AddMessage( str1 + str2, 1);
	//	}
	//
	//	return msg;
	//}
	//

	//
	//void CDeriveLiDARVariable::GetProfileCell(CMFCGDALDataset* poDataset, const CGeoPoint& pt, double resolution, CArray<CFloatArray>& profiles)
	//{
	//	GDALRasterBand* poBand = poDataset->m_poDataset->GetRasterBand(1);
	//
	//	double GT[6] = {6};
	//	
	//	poDataset->m_poDataset->GetGeoTransform(GT);
	//	
	//	double cellSizeX = fabs(GT[GT_CELLSIZE_X]);
	//	double cellSizeY = fabs(GT[GT_CELLSIZE_Y]);
	//
	//	//Compute bounding box
	//	double xRes = resolution/cellSizeX;
	//	double yRes = resolution/cellSizeY;
	//
	////	OGRSpatialReference oSRS;
	////	oSRS.
	//
	//	
	//	CPoint xy;
	//	if( poDataset->CoordToXYPos(pt, xy, true) )
	//	{
	//		long firstCol=Max(0l, xy.x-long(xRes/2));
	//		long nextCol=Min(poDataset->m_poDataset->GetRasterXSize(), int(firstCol+xRes+0.5));
	//		long nXSize = nextCol-firstCol;
	//		long firstRow=Max(0l, xy.y-long(xRes/2));
	//		long nextRow=Min(poDataset->m_poDataset->GetRasterYSize(), int(firstRow+xRes+0.5));
	//		long nYSize = nextRow-firstRow;
	//
	// 		profiles.SetSize(nYSize);
	//		for(int i=0; i<nYSize; i++)
	//		{
	//			profiles[i].SetSize(nXSize);
	//			poBand->RasterIO( GF_Read, firstCol, firstRow+i, nXSize, 1, profiles[i].GetData(), nXSize, 1, GDT_Float32, 0, 0 );
	//		}
	//	}
	//}
	//
	//ERMsg CDeriveLiDARVariable::ExtractDataset(CFL::CCallback& callback)
	//{
	//	callback.SetNbStep(1);
	//	CLiDARStat::SetVMiss(-999);
	//
	//	ERMsg msg;
	//	
	//	CMFCGDALDataset poDataset;
	//	CShapeFileBase inputDataset;
	//
	//	msg += poDataset.Open( m_inputImageFilePath, GA_ReadOnly );
	//	//GDALDataset* poDataset = (GDALDataset *) GDALOpen( inputFilePath, GA_ReadOnly );
	//
	//	//if( poDataset == NULL)
	//	//{
	//	//	msg.ajoute(CPLGetLastErrorMsg());
	//	//	return msg;
	//	//}
	//
	//	
	//	
	//	msg += inputDataset.Read(m_inputDatasetFilePath);
	//	
	//	if( msg )
	//	{
	//		callback.SetCurrentDescription( GetString(IDS_EXTRACT_DATASET) );
	//		callback.SetCurrentStepRange(0, inputDataset.GetRecords().GetSize(), 1);
	//		callback.SetStartNewStep();
	//
	//		//create DBF Table
	//		CShapeFileBase outputDataset = inputDataset;
	//
	//		CDBF3& dbf = outputDataset.GetDBF();
	//		int colBase = dbf.GetTableField().GetSize();
	//		for(int i=0; i<m_variables.GetSize(); i++)
	//		{
	//			dbf.AddField(CDBFField(CLiDARStat::GetID(m_variables[i]),"8.3f"));
	//		}
	//
	//		//GDALRasterBand* poBand = poDataset->GetRasterBand(1);
	//		const CSFRecordArray& records = inputDataset.GetRecords();
	//		for(int i=0; i<records.GetSize()&&msg; i++)
	//		{
	//			const CSFShape& shape = records[i]->GetShape();
	//
	//			CArray<CFloatArray> profiles;
	//			GetProfileCell(&poDataset, shape.GetPoint(0), m_resolution, profiles);
	//			
	//			CLiDARStat stat;
	//			if( profiles.GetSize() > 0)
	//				ComputeStatistic( profiles, 0, profiles[0].GetSize(), stat);
	//				
	//			for(int j=0; j<m_variables.GetSize(); j++)
	//			{
	//				dbf[i].SetElement(colBase+j, float(stat[m_variables[j]]*m_scaleFactorOut) );
	//			}
	//
	//			msg += callback.StepIt();
	//		}
	//
	//		msg += outputDataset.Write(m_outputDatasetFilePath);
	//
	//		// Once we're done, close properly the dataset 
	//	    poDataset.Close();
	//	}
	//
	//	return msg;
	//}
}