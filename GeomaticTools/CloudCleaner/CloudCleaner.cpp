//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 1.0.0	31/10/2017	Rémi Saint-Amant	Creation

//-co "compress=LZW" -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite  "D:\Travaux\CloudCleaner\Model\See5_Cloud_T123" "D:\Travaux\CloudCleaner\Input\Nuage.vrt" "D:\Travaux\CloudCleaner\Output\Output.vrt"

//Et un CODE DT avec T1 T2 T3 ici(1 = feu, 2 = coupe, 112 et 113 ombre et nuage).C’est vraiment rough comme DT mais l’arbre sera amélioré, dans les prochaine semaines.


#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <bitset>
#include <boost/dynamic_bitset.hpp>

#include "CloudCleaner.h"
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
static const short NOT_TRIGGED_CODE = (short)::GetDefaultNoData(GDT_Int16);
const char* CCloudCleanerOption::DEBUG_NAME[NB_DBUG] = { "ID" };

std::string CCloudCleaner::GetDescription()
{ 
	return  std::string("CloudCleaner version ") + version + " (" + _T(__DATE__) + ")\n"; 
}


CCloudCleanerOption::CCloudCleanerOption()
{
	m_nbPixel = 0;
	m_nbPixelDT = 0;
	m_scenesSize = Landsat::SCENES_SIZE;
	m_bDebug = false;
	m_bOutputDT = false;
	m_B1threshold = -175;
	m_TCBthreshold = 600;

	m_appDescription = "This software look up for cloud from Landsat images series (composed of " + to_string(SCENES_SIZE) + " bands) with a decision tree model";
	
	static const COptionDef OPTIONS[] =
	{
		{ "-B1", 1, "threshold", true, "trigger threshold for band 1 to execute decision tree. -175 by default." },
		{ "-TCB", 1, "threshold", true, "trigger threshold for Tassel Cap Brightness (TCB) to execute decision tree. 600 by default." },
		{ "-OutputDT", 0, "", false, "Output Decision Tree Code." },
		{ "-Debug",0,"",false,"Output debug information."},
		{ "DTModel", 0, "", false, "Decision tree cloud model file path." },
		{ "srcfile", 0, "", false, "Input LANDSAT scenes image file path." },
		{ "dstfile", 0, "", false, "Output LANDSAT scenes image file path." }
	};

	for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
		AddOption(OPTIONS[i]);

	static const CIOFileInfoDef IO_FILE_INFO[] =
	{
		{ "Input Model", "DTModel", "", "", "", "Decision tree model file generate by See5." },
		{ "LANDSAT Image", "src1file", "", "ScenesSize(9)*nbScenes", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Date(Julian day 1970)|... for each scene" },
		{ "Output Image", "dstfile", "One file per input landsat images", "1", "Decision tree result. 100 for DT not trigged" },
		{ "Optional Output Image", "dstfile_debug","1","nbScnenes","Decision tree result"}
	};

	for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
		AddIOFileInfo(IO_FILE_INFO[i]);

}

ERMsg CCloudCleanerOption::ProcessOption(int& i, int argc, char* argv[])
{
	ERMsg msg;

	if (IsEqual(argv[i], "-B1"))
	{
		m_B1threshold = atof(argv[++i]);
	}
	else if (IsEqual(argv[i], "-TCB"))
	{
		m_TCBthreshold = atof(argv[++i]);
	}
	else if (IsEqual(argv[i], "-OutputDT"))
	{
		m_bOutputDT = true;
	}
	
	else if (IsEqual(argv[i], "-debug"))
	{
		m_bDebug = true;
	}
	else
	{
		//Look to see if it's a know base option
		msg = CBaseOptions::ProcessOption(i, argc, argv);
	}

	return msg;
}

ERMsg CCloudCleanerOption::ParseOption(int argc, char* argv[])
{
	ERMsg msg = CBaseOptions::ParseOption(argc, argv);

	ASSERT(NB_FILE_PATH == 3);
	if (msg && m_filesPath.size() != NB_FILE_PATH)
	{
		msg.ajoute("ERROR: Invalid argument line. 3 files are needed: decision tree model, the LANDSAT images series and the destination image.");
		msg.ajoute("Argument found: ");
		for (size_t i = 0; i < m_filesPath.size(); i++)
			msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
	}


	if (m_outputType == GDT_Unknown)
		m_outputType = GDT_Int16;

	if (m_outputType != GDT_Int16 && m_outputType != GDT_Int32)
		msg.ajoute("Invalid -ot option. Only GDT_Int16 or GDT_Int32 are supported");

	return msg;
}


CDecisionTreeBlock CCloudCleaner::GetDataRecord(array<CLandsatPixel, 3> p, CDecisionTreeBaseEx& DT)
{
	CDecisionTreeBlock block(DT.MaxAtt + 1);

	//fill the data structure for decision tree
	size_t c = 0;
	DVal(block, c++) = DT.MaxClass + 1;
	DVal(block, c++) = Continuous(DT, 1) ? DT_UNKNOWN : 0;

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

ERMsg CCloudCleaner::ReadRules(CDecisionTree& DT)
{
	ERMsg msg;
	if(!m_options.m_bQuiet) 
	{
		cout << "Read rules..."<<endl;
	}

	CTimer timer(true);

	msg += DT.Load(m_options.m_filesPath[CCloudCleanerOption::DT_FILE_PATH], m_options.m_CPU, m_options.m_IOCPU);
	timer.Stop();

	if( !m_options.m_bQuiet )
		cout << "Read rules time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;


	return msg;
}	


ERMsg CCloudCleaner::OpenAll(CLandsatDataset& landsatDS, CGDALDatasetEx& maskDS, CLandsatDataset& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS)
{
	ERMsg msg;
	
	
	if(!m_options.m_bQuiet) 
		cout << endl << "Open input image..." << endl;

	msg = landsatDS.OpenInputImage(m_options.m_filesPath[CCloudCleanerOption::LANDSAT_FILE_PATH], m_options);

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
				msg.ajoute("CloudCleaner can't be performed over only one scene");
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

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		CCloudCleanerOption options = m_options;
		
		//replace the common part by the new name
		size_t common_begin = MAX_PATH;//common begin
		for (size_t i = 0; i < landsatDS.GetRasterCount() - 1; i++)
		{
			for (size_t j = i; j < landsatDS.GetRasterCount(); j++)
			{
				string title0 = GetFileTitle(landsatDS.GetInternalName(i));
				string title1 = GetFileTitle(landsatDS.GetInternalName(j));
				size_t k = 0;//common begin
				while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
					k++;

				common_begin = min(common_begin, k);
			}
		}

		if (common_begin != MAX_PATH)
		{
			for (size_t i = 0; i < landsatDS.GetRasterCount(); i++)
			{
				string title = GetFileTitle(landsatDS.GetInternalName(i));
				string name = GetFileTitle(filePath) + "_" + title.substr(common_begin) + ".tif|";
				options.m_VRTBandsName += name;
			}
		}
		

		
		msg += outputDS.CreateImage(filePath, options);
	}

	
	if (msg && m_options.m_bOutputDT)
	{
		if (!m_options.m_bQuiet)
			cout << "Create DTcode image..." << endl;

		CCloudCleanerOption options = m_options;
		options.m_outputType = GDT_Int16;
		options.m_nbBands = landsatDS.GetNbScenes();
		options.m_dstNodata = (short)::GetDefaultNoData(GDT_Int16);

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		string title = GetFileTitle(filePath) + "_DT";
		SetFileTitle(filePath, title );
		for (size_t ii = 0; ii < landsatDS.GetNbScenes(); ii++)
		{
			//replace the common part by the new name
			size_t common_begin = MAX_PATH;//common begin
			for (size_t i = 0; i < landsatDS.GetSceneSize() - 1; i++)
			{
				for (size_t j = i; j < landsatDS.GetSceneSize(); j++)
				{

					string title0 = GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize() + i));
					string title1 = GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize() + j));
					size_t k = 0;//common begin
					while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
						k++;

					common_begin = min(common_begin, k);
				}
			}

			string name = title + FormatA("_%02d_DT", ii + 1);
			if (common_begin != MAX_PATH)
			{
				size_t beg = GetFileTitle(landsatDS.GetFilePath()).length();
				name = title + GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize())).substr(beg, common_begin - beg);

				if (!name.empty() && name[name.length() - 1] == '_')
					name = name.substr(0, name.length()-1);
			}

			options.m_VRTBandsName += name + ".tif|";
		}

		msg += DTCodeDS.CreateImage(filePath, options);
	}

	if (msg && m_options.m_bDebug)
	{

		if (!m_options.m_bQuiet)
			cout << "Create debug image..." << endl;

		CCloudCleanerOption options = m_options;
		options.m_outputType = GDT_Int16;
		options.m_nbBands = landsatDS.GetNbScenes()*CCloudCleanerOption::NB_DBUG;

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		string title = GetFileTitle(filePath) + "_debug";
		SetFileTitle(filePath, title);
		for (size_t ii = 0; ii < landsatDS.GetNbScenes(); ii++)
		{
			//replace the common part by the new name
			size_t common_begin = MAX_PATH;//common begin
			for (size_t i = 0; i < landsatDS.GetSceneSize() - 1; i++)
			{
				for (size_t j = i; j < landsatDS.GetSceneSize(); j++)
				{
					
					string title0 = GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize()+i));
					string title1 = GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize() + j));
					size_t k = 0;//common begin
					while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
						k++;

					common_begin = min(common_begin, k);
				}
			}

			string name = title + FormatA("_%02d", ii+1);
			if (common_begin != MAX_PATH)
			{
				size_t beg = GetFileTitle(landsatDS.GetFilePath()).length();
				name = title + GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize())).substr(beg, common_begin - beg);
			}
			
			for (size_t i = 0; i < CCloudCleanerOption::NB_DBUG; i++)
			{
				options.m_VRTBandsName += name + CCloudCleanerOption::DEBUG_NAME[i] + ".tif|";
			}
			
		}
		
		msg += debugDS.CreateImage(filePath, options);
	}

	return msg;
}



ERMsg CCloudCleaner::Execute()
{
	ERMsg msg;

	if( !m_options.m_bQuiet )		
	{
		cout << "Output: " << m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[CCloudCleanerOption::LANDSAT_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[CCloudCleanerOption::DT_FILE_PATH] << endl;
		
		//for(size_t m=0; m<m_options.m_optionalDT.size(); m++)
			//cout << "        " << m_options.m_optionalDT[m].m_filePath << endl;

		if( !m_options.m_maskName.empty() )
			cout << "Mask:   "  << m_options.m_maskName << endl;
	}

	GDALAllRegister();


	CDecisionTree DT;
	msg += ReadRules(DT);
	if (!msg)
		return msg;
	
	CLandsatDataset lansatDS;
	CGDALDatasetEx maskDS;
	CLandsatDataset outputDS;
	CGDALDatasetEx DTCodeDS;
	CGDALDatasetEx debugDS;
	

	msg = OpenAll(lansatDS, maskDS, outputDS, DTCodeDS, debugDS);
	
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
			cout << "Create debug images " << " x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << m_options.m_nbBands << " years) with " << m_options.m_CPU << " threads..." << endl;


		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar(extents.m_xSize*extents.m_ySize);

		vector<pair<int,int>> XYindex = extents.GetBlockList();
		
		omp_set_nested(1);
		#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
		for(int b=0; b<(int)XYindex.size(); b++)
		{
			int thread = omp_get_thread_num();
			int xBlock=XYindex[b].first;
			int yBlock=XYindex[b].second;

			//data
			LansatData data;
			DTCodeData DTCode;
			DebugData debug;
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			ProcessBlock(xBlock, yBlock, bandHolder[thread], DT, data, DTCode, debug);
			WriteBlock(xBlock, yBlock, bandHolder[thread], data, DTCode, debug, outputDS, DTCodeDS, debugDS);
		}//for all blocks


		//  Close decision tree and free allocated memory  
		DT.FreeMemory();

		//close inputs and outputs
		CloseAll(lansatDS, maskDS, outputDS, DTCodeDS, debugDS);

	}
	
    return msg;
}


void CCloudCleaner::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
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
void CCloudCleaner::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, CDecisionTree& DT, LansatData& data, DTCodeData& DTCode, DebugData& debug)
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

		//allocate process memory and load data
		LoadData(bandHolder, data);
		if (m_options.m_bOutputDT)
		{
			DTCode.resize(nbScenes);
			for (size_t i = 0; i < DTCode.size(); i++)
				DTCode[i].resize(blockSize.m_x*blockSize.m_y);
		}
		if (m_options.m_bDebug)
		{
			debug.resize(nbScenes*CCloudCleanerOption::NB_DBUG);
			for (size_t i = 0; i < debug.size(); i++)
				debug[i].resize(blockSize.m_x*blockSize.m_y);
		}

		CLandsatPixel NO_PIXEL;
		//process all x and y 
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)  
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				int thread = ::omp_get_thread_num();
				size_t xy = y*blockSize.m_x + x;
				if (!data[xy].empty())
				{
					array <CLandsatPixel, 3> p;
					size_t z1 = 0;
					p[1] = data[xy][z1];
					//process all scenes 
					while (z1 < data[xy].size())
					{
#pragma omp atomic
						m_options.m_nbPixel++;

						size_t z2 = GetNext(data[xy], z1 + 1);
						p[2] = z2<data[xy].size() ? data[xy][z2] : NO_PIXEL;

						if (debug.size())
						{
							
							debug[z1*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DEBUG_ID][xy] = m_options.GetDebugID(p);
							//debug[z1*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_B1_T3][xy] = p[2].IsInit() ? (p[2][Landsat::B1] - p[1][Landsat::B1]) : -32768;
							//debug[z1*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_TCB_T1][xy] = p[0].IsInit() ? (p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB]) : -32768;
							//debug[z1*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_TCB_T3][xy] = p[2].IsInit() ? (p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB]) : -32768;
						}

						if (m_options.IsTrigged(p))
						{
#pragma omp atomic
							m_options.m_nbPixelDT++;

							vector <AttValue> block = GetDataRecord(p, DT[thread]);
							ASSERT(!block.empty());
							int predict = (int)DT[thread].Classify(block.data());
							ASSERT(predict >= 1 && predict <= DT[thread ].MaxClass);
							int DTexit = atoi(DT[thread].ClassName[predict]);
							if (!DTCode.empty())
								DTCode[z1][xy] = DTexit;

							//change data if cloud
							if (DTexit>100)
								data[xy][z1].Reset();
						}
						else //if not trigger
						{
							if (!DTCode.empty())
								DTCode[z1][xy] = NOT_TRIGGED_CODE;
						}

						p[0] = p[1];
						p[1] = p[2];
						z1 = z2;
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

void CCloudCleaner::WriteBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, const LansatData& data, DTCodeData& DTCode, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS)
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
			__int16 noData = (__int16)outputDS.GetNoData(0);
			vector<__int16> tmp(blockSize.m_x*blockSize.m_y, noData);
			
			for (size_t b = 0; b < outputDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(b);

				if (!data.empty())
				{
					ASSERT(data.size() == blockSize.m_x*blockSize.m_y);
					

					size_t i = b / SCENES_SIZE;
					size_t bb = b % SCENES_SIZE;
					for (int y = 0; y < blockSize.m_y; y++)
					{
						for (int x = 0; x < blockSize.m_x; x++)
						{
							int xy = int(y*blockSize.m_x + x);
							tmp[xy] = data[xy][i][bb];
						}//x
					}//y

					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(tmp[0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				}
				else
				{
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
				}

			}//for all debug bands

		}//if create image


		if (m_options.m_bOutputDT)
		{
			ASSERT(DTCode.empty() || DTCode.size() == DTCodeDS.GetRasterCount());
			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);

			for (size_t b = 0; b<DTCodeDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = DTCodeDS.GetRasterBand(b);
				if (!DTCode.empty())
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(DTCode[b][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				else
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
			}//for all debug variable
		}//debug

	
		if( m_options.m_bDebug )
		{
			ASSERT(debug.empty() || debug.size() == debugDS.GetRasterCount());
			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);

			for (size_t b = 0; b<debugDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = debugDS.GetRasterBand(b);
				if (!debug.empty())
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(debug[b][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				else
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
			}//for all debug variable
		}//debug
	}
	
	m_options.m_timerWrite.Stop(); 
	
}

void CCloudCleaner::CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS)
{
	if( !m_options.m_bQuiet )		
		_tprintf("\nClose all files...\n");

	landsatDS.Close();
	maskDS.Close(); 

	//close debug
	m_options.m_timerWrite.Start();
	if( m_options.m_bComputeStats )
		outputDS.ComputeStats(m_options.m_bQuiet);
	if( !m_options.m_overviewLevels.empty() )
		outputDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);
	outputDS.Close();

	if (m_options.m_bComputeStats)
		DTCodeDS.ComputeStats(m_options.m_bQuiet);
	if (!m_options.m_overviewLevels.empty())
		DTCodeDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);
	DTCodeDS.Close();

	if (m_options.m_bComputeStats)
		debugDS.ComputeStats(m_options.m_bQuiet);
	if (!m_options.m_overviewLevels.empty())
		debugDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);
	debugDS.Close();

	
	m_options.m_timerWrite.Stop();
		
	if( !m_options.m_bQuiet )
	{
		double percent = m_options.m_nbPixel>0?(double)m_options.m_nbPixelDT/m_options.m_nbPixel*100:0;

		_tprintf ("\n");
		_tprintf ("Percentage of pixel treated by DecisionTree: %0.3lf %%\n\n", percent);
	}

	m_options.PrintTime();
}

void CCloudCleaner::LoadData(const CBandsHolder& bandHolder, LansatData& data)
{
	CRasterWindow landsat = bandHolder.GetWindow();
	size_t nbScenes = landsat.GetNbScenes();

	CGeoSize blockSize = landsat.GetGeoSize();
	data.resize(blockSize.m_x*blockSize.m_y);
	for (int x = 0; x<blockSize.m_x; x++)
	{
		for (int y = 0; y < blockSize.m_y; y++)
		{
			data[y*blockSize.m_x + x].resize(nbScenes);
			for (size_t z = 0; z < landsat.GetNbScenes(); z++)
			{
				data[y*blockSize.m_x + x][z] = ((CLandsatWindow&)landsat).GetPixel(z, x, y);
			}
		}
	}
}
