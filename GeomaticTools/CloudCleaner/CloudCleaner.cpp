//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 2.0.0    17/05/2018	Rémi Saint-Amant	New random forest model with ranger
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

//-Scenes 2 2 -debug -outputcode -co "compress=LZW" -RGB Natural -of VRT -overwrite "D:\Travaux\CloudCleaner\Model\RF_BEG_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_MID_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_END_v1.classification.forest" "D:\Travaux\CloudCleaner\Input\subset\subset.tif" "D:\Travaux\CloudCleaner\Output\subset\subset.vrt"
//-Scenes 1 1 -FillCloud -debug -outputcode -multi -co "tiled=YES" -co "BLOCKXSIZE=512" -co "BLOCKYSIZE=512" -co "compress=LZW" -RGB Natural -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite "D:\Travaux\CloudCleaner\Model\RF_BEG_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_MID_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_END_v1.classification.forest" "D:\Travaux\CloudCleaner\Input\puff_partout.vrt" "D:\Travaux\CloudCleaner\Output\puff_partout\puff_partout.vrt"
//-DoubleTrigger 4 -FillCloud -debug -outputcode -multi -co "tiled=YES" -co "BLOCKXSIZE=512" -co "BLOCKYSIZE=512" -co "compress=LZW" -RGB Natural -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite "D:\Travaux\CloudCleaner\Model\RF_BEG_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_MID_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_END_v1.classification.forest" "D:\Travaux\CloudCleaner\Input\haze_et_ombre.vrt" "D:\Travaux\CloudCleaner\Output\haze_et_ombre\haze_et_ombre.vrt"
//-FillCloud -debug -outputcode -multi -co "compress=LZW" -RGB Natural -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite "D:\Travaux\CloudCleaner\Model\RF_BEG_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_MID_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_END_v1.classification.forest" "D:\Travaux\CloudCleaner\Input\Nuage_1999-2014.vrt" "D:\Travaux\CloudCleaner\Output\Nuage_1999-2014\Nuage.vrt"
//-te 1622910 6987120 1623120 6987330 
//-te 1622590 6987810 1622890 6988110 
//-te 1361820 6825390 1361970 6825540 
//-scenes 1 1 -te 1357320 6797160 1357470 6797310 -DoubleTrigger 4 -FillCloud -debug -outputcode -multi -co "compress=LZW" -RGB Natural -of VRT --config GDAL_CACHEMAX 4096 -overwrite "D:\Travaux\CloudCleaner\Model\RF_BEG_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_MID_v1.classification.forest" "D:\Travaux\CloudCleaner\Model\RF_END_v1.classification.forest" "D:\Travaux\CloudCleaner\Input\puff_partout.vrt" "D:\Travaux\CloudCleaner\Output\puff_partout\test.vrt"
//

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
#include "Geomatic/LandsatDataset.h"




#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF;
using namespace WBSF::Landsat;



static const char* version = "2.0.0";
static const int NB_THREAD_PROCESS = 2;
static const __int16 NOT_TRIGGED_CODE = (__int16)::GetDefaultNoData(GDT_Int16);
static const CLandsatPixel NO_PIXEL;
const char* CCloudCleanerOption::DEBUG_NAME[NB_DBUG] = { "_ID", "_B1f", "_TCBf", "_ZSWf", "_nbScenes", "_fill" };


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
	m_B1threshold = { -175, -55 };
	m_TCBthreshold = { 600, 200 };
	m_ZSWthreshold = { 500, 150 };
	m_bFillCloud = false;
	m_doubleTrigger = 4;

	m_buffer = 1;
	m_scenes = { {NOT_INIT, NOT_INIT } };
	//m_maxScene = 2;

	m_appDescription = "This software look up for cloud from Landsat images series (composed of " + to_string(SCENES_SIZE) + " bands) with a random forest model (from Ranger)";

	AddOption("-RGB");

	static const COptionDef OPTIONS[] =
	{
		//{ "-NoTrigger", 0, "", false, "Don't use trigger execute random forest for all pixels. " },
		//{ "-B1", 1, "threshold", false, "trigger threshold for band 1 to set pixel as suspect and execute random forest. -175 by default." },
		//{ "-TCB", 1, "threshold", false, "trigger threshold for Tassel Cap Brightness (TCB) to set pixel as suspect and execute random forest. 600 by default." },
		//{ "-ZSW", 1, "threshold", false, "trigger threshold for Z-Score Water (ZSW) to set pixel as suspect and execute random forest. 500 by default." },
		{ "-Thres", 4, "type B1 TCB ZSW", false, "Set trigger threshold for B1, TCB and ZSW to set pixel as suspect and execute random forest. Type is 1 for primary threshold and 2 for secondary threshod. -175 600 500 for primary and -55 200 150 for secondary." },
		{ "-FillCloud", 0, "", false, "Fill cloud with next or previous valid scenes (+1,-1,+2,-2,...). Fill cloud by default." },
		//{ "-MaxScene", 1, "nbScenes", false, "Use to limit the number of scenes read (around the working scene) to find and fill clouds. 2 by default (from ws -2 to ws + 2)." },
		{ "-Scenes", 2, "first last", false, "Select a first and the last scene (1..nbScenes) to clean cloud. All scenes are selected by default." },
		{ "-Buffer", 1, "nbPixel", false, "Set suspicious pixels arround cloud pixels as cloud. 1 by default." },
		//{ "-BufferSmoot", 4, "nbPixel", false, "merge nbPixels arround cloud pixels . 0 by default." },
		{ "-DoubleTrigger", 1, "nbPixel", false, "Set the buffer size for secondary suspicious pixel. 4 by default" },
		{ "-OutputCode", 0, "", false, "Output random forest result code." },
		{ "-Debug",0,"",false,"Output debug information."},
		{ "ModelBeg", 0, "", false, "random forest debin cloud model file path." },
		{ "ModelMid", 0, "", false, "random forest middle cloud model file path." },
		{ "ModelEnd", 0, "", false, "random forest end cloud model file path." },
		{ "srcfile", 0, "", false, "Input LANDSAT scenes image file path." },
		{ "dstfile", 0, "", false, "Output LANDSAT scenes image file path." }
	};

	for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
		AddOption(OPTIONS[i]);

	static const CIOFileInfoDef IO_FILE_INFO[] =
	{
		{ "Input Model", "ModelBegin", "", "", "", "random forest begin model file generate by Ranger." },
		{ "Input Model", "ModelMiddle", "", "", "", "random forest middle model file generate by Ranger." },
		{ "Input Model", "ModelEnd", "", "", "", "random forest end model file generate by Ranger." },
		{ "LANDSAT Image", "src1file", "NbScenes", "ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Date(Julian day 1970)|... for each scene" },
		{ "Output Image", "dstfile", "Nb scenes processed", "ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Date(Julian day 1970)" },
		{ "Optional Code Image", "dstfile_code","Nb scenes processed","1","random forest result"},
		{ "Optional Debug Image", "dstfile_debug", "Nb scenes processed", "6", "Bit fields [ZSW|TCB|B1|nbImages(0..3)]|Cloud trigged (B1)|Shadow trigged (TCB)|Haze trigged (Z-Score Water)|Nb scene|scene selected to fill cloud (relative to the filled scene)"}

	};

	for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
		AddIOFileInfo(IO_FILE_INFO[i]);

}

ERMsg CCloudCleanerOption::ProcessOption(int& i, int argc, char* argv[])
{
	ERMsg msg;


	//if (IsEqual(argv[i], "-B1"))
	//{
	//	m_B1threshold[0] = atof(argv[++i]);
	//}
	//else if (IsEqual(argv[i], "-TCB"))
	//{
	//	m_TCBthreshold[0] = atof(argv[++i]);
	//}
	//else if (IsEqual(argv[i], "-ZSW"))
	//{
	//	m_ZSWthreshold[0] = atof(argv[++i]);
	//}
	if (IsEqual(argv[i], "-Thres"))
	{
		size_t type = atoi(argv[++i]) - 1;
		if (type < 2)
		{
			m_B1threshold[type] = atoi(argv[++i]);
			m_TCBthreshold[type] = atoi(argv[++i]);
			m_ZSWthreshold[type] = atoi(argv[++i]);
		}
		else
		{
			msg.ajoute("invalide threshold type. type must be 1 or 2");
		}
	}
	else if (IsEqual(argv[i], "-FillCloud"))
	{
		m_bFillCloud = true;
	}
	else if (IsEqual(argv[i], "-Buffer"))
	{
		m_buffer = atoi(argv[++i]);

	}
	else if (IsEqual(argv[i], "-Scenes"))
	{
		m_scenes[0] = atoi(argv[++i]) - 1;
		m_scenes[1] = atoi(argv[++i]) - 1;
	}
	else if (IsEqual(argv[i], "-DoubleTrigger"))
	{
		m_doubleTrigger = atoi(argv[++i]);
	}
	else if (IsEqual(argv[i], "-OutputCode"))
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

	ASSERT(NB_FILE_PATH == 5);
	if (msg && m_filesPath.size() != NB_FILE_PATH)
	{
		msg.ajoute("ERROR: Invalid argument line. 5 files are needed: 3 random forest models (begin, middle, end), the LANDSAT images series and the destination image.");
		msg.ajoute("Argument found: ");
		for (size_t i = 0; i < m_filesPath.size(); i++)
			msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
	}

	if (m_outputType == GDT_Unknown)
		m_outputType = GDT_Int16;

	if (m_outputType != GDT_Int16 && m_outputType != GDT_Int32)
		msg.ajoute("Invalid -ot option. Only GDT_Int16 or GDT_Int32 are supported");

	//if (m_maxScene < 1)
		//msg.ajoute("Invalid -MaxScene. -FillMaxScene must be greater than 1.");

	return msg;
}


//CDecisionTreeBlock CCloudCleaner::GetDataRecord(array<CLandsatPixel, 3> p, CDecisionTreeBaseEx& DT)
//{
//	CDecisionTreeBlock block(DT.MaxAtt + 1);
//
//	//fill the data structure for random forest
//	size_t c = 0;
//	DVal(block, c++) = DT.MaxClass + 1;
//	DVal(block, c++) = Continuous(DT, 1) ? DT_UNKNOWN : 0;
//
//	for (size_t z = 0; z < 3; z++)
//	{
//		for (size_t j = 0; j < Landsat::SCENES_SIZE; j++)
//			CVal(block, c++) = (ContValue)(p[z].at(j));
//	}//for
//
//
//	ASSERT(c == (3 * 9 + 2));
//	//fill virtual bands 
//	for (; c <= DT.MaxAtt; c++)
//	{
//		ASSERT(DT.AttDef[c]);
//		//assuming virtual band never return no data
//		block[c] = DT.EvaluateDef(DT.AttDef[c], block.data());
//	}
//
//	return block;
//}

//***********************************************************************


ERMsg CCloudCleaner::ReadModel(std::string filePath, int CPU, ForestPtr& forest)
{
	ERMsg msg;

	TreeType treetype = GetTreeType(filePath);

	//forests.reserve(m_options.m_CPU);
	forest.reset(CreateForest(treetype));
	forest->init_predict(0, CPU, false, DEFAULT_PREDICTIONTYPE);
	forest->loadFromFile(filePath);
	cout << "Forest name:                       " << GetFileTitle(GetFileTitle(filePath)) << std::endl;
	cout << "Forest type:                       " << GetTreeTypeStr(treetype) << std::endl;
	cout << "Number of trees:                   " << forest->getNumTrees() << std::endl;
	cout << "Dependent variable column:         " << forest->getDependentVarId() + 1 << std::endl;
	cout << "Number of independent variables:   " << forest->getNumIndependentVariables() << std::endl;
	cout << std::endl;

	return msg;
}

ERMsg CCloudCleaner::ReadModel(ForestPtr forests[3])
{
	ERMsg msg;

	CTimer timer(true);

	if (!m_options.m_bQuiet)
		cout << "Read forest..." << endl;

	for (size_t i = 0; i < 3; i++)
		msg += ReadModel(m_options.m_filesPath[CCloudCleanerOption::RF_BEG_FILE_PATH + i], m_options.m_bMulti ? m_options.m_CPU : -1, forests[i]);

	timer.Stop();

	if (!m_options.m_bQuiet)
		cout << "Read forest time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;

	return msg;
}

//
//ERMsg CCloudCleaner::ReadRules(CDecisionTree& DT)
//{
//	ERMsg msg;
//	if(!m_options.m_bQuiet) 
//	{
//		cout << "Read rules..."<<endl;
//	}
//
//	CTimer timer(true);
//
//	msg += DT.Load(m_options.m_filesPath[CCloudCleanerOption::DT_FILE_PATH], m_options.m_CPU, m_options.m_IOCPU);
//	timer.Stop();
//
//	if( !m_options.m_bQuiet )
//		cout << "Read rules time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;
//
//
//	return msg;
//}	


ERMsg CCloudCleaner::OpenAll(CLandsatDataset& landsatDS, CGDALDatasetEx& maskDS, CLandsatDataset& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS)
{
	ERMsg msg;



	if (!m_options.m_bQuiet)
		cout << endl << "Open input image..." << endl;

	msg = landsatDS.OpenInputImage(m_options.m_filesPath[CCloudCleanerOption::LANDSAT_FILE_PATH], m_options);
	if (msg)
	{
		if (landsatDS.GetNbScenes() < 2)
			msg.ajoute("CloudCleaner need at least 2 scenes");

		if (m_options.m_scenes[0] == NOT_INIT)
			m_options.m_scenes[0] = 0;

		if (m_options.m_scenes[1] == NOT_INIT)
			m_options.m_scenes[1] = landsatDS.GetNbScenes() - 1;

		if (m_options.m_scenes[0] >= landsatDS.GetNbScenes() || m_options.m_scenes[1] >= landsatDS.GetNbScenes())
			msg.ajoute("Scenes {" + to_string(m_options.m_scenes[0] + 1) + ", " + to_string(m_options.m_scenes[1] + 1) + "} must be in range {1, " + to_string(landsatDS.GetNbScenes()) + "}");

		if (m_options.m_scenes[0] > m_options.m_scenes[1])
			msg.ajoute("First scene (" + to_string(m_options.m_scenes[0] + 1) + ") must be smaller or equal to the last scene (" + to_string(m_options.m_scenes[1] + 1) + ")");

	}

	if (!msg)
		return msg;


	//if (msg && m_options.m_maxScene > landsatDS.GetNbScenes()/2)
	//	msg.ajoute("maxScene (" + to_string(m_options.m_maxScene) + ") must be between 1 and nbScenes (" + to_string(landsatDS.GetNbScenes()) + ")");


	landsatDS.UpdateOption(m_options);

	//update period from scene
	m_options.m_period = landsatDS.GetPeriod();
	size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;

	CTPeriod processPeriod;
	const std::vector<CTPeriod>& p = landsatDS.GetScenePeriod();

	ASSERT(m_options.m_scenes[0] < p.size());
	ASSERT(m_options.m_scenes[1] < p.size());

	for (size_t i = 0; i < nbScenedProcess; i++)
	{
		size_t ii = size_t(m_options.m_scenes[0] + i);
		ASSERT(ii < p.size());

		processPeriod += p[ii];
	}

	if (!m_options.m_bQuiet)
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
		cout << "    Process period   = " << processPeriod.GetFormatedString() << endl;
	}



	if (msg && !m_options.m_maskName.empty())
	{
		if (!m_options.m_bQuiet)
			cout << "Open mask image..." << endl;

		msg += maskDS.OpenInputImage(m_options.m_maskName);
	}


	if (msg && m_options.m_bCreateImage)
	{
		if (!m_options.m_bQuiet)
			cout << "Create output image..." << endl;

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		//string title = GetFileTitle(filePath);
		CCloudCleanerOption options = m_options;
		options.m_nbBands = nbScenedProcess * landsatDS.GetSceneSize();

		size_t begin = m_options.m_scenes[0] * landsatDS.GetSceneSize();
		size_t end = (m_options.m_scenes[1] + 1)*landsatDS.GetSceneSize();

		//replace the common part by the new name
		for (size_t i = begin; i < end; i++)
		{
			string subName = WBSF::TrimConst(landsatDS.GetSpecificBandName(i), "_");
			options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + ".tif|";
		}

		msg += outputDS.CreateImage(filePath, options);
	}


	if (msg && m_options.m_bOutputDT)
	{
		if (!m_options.m_bQuiet)
			cout << "Create Ranger code image..." << endl;

		CCloudCleanerOption options = m_options;
		options.m_outputType = GDT_Int16;
		options.m_nbBands = nbScenedProcess;
		options.m_dstNodata = (__int16)::GetDefaultNoData(GDT_Int16);

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		SetFileTitle(filePath, GetFileTitle(filePath) + "_code");

		size_t begin = m_options.m_scenes[0];
		size_t end = (m_options.m_scenes[1] + 1);

		//replace the common part by the new name
		for (size_t i = begin; i < end; i++)
		{
			string subName = WBSF::TrimConst(landsatDS.GetCommonImageName(i), "_");
			options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + ".tif|";
		}

		msg += DTCodeDS.CreateImage(filePath, options);
	}

	if (msg && m_options.m_bDebug)
	{
		if (!m_options.m_bQuiet)
			cout << "Create debug image..." << endl;

		CCloudCleanerOption options = m_options;
		options.m_outputType = GDT_Int16;
		options.m_nbBands = nbScenedProcess * CCloudCleanerOption::NB_DBUG;

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		SetFileTitle(filePath, GetFileTitle(filePath) + "_debug");

		//replace the common part by the new name
		for (size_t zz = 0; zz < nbScenedProcess; zz++)
		{
			size_t z = m_options.m_scenes[0] + zz;
			string subName = WBSF::TrimConst(landsatDS.GetCommonImageName(z), "_");
			if (subName.empty())
				subName = FormatA("%02d", z + 1);

			for (size_t j = 0; j < CCloudCleanerOption::NB_DBUG; j++)
			{
				//title = FormatA("%d_%s", int(b / SCENES_SIZE) + 1, Landsat::GetSceneName(b%SCENES_SIZE));
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + CCloudCleanerOption::DEBUG_NAME[j] + ".tif|";
			}
		}

		msg += debugDS.CreateImage(filePath, options);
	}

	return msg;
}



ERMsg CCloudCleaner::Execute()
{
	ERMsg msg;

	if (!m_options.m_bQuiet)
	{
		cout << "Output: " << m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[CCloudCleanerOption::LANDSAT_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[CCloudCleanerOption::RF_BEG_FILE_PATH + 0] << endl;
		cout << "Using:  " << m_options.m_filesPath[CCloudCleanerOption::RF_BEG_FILE_PATH + 1] << endl;
		cout << "Using:  " << m_options.m_filesPath[CCloudCleanerOption::RF_BEG_FILE_PATH + 2] << endl;

		if (!m_options.m_maskName.empty())
			cout << "Mask:   " << m_options.m_maskName << endl;
	}

	GDALAllRegister();

	ForestPtr forests[3];
	msg += ReadModel(forests);

	if (!msg)
		return msg;

	CLandsatDataset lansatDS;
	CGDALDatasetEx maskDS;
	CLandsatDataset outputDS;
	CGDALDatasetEx DTCodeDS;
	CGDALDatasetEx debugDS;


	msg = OpenAll(lansatDS, maskDS, outputDS, DTCodeDS, debugDS);

	if (msg)
	{
		size_t nbScenedProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
		size_t nbScenes = lansatDS.GetNbScenes();
		size_t sceneSize = lansatDS.GetSceneSize();
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

		if (maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(lansatDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if (!msg)
			return msg;


		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create debug images " << " x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << m_options.m_nbBands << " years) with " << m_options.m_CPU << " threads..." << endl;


		CGeoExtents extents = bandHolder.GetExtents();


		CloudBitset suspects1(nbScenedProcess);
		for (size_t i = 0; i < suspects1.size(); i++)
			suspects1[i].resize((size_t)extents.m_xSize*extents.m_ySize);

		CloudBitset suspects2;
		if (m_options.m_doubleTrigger > 0)
		{
			suspects2.resize(nbScenedProcess);
			for (size_t i = 0; i < suspects2.size(); i++)
				suspects2[i].resize((size_t)extents.m_xSize*extents.m_ySize);
		}

		CloudBitset clouds(nbScenedProcess);
		for (size_t i = 0; i < clouds.size(); i++)
			clouds[i].resize((size_t)extents.m_xSize*extents.m_ySize);

		if (!m_options.m_bQuiet)
			cout << "Find clouds..." << endl;

		m_options.ResetBar((size_t)nbScenedProcess*extents.m_xSize*extents.m_ySize);
		vector<pair<int, int>> XYindex = extents.GetBlockList();

		//pass 1 : find counds
		omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int thread = omp_get_thread_num();
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			//data
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			Preprocess(xBlock, yBlock, bandHolder[thread], forests, suspects1, suspects2);
		}//for all blocks

		for (size_t zz = 0; zz < suspects1.size(); zz++)
			cout << "Suspicious1 for scene " << zz + 1 << ": " << std::fixed << std::setprecision(2) << (double)suspects1[zz].count() / suspects1[zz].size() * 100.0 << "%" << endl;


		if (!suspects2.empty())
		{
			/*
			cout << "Clean suspicious2..." << endl;
			for (size_t zz = 0; zz < suspects2.size(); zz++)
			{
				for (size_t y = 0; y < extents.m_ySize; y++)
				{
					for (size_t x = 0; x < extents.m_xSize; x++)
					{
						size_t xy = y * extents.m_xSize + x;

						if (suspects2[zz].test(xy))
						{
							bool adjacent = false;
							for (size_t yy = 0; yy < 2 * m_options.m_doubleTrigger + 1 && !adjacent; yy++)
							{
								for (size_t xx = 0; xx < 2 * m_options.m_doubleTrigger + 1 && !adjacent; xx++)
								{
									size_t yyy = y + yy - m_options.m_doubleTrigger;
									size_t xxx = x + xx - m_options.m_doubleTrigger;
									if (yyy < extents.m_ySize && xxx < extents.m_xSize)
									{
										size_t xy2 = yyy * extents.m_xSize + xxx;
										if (suspects1[zz].test(xy2))
											adjacent = true;
									}
								}
							}

							if (!adjacent)
								suspects2[zz].reset(xy);
						}
					}
				}
			}*/

			cout << "Merge primary and secondary suspicious pixels" << endl;

#pragma omp parallel for schedule(static, 1) num_threads(m_options.m_CPU) if (m_options.m_bMulti)
			for (__int64 zz = 0; zz < (__int64)nbScenedProcess; zz++)
			{
				CleanSuspect2(extents, suspects1[zz], suspects2[zz]);
			}



			for (size_t zz = 0; zz < suspects2.size(); zz++)
				cout << "Suspicious2 for scene " << zz + 1 << ": " << std::fixed << std::setprecision(2) << (double)suspects2[zz].count() / suspects2[zz].size() * 100.0 << "%" << endl;
		}





		cout << "Call ranger for all suspicious pixels" << endl;
		m_options.ResetBar((size_t)nbScenedProcess*extents.m_xSize*extents.m_ySize);
#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int thread = omp_get_thread_num();
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			//data
			RFCodeData RFcode;
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			ProcessBlock1(xBlock, yBlock, bandHolder[thread], forests, RFcode, suspects1, suspects2, clouds);
			WriteBlock1(xBlock, yBlock, bandHolder[thread], RFcode, DTCodeDS);
		}//for all blocks

		if (m_options.m_buffer > 0)
		{

			//set buffer around cloud
			for (size_t zz = 0; zz < clouds.size(); zz++)
			{
				boost::dynamic_bitset<size_t> cloudsCopy = clouds[zz];
				//size_t z = m_options.m_scenes[0] + zz;
				for (size_t y = 0; y < extents.m_ySize; y++)
				{
					for (size_t x = 0; x < extents.m_xSize; x++)
					{
						//size_t xy = (size_t)y*blockSize.m_x + x;
						size_t xy2 = y * extents.m_xSize + x;

						//he suspicious = 
						if (cloudsCopy.test(xy2))
						{
							for (size_t yy = 0; yy < 2 * m_options.m_buffer + 1; yy++)
							{
								for (size_t xx = 0; xx < 2 * m_options.m_buffer + 1; xx++)
								{
									size_t yyy = y + yy - m_options.m_buffer;
									size_t xxx = x + xx - m_options.m_buffer;
									if (yyy < extents.m_ySize && xxx < extents.m_xSize)
									{
										size_t xy3 = yyy * extents.m_xSize + xxx;
										if (suspects1[zz].test(xy3) || suspects2[zz].test(xy3))
											clouds[zz].set(xy3);
									}
								}//for buffer x
							}//for buffer y 
						}//if cloud
					}//x
				}//y
			}//zz
		}
		//suspects1.clear();

		if (m_options.m_bCreateImage || m_options.m_bDebug)
		{
			if (!m_options.m_bQuiet)
				cout << "Replace clouds..." << endl;

			m_options.ResetBar((size_t)nbScenedProcess*extents.m_xSize*extents.m_ySize);

			//pass 2 : reset or replace clouds
// no omp here
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int thread = omp_get_thread_num();
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				//data
				LansatData data;
				DebugData debug;
				ReadBlock(xBlock, yBlock, bandHolder[thread]);

				if (m_options.m_bCreateImage || m_options.m_bDebug)
				{
					ProcessBlock2(xBlock, yBlock, bandHolder[thread], data, debug, suspects1, suspects2, clouds);
					WriteBlock2(xBlock, yBlock, bandHolder[thread], data, debug, outputDS, debugDS);
				}
			}//for all blocks
		}

		//close inputs and outputs
		CloseAll(lansatDS, maskDS, outputDS, DTCodeDS, debugDS);

		for (size_t zz = 0; zz < clouds.size(); zz++)
			cout << "Cloud for scene " << zz + 1 << ": " << std::fixed << std::setprecision(2) << (double)clouds[zz].count() / clouds[zz].size() * 100.0 << "%" << endl;

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
	for (size_t zz = z - 1; zz < landsat.size() && previous == NOT_INIT; zz--)
		if (landsat[zz].IsValid())
			previous = zz;

	return previous;
}

size_t GetNext(CLandsatPixelVector& landsat, size_t z)
{
	size_t next = NOT_INIT;
	for (size_t zz = z + 1; zz < landsat.size() && next == NOT_INIT; zz++)
		if (landsat[zz].IsValid())
			next = zz;

	return next;
}

array <CLandsatPixel, 3> GetP(size_t z1, CLandsatPixelVector& data)
{
	ASSERT(z1 < data.size());

	array <CLandsatPixel, 3> p;

	p[1] = data[z1];
	size_t z0 = GetPrevious(data, z1);
	p[0] = z0 < data.size() ? data[z0] : NO_PIXEL;
	size_t z2 = GetNext(data, z1);
	p[2] = z2 < data.size() ? data[z2] : NO_PIXEL;

	return p;
}

//Get input image reference
void CCloudCleaner::Preprocess(int xBlock, int yBlock, const CBandsHolder& bandHolder, ForestPtr forests[3], CloudBitset& suspects1, CloudBitset& suspects2)
{
	size_t nbScenesProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect(xBlock, yBlock);
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
	size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;


	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += nbScenesProcess * (std::min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));
		m_options.UpdateBar();

		return;
	}


	//allocate process memory and load data
	LansatData data;
	LoadData(bandHolder, data);

#pragma omp critical(ProcessBlock)
	for (size_t zz = 0; zz < nbScenesProcess; zz++)
	{
		size_t z = m_options.m_scenes[0] + zz;

		for (size_t y = 0; y < blockSize.m_y; y++)
		{
			for (size_t x = 0; x < blockSize.m_x; x++)
			{
				size_t xy = y * blockSize.m_x + x;
				array <CLandsatPixel, 3> p = GetP(z, data[xy]);
				size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;

				if (m_options.IsTrigged(p, CCloudCleanerOption::T_PRIMARY))
				{
					suspects1[zz].set(xy2);

					/*for (size_t yy = 0; yy < 2 * m_options.m_buffer + 1; yy++)
					{
						for (size_t xx = 0; xx < 2 * m_options.m_buffer + 1; xx++)
						{
							size_t yyy = index.m_y + y + yy - m_options.m_buffer;
							size_t xxx = index.m_x + x + xx - m_options.m_buffer;
							if (yyy < extents.m_ySize && xxx < extents.m_xSize)
							{
								if (!suspects2.empty() && m_options.IsTrigged(p, CCloudCleanerOption::T_SECONDARY))
								{
									suspects2[zz].set(xy2);
								}
							}
						}
					}*/

				}//if suspect

				if (!suspects2.empty() &&
					//(z!=0 && (z+1)!=nbScenes) &&
					m_options.IsTrigged(p, CCloudCleanerOption::T_SECONDARY))
				{
					suspects2[zz].set(xy2);
				}

			}//x
		}//y

		m_options.m_xx += (std::min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));
		m_options.UpdateBar();

	}//z
}

//Get input image reference
void CCloudCleaner::ProcessBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, ForestPtr forests[3], RFCodeData& RFcode, CloudBitset& suspects1, CloudBitset& suspects2, CloudBitset& clouds)
{
	size_t nbScenesProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect(xBlock, yBlock);
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
	size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;


	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += nbScenesProcess * (std::min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));
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
			RFcode.resize(nbScenesProcess);
			for (size_t z = 0; z < RFcode.size(); z++)
				RFcode[z].insert(RFcode[z].begin(), blockSize.m_x*blockSize.m_y, (__int16)GetDefaultNoData(GDT_Int16));
		}


		for (size_t zz = 0; zz < nbScenesProcess; zz++)
		{
			size_t z = m_options.m_scenes[0] + zz;

			boost::dynamic_bitset<size_t> suspectPixel((size_t)blockSize.m_x*blockSize.m_y);
			for (size_t y = 0; y < blockSize.m_y; y++)
			{
				for (size_t x = 0; x < blockSize.m_x; x++)
				{
					m_options.m_nbPixel++;
					size_t xy = y * blockSize.m_x + x;
					size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;

					bool b1 = suspects1[zz].test(xy2);
					bool b2 = !suspects2.empty() ? suspects2[zz].test(xy2) : false;
					if (b1 || b2)
					{
						suspectPixel.set(xy);
						m_options.m_nbPixelDT++;
					}
				}
			}

			if (suspectPixel.count() > 0)
			{
				//forest model, 0: beg,1: mid, 2: end
				size_t fm = (z == 0) ? 0 : ((z + 1) == nbScenes) ? 2 : 1;
				size_t nbCols = (fm == 1 ? 3 : 2) * 7;

				DataShort input;
				input.resize(suspectPixel.count(), nbCols);

				size_t cur_xy = 0;
				for (size_t y = 0; y < blockSize.m_y; y++)
				{
					for (size_t x = 0; x < blockSize.m_x; x++)
					{
						size_t xy = y * blockSize.m_x + x;
						if (suspectPixel.test(xy))
						{
							array <CLandsatPixel, 3> p = GetP(z, data[xy]);

							size_t c = 0;
							for (size_t t = 0; t < 3; t++)
							{
								for (size_t b = 0; b < 7; b++)
								{
									bool error = false;
									if (t == 0 && fm > 0)
										input.set(c++, cur_xy, p[t][b], error);
									else if (t == 1)
										input.set(c++, cur_xy, p[t][b], error);
									else if (t == 2 && fm < 2)
										input.set(c++, cur_xy, p[t][b], error);
								}
							}

							ASSERT(c == nbCols);
							cur_xy++;
						}//if suspect
					}//x
				}//y

				forests[fm]->run_predict(&input);

				cur_xy = 0;
				for (size_t y = 0; y < blockSize.m_y; y++)
				{
					for (size_t x = 0; x < blockSize.m_x; x++)
					{
						size_t xy = y * blockSize.m_x + x;
						if (suspectPixel.test(xy))
						{
							int RFexit = int(forests[fm]->getPredictions().at(0).at(0).at(cur_xy));

							if (RFexit > 100)
							{
								size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;
								clouds[zz].set(xy2);
							}//if cloud

							if (!RFcode.empty())
								RFcode[zz][xy] = (__int16)(RFexit);

							cur_xy++;
						}//if suspect
					}//X
				}//y
			}//if have suspect

			m_options.m_xx += (std::min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));
			m_options.UpdateBar();
		}//z

		m_options.m_timerProcess.Stop();
	}//omp
}

void CCloudCleaner::WriteBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, RFCodeData& RFcode, CGDALDatasetEx& DTCodeDS)
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

			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);

			//size_t z1 = m_options.m_scene;
			for (size_t z = 0; z < DTCodeDS.GetRasterCount(); z++)
			{
				ASSERT(RFcode[z].empty() || RFcode[z].size() == outputRect.Width()*outputRect.Height());

				GDALRasterBand *pBand = DTCodeDS.GetRasterBand(z);
				if (!RFcode.empty())
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(RFcode[z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				else
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
			}
		}


	}

	m_options.m_timerWrite.Stop();

}


bool CCloudCleaner::TouchSuspect1(size_t level, const CGeoExtents& extents, CGeoPointIndex xy, const boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& suspects2, boost::dynamic_bitset<size_t>& treated)
{
	ASSERT(!treated.test(xy.m_y * extents.m_xSize + xy.m_x));

	bool bTouch = false;

	
	treated.set(xy.m_y * extents.m_xSize + xy.m_x);
	if (suspects1.test(xy.m_y * extents.m_xSize + xy.m_x))
		bTouch = true;
	
	//on the edge case when suspect pixel touch to cloud 
	for (size_t yy = 0; yy < 3; yy++)
	{
		size_t yyy = xy.m_y + yy - 1;
		if (yyy < extents.m_ySize)
		{
			for (size_t xx = 0; xx < 3; xx++)
			{
				size_t xxx = xy.m_x + xx - 1;
				if (xxx < extents.m_xSize)
				{
					size_t xy2 = yyy * extents.m_xSize + xxx;

					
					if (!treated.test(xy2) && (suspects1.test(xy2) || suspects2.test(xy2)) && level < 100)
					{
						if (TouchSuspect1(level + 1, extents, CGeoPointIndex((int)xxx, (int)yyy), suspects1, suspects2, treated))
							bTouch = true;
					}//not treated
				}//inside extent
			}//for buffer x
		}//inside extent
	}//for buffer y 

	if(!bTouch)
		suspects2.reset(xy.m_y * extents.m_xSize + xy.m_x);

	return bTouch;
}

void CCloudCleaner::CleanSuspect2(const CGeoExtents& extents, const boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& suspects2)
{
	ASSERT(suspects1.size() == suspects2.size());
	boost::dynamic_bitset<size_t> treated(suspects1.size());

	for (size_t y = 0; y < extents.m_ySize; y++)
	{
		for (size_t x = 0; x < extents.m_xSize; x++)
		{
			size_t xy = y * extents.m_xSize + x;
			if (!treated.test(xy) && suspects2.test(xy))
			{
				TouchSuspect1(0, extents, CGeoPointIndex((int)x, (int)y), suspects1, suspects2, treated);
			}
		}
	}//for buffer y 
}


void CCloudCleaner::ProcessBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder, LansatData& data, DebugData& debug, CloudBitset& suspects1, CloudBitset& suspects2, CloudBitset& clouds)
{
	//ASSERT(suspects.size() == clouds.size());

	size_t nbScenesProcess = m_options.m_scenes[1] - m_options.m_scenes[0] + 1;
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
	LansatData dataCopy = data;


	if (m_options.m_bDebug)
	{
		debug.resize(nbScenesProcess*CCloudCleanerOption::NB_DBUG);
		for (size_t i = 0; i < debug.size(); i++)
			debug[i].insert(debug[i].begin(), blockSize.m_x*blockSize.m_y, (__int16)GetDefaultNoData(GDT_Int16));
	}

	vector<vector<size_t>> replacement;
	if (m_options.m_bCreateImage && m_options.m_bFillCloud)
	{
		replacement.resize(nbScenesProcess);
		for (size_t i = 0; i < replacement.size(); i++)
			replacement[i].insert(replacement[i].begin(), blockSize.m_x*blockSize.m_y, NOT_INIT);
	}


	//CloudBitset cloudsCopy = clouds;


#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();



		//if (m_options.m_buffer > 0)
		//{
		//	
		//	//set buffer around cloud
		//	for (size_t zz = 0; zz < nbScenesProcess; zz++)
		//	{
		//		size_t z = m_options.m_scenes[0] + zz;
		//		for (size_t y = 0; y < blockSize.m_y; y++)
		//		{
		//			for (size_t x = 0; x < blockSize.m_x; x++)
		//			{
		//				//size_t xy = (size_t)y*blockSize.m_x + x;
		//				size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;

		//				//he suspicious = 
		//				if (clouds[zz].test(xy2))
		//				{
		//					for (size_t yy = 0; yy < 2 * m_options.m_buffer + 1; yy++)
		//					{
		//						for (size_t xx = 0; xx < 2 * m_options.m_buffer + 1; xx++)
		//						{
		//							size_t yyy = index.m_y + y + yy - m_options.m_buffer;
		//							size_t xxx = index.m_x + x + xx - m_options.m_buffer;
		//							if (yyy < extents.m_ySize && xxx < extents.m_xSize)
		//							{
		//								size_t xy3 = yyy * extents.m_xSize + xxx;
		//								if(suspects1[zz].test(xy3) || suspects2[zz].test(xy3))
		//									cloudsCopy[zz].set(xy3);
		//							}
		//						}//for buffer x
		//					}//for buffer y 
		//				}//if cloud
		//			}//x
		//		}//y
		//	}//zz
		//}

		//process debug and find replacement pixel 
//no ompMP here
		for (size_t zz = 0; zz < nbScenesProcess; zz++)
		{
			size_t z = m_options.m_scenes[0] + zz;
			for (size_t y = 0; y < blockSize.m_y; y++)
			{
				for (size_t x = 0; x < blockSize.m_x; x++)
				{
					size_t xy = (size_t)y*blockSize.m_x + x;
					size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;


					if (!debug.empty())
					{
						array <CLandsatPixel, 3> p = GetP(z, dataCopy[xy]);
						debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DEBUG_ID][xy] = m_options.GetDebugID(p);
						if (!suspects1[zz].test(xy2) && suspects2[zz].test(xy2))
							debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DEBUG_ID][xy] = m_options.GetDebugID(p, CCloudCleanerOption::T_SECONDARY);

						debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DEBUG_B1][xy] = m_options.IsB1Trigged(p);
						debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DEBUG_TCB][xy] = m_options.IsTCBTrigged(p);
						debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DEBUG_ZSW][xy] = m_options.IsZSWTrigged(p);
						debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_NB_SCENE][xy] = (p[0].IsInit() ? 1 : 0) + (p[1].IsInit() ? 1 : 0) + (p[2].IsInit() ? 1 : 0);
					}

					if (!dataCopy[xy][z].IsInit() || clouds[zz].test(xy2))
					{
						size_t z2 = NOT_INIT;
						for (size_t i = 0; i < dataCopy[xy].size() * 2 && z2 == NOT_INIT; i++)
						{
							size_t iz = size_t(z + (int)pow(-1, i)*(i / 2 + 1));
							if (iz < dataCopy[xy].size() && dataCopy[xy][iz].IsInit())
							{
								size_t izz = size_t(zz + (int)pow(-1, i)*(i / 2 + 1));
								//bool bReplace = (izz>0 && (izz+1) < clouds.size() )? !(clouds[izz].test(xy2) && suspects1[izz].test(xy2)) : true;
								bool bReplace = (izz < clouds.size()) ? !(clouds[izz].test(xy2) && suspects1[izz].test(xy2)) : true;
								if (bReplace)
									z2 = iz;
							}
						}

						//if z2 is not init
						if (z2 == NOT_INIT)
						{
							__int32 minVal = 32767;
							for (size_t i = 0; i < dataCopy[xy].size() * 2; i++)
							{
								size_t iz = size_t(z + (int)pow(-1, i)*(i / 2 + 1));
								if (iz < dataCopy[xy].size() && dataCopy[xy][iz].IsInit())
								{
									size_t izz = size_t(zz + (int)pow(-1, i)*(i / 2 + 1));
									array <CLandsatPixel, 3> p = GetP(izz, dataCopy[xy]);
									if (m_options.GetTrigger(p) < minVal)
									{
										minVal = m_options.GetTrigger(p);
										z2 = iz;
									}
								}
							}
						}



						if (!debug.empty())
						{
							debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_SCENE_USED][xy] = (z2 != NOT_INIT) ? (__int16)(z2 - z) : -999;
						}

						data[xy][z].Reset();

						if (!replacement.empty())
							replacement[zz][xy] = z2;
					}
					m_options.m_xx++;
				}//x
			}//y
			m_options.UpdateBar();
		}// z

		//replace cloud by valid pixel for all x and y 
		for (size_t zz = 0; zz < replacement.size(); zz++)
		{
			size_t z = m_options.m_scenes[0] + zz;
			for (size_t xy = 0; xy < replacement[zz].size(); xy++)
			{
				if (replacement[zz][xy] != NOT_INIT)
				{
					data[xy][z] = dataCopy[xy][replacement[zz][xy]];
				}
			}//for xy
		}//z


		m_options.m_timerProcess.Stop();
	}//critical
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
			ASSERT(outputDS.GetRasterCount() % SCENES_SIZE == 0);

			__int16 noData = (__int16)outputDS.GetNoData(0);
			vector<__int16> tmp(blockSize.m_x*blockSize.m_y, noData);

			//size_t z1 = m_options.m_scene;
			for (size_t bb = 0; bb < outputDS.GetRasterCount(); bb++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(bb);

				if (!data.empty())
				{
					ASSERT(data.size() == blockSize.m_x*blockSize.m_y);

					size_t z = m_options.m_scenes[0] + bb / SCENES_SIZE;
					size_t b = bb % SCENES_SIZE;
					for (int y = 0; y < blockSize.m_y; y++)
					{
						for (int x = 0; x < blockSize.m_x; x++)
						{
							int xy = int(y*blockSize.m_x + x);
							tmp[xy] = data[xy][z][b];
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
			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);

			for (size_t b = 0; b < debugDS.GetRasterCount(); b++)
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
	if (!m_options.m_bQuiet)
		_tprintf("\nClose all files...\n");

	landsatDS.Close();
	maskDS.Close();


	m_options.m_timerWrite.Start();

	outputDS.Close(m_options);
	DTCodeDS.Close(m_options);
	debugDS.Close(m_options);


	m_options.m_timerWrite.Stop();

	if (!m_options.m_bQuiet)
	{
		double percent = m_options.m_nbPixel > 0 ? (double)m_options.m_nbPixelDT / m_options.m_nbPixel * 100 : 0;

		_tprintf("\n");
		_tprintf("Percentage of pixel treated by Ranger: %0.3lf %%\n\n", percent);
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
	for (int x = 0; x < blockSize.m_x; x++)
	{
		for (int y = 0; y < blockSize.m_y; y++)
		{
			data[y*blockSize.m_x + x].resize(nbScenes);

			for (size_t z = 0; z < nbScenes; z++)
			{
				data[y*blockSize.m_x + x][z] = ((CLandsatWindow&)window).GetPixel(z, x, y);
			}
		}
	}
}




__int32 CCloudCleanerOption::GetB1Trigger(std::array <CLandsatPixel, 3>& p, size_t t)
{
	if (!p[1].IsInit())
		return 32767;

	if (!p[0].IsInit() && !p[2].IsInit())
		return 32767;

	__int32 t1 = p[0].IsInit() ? max(0, m_B1threshold[t] - (p[0][Landsat::B1] - p[1][Landsat::B1])) : 0;
	__int32 t2 = p[2].IsInit() ? max(0, m_B1threshold[t] - (p[2][Landsat::B1] - p[1][Landsat::B1])) : 0;
	__int32 t3 = (p[0].IsInit() ? 1 : 0) + (p[2].IsInit() ? 1 : 0);

	return (t1 + t2) / t3;
}

__int32 CCloudCleanerOption::GetTCBTrigger(std::array <CLandsatPixel, 3>& p, size_t t)
{
	if (!p[1].IsInit())
		return 32767;

	if (!p[0].IsInit() && !p[2].IsInit())
		return 32767;

	__int32 t1 = p[0].IsInit() ? max(0, (p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB]) - m_TCBthreshold[t]) : 0;
	__int32 t2 = p[2].IsInit() ? max(0, (p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB]) - m_TCBthreshold[t]) : 0;
	__int32 t3 = (p[0].IsInit() ? 1 : 0) + (p[2].IsInit() ? 1 : 0);


	return (t1 + t2) / t3;
}

__int32 CCloudCleanerOption::GetZSWTrigger(std::array <CLandsatPixel, 3>& p, size_t t)
{
	if (!p[1].IsInit())
		return 32767;

	if (!p[0].IsInit() && !p[2].IsInit())
		return 32767;

	__int32 t1 = p[0].IsInit() ? max(0, (p[0][Landsat::I_ZSW] - p[1][Landsat::I_ZSW]) - m_ZSWthreshold[t]) : 0;
	__int32 t2 = p[2].IsInit() ? max(0, (p[2][Landsat::I_ZSW] - p[1][Landsat::I_ZSW]) - m_ZSWthreshold[t]) : 0;
	__int32 t3 = (p[0].IsInit() ? 1 : 0) + (p[2].IsInit() ? 1 : 0);

	return (t1 + t2) / t3;
}