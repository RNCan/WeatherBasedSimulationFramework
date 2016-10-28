#include "StdAfx.h"
#include "SpectralInterpol.h"
#include "UtilGDAL.h"
#include "KNearestNeighbor.h"
#include "tmplEx\ArrayEx.h"
#include "resource.h"
#include "CommonRes.h"
#include "Statistic.h"
#include "TmplEx\SortedKeyArray.h"
#include "2DimArray.h"
//#include "GeomaticsTools\Source\GDALBasic.h"


#ifdef _OPENMP
#include "omp.h"
#endif

static const int FIXED_SEED = 100;


using namespace UtilWin;
using namespace CFL;

const char* CSpectralInterpol::XML_FLAG = "KNNMapping";
const char* CSpectralInterpol::MEMBER_NAME[NB_MEMBER] = {"InputImageFilePath",/*"InputMask",*/"InputTrainingSetFilePath", "Power", "NbPoints", "OutputImageFilePath", "CellType", "NoDataIn", "NoDataOut", "Standardized", "StandardizedFromImage", "UnifiedNoData", "CreateError", "CreateNearestNeightborg" };


CSpectralInterpol::CSpectralInterpol()
{
	Reset();
}

CSpectralInterpol::~CSpectralInterpol(void)
{
}

void CSpectralInterpol::Reset()
{
	m_projectFilePath.Empty();
	m_inputImageFilePath.Empty();
	m_trainingSetFilePath.Empty();
	m_outputImageFilePath.Empty();
//	m_mask.Empty();

	m_nbPoints=10;
	m_power=2;
	m_cellType=GDT_Float32;
	m_noDataIn=0;
	m_noDataOut=0;
	m_bStandardized=true;
	m_bStandardizedFromImage=false;
	m_bAllBandNoData=false;
	m_bCreateError=false;
	m_bCreateNearestNeightBorg=false;

}

CString CSpectralInterpol::GetMember(int i, LPXNode&)const
{
	CString str;
	CString path=UtilWin::GetPath(m_projectFilePath);

	switch(i)
	{
	case INPUT_IMAGE_FILEPATH: str = GetRelativePath(path, m_inputImageFilePath); break;
//	case INPUT_MASK: str = m_mask; break;
	case INPUT_TRANINGSET_FILEPATH: str = GetRelativePath(path, m_trainingSetFilePath); break;
	case POWER: str = ToString(m_power); break;
	case NB_POINTS: str = ToString(m_nbPoints); break;
	case OUTPUT_IMAGE_FILEPATH: str = GetRelativePath(path, m_outputImageFilePath); break;
	case CELL_TYPE: str = ToString(m_cellType); break;
	case NO_DATA_IN: str = ToString(m_noDataIn); break;
	case NO_DATA_OUT: str = ToString(m_noDataOut); break;
	case STANDARDIZED: str = ToString(m_bStandardized); break;
	case STANDARDIZED_FROM_IMAGE: str = ToString(m_bStandardizedFromImage); break;
	case UNIFIED_NO_DATA: str = ToString(m_bAllBandNoData); break;
	case CREATE_ERROR: str = ToString(m_bCreateError); break;
	case CREATE_NEAREST_NEIGHTBORG: str = ToString(m_bCreateNearestNeightBorg); break;
	default: ASSERT(false);
	}
	
	
	return str;
}

void CSpectralInterpol::SetMember(int i, const CString& str, const LPXNode&)
{
	CString path=UtilWin::GetPath(m_projectFilePath);

	switch(i)
	{
	case INPUT_IMAGE_FILEPATH: m_inputImageFilePath = GetAbsolutePath(path, str); break;
//	case INPUT_MASK: m_mask = str; break;
	case INPUT_TRANINGSET_FILEPATH: m_trainingSetFilePath = GetAbsolutePath(path, str); break;
	case POWER: m_power = ToDouble(str); break;
	case NB_POINTS: m_nbPoints = ToInt(str); break;
	case OUTPUT_IMAGE_FILEPATH: m_outputImageFilePath = GetAbsolutePath(path, str); break;
	case CELL_TYPE: m_cellType = ToInt(str); break;
	case NO_DATA_IN: m_noDataIn = ToFloat(str); break;
	case NO_DATA_OUT: m_noDataOut = ToFloat(str); break;
	case STANDARDIZED:  m_bStandardized = ToBool(str); break;
	case STANDARDIZED_FROM_IMAGE: m_bStandardizedFromImage = ToBool(str); break;
	case UNIFIED_NO_DATA: m_bAllBandNoData= ToBool(str); break;
	case CREATE_ERROR: m_bCreateError= ToBool(str); break;
	case CREATE_NEAREST_NEIGHTBORG: m_bCreateNearestNeightBorg= ToBool(str); break;
	default: ASSERT(false);
	}
}

ERMsg CSpectralInterpol::VerifyInput(CKNearestNeighbor& dataTable, GDALDataset* poInputDS)const
{
	ERMsg msg;
	if( m_noDataOut < GDAL_CELL_LIMIT[m_cellType][0] || m_noDataOut > GDAL_CELL_LIMIT[m_cellType][1] )
	{
		CString str;
		str.FormatMessage( IDS_INVALID_NO_DATA, ToString(m_noDataOut), ToString( GDAL_CELL_LIMIT[m_cellType][0] ), ToString( GDAL_CELL_LIMIT[m_cellType][1] ) );
		msg.ajoute(str);
	}

	if( dataTable.GetNbDimension() != poInputDS->GetRasterCount() )
	{
		CString error;
		error.FormatMessage(IDS_DIMENSION_MISMATCH, ToString(poInputDS->GetRasterCount()), ToString(dataTable.GetNbDimension()) );
		msg.ajoute(error);
		return msg;
	}

	return msg;
}

ERMsg CSpectralInterpol::OpenInputImage(GDALDataset** poInputDS)const
{
	ERMsg msg;

	//open image
	*poInputDS = (GDALDataset *) GDALOpen( m_inputImageFilePath, GA_ReadOnly );

	if( *poInputDS == NULL)
		msg.ajoute(CPLGetLastErrorMsg());

	return msg;
}

//CString CSpectralInterpol::GetMaskFilePath(const CString& imageFilePath)
//{
//	CString tmp = imageFilePath;
//	SetFileTitle(tmp, GetFileTitle(imageFilePath) + "Mask" );
//	
//	return tmp;
//}
//
//ERMsg CSpectralInterpol::OpenMaskImage(GDALDataset** poMaskDS)const
//{
//	ERMsg msg;
//
//	//open image
//	*poMaskDS = (GDALDataset *) GDALOpen( GetMaskFilePath(m_inputImageFilePath), GA_ReadOnly );
//
//	if( *poMaskDS == NULL)
//	{
//		msg.ajoute(CPLGetLastErrorMsg());
//		msg.ajoute( GetString( IDS_MASK_NOT_FOUND) );
//	}
//
//	return msg;
//}

ERMsg CSpectralInterpol::OpenOutputImage(GDALDataset* poInputDS, GDALDataset** poOutputDS)const
{
	ERMsg msg;

	if( poInputDS==NULL)
		return msg;

	GDALDriver* poDriverOut = GetDriverFromExtension(GetFileExtension(m_outputImageFilePath), true);
	
	if( poDriverOut == NULL)
	{
		CString error;
		error.FormatMessage( IDS_UNKNOWN_OUTPUT_FORMAT, GetFileName(m_outputImageFilePath) );
		msg.ajoute( error );
		return msg;
	}


    char **papszOptions = NULL;
	GDALDataType cellType = m_cellType==0?GDT_Byte:GDALDataType(m_cellType);
	*poOutputDS = poDriverOut->Create( m_outputImageFilePath, poInputDS->GetRasterXSize(), poInputDS->GetRasterYSize(), 1, cellType, papszOptions );

	if( *poOutputDS == NULL)
	{
		msg.ajoute(CPLGetLastErrorMsg());
		return msg;
	}
	
	
	//init output image with input image
	double adfGeoTransform[6] = {0};
	poInputDS->GetGeoTransform(adfGeoTransform);
    (*poOutputDS)->SetGeoTransform( adfGeoTransform );
	(*poOutputDS)->SetProjection( poInputDS->GetProjectionRef() );

	return msg;
}

ERMsg CSpectralInterpol::Interpol( CSCCallBack& callback)const
{
	ERMsg msg;

	//fixed seed randomization for selection best frequency value
	CFL::Randomize(FIXED_SEED);
	//load mask value to process
//	bool bUseMask = LoadMask();

	//load data table and images
	CKNearestNeighbor dataTable;
	//GDALDataset* poMaskDS = NULL;
	GDALDataset* poInputDS = NULL;
	GDALDataset* poOutputDS = NULL;
	GDALDataset* poErrorDS = NULL;
	GDALDataset* poNeirestNeighborDS = NULL;
	CMFCGDALDataset errorDataset;
	CMFCGDALDataset nnDataset;

	msg += LoadDataTable(dataTable, false, callback);
	msg += OpenInputImage(&poInputDS);
	msg += OpenOutputImage(poInputDS, &poOutputDS);
	if(msg)
		msg += VerifyInput(dataTable, poInputDS);	

	if( msg && m_bCreateError )
	{
		CStdString filePath(m_outputImageFilePath);
		filePath.SetFileTitle( filePath.GetFileTitle() + "_error");
		filePath.SetFileExtension(".tif");
		
		errorDataset.OpenCopy( filePath, poInputDS, "GTIFF", GDT_Float32, 1, -9999);
		poErrorDS = errorDataset.m_poDataset;
	}
	
	if( msg && m_bCreateNearestNeightBorg )
	{
		CStdString filePath(m_outputImageFilePath);
		filePath.SetFileTitle( filePath.GetFileTitle() + "_NN");
		filePath.SetFileExtension(".tif");
		
		nnDataset.OpenCopy( filePath, poInputDS, "GTIFF", GDT_Float32, 1, -9999);
		poNeirestNeighborDS = nnDataset.m_poDataset;
	}
		
	//if( bUseMask )
		//msg += OpenMaskImage(&poMaskDS);

	if(!msg)
		return msg;

	
	int nbDimension = dataTable.GetNbDimension();
	int nXSize = poInputDS->GetRasterXSize();
	int nYSize = poInputDS->GetRasterYSize();
	int nbCells = nXSize*nYSize;
	
	//int maskIndex = nbDimension;
//	ASSERT( maskIndex>=0 && maskIndex<poInputDS->GetRasterCount() );
	//ASSERT( m_maskArray.IsEmpty() || poMaskDS!= NULL);

	//input image
	CArray<GDALRasterBand*> poBandArray;
	CArrayEx < CArrayEx<float> > profiles;
	
	poBandArray.SetSize(nbDimension);
	profiles.SetSize(nbDimension);
	std::vector<double> noData(nbDimension);
	CString noDataStr;

	for(int k=0; k<nbDimension; k++)
	{
		poBandArray[k] = poInputDS->GetRasterBand( k+1 );//1 base
		profiles[k].SetSize(nXSize);
		if(poBandArray[k])
		{
			int bSuccess=false;
			noData[k] = poBandArray[k]->GetNoDataValue(&bSuccess);
			if( !bSuccess )
				noData[k] = m_noDataIn;

			if( k!=0) noDataStr += ", ";
			noDataStr += ToString(noData[k]);
		}
	}

	CStringArrayEx outputTypeName( IDS_OUTPUT_TYPE_NAME);
	ASSERT( outputTypeName.GetSize() == 7);
	

	//init message
	callback.AddMessage(GetString(IDS_INPUT_DATASET) + m_trainingSetFilePath, 1);
	callback.AddMessage(GetString(IDS_INPUT_MAP) + m_inputImageFilePath, 1);
//	callback.AddMessage(GetString(IDS_INPUT_MASK) + (bUseMask?GetMaskFilePath(m_inputImageFilePath):""), 1);
	callback.AddMessage(GetString(IDS_OUTPUT_MAP) + m_outputImageFilePath, 1);
	callback.AddMessage(GetString(IDS_OUTPUT_IMAGE_FORMAT) + poOutputDS->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ), 1 );
//	callback.AddMessage(GetString(IDS_MASK_USED) + m_mask, 1);
	
	callback.AddMessage(GetString(IDS_NB_PLOT) + ToString(dataTable.GetSize()), 1);
	callback.AddMessage(GetString(IDS_NB_CELL) + ToString(nXSize) + " x " + ToString(nYSize) + " = " + ToString(nbCells), 1);
	callback.AddMessage(GetString(IDS_NB_BAND) + ToString(nbDimension), 1);
	callback.AddMessage("K= " + ToString(m_nbPoints), 1);
	callback.AddMessage("T= " + ToString(m_power,2), 1);
	callback.AddMessage(GetString(IDS_NO_DATA_IN) + noDataStr, 1);
	callback.AddMessage(GetString(IDS_NO_DATA) + ToString(m_noDataOut), 1);
	callback.AddMessage(GetString(IDS_OUTPUT_TYPE) + outputTypeName[m_cellType] + " (" + ToString(m_cellType) + ")", 1);
	callback.AddMessage("");
	
	
	//init progress bar
	callback.SetCurrentDescription(GetString(IDS_EXEC_DESCRIPTION));
	callback.SetCurrentStepRange(0, nYSize, 1);
	callback.SetStartNewStep();

	
	
	//mask
	//GDALRasterBand* poMaskBand = NULL;
	//CArrayEx<int> maskValue;
	//if( bUseMask )
	//{
		//poMaskBand = poMaskDS->GetRasterBand(1);
		//maskValue.SetSize(nXSize);
	//}

	//Output
	CArrayEx<float>  outputProfile;
	outputProfile.SetSize(nXSize);
	
	CArrayEx<float>  errorProfile;
	errorProfile.SetSize(nXSize);
	
	CArrayEx<float>  nearestNeighborProfile;
	nearestNeighborProfile.SetSize(nXSize);
	

	GDALRasterBand *poOutputBand = poOutputDS->GetRasterBand(1);
	poOutputBand->SetNoDataValue( m_noDataOut );

	GDALRasterBand *poErrorBand = poErrorDS?poErrorDS->GetRasterBand(1):NULL;
	GDALRasterBand *poNeirestNeighborBand = poNeirestNeighborDS?poNeirestNeighborDS->GetRasterBand(1):NULL;
	
	

	bool bValueExceedLimit = false;
	//for all profile
	for(int i=0; i<nYSize&&msg; i++)
	{
		//read onle line of all band
		for(int k=0; k<nbDimension; k++)
		{
			CPLErr err = poBandArray[k]->RasterIO( GF_Read, 0, i, nXSize, 1, profiles[k].GetData(), nXSize, 1, GDT_Float32, 0, 0 );
			
			if( err != CE_None )
			{
				msg.ajoute(CPLGetLastErrorMsg());
				return msg;
			}
		}
		
		//if( bUseMask )
		//{
		//	CPLErr err = poMaskBand->RasterIO( GF_Read, 0, i, nXSize, 1, maskValue.GetData(), nXSize, 1, GDT_UInt32, 0, 0 );
		//	
		//	if( err != CE_None )
		//	{
		//		msg.ajoute(CPLGetLastErrorMsg());
		//		return msg;
		//	}
		//}
		
		
		//for all cell in the profile
#ifdef _OPENMP
		#pragma omp parallel for schedule(static, 100)
#endif
		for(int j=0; j<nXSize; j++)
		{
			CMDPoint pt(nbDimension);
			//bool bCellIncluded = m_maskArray.IsEmpty() || m_maskArray.Lookup( maskValue[j] ) != -1;
			//look the mask first
			//if( bCellIncluded )
			//{
			bool bCellIncluded = m_bAllBandNoData?false:true;
			//fill search point
			for(int k=0; k<nbDimension; k++)
			{
				pt[k] = profiles[k][j];
				if( m_bAllBandNoData )
				{
					//if one band is take, we thake it
					if( pt[k] >= noData[k] )
						bCellIncluded = true;
				}
				else
				{
					//if one band is bad, we exclude it
					if( pt[k] <= noData[k] )
						bCellIncluded = false;
				}
			}
			//}

			if( bCellIncluded )
			{
				//look for the nearest point
				CSearchResultArray result;
				dataTable.Search(pt, m_nbPoints, result);

				//compute the mean

				double bio=0;
				if( m_power==FREQUENCY)
				{
					//sorting by value
					CSortedKeyIndexLArray indexs;
					for(int k=0; k<result.GetSize(); k++)
					{
						int v = int(dataTable.GetBio(result[k].m_index)+0.5);
						int pos = (int)indexs.LookupKey(v);
						if(pos == -1)
							indexs.AddSorted(CSortedKeyIndexL(v, 0));
						else indexs[pos].m_index++;
					}
					ASSERT( indexs.GetSize() > 0);

					//now sorting by occurence
					CSortedKeyIndexLArray indexs2;
					for(int i=0; i<indexs.GetSize(); i++)
						indexs2.AddSorted( CSortedKeyIndexL(indexs[i].m_index, indexs[i].m_key) );
			
					int nbMaxValue = 1;
					for(int i=1; i<indexs2.GetSize(); i++)
						if( indexs2[i].m_key == indexs2[0].m_key )
							nbMaxValue++;


					//ici le rand ne sera pas toujours pareil au cose d'OMP
					int bestIndex = CFL::Rand(0, nbMaxValue-1);
				
					bio = indexs2[bestIndex].m_index;
				}
				else
				{
					double xSum=result.GetXSum(m_power);
					for(int k=0; k<result.GetSize(); k++)
					{
						double weight = result[k].GetXTemp(m_power)/xSum; 
						bio+=dataTable.GetBio(result[k].m_index)*weight;
					}
				}

				
				if( bio < GDAL_CELL_LIMIT[m_cellType][0] || bio > GDAL_CELL_LIMIT[m_cellType][1] )
				{
					bValueExceedLimit=true;
					bio = Min( GDAL_CELL_LIMIT[m_cellType][1], Max( bio, GDAL_CELL_LIMIT[m_cellType][0])  );
				}

				outputProfile[j] = float(bio);
				
				if( m_bCreateError && result.GetSize()>1)
				{
					double sum=0;
					for(int k=0; k<result.GetSize(); k++)
					{
						sum+=CFL::Square(dataTable.GetBio(result[k].m_index)-bio);
					}

					errorProfile[j] = (float)(sum/(result.GetSize()-1));
				}
				
				if( m_bCreateNearestNeightBorg )
					nearestNeighborProfile[j] = (float)result[0].m_distance;
			}//mask
			else 
			{
				//outside the mask, then nodata
				outputProfile[j] = float(m_noDataOut);
				errorProfile[j] = -9999;
				nearestNeighborProfile[j] = -9999;
			}
		}

		msg += callback.StepIt();

		//Write profile to disk
		poOutputBand->RasterIO( GF_Write, 0, i, nXSize, 1, outputProfile.GetData(), nXSize, 1, GDT_Float32, 0, 0  );    

		if( poErrorBand )
			poErrorBand->RasterIO( GF_Write, 0, i, nXSize, 1, errorProfile.GetData(), nXSize, 1, GDT_Float32, 0, 0  );    

		if(poNeirestNeighborBand)
			poNeirestNeighborBand->RasterIO( GF_Write, 0, i, nXSize, 1, nearestNeighborProfile.GetData(), nXSize, 1, GDT_Float32, 0, 0  );    
	}

	// Once we're done, close properly the dataset 
    GDALClose( (GDALDatasetH) poInputDS );
    GDALClose( (GDALDatasetH) poOutputDS );
	errorDataset.Close();
	nnDataset.Close();

	//If some value exceed limit, we put an warning
	if( bValueExceedLimit )
	{
		CString str1 = GetString(IDS_STR_WARNING) + ": " ;
		CString str2;
		str2.FormatMessage( IDS_VALUE_EXCEED_LIMIT, ToString( GDAL_CELL_LIMIT[m_cellType][0] ), ToString( GDAL_CELL_LIMIT[m_cellType][1] )  );
		
		callback.AddMessage( str1 + str2, 1);
	}

	return msg;
}

ERMsg CSpectralInterpol::GetStatistic(CFL::CStatisticVector& stats, CSCCallBack& callback)const
{
	ERMsg msg;

	//open image
	GDALDataset* poInputDS = (GDALDataset *) GDALOpen( m_inputImageFilePath, GA_ReadOnly );

	if( poInputDS == NULL)
	{
		msg.ajoute(CPLGetLastErrorMsg());
		return msg;
	}

	int nXSize = poInputDS->GetRasterXSize();
	int nYSize = poInputDS->GetRasterYSize();
	int nbBand = poInputDS->GetRasterCount();
	int nbCells = nXSize*nYSize;
	
	callback.SetCurrentDescription(GetString(IDS_LOAD_STAT_DESCRIPTION));
	callback.SetCurrentStepRange(0, nYSize*nbBand, 1);
	callback.SetStartNewStep();

	CArray<GDALRasterBand*> poBandArray;
	CArrayEx<float> profiles;
	
	
	poBandArray.SetSize(nbBand);
	stats.resize(nbBand);
	profiles.SetSize(nXSize);
	std::vector<double> noData(nbBand);

	for(int k=0; k<nbBand; k++)
	{
		poBandArray[k] = poInputDS->GetRasterBand( k+1 );//1 base
		if( poBandArray[k] )
		{
			int bSuccess = false;
			noData[k] = poBandArray[k]->GetNoDataValue(&bSuccess);
			if( !bSuccess )
				noData[k] = m_noDataIn;
		}
	}

	

	//for all profile
	for(int i=0; i<nYSize&&msg; i++)
	{
		//read onle line of all band
		for(int k=0; k<nbBand; k++)
		{
			CPLErr err = poBandArray[k]->RasterIO( GF_Read, 0, i, nXSize, 1, profiles.GetData(), nXSize, 1, GDT_Float32, 0, 0 );
			
			if( err != CE_None )
			{
				msg.ajoute(CPLGetLastErrorMsg());
				return msg;
			}
		
		
			//for all cell in the profile
			for(int j=0; j<nXSize&&msg; j++)
			{
				if( profiles[j] > noData[k] )
					stats[k] += profiles[j];
			}

			msg += callback.StepIt();
		}
	}

	// Once we're done, close properly the dataset 
    GDALClose( (GDALDatasetH) poInputDS );

	return msg;
}

ERMsg CSpectralInterpol::LoadDataTable(CKNearestNeighbor& dataTable, bool bShowInfo, CSCCallBack& callback)const
{
	ERMsg msg;

	dataTable.EnableStadardization(m_bStandardized);
	if( UtilWin::GetFileExtension(m_trainingSetFilePath).CompareNoCase(".shp") == 0)
		msg = dataTable.LoadShapefile(m_trainingSetFilePath, m_inputImageFilePath, callback);
	else msg = dataTable.LoadCSV(m_trainingSetFilePath);

	if( msg && m_bStandardized && m_bStandardizedFromImage )
	{
		CFL::CStatisticVector stats;
		msg = GetStatistic(stats, callback);
		if( msg )
			dataTable.SetStatistic(stats);//force to use stat from input image intead
	}

	if( msg && m_bStandardized && bShowInfo)
	{
		const CFL::CStatisticVector& stats = dataTable.GetStatistic();
		callback.AddMessage(GetString(IDS_BAND_DESCRIPTION), 1);
		for(size_t i=0; i<stats.size(); i++)
		{
			CString tmp;
			tmp.Format("%d\t\t%.5lf\t\t%.5lf", i+1, stats[i][CFL::MEAN], stats[i][CFL::STD_DEV]);
			callback.AddMessage(tmp, 1);
		}

		callback.AddMessage("");
	}

	if( msg)
		dataTable.Init();

	return msg;
}


double GetKappa(const C2DimArray<int>& kappa)
{
	ASSERT( kappa.GetXSize() == kappa.GetYSize());

	double diagonal=0;
	double sum=0;
	std::vector<int> X(kappa.GetXSize());
	std::vector<int> Y(kappa.GetYSize());

	for(int y=0; y<kappa.GetYSize(); y++)
	{
		for(int x=0; x<kappa.GetXSize(); x++)
		{
			if( x==y)
				diagonal+=kappa[y][x];

			X[x] += kappa[y][x];
			Y[y] += kappa[y][x];
			sum += kappa[y][x];
		}
	}


	ASSERT( sum > 0);
	double prA = diagonal/sum;
	double prE = 0;
	for(int i=0; i<kappa.GetYSize(); i++)
		prE += X[i]*Y[i]/(sum*sum);


	return (prA-prE)/(1-prE);
}

ERMsg CSpectralInterpol::XValidation( CSCCallBack& callback)const
{
	ERMsg msg;
	
	CFL::Randomize(FIXED_SEED);//fixed seed randomization for selecting best index


	CString filePathOut2(m_outputImageFilePath);
	UtilWin::SetFileExtension( filePathOut2, ".csv");
	CStdioFileEx file;
	msg += file.Open( filePathOut2, CFile::modeWrite|CFile::modeCreate);


	CKNearestNeighbor dataTable;
	msg += LoadDataTable(dataTable, true, callback);
	
	if(!msg)
		return msg;

	int nbDimension = dataTable.GetNbDimension();
	
	callback.SetCurrentDescription(GetString(IDS_XVALIDATION_DESCRIPTION));
	callback.SetCurrentStepRange(0, dataTable.GetSize(), 1);
	callback.SetStartNewStep();

	CString tmp="pt";
	if( m_power!=FREQUENCY)
	{
		for(int k=0; k<m_nbPoints; k++)
			tmp+=",Distance"+ToString(k+1)+",Weight"+ToString(k+1)+",Value"+ToString(k+1);
	}

	tmp+=",Observed,Predicted";

	//callback.AddMessage(tmp,1);
	file.WriteString(tmp+"\n");

	int nbClass = (m_power==FREQUENCY)?dataTable.GetNbClass():0;

	CFL::CStatisticXYEx stat;
	C2DimArray<int> kappa;
	kappa.SetSize(nbClass, nbClass);
	for(int x=0; x<kappa.GetXSize(); x++)	
		for(int y=0; y<kappa.GetYSize(); y++)
			kappa[y][x] = 0;

	//for all point in the table
	for(int i=0; i<dataTable.GetSize()&&msg; i++)
	{
		CMDPoint pt(dataTable[i]);

		//look for the nearest point
		CSearchResultArray result;
		msg = dataTable.Search(pt, m_nbPoints+1, result);
		if( !msg)
			return msg;

		ASSERT(result[0].m_distance == 0);
		result.RemoveAt(0);

		CString tmp=ToString(i+1);

		//compute the mean
		double bio=0;
		if( m_power==FREQUENCY)
		{
			//sorting by value
			CSortedKeyIndexLArray indexs;
			for(int k=0; k<result.GetSize(); k++)
			{
				//CSortedIndexL e(0, result[k].m_index);
				int v = int(dataTable.GetBio(result[k].m_index)+0.5);
				int pos = (int)indexs.LookupKey(v);
				if(pos == -1)
					indexs.AddSorted(CSortedKeyIndexL(v, 1));
				else indexs[pos].m_index++;
			}
			ASSERT( indexs.GetSize() > 0);

			//now sorting by occurence
			CSortedKeyIndexLArray indexs2;
			for(int k=0; k<indexs.GetSize(); k++)
				indexs2.AddSorted( CSortedKeyIndexL(-indexs[k].m_index, indexs[k].m_key) );
			
			int nbMaxValue = 1;
			for(int k=1; k<indexs2.GetSize(); k++)
				if( indexs2[k].m_key == indexs2[0].m_key )
					nbMaxValue++;


			int bestIndex = CFL::Rand(0, nbMaxValue-1);
			bio = indexs2[bestIndex].m_index;

			int iR = dataTable.GetBioIndex(int(dataTable.GetBio(i)+0.5));
			int iC = dataTable.GetBioIndex(int(bio));
			kappa[iR][iC]++;
		}
		else
		{
			double xSum=result.GetXSum(m_power);
			for(int k=0; k<result.GetSize(); k++)
			{
				double weight = result[k].GetXTemp(m_power)/xSum; 
				bio+=dataTable.GetBio(result[k].m_index)*weight;
				
				tmp+=","+ToString(result[k].m_distance,5)+","+ToString(weight,5)+","+ToString(dataTable.GetBio(result[k].m_index),5);
			}
		}

		tmp+=","+ToString(dataTable.GetBio(i),5);
		tmp+=","+ToString(bio,5);
		file.WriteString(tmp+"\n");
		//callback.AddMessage(tmp,1);

		stat.Add(bio, dataTable.GetBio(i));
		msg += callback.StepIt();
	}

	
	callback.AddMessage("");

	if( m_power==FREQUENCY)
	{
		CString tmp = "Observed\\Predicted";
		for(int x=0; x<kappa.GetXSize(); x++)
			tmp += "," + ToString(dataTable.GetBioValueFromIndex(x));
		file.WriteString(tmp+"\n");
		//callback.AddMessage( tmp,1 );
		for(int y=0; y<kappa.GetYSize(); y++)
		{
			tmp = ToString(dataTable.GetBioValueFromIndex(y));
			for(int x=0; x<kappa.GetXSize(); x++)
			{
				tmp += "," + ToString(kappa[y][x]);
			}
			file.WriteString(tmp+"\n");
		}
		
		
		double k = GetKappa(kappa);
		file.WriteString("kappa = " + ToString(k) + "\n");


		
		//callback.AddMessage( "",1 );
		//callback.AddMessage( "kappa = " + ToString(k),1 );
		
	}
	else
	{
		tmp.Format( "Bias,MAD,RMSE,RMSE%%,R²(det),R²(corr)\n%.2lf,%.2lf,%.2lf,%.2lf,%.3lf,%.3lf\n", stat[CFL::BIAS], stat[CFL::MAE], stat[CFL::RMSE], stat[CFL::RMSE]/stat.GetY()[CFL::MEAN]*100, stat[CFL::COEF_D], stat[CFL::STAT_R²]);
		file.WriteString(tmp);
		//file.WriteString("Observed,Predicted\n");
		//for(int i=0; i<(int)stat.x().size(); i++)
		//{
		//	tmp.Format("%.3lf,%.3lf\n", stat.y()[i], stat.x()[i]);
		//	file.WriteString(tmp);
		//}

		callback.AddMessage("RMSE = " + ToString( stat[CFL::RMSE], 2 ) + " (" + ToString( stat[CFL::RMSE]/stat.GetY()[CFL::MEAN]*100, 2 ) + "%)",1 );
		callback.AddMessage("Bias = " + ToString( stat[CFL::BIAS], 2 ) + " (" + ToString( stat[CFL::BIAS]/stat.GetY()[CFL::MEAN]*100, 2 ) + "%)",1 );
	
	}

	file.Close();
	
	
	

	return msg;
}

//bool CSpectralInterpol::LoadMask()const
//{
//	//m_mask = mask;
//	CSpectralInterpol& me = const_cast<CSpectralInterpol&>(*this);
//	me.m_maskArray.RemoveAll();
//
//	int pos = 0;
//	CString tmp = m_mask.Tokenize(" ,;\t|", pos); 
//	while( pos != -1)
//	{
//		me.m_maskArray.AddSorted( atoi(tmp) );
//		tmp = m_mask.Tokenize(" ,;\t|", pos);
//	}
//
//	return !m_maskArray.IsEmpty();
//}
//
