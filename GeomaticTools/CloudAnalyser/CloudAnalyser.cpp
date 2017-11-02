//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 1.0.0	31/10/2017	Rémi Saint-Amant	Creation

//-multi -ot Int16 -co "compress=LZW" -stats -overview {2,4,8,16} -overwrite "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\Model\DTv4" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\input\lansat.tif" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\input\physics.tif" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\Output\zone2.tif"

//Et un CODE DT avec T1 T2 T3 ici(1 = feu, 2 = coupe, 112 et 113 ombre et nuage).C’est vraiment rough comme DT mais l’arbre sera amélioré, dans les prochaine semaines.


#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <bitset>
#include <boost/dynamic_bitset.hpp>

#include "CloudAnalyser.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Basic/OpenMP.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/See5hooks.h"
#include "Geomatic/LandsatDataset.h"



#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF;
using namespace WBSF::Landsat;

 

static const char* version = "1.0.0";
static const int NB_THREAD_PROCESS = 2; 
static const int NOT_TRIGGED_CODE = 100;


std::string CCloudAnalyser::GetDescription()
{ 
	return  std::string("CloudAnalyser version ") + version + " (" + _T(__DATE__) + ")\n"; 
}


CCloudAnalyserOption::CCloudAnalyserOption()
{
	m_nbPixel = 0;
	m_nbPixelDT = 0;
	m_scenesSize = Landsat::SCENES_SIZE;

	m_appDescription = "This software look up for cloud from Landsat images series (composed of " + to_string(SCENES_SIZE) + " bands) with a decision tree model";
	
	static const COptionDef OPTIONS[] =
	{
		{ "-B1", 1, "threshold", true, "trigger threshold for band 1 to execute decision tree. -175 by default." },
		{ "-TCB", 1, "threshold", true, "trigger threshold for Tassel Cap Brightness (TCB) to execute decision tree. 600 by default." },
		//{ "-Debug",0,"",false,"Output debug information."},
		{ "DTModel", 0, "", false, "Decision tree cloud model file path." },
		{ "srcfile", 0, "", false, "LANDSAT scenes image file path." },
		{ "dstfile", 0, "", false, "Output image file path." }
	};

	for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
		AddOption(OPTIONS[i]);

	static const CIOFileInfoDef IO_FILE_INFO[] =
	{
		{ "Input Model", "DTModel", "", "", "", "Decision tree model file generate by See5." },
		{ "LANDSAT Image", "src1file", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|B9: Date(Julian day 1970 or YYYYMMDD format) and cloud mask(NoData)|... for each scene" },
		{ "Output Image", "dstfile", "One file per perturbation", "6", "FirstDate: date of the first image analysed|DTCode: disturbance code|D1: disturbace date of the first image|D2: disturbance date of the second image|NbContirm: number of image without disturbance after the disturbance|LastDate: date of the last image analysed" },
		//			{ "Optional Output Image", "dstfile_debug","1","9"," NbPairs: number of \"Pair\", a pair is composed of 2 valid images|NbDisturbances: number of disturbances found in the series.|Disturbance: last diturbance|D1: date of the first image of the last disturbance|D2: date of the second image of the last disturbance|MeanD: mean NBR distance|MaxD1: highest NBR distance|MaxD2: second highest NBR distance|MaxD3: third highest NBR distance"}
	};

	for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
		AddIOFileInfo(IO_FILE_INFO[i]);

}

CDecisionTreeBlock CCloudAnalyser::GetDataRecord(array<CLandsatPixel, 3> p, CDecisionTreeBaseEx& DT)
{
	CDecisionTreeBlock block(DT.MaxAtt + 1);

	//fill the data structure for decision tree
	size_t c = 0;
	DVal(block, c++) = DT.MaxClass + 1;
	DVal(block, c++) = Continuous(DT, 1) ? DT_UNKNOWN : 0;
	//for (size_t i = 0; i < m_physical.size(); i++)
		//CVal(block, c++) = (ContValue)m_physical[i];

	for (size_t z = 0; z < 3; z++)
	{
		for (size_t j = 0; j < Landsat::SCENES_SIZE; j++)
			CVal(block, c++) = (ContValue)(p[z].at(j));
	}//for


	ASSERT(c == (3 * 9 + 2));
	//fill virtual bands 
	for (; c <= DT.MaxAtt; c++)
	{
		ASSERT(DT.AttDef[c]);
		//assuming virtual band never return no data
		block[c] = DT.EvaluateDef(DT.AttDef[c], block.data());
	}

	return block;
}

//***********************************************************************

ERMsg CCloudAnalyser::ReadRules(CDecisionTree& DT)
{
	ERMsg msg;
	if(!m_options.m_bQuiet) 
	{
		cout << "Read rules..."<<endl;
	}

	CTimer timer(true);

	msg += DT.Load(m_options.m_filesPath[CCloudAnalyserOption::DT_FILE_PATH], m_options.m_CPU, m_options.m_IOCPU);
	timer.Stop();

	if( !m_options.m_bQuiet )
		cout << "Read rules time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;


	return msg;
}	


ERMsg CCloudAnalyser::OpenAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, vector<CGDALDatasetEx>& outputDS)
{
	ERMsg msg;
	
	
	if(!m_options.m_bQuiet) 
		cout << endl << "Open input image..." << endl;

	msg = landsatDS.OpenInputImage(m_options.m_filesPath[CCloudAnalyserOption::LANDSAT_FILE_PATH], m_options);

	if(msg)
	{
		landsatDS.UpdateOption(m_options);
			
		if(!m_options.m_bQuiet) 
		{
			CGeoExtents extents = landsatDS.GetExtents();
			CProjectionPtr pPrj = landsatDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << landsatDS->GetRasterXSize() << " cols x " << landsatDS->GetRasterYSize() << " rows x " << landsatDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << landsatDS.GetRasterCount() << endl;
			cout << "    Scene size     = " << landsatDS.GetSceneSize() << endl;
			cout << "    Nb. Scenes     = " << landsatDS.GetNbScenes() << endl;
			cout << "    First image    = " << landsatDS.GetPeriod().Begin().GetFormatedString() << endl;
			cout << "    Last image     = " << landsatDS.GetPeriod().End().GetFormatedString() << endl;
			cout << "    Input period   = " << m_options.m_period.GetFormatedString() << endl;

			if (landsatDS.GetNbScenes() <= 1)
				msg.ajoute("CloudAnalyser can't be performed over only one scene");
		}
	}


	if( msg && !m_options.m_maskName.empty() )
	{
		if(!m_options.m_bQuiet) 
			cout << "Open mask image..." << endl;
		msg += maskDS.OpenInputImage( m_options.m_maskName );
	}

	if(msg && m_options.m_bCreateImage)
	{

		if(!m_options.m_bQuiet) 
			cout << "Create output image..." << endl;

		CCloudAnalyserOption option = m_options;
		option.m_nbBands = landsatDS.GetNbScenes();

		outputDS.resize(option.m_nbBands);
		for(size_t i=0; i<outputDS.size()&&msg; i++)
		{
			string filePath = option.m_filesPath[CCloudAnalyserOption::OUTPUT_FILE_PATH];
			if (i>0)
				SetFileTitle(filePath, GetFileTitle(filePath) + ToString(i + 1));
			
			msg += outputDS[i].CreateImage(filePath, option);
		}
	}
	

	//open exportStats files
	//if (msg && m_options.m_bFireSeverity)
	//{
	//	if(!m_options.m_bQuiet) 
	//		_tprintf("Create fire severity image...\n");

	//	CCloudAnalyserOption options = m_options;
	//	options.m_nbBands = NB_FIRE_SEVERITY;
	//	options.m_outputType = GDT_Float32;
	//	options.m_dstNodata=::GetDefaultNoData(GDT_Int32);
	//	string exportPath = options.m_filesPath[OUTPUT_FILE_PATH];
	//	SetFileTitle(exportPath, GetFileTitle(exportPath) + "_fireSeverity");
	//	for (size_t m = 0; m < NB_FIRE_SEVERITY; m++)
	//		options.m_VRTBandsName += GetFileTitle(exportPath) + "_" + FIRE_SEVERITY_MODEL_NAME[m] + ".tif|";

	//	msg += fireSeverityDS.CreateImage(exportPath, options);
	//}
	//
	//if( msg && m_options.m_bExportBands )
	//{
	//	if(!m_options.m_bQuiet) 
	//		cout << "Create export bands images..." << endl;
	//		

	//	CCloudAnalyserOption options = m_options;
	//	options.m_nbBands = NB_EXPORT_BANDS*NB_EXPORT_TEMPORAL;//T-2 à T+3
	//	options.m_outputType = GDT_Int16;
	//	options.m_dstNodata=::GetDefaultNoData(GDT_Int16);

	//	exportBandsDS.resize(options.m_nbDisturbances);
	//	for(size_t i=0; i<exportBandsDS.size(); i++)
	//	{
	//		string exportPath = options.m_filesPath[OUTPUT_FILE_PATH];
	//		if (i>0)
	//			SetFileTitle(exportPath, GetFileTitle(exportPath) + ToString(i + 1));

	//		SetFileTitle(exportPath, GetFileTitle(exportPath) +"_exportBands");

	//		assert(NB_EXPORT_BANDS == SCENES_SIZE);
	//		for (size_t s = 0; s < NB_EXPORT_TEMPORAL; s++)
	//		{
	//			
	//			for (size_t b = 0; b < SCENES_SIZE; b++)
	//				options.m_VRTBandsName += GetFileTitle(exportPath) + string("_T") + FormatA("%+d", s - 2) + string("_") + CLandsatDataset::SCENE_NAME[b] + ".tif|";
	//		}

	//		msg += exportBandsDS[i].CreateImage(exportPath, options);
	//	}
	//}
	//
	//if (msg && m_options.m_bExportTimeSeries)
	//{
	//	if (!m_options.m_bQuiet)
	//		cout << "Create export time series images..." << endl;


	//	ASSERT(landsatDS.GetNbScenes() == landsatDS.GetPeriod().GetNbYears());
	//	CCloudAnalyserOption options = m_options;
	//	options.m_nbBands = landsatDS.GetNbScenes() - 1;
	//	options.m_outputType = GDT_Byte;
	//	options.m_dstNodata = ::GetDefaultNoData(GDT_Byte);
	//	
	//	string exportPath = options.m_filesPath[OUTPUT_FILE_PATH];
	//	SetFileTitle(exportPath, GetFileTitle(exportPath) + "_timeSeries");
	//	
	//	for (size_t y = 0; y < landsatDS.GetScenePeriod().size()-1; y++)
	//	{
	//		ASSERT(landsatDS.GetScenePeriod().at(y).Begin().GetYear() == landsatDS.GetScenePeriod().at(y).End().GetYear());
	//		int year1 = landsatDS.GetScenePeriod().at(y).Begin().GetYear();
	//		//int year2 = landsatDS.GetScenePeriod().at(y+1).Begin().GetYear();
	//		options.m_VRTBandsName += GetFileTitle(exportPath) + string("_") + FormatA("%d", year1) + ".tif|";
	//	}

	//	msg += exportTSDS.CreateImage(exportPath, options);
	//}

	//

	//
	//if( msg && m_options.m_bDebug )
	//{
	//	if(!m_options.m_bQuiet) 
	//		cout << "Create debug image..." << endl;

	//	CCloudAnalyserOption options = m_options;
	//	options.m_nbBands = NB_DEBUGS;
	//	options.m_outputType = GDT_Int32;
	//	options.m_dstNodata= ::GetDefaultNoData(GDT_Int32);
	//	string filePath = options.m_filesPath[OUTPUT_FILE_PATH];
	//	SetFileTitle(filePath, GetFileTitle(filePath) + "_debug");
	//	for (size_t d = 0; d < NB_DEBUGS; d++)
	//	{
	//		options.m_VRTBandsName += GetFileTitle(filePath) + "_" + DEBUG_NAME[d] + ".tif|";
	//	}

	//	msg += debugDS.CreateImage(filePath, options);
	//}

	return msg;
}



ERMsg CCloudAnalyser::Execute()
{
	ERMsg msg;

	if( !m_options.m_bQuiet )		
	{
		cout << "Output: " << m_options.m_filesPath[CCloudAnalyserOption::OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[CCloudAnalyserOption::LANDSAT_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[CCloudAnalyserOption::DT_FILE_PATH] << endl;
		
		//for(size_t m=0; m<m_options.m_optionalDT.size(); m++)
			//cout << "        " << m_options.m_optionalDT[m].m_filePath << endl;

		if( !m_options.m_maskName.empty() )
			cout << "Mask:   "  << m_options.m_maskName << endl;
	}

	GDALAllRegister();

	if( m_options.m_outputType != GDT_Unknown && m_options.m_outputType != GDT_Int16 && m_options.m_outputType != GDT_Int32)
		msg.ajoute("Invalid -ot option. Only GDT_Int16 or GDT_Int32 are supported");
	
	if(!msg)
		return msg;


	CDecisionTree DT;
	msg += ReadRules(DT);
	if (!msg)
		return msg;
	
	CLandsatDataset lansatDS;
	CGDALDatasetEx physicalDS;
	CGDALDatasetEx maskDS;

	vector<CGDALDatasetEx> outputDS;
	//CGDALDatasetEx debugDS;

	msg = OpenAll(lansatDS, maskDS, outputDS);
	
	if( msg)
	{
		size_t nbScenes = lansatDS.GetNbScenes();
		size_t sceneSize = lansatDS.GetSceneSize();
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

		if( maskDS.IsOpen() )
			bandHolder.SetMask( maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed );

		msg += bandHolder.Load(lansatDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if(!msg)
			return msg;


		if( !m_options.m_bQuiet && m_options.m_bCreateImage) 
			cout << "Create output images " << " x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << m_options.m_nbBands << " years) with " << m_options.m_CPU << " threads..." << endl;


		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar(extents.m_xSize*extents.m_ySize);

		vector<pair<int,int>> XYindex = extents.GetBlockList();
		
		omp_set_nested(1);
		#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
		for(int b=0; b<(int)XYindex.size(); b++)
		{
			int threadBlockNo = omp_get_thread_num();
			int xBlock=XYindex[b].first;
			int yBlock=XYindex[b].second;

			//data
			vector< vector< CLandsatPixelVector >> data;
			vector<vector<vector<short>>> output;
			ReadBlock(xBlock, yBlock, bandHolder[threadBlockNo]);
			ProcessBlock(xBlock, yBlock, bandHolder[threadBlockNo], DT, data, output);
			WriteBlock(xBlock, yBlock, bandHolder[threadBlockNo], output, outputDS);// debugDS
		}//for all blocks


		//  Close decision tree and free allocated memory  
		DT.FreeMemory();

		//close inputs and outputs
		CloseAll(lansatDS, maskDS, outputDS);

	}
	
    return msg;
}


void CCloudAnalyser::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
{
	#pragma omp critical(BlockIO)
	{
	
		m_options.m_timerRead.Start();
	
		bandHolder.LoadBlock(xBlock, yBlock);

		m_options.m_timerRead.Stop();
	}
}

size_t GetNext(CLandsatPixelVector& landsat, size_t z)
{
	size_t next=NOT_INIT;
	for (size_t zz = z; zz < landsat.size() && next == NOT_INIT; zz++)
		if (landsat[zz].IsValid())
			next = zz;

	return next;
}

//Get input image reference
void CCloudAnalyser::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, CDecisionTree& DT, vector< vector< CLandsatPixelVector >>& data, vector<vector<vector<short>>>& output)
{
	//CGDALDatasetEx& inputDS, 

	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
	int nbCells = extents.m_xSize*extents.m_ySize;

	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += (std::min(nbCells, blockSize.m_x*blockSize.m_y));
		m_options.UpdateBar();

		return;
	}


#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();

		//allocate process memory
		LoadData(bandHolder, data);

		CLandsatPixel NO_PIXEL;
		//process all x and y 
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)  
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				int thread = ::omp_get_thread_num();

				if (!data[x][y].empty())
				{
					array <CLandsatPixel, 3> p;
					size_t z2 = 0;
					p[1] = data[x][y][z2];
					//process all scenes 
					while ( z2 < data[x][y].size())
					{
#pragma omp atomic
						m_options.m_nbPixel++;

						size_t z3 = GetNext(data[x][y], z2 + 1);
						p[2] = z3<data[x][y].size() ? data[x][y][z3] : NO_PIXEL;

						if (m_options.IsTrigged(p))
						{
#pragma omp atomic
							m_options.m_nbPixelDT++;

							vector <AttValue> block = GetDataRecord(p, DT[thread]);
							ASSERT(!block.empty());
							int predict = (int)DT[thread].Classify(block.data());
							ASSERT(predict >= 1 && predict <= DT[threadNo].MaxClass);
							int DTCode = atoi(DT[thread].ClassName[predict]);
							output[x][y][z2] = DTCode;

						}
						else //if not trigger
						{
							output[x][y][z2] = NOT_TRIGGED_CODE;
						}

						p[0] = p[1];
						p[1] = p[2];
					}//for z2
				}
#pragma omp atomic	
				m_options.m_xx++;
			}//for x


			m_options.UpdateBar();
		}//for y



		m_options.m_timerProcess.Stop();
	}
}

void CCloudAnalyser::WriteBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, const vector<vector<vector<short>>>& data, vector<CGDALDatasetEx>& outputDS)//CGDALDatasetEx& debugDS
{
	if( data.empty() )
		return;

#pragma omp critical(BlockIO)
	{

		m_options.m_timerWrite.Start();


		size_t nbScenes = data[0][0].size();
		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
		assert(bandHolder.GetNbScenes() == bandHolder.GetPeriod().GetNbYears());


		ASSERT(outputRect.Width() == blockSize.m_x);
		ASSERT(outputRect.Height() == blockSize.m_y);

		if (m_options.m_bCreateImage)
		{
			__int32 noDataOut = (__int32)outputDS[0].GetNoData(0);
			for (size_t i = 0; i < outputDS.size(); i++)
			{
				ASSERT(data.size() == blockSize.m_x);
				ASSERT(data[0].size() == blockSize.m_y);
				//ASSERT(outputDS[i].GetRasterCount() == NB_OUTPUT_BANDS);

				for (size_t b = 0; b < outputDS[i].GetRasterCount(); b++)
				{
					GDALRasterBand *pBand = outputDS[i].GetRasterBand(b);

					vector<__int32> output(blockSize.m_x*blockSize.m_y, noDataOut);
					for (int y = 0; y < blockSize.m_y; y++)
					{
						for (int x = 0; x < blockSize.m_x; x++)
						{
							int xy = int(y*blockSize.m_x + x);
							output[xy] = data[x][y][b];
						}//x
					}//y

					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Int32, 0, 0);
					//pBand->FlushCache();
				}//for all output bands
			}//for all disturbance
		}//if create image


		//if (m_options.m_bFireSeverity)
		//{
		//	__int16 noDataIn = (__int16)::GetDefaultNoData(GDT_Int16);
		//	float noDataOut = (float)::GetDefaultNoData(GDT_Int32);


		//	for (size_t b = 0; b < NB_FIRE_SEVERITY; b++)
		//	{
		//		GDALRasterBand *pBand = fireSeverityDS.GetRasterBand(b);
		//		vector<float> output(blockSize.m_x*blockSize.m_y, noDataOut);

		//		for (int y = 0; y < blockSize.m_y; y++)
		//		{
		//			for (int x = 0; x < blockSize.m_x; x++)
		//			{
		//				if (data[x][y].IsInit())
		//				{
		//					for (size_t z = 0; z < data[x][y].GetNbDTCode(); z++)
		//					{

		//						if (data[x][y][z].IsInit() &&
		//							data[x][y].GetDTCode(z) == FIRE_CODE &&
		//							data[x][y][z][E_B4] != noDataIn &&
		//							data[x][y][z][E_B7] != noDataIn &&
		//							data[x][y][z + 1][E_B4] != noDataIn &&
		//							data[x][y][z + 1][E_B7] != noDataIn)
		//						{
		//							static const double _a_[2] = { 0.22, 0.311 };
		//							static const double _b_[2] = { 0.09, 0.019 };

		//							double NBR1 = data[x][y][z].NBR();
		//							double NBR2 = data[x][y][z + 1].NBR();
		//							double dNBR = NBR1 - NBR2;

		//							//Genreal Saturated Growth
		//							double CBI_Ron = dNBR / (_a_[FS_RON] * dNBR + _b_[FS_RON]);
		//							double CBI_Jo = dNBR / (_a_[FS_JO] * dNBR + _b_[FS_JO]);

		//							double fireSeverity = 0;
		//							switch (b)
		//							{
		//							case FS_RON: fireSeverity = CBI_Ron; break;
		//							case FS_JO:	fireSeverity = CBI_Jo; break;
		//							case FS_MEAN:	fireSeverity = (CBI_Ron + CBI_Jo) / 2; break;
		//							default: ASSERT(false);
		//							}

		//							output[y*blockSize.m_x + x] = (float)fireSeverity;
		//						}//il all is presnet????
		//					}//for all disterbance
		//				}//if disturbance
		//			}//x
		//		}//y

		//		pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
		//		//pBand->FlushCache();
		//	}//all model (3)

		//}//stats

		//if (m_options.m_bExportBands)
		//{
		//	__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);
		//	for (size_t i = 0; i < exportBandsDS.size(); i++)
		//	{
		//		ASSERT(exportBandsDS[i].GetRasterCount() == NB_EXPORT_BANDS*NB_EXPORT_TEMPORAL);

		//		for (size_t t = 0; t < NB_EXPORT_TEMPORAL; t++)
		//		{
		//			for (size_t b = 0; b < NB_EXPORT_BANDS; b++)
		//			{
		//				GDALRasterBand *pBand = exportBandsDS[i].GetRasterBand(t*NB_EXPORT_BANDS + b);

		//				vector<__int16> exportBands(blockSize.m_x*blockSize.m_y, noData);
		//				for (int y = 0; y < blockSize.m_y; y++)
		//				{
		//					for (int x = 0; x < blockSize.m_x; x++)
		//					{
		//						size_t z = data[x][y].GetDisturbanceIndex(i);
		//						if (z < data[x][y].size())
		//						{
		//							int tt = int(t) - 2;
		//							size_t Tindex = data[x][y].GetIndex(z, tt);

		//							if (Tindex < data[x][y].size())
		//							{
		//								exportBands[y*blockSize.m_x + x] = data[x][y][Tindex][EXPORT_BANDS[b]];
		//							}
		//						}
		//					}
		//				}

		//				pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(exportBands[0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
		//				//pBand->FlushCache();
		//			}//for all bands in a scene
		//		}//for T1 and T2
		//	}//for all disturbance
		//}//export bands


		//if (m_options.m_bExportTimeSeries)
		//{
		//	char noDataOut = (char)::GetDefaultNoData(GDT_Byte);

		//	for (size_t b = 0; b < exportTSDS.GetRasterCount(); b++)
		//	{
		//		GDALRasterBand *pBand = exportTSDS.GetRasterBand(b);
		//		int year = bandHolder.GetPeriod().Begin().GetYear() + int(b);

		//		vector<byte> output(blockSize.m_x*blockSize.m_y, noDataOut);
		//		for (int y = 0; y < blockSize.m_y; y++)
		//		{
		//			for (int x = 0; x < blockSize.m_x; x++)
		//			{
		//				size_t z = data[x][y].FindYear(year);
		//				if (z != NOT_INIT)
		//				{
		//					int xy = y*blockSize.m_x + x;
		//					output[xy] = (char)data[x][y].GetDTCode(z);
		//					if (output[xy] == 0)
		//						output[xy] = noDataOut;
		//				}
		//			}//x
		//		}//y

		//		pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Byte, 0, 0);
		//	}//for all debug variable
		//}//time series

		//if( m_options.m_bDebug )
		//{
		//	__int32 noDataOut = (__int32)::GetDefaultNoData(GDT_Int32);

		//	for(size_t b=0; b<NB_DEBUGS; b++)
		//	{
		//		GDALRasterBand *pBand = debugDS.GetRasterBand(b);

		//		vector<__int32> output(blockSize.m_x*blockSize.m_y, noDataOut);
		//		for(int y=0; y<blockSize.m_y; y++)
		//		{
		//			for(int x=0; x<blockSize.m_x; x++)
		//			{
		//				if( data[x][y].IsInit() )
		//				{
		//					size_t z° = data[x][y].GetFirstDisturbanceIndex();
		//					size_t zⁿ = data[x][y].GetLastDisturbanceIndex();
		//					int xy = y*blockSize.m_x+x;
		//					switch(b)
		//					{
		//					case D_NB_PAIRS:			output[xy] = (int)data[x][y].size(); break;
		//					case D_NB_DISTURBANCES:		output[xy] = (int)data[x][y].GetNbDisturbances(); break;
		//					case D_FIRST_DISTURBANCE:	if (z° != UNKNOWN_POS) output[xy] = data[x][y].GetDTCode(z°); break;
		//					case D_F_DATE1:				if (z° != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][z°].GetTRef()); break;
		//					case D_F_DATE2:				if (z° != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][z° + 1].GetTRef()); break;
		//					case D_LAST_DISTURBANCE:	if (zⁿ != UNKNOWN_POS) output[xy] = data[x][y].GetDTCode(zⁿ); break;
		//					case D_L_DATE1:				if (zⁿ != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][zⁿ].GetTRef()); break;
		//					case D_L_DATE2:				if (zⁿ != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][zⁿ + 1].GetTRef()); break;
		//					//case D_MEAN_D:				output[xy] = (int)(data[x][y].GetMeanDistance()*m_options.m_NBRFactor); break;
		//					//case D_MAX1:				output[xy] = (int)(data[x][y].GetMaxDistance(0)*m_options.m_NBRFactor); break;
		//					//case D_MAX2:				output[xy] = (int)(data[x][y].GetMaxDistance(1)*m_options.m_NBRFactor); break;
		//					//case D_MAX3:				output[xy] = (int)(data[x][y].GetMaxDistance(2)*m_options.m_NBRFactor); break;
		//					default: ASSERT(FALSE);
		//					}
		//				}
		//			}//x
		//		}//y
		//		
		//		pBand->RasterIO( GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Int32, 0, 0  );
		//		//pBand->FlushCache();
		//	}//for all debug variable
		//}//debug
	}
	
	m_options.m_timerWrite.Stop(); 
	
}

void CCloudAnalyser::CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, vector<CGDALDatasetEx>& outputDS)
{
	if( !m_options.m_bQuiet )		
		_tprintf("\nClose all files...\n");

	landsatDS.Close();
	maskDS.Close(); 

	//close output
	m_options.m_timerWrite.Start();
	for(size_t i=0; i<outputDS.size(); i++)
	{
		if( m_options.m_bComputeStats )
			outputDS[i].ComputeStats(i==0?m_options.m_bQuiet:true);
		if( !m_options.m_overviewLevels.empty() )
			outputDS[i].BuildOverviews(m_options.m_overviewLevels, i==0?m_options.m_bQuiet:true);
		outputDS[i].Close();
	}

	/*if( m_options.m_bComputeStats )
		fireSeverityDS.ComputeStats(true);
	if( !m_options.m_overviewLevels.empty() )
		fireSeverityDS.BuildOverviews(m_options.m_overviewLevels, true);
	fireSeverityDS.Close();

	for(size_t i=0; i<exportBandsDS.size(); i++)
	{
		if( m_options.m_bComputeStats )
			exportBandsDS[i].ComputeStats(true); 
		if( !m_options.m_overviewLevels.empty() )
			exportBandsDS[i].BuildOverviews(m_options.m_overviewLevels, true);
		exportBandsDS[i].Close();
	}


	if (m_options.m_bComputeStats)
		exportTSDS.ComputeStats(true);
	if (!m_options.m_overviewLevels.empty())
		exportTSDS.BuildOverviews(m_options.m_overviewLevels, true);
	exportTSDS.Close();

	
	if (m_options.m_bComputeStats)
		debugDS.ComputeStats(true);
	if (!m_options.m_overviewLevels.empty())
		debugDS.BuildOverviews(m_options.m_overviewLevels, true);
	debugDS.Close();
	*/

		
	m_options.m_timerWrite.Stop();
		
	if( !m_options.m_bQuiet )
	{
		double percent = m_options.m_nbPixel>0?(double)m_options.m_nbPixelDT/m_options.m_nbPixel*100:0;

		_tprintf ("\n");
		_tprintf ("Percentage of pixel treated by DecisionTree: %0.3lf %%\n\n", percent);
	}

	m_options.PrintTime();
}

void CCloudAnalyser::LoadData(const CBandsHolder& bandHolder, vector< vector< CLandsatPixelVector>>& data)
{
	CRasterWindow landsat = bandHolder.GetWindow();

	CGeoSize blockSize = landsat.GetGeoSize();
	data.resize(blockSize.m_x);
	for (int x = 0; x<blockSize.m_x; x++)
	{
		data[x].resize(blockSize.m_y);
		for (int y = 0; y < blockSize.m_y; y++)
		{
			size_t nbScenes = landsat.GetNbScenes();
			data[x][y].resize(nbScenes);
			for (size_t z = 0; z < landsat.GetNbScenes(); z++)
			{
				data[x][y][z] = ((CLandsatWindow&)landsat).GetPixel(z, x, y);
			}
		}
	}
}
//
//
//int _tmain(int argc, _TCHAR* argv[])
//{
//	CTimer timer(true);
//	
//	CCloudAnalyser regressionTree;
//	ERMsg msg = regressionTree.m_options.ParseOption(argc, argv);
//
//	if( !msg || !regressionTree.m_options.m_bQuiet )
//		cout << regressionTree.GetDescription() << endl ;
//
//
//	if( msg )  
//		msg = regressionTree.Execute();
//
//	if( !msg)  
//	{
//		PrintMessage(msg);
//		return -1;
//	}
//
//	timer.Stop();
//
//	if( !regressionTree.m_options.m_bQuiet )
//		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;
//
//	int nRetCode = 0;
//}
//
//
