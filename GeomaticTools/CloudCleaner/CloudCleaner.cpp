//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 1.0.6	21/12/2017	Rémi Saint-Amant	Bug correction in data layer
// 1.0.5	13/12/2017	Rémi Saint-Amant	Add debug layer for B1 and TCB. Add TZW.
// 1.0.4	13/12/2017	Rémi Saint-Amant	bug correction for big images
// 1.0.3	08/12/2017	Rémi Saint-Amant	bug correction for big images
// 1.0.2	15/11/2017	Rémi Saint-Amant	Add debug for buffer
// 1.0.1	14/11/2017	Rémi Saint-Amant	Output only one scene at a time. Add buffer and MaxScene options
// 1.0.0	31/10/2017	Rémi Saint-Amant	Creation

//-co "compress=LZW" -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite  "D:\Travaux\CloudCleaner\Model\See5_Cloud_T123" "D:\Travaux\CloudCleaner\Input\Nuage.vrt" "D:\Travaux\CloudCleaner\Output\Output.vrt"
//-scene 2 -Debug -NoResult -te 1036980 6393000 2497980 7753980 -of VRT -overwrite -ot Int16 -co "bigtiff=yes" -co "COMPRESS=LZW" -multi -IOCPU 4 -dstnodata -32768 -stats -hist -overview {8,16} -co tiled=yes  -co blockxsize=1024 -co blockysize=1024  --config GDAL_CACHEMAX 2048 "U:\GIS1\LANDSAT_SR\mos\20160909_MergeImages\CLOUD\Cloud_Cleaner\test\DT\See5_Cloud_T123" "U:\GIS1\LANDSAT_SR\mos\20160909_MergeImages\CLOUD\Cloud_Cleaner\test\2016\L57_20141516_UGIS_vrt.vrt" "U:\GIS1\LANDSAT_SR\mos\20160909_MergeImages\CLOUD\Cloud_Cleaner\test\testRemi.vrt"
//-B1 -175 -TCB 600 -ZSW 200 -Debug -NoResult -scene 3 -of VRT -IOCPU 4 -overwrite -ot Int16 -co "COMPRESS=LZW" -multi -dstnodata -32768 -stats -hist -overview {8,16} -co tiled=yes  -co blockxsize=1024 -co blockysize=1024  --config GDAL_CACHEMAX 2048 U:\GIS1\LANDSAT_SR\mos\20160909_MergeImages\CLOUD\Cloud_Cleaner\test\DT\See5_Cloud_T123 "U:\GIS\#documents\TestCodes\CloudCleaner\2014-2016\input\2014-2016.vrt" "U:\GIS\#documents\TestCodes\CloudCleaner\2014-2016\output\2016test.vrt"
//Et un CODE DT avec T1 T2 T3 ici(1 = feu, 2 = coupe, 112 et 113 ombre et nuage).C’est vraiment rough comme DT mais l’arbre sera amélioré, dans les prochaine semaines.
//-te 1708590 7055150 1708770 7055330 
//-te 1705590 7052150 1711770 7058330 
//-te 1413000 7140000 1455000 7203000
//-te 1036980 6393000 2497980 7753980 


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

 

static const char* version = "1.0.6";
static const int NB_THREAD_PROCESS = 2; 
static const __int16 NOT_TRIGGED_CODE = (__int16)::GetDefaultNoData(GDT_Int16);
static const CLandsatPixel NO_PIXEL;
const char* CCloudCleanerOption::DEBUG_NAME[NB_DBUG] = { "_ID", "_B1", "_TCB", "_ZSW", "_nbScenes", "_fill" };


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
	m_ZSWthreshold = 500;
	m_bFillCloud = false;
	m_buffer = 0;
	m_scene = 0;
	m_maxScene = 2;

	m_appDescription = "This software look up for cloud from Landsat images series (composed of " + to_string(SCENES_SIZE) + " bands) with a decision tree model";
	
	AddOption("-RGB");

	static const COptionDef OPTIONS[] =
	{
		{ "-B1", 1, "threshold", false, "trigger threshold for band 1 to execute decision tree. -175 by default." },
		{ "-TCB", 1, "threshold", false, "trigger threshold for Tassel Cap Brightness (TCB) to execute decision tree. 600 by default." },
		{ "-ZSW", 1, "threshold", false, "trigger threshold for Z-Score Water (ZSW) to execute decision tree. 500 by default." },
		{ "-FillCloud", 0, "", false, "Fill cloud with next or previous valid scenes (+1,-1,+2,-2,...) up to MaxScene. " },
		{ "-MaxScene", 1, "nbScenes", false, "Use to limit the number of scenes read (around the working scene) to find and fill clouds. 2 by default (from ws -2 to ws + 2)." },
		{ "-Scene", 1, "ws", false, "Select a working scene (1..nbScenes) to clean cloud. The first scene is select by default." },
		{ "-Buffer", 1, "nbPixel", false, "Reset or filled (if -FillCloud enable) nbPixels arround the pixels set as cloud. 0 by default." },
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
		{ "Output Image", "dstfile", "1", "ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Date(Julian day 1970)" },
		{ "Optional DTCode Image", "dstfile_DT","1","1","Decision tree result"},
		{ "Optional Debug Image", "dstfile_debug", "1", "2", "TriggerID|scene selected to fill cloud (relative to the working scene)"}
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
	else if (IsEqual(argv[i], "-ZSW"))
	{
		m_ZSWthreshold = atof(argv[++i]);
	}
	else if (IsEqual(argv[i], "-FillCloud"))
	{
		m_bFillCloud = true;
	}
	else if (IsEqual(argv[i], "-MaxScene"))
	{
		m_maxScene = atoi(argv[++i]);
	}
	else if (IsEqual(argv[i], "-Scene"))
	{
		m_scene = atoi(argv[++i])-1;
	}
	else if (IsEqual(argv[i], "-Buffer"))
	{
		m_buffer = atoi(argv[++i]) ;
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

	if (m_maxScene < 1)
		msg.ajoute("Invalid -MaxScene. -FillMaxScene must be greater than 1.");

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
	
	if (msg && m_options.m_scene >= landsatDS.GetNbScenes())
		msg.ajoute("Scene (" + to_string(m_options.m_scene) + ") must be between 1 and nbScenes (" + to_string(landsatDS.GetNbScenes()) + ")");
	//if (msg && m_options.m_maxScene > landsatDS.GetNbScenes()/2)
	//	msg.ajoute("maxScene (" + to_string(m_options.m_maxScene) + ") must be between 1 and nbScenes (" + to_string(landsatDS.GetNbScenes()) + ")");


	if(msg)
	{
		landsatDS.UpdateOption(m_options);

		//update period from scene and maxScene
		const std::vector<CTPeriod>& p = landsatDS.GetScenePeriod();
		ASSERT(m_options.m_scene < p.size());
		
		//int fillMaxScene = m_options.m_bFillCloud ? (int)m_options.m_maxScene : 1;
		//m_options.m_z1 = m_options.m_maxScene/2;
		CTPeriod period;
		for (int i = -(int)m_options.m_maxScene; i <= (int)m_options.m_maxScene; i++)
		{
			size_t ii = size_t(m_options.m_scene + i);
			if (ii < p.size())
			{
				period += p[ii];
			}
		}

		/*m_options.m_z1 = nbSceneLoaded/2;
		if (m_options.m_scene < nbSceneLoaded / 2)
			m_options.m_z1 = m_options.m_scene;
		else if (landsatDS.GetNbScenes() - m_options.m_scene - 1 < nbSceneLoaded / 2)
			m_options.m_z1 = nbSceneLoaded - (landsatDS.GetNbScenes() - m_options.m_scene - 1);

		ASSERT(m_options.m_z1 < 2 * m_options.m_maxScene + 1);*/

		
		m_options.m_period = period;
			
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

			if (landsatDS.GetNbScenes() < 3)
				msg.ajoute("CloudCleaner need at least 3 scenes");
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
		options.m_nbBands = landsatDS.GetSceneSize();
		
		//replace the common part by the new name
		//size_t common_begin = MAX_PATH;//common begin
		//for (size_t i = 0; i < landsatDS.GetRasterCount() - 1; i++)
		//{
		//	for (size_t j = i; j < landsatDS.GetRasterCount(); j++)
		//	{
		//		string title0 = GetFileTitle(landsatDS.GetInternalName(i));
		//		string title1 = GetFileTitle(landsatDS.GetInternalName(j));
		//		size_t k = 0;//common begin
		//		while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
		//			k++;

		//		common_begin = min(common_begin, k);
		//	}
		//}

		//if (common_begin != MAX_PATH)
		//{
		//	size_t begin = m_options.m_scene*landsatDS.GetSceneSize();
		//	size_t end = (m_options.m_scene+1)*landsatDS.GetSceneSize();

		//	for (size_t i = begin; i < end; i++)
		//	{
		//		string title = GetFileTitle(landsatDS.GetInternalName(i));
		//		string name = GetFileTitle(filePath) + "_" + title.substr(common_begin) + ".tif|";
		//		options.m_VRTBandsName += name;
		//	}
		//}
		

		
		msg += outputDS.CreateImage(filePath, options);
	}

	
	if (msg && m_options.m_bOutputDT)
	{
		if (!m_options.m_bQuiet)
			cout << "Create DTcode image..." << endl;

		CCloudCleanerOption options = m_options;
		options.m_format = "GTIFF";
		options.m_outputType = GDT_Int16;
		options.m_nbBands = 1;
		options.m_dstNodata = (__int16)::GetDefaultNoData(GDT_Int16);

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		string title = GetFileTitle(filePath) + "_DT";
		SetFileTitle(filePath, title );
		SetFileExtension(filePath, ".tif");

		//size_t ii = m_options.m_scene;
		//for (size_t ii = m_options.m_scene; ii < landsatDS.GetNbScenes(); ii++)
		//{
		//	//replace the common part by the new name
		//	size_t common_begin = MAX_PATH;//common begin
		//	for (size_t i = 0; i < landsatDS.GetSceneSize() - 1; i++)
		//	{
		//		for (size_t j = i; j < landsatDS.GetSceneSize(); j++)
		//		{

		//			string title0 = GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize() + i));
		//			string title1 = GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize() + j));
		//			size_t k = 0;//common begin
		//			while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
		//				k++;

		//			common_begin = min(common_begin, k);
		//		}
		//	}

		//	string name = title + FormatA("_%02d_DT", ii + 1);
		//	if (common_begin != MAX_PATH)
		//	{
		//		size_t beg = GetFileTitle(landsatDS.GetFilePath()).length();
		//		name = title + GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize())).substr(beg, common_begin - beg);

		//		if (!name.empty() && name[name.length() - 1] == '_')
		//			name = name.substr(0, name.length()-1);
		//	}

		//	options.m_VRTBandsName += name + ".tif|";
		//}

		msg += DTCodeDS.CreateImage(filePath, options);
	}

	if (msg && m_options.m_bDebug)
	{

		if (!m_options.m_bQuiet)
			cout << "Create debug image..." << endl;

		CCloudCleanerOption options = m_options;
		options.m_outputType = GDT_Int16;
		options.m_nbBands = CCloudCleanerOption::NB_DBUG;

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		string title = GetFileTitle(filePath) + "_debug";
		SetFileTitle(filePath, title);
		//for (size_t ii = 0; ii < landsatDS.GetNbScenes(); ii++)
		//size_t ii = m_options.m_scene;
		//{
		//	//replace the common part by the new name
		//	size_t common_begin = MAX_PATH;//common begin
		//	for (size_t i = 0; i < landsatDS.GetSceneSize() - 1; i++)
		//	{
		//		for (size_t j = i; j < landsatDS.GetSceneSize(); j++)
		//		{
		//			
		//			string title0 = GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize() + i));
		//			string title1 = GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize() + j));
		//			size_t k = 0;//common begin
		//			while (k < title0.size() && k < title1.size() && title0[k] == title1[k])
		//				k++;

		//			common_begin = min(common_begin, k);
		//		}
		//	}

		//	string name = title + FormatA("_%02d", ii+1);
		//	if (common_begin != MAX_PATH)
		//	{
		//		size_t beg = GetFileTitle(landsatDS.GetFilePath()).length();
		//		name = title + GetFileTitle(landsatDS.GetInternalName(ii*landsatDS.GetSceneSize())).substr(beg, common_begin - beg);
		//	}
		//	
		//	for (size_t i = 0; i < CCloudCleanerOption::NB_DBUG; i++)
		//	{
		//		options.m_VRTBandsName += name + CCloudCleanerOption::DEBUG_NAME[i] + ".tif|";
		//	}
		//	
		//}
		
		for (size_t i = 0; i < CCloudCleanerOption::NB_DBUG; i++)
		{
			options.m_VRTBandsName += title + CCloudCleanerOption::DEBUG_NAME[i] + ".tif|";
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
		boost::dynamic_bitset<size_t> clouds((size_t)extents.m_xSize*extents.m_ySize);

		if (!m_options.m_bQuiet )
			cout << "Find clouds..." << endl;

		m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);
		vector<pair<int,int>> XYindex = extents.GetBlockList();
		//while (XYindex.size() > 5)
			//XYindex.erase(XYindex.begin());
		
		//pass 1 : find counds
		omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
		for(int b=0; b<(int)XYindex.size(); b++)
		{
			int thread = omp_get_thread_num();
			int xBlock=XYindex[b].first;
			int yBlock=XYindex[b].second;

			//data
			DTCodeData DTCode;
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			ProcessBlock1(xBlock, yBlock, bandHolder[thread], DT, DTCode, clouds);
			WriteBlock1(xBlock, yBlock, bandHolder[thread], DTCode, DTCodeDS);
		}//for all blocks

		if (m_options.m_bCreateImage || m_options.m_bDebug)
		{
			if (!m_options.m_bQuiet)
				cout << "Replace clouds..." << endl;

			m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);

			//pass 2 : reset or replace clouds
			omp_set_nested(1);
//#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int thread = omp_get_thread_num();
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				//data
				LansatData data;
				DebugData debug;
				ReadBlock(xBlock, yBlock, bandHolder[thread]);
				ProcessBlock2(xBlock, yBlock, bandHolder[thread], data, debug, clouds);
				WriteBlock2(xBlock, yBlock, bandHolder[thread], data, debug, outputDS, debugDS);
			}//for all blocks
		}

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

size_t GetPrevious(CLandsatPixelVector& landsat, size_t z)
{
	size_t previous = NOT_INIT;
	for (size_t zz = z-1; zz < landsat.size() && previous == NOT_INIT; zz--)
		if (landsat[zz].IsValid())
			previous = zz;

	return previous;
}

size_t GetNext(CLandsatPixelVector& landsat, size_t z)
{
	size_t next=NOT_INIT;
	for (size_t zz = z+1; zz < landsat.size() && next == NOT_INIT; zz++)
		if (landsat[zz].IsValid())
			next = zz;

	return next;
}

array <CLandsatPixel, 3> GetP(size_t z1, CLandsatPixelVector& data)
{
	ASSERT(z1<data.size());

	array <CLandsatPixel, 3> p;
	
	p[1] = data[z1];
	size_t z0 = GetPrevious(data, z1);
	p[0] = z0 < data.size() ? data[z0] : NO_PIXEL;
	size_t z2 = GetNext(data, z1);
	p[2] = z2 < data.size() ? data[z2] : NO_PIXEL;
	
	return p;
}

//Get input image reference
void CCloudCleaner::ProcessBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, CDecisionTree& DT, DTCodeData& DTCode, boost::dynamic_bitset<size_t>& clouds)
{
	//CGDALDatasetEx& inputDS, 

	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect(xBlock, yBlock);
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
	size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;

	
	if (bandHolder.IsEmpty() )
	{
#pragma omp atomic		
		m_options.m_xx += (std::min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));
		m_options.UpdateBar();

		return;
	}


	//allocate process memory and load data
	LansatData data;
	LoadData(bandHolder, data);

#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();


		if (m_options.m_bOutputDT)
		{
			//DTCode.resize(nbScenes);
			//for (size_t i = 0; i < DTCode.size(); i++)
			DTCode.insert(DTCode.begin(), blockSize.m_x*blockSize.m_y, (__int16)GetDefaultNoData(GDT_Int16));
		}
		//if (m_options.m_bDebug)
		//{
		//	debug.resize(CCloudCleanerOption::NB_DBUG);
		//	//for (size_t i = 0; i < debug.size(); i++)
		//	debug[CCloudCleanerOption::D_DEBUG_ID].insert(debug[CCloudCleanerOption::D_DEBUG_ID].begin(), blockSize.m_x*blockSize.m_y, (__int16)GetDefaultNoData(GDT_Int16));
		//}

		
		//process all x and y 
#pragma omp parallel for schedule(static, 1) num_threads( max(1, m_options.m_CPU-1) ) if (m_options.m_bMulti)  
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				int thread = ::omp_get_thread_num();
				size_t xy = y*blockSize.m_x + x;

				size_t z1 = m_options.m_scene;
				array <CLandsatPixel, 3> p = GetP(z1, data[xy]);

#pragma omp atomic
				m_options.m_nbPixel++;

				if (m_options.IsTrigged(p))
				{
#pragma omp atomic
					m_options.m_nbPixelDT++;

					vector <AttValue> block = GetDataRecord(p, DT[thread]);
					ASSERT(!block.empty());
					int predict = (int)DT[thread].Classify(block.data());
					ASSERT(predict >= 1 && predict <= DT[thread].MaxClass);
					int DTexit = atoi(DT[thread].ClassName[predict]);
					if (!DTCode.empty())
						DTCode[xy] = DTexit;

					//change data if cloud
					if (DTexit > 100)
					{
						//for (int yy = -(int)m_options.m_buffer; yy <= (int)m_options.m_buffer; yy++)
						for (size_t yy = 0; yy < 2*m_options.m_buffer+1; yy++)
						{
							size_t yyy = index.m_y + y + yy - m_options.m_buffer;
							if (yyy < extents.m_ySize)
							{
								//for (int xx = -(int)m_options.m_buffer; xx <= -(int)m_options.m_buffer; xx++)
								for (size_t xx = 0; xx < 2 * m_options.m_buffer + 1; xx++)
								{
									size_t xxx = index.m_x + x + xx - m_options.m_buffer;
									if (xxx < extents.m_xSize)
									{
										size_t xy2 = yyy* extents.m_xSize + xxx;
										clouds.set(xy2);
									}
								}//for buffer x
							}
						}//for buffer y 
					}//if cloud
				}
			
				//}//if data
#pragma omp atomic	
				m_options.m_xx++;
			}//for x


			m_options.UpdateBar();
		}//for y



		m_options.m_timerProcess.Stop();
	}
}

void CCloudCleaner::WriteBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, DTCodeData& DTCode, CGDALDatasetEx& DTCodeDS)
{

#pragma omp critical(BlockIO)
	{

		m_options.m_timerWrite.Start();

		
		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
		assert(bandHolder.GetNbScenes() == bandHolder.GetPeriod().GetNbYears());


		ASSERT(outputRect.Width() == blockSize.m_x);
		ASSERT(outputRect.Height() == blockSize.m_y);


		if (m_options.m_bOutputDT)
		{
			ASSERT(DTCode.empty() || DTCode.size() == outputRect.Width()*outputRect.Height());
			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);

			GDALRasterBand *pBand = DTCodeDS.GetRasterBand(0);
			if (!DTCode.empty())
				pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(DTCode[0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
			else
				pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
		}

	
	}
	
	m_options.m_timerWrite.Stop(); 
	
}

void CCloudCleaner::ProcessBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder, LansatData& data, DebugData& debug, const boost::dynamic_bitset<size_t>& clouds)
{
	//CGDALDatasetEx& inputDS, 

	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect(xBlock, yBlock);
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
	size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;

	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += (std::min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));
		m_options.UpdateBar();

		return;
	}

	//allocate process memory and load data
	LoadData(bandHolder, data);


	if (m_options.m_bDebug)
	{
		debug.resize(CCloudCleanerOption::NB_DBUG);
		for (size_t i = 0; i < debug.size(); i++)
			debug[i].insert(debug[i].begin(), blockSize.m_x*blockSize.m_y, (__int16)GetDefaultNoData(GDT_Int16));
	}

#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();

	

		//process all x and y 
//#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)  
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				size_t xy = (size_t)y*blockSize.m_x + x;
				size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;


				if (!debug.empty())
				{
					size_t z1 = m_options.m_scene;
					array <CLandsatPixel, 3> p = GetP(z1, data[xy]);
					debug[CCloudCleanerOption::D_DEBUG_ID][xy] = m_options.GetDebugID(p);
					debug[CCloudCleanerOption::D_DEBUG_B1][xy] = m_options.IsB1Trigged(p);
					debug[CCloudCleanerOption::D_DEBUG_TCB][xy] = m_options.IsTCBTrigged(p);
					debug[CCloudCleanerOption::D_DEBUG_ZSW][xy] = m_options.IsZSWTrigged(p);
					debug[CCloudCleanerOption::D_NB_SCENE][xy] = (p[0].IsInit() ? 1 : 0) + (p[1].IsInit() ? 1 : 0) + (p[2].IsInit() ? 1 : 0);
				}

				if (clouds.test(xy2))
				{

					size_t z1 = m_options.m_scene;
					size_t z2 = NOT_INIT;
					for (size_t zz = 0; zz < data[xy].size() * 2 && z2 == NOT_INIT; zz++)
					{
						size_t zzz = size_t(z1 + (int)pow(-1, zz)*(zz / 2 + 1));
						if (zzz < data[xy].size() && data[xy][zzz].IsInit())
							z2 = zzz;
					}

					if (!debug.empty())
					{
						//array <CLandsatPixel, 3> p = GetP(z1, data[xy]);
						//debug[CCloudCleanerOption::D_DEBUG_ID][xy] = m_options.GetDebugID(p);
						debug[CCloudCleanerOption::D_SCENE_USED][xy] = (__int16)(z2 - z1);
					}

					data[xy][z1].Reset();
					if (m_options.m_bFillCloud && z2 != NOT_INIT)
					{
						data[xy][z1] = data[xy][z2];
					}

				}

#pragma omp atomic	
				m_options.m_xx++;
			}//for x

			m_options.UpdateBar();
		}//for y


		m_options.m_timerProcess.Stop();
	}
}


void CCloudCleaner::WriteBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder, const LansatData& data, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
{

#pragma omp critical(BlockIO)
	{

		m_options.m_timerWrite.Start();


		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
		assert(bandHolder.GetNbScenes() == bandHolder.GetPeriod().GetNbYears());


		ASSERT(outputRect.Width() == blockSize.m_x);
		ASSERT(outputRect.Height() == blockSize.m_y);

		if (m_options.m_bCreateImage)
		{
			ASSERT(outputDS.GetRasterCount() == SCENES_SIZE);

			__int16 noData = (__int16)outputDS.GetNoData(0);
			vector<__int16> tmp(blockSize.m_x*blockSize.m_y, noData);

			size_t z1 = m_options.m_scene;
			for (size_t b = 0; b < outputDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(b);

				if (!data.empty())
				{
					ASSERT(data.size() == blockSize.m_x*blockSize.m_y);
					
					for (int y = 0; y < blockSize.m_y; y++)
					{
						for (int x = 0; x < blockSize.m_x; x++)
						{
							int xy = int(y*blockSize.m_x + x);
							tmp[xy] = data[xy][z1][b];
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

		if (m_options.m_bDebug)
		{
			ASSERT(debug.empty() || debug.size() == CCloudCleanerOption::NB_DBUG);
			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);

			for (size_t b = 0; b<debugDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = debugDS.GetRasterBand(b);
				if (!debug.empty() && !debug[b].empty())
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(debug[b][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				else
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
			}//for all debug variable
		}//debug
	}//critical
}

void CCloudCleaner::CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS)
{
	if( !m_options.m_bQuiet )		
		_tprintf("\nClose all files...\n");

	landsatDS.Close();
	maskDS.Close(); 


	m_options.m_timerWrite.Start();

	outputDS.Close(m_options);
	DTCodeDS.Close(m_options);
	debugDS.Close(m_options);

	
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
	CRasterWindow window = bandHolder.GetWindow();
	size_t nbScenes = bandHolder.GetNbScenes();
	//size_t nbScenes = m_options.m_maxScene * 2 + 1;

	CGeoSize blockSize = window.GetGeoSize();
	data.resize(blockSize.m_x*blockSize.m_y);
	for (int x = 0; x<blockSize.m_x; x++)
	{
		for (int y = 0; y < blockSize.m_y; y++)
		{
			data[y*blockSize.m_x + x].resize(nbScenes);
			//for (int z = -(int)m_options.m_maxScene; z <= (int)m_options.m_maxScene; z++)
			for (size_t z = 0; z < nbScenes; z++)
			{
				//size_t zz = m_options.m_scene + z - m_options.m_maxScene;
				//if (zz<window.GetNbScenes())
				data[y*blockSize.m_x + x][z] = ((CLandsatWindow&)window).GetPixel(z, x, y);
			}
		}
	}
}
