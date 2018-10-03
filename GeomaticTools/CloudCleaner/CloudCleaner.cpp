//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 2.1.5    03/10/2018	Rémi Saint-Amsant	try ranger optimization
// 2.1.4    02/10/2018	Rémi Saint-Amsant	Add BLOCK_THREADS options
// 2.1.3    29/09/2018	Rémi Saint-Amsant	Add -PeriodTreated,-Rename and -OutputJD and many optimization.
// 2.1.2	26/09/2018	Rémi Saint-Amant	Bug correction in seive. change in secondary definition.
// 2.1.1	25/09/2018	Rémi Saint-Amant	replace buffer and bufferEx. Separate cloud and shadow suspect pixel, 
//											change in secondary definition.
// 2.1.0	20/09/2018  Rémi Saint-Amant	Add the use of a reference image
// 2.0.7	07/09/2018	Rémi Saint-Amant	Bug correctionn in flush cache
// 2.0.6    06/09/2018	Rémi Saint-Amant	Optimization of clear suspicious and LoadData. correction of memory leak in Ranger. 
// 2.0.5    05/09/2018	Rémi Saint-Amant	Bug correction in sieve (stack overflow)
// 2.0.4	04/07/2018	Rémi Saint-Amant	Add sieve and some debug layers
// 2.0.3	26/06/2018	Rémi Saint-Amant	correction in help
// 2.0.2	06/06/2018	Rémi Saint-Amant	add virtual columns to the model
// 2.0.1    17/05/2018	Rémi Saint-Amant	Use 3 random forest model
// 2.0.0    17/05/2018	Rémi Saint-Amant	Use random forest model with ranger
// 1.0.6	21/12/2017	Rémi Saint-Amant	Bug correction in data layer
// 1.0.5	13/12/2017	Rémi Saint-Amant	Add debug layer for B1 and TCB. Add TZW.
// 1.0.4	13/12/2017	Rémi Saint-Amant	bug correction for big images
// 1.0.3	08/12/2017	Rémi Saint-Amant	bug correction for big images
// 1.0.2	15/11/2017	Rémi Saint-Amant	Add debug for buffer
// 1.0.1	14/11/2017	Rémi Saint-Amant	Output only one scene at a time. Add buffer and MaxScene options
// 1.0.0	31/10/2017	Rémi Saint-Amant	Creation

//-te 1622910 6987120 1623120 6987330 
//-te 1622590 6987810 1622890 6988110 
//-te 1361820 6825390 1361970 6825540 

//tes v2
//-RGB Natural -debug -outputcode -multi -co "compress=LZW" -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite "D:\Travaux\CloudCleaner\Model\RF_v1" "D:\Travaux\CloudCleaner\Input\Nuage_1999-2014.vrt" "D:\Travaux\CloudCleaner\Output\Nuage_1999-2014\NuageOut.vrt"
//-RGB Natural -debug -outputcode -multi -co "tiled=YES" -co "BLOCKXSIZE=512" -co "BLOCKYSIZE=512" -co "compress=LZW" -RGB Natural -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite "D:\Travaux\CloudCleaner\Model\RF_v2" "D:\Travaux\CloudCleaner\Input\haze_et_ombre.vrt" "D:\Travaux\CloudCleaner\Output\haze_et_ombre\haze_et_ombre.vrt"
//-RGB Natural -debug -outputcode -multi -co "tiled=YES" -co "BLOCKXSIZE=512" -co "BLOCKYSIZE=512" -co "compress=LZW" -RGB Natural -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite "D:\Travaux\CloudCleaner\Model\RF_v2" "D:\Travaux\CloudCleaner\Input\puff_partout.vrt" "D:\Travaux\CloudCleaner\Output\puff_partout\puff_partout.vrt"
//-RGB Natural -debug -outputcode -multi -stats -NoResult -of VRT -co "compress=LZW" --config GDAL_CACHEMAX 4096  -overview {2,4,8,16} -multi -overwrite "D:\Travaux\CloudCleaner\Model\RF_v1" "D:\Travaux\CloudCleaner\Input\34 ans\Te1.vrt" "D:\Travaux\CloudCleaner\Output\34 ans\Te1_out.vrt"

//-period 2001-01-01 2009-12-31 -PeriodTreated 2002-01-01 2008-12-31 -rename %F -of VRT -outputcode -NoResult -multi -IOCPU 6 -CPU 6 -co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -co "compress=LZW" --config GDAL_CACHEMAX 4096 -stats -overview {4,8,16} -overwrite "D:\Travaux\CloudCleaner\Model\RF_Remi.csv" "C:\i subset\i_subset.vrt" "D:\Travaux\CloudCleaner\Output\i\i.vrt"




#include "stdafx.h"
#include "VisualLeakDetector\include\vld.h"

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

#include "windows.h"
#include "psapi.h"



using namespace std;
using namespace WBSF;
using namespace WBSF::Landsat;



static const char* version = "2.1.5";
//static const int NB_THREAD_PROCESS = 2;
static const __int16 NOT_TRIGGED_CODE = (__int16)::GetDefaultNoData(GDT_Int16);
static const CLandsatPixel NO_PIXEL;
const char* CCloudCleanerOption::DEBUG_NAME[NB_DBUG] = { "_flag","_nbScenes", "_fill", "_model", "_delta_B1", "_delta_TCB" };


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
	m_bOutputCode = false;
	//	m_bOutputJD = false;
	m_B1threshold = { -175, -5, 250 };
	m_TCBthreshold = { 600, 100, 2400 };
	m_bFillClouds = false;
	m_bFillMissing = false;
	m_bUseMedian = false;
	m_bVerifyDoubleCloud = true;

	m_buffer = 7;
	m_bufferEx = { {2,1} };
	m_scenesTreated = { {NOT_INIT, NOT_INIT } };
	m_sieve = 5;

	m_appDescription = "This software look up for cloud from Landsat images series (composed of " + to_string(SCENES_SIZE) + " bands) with a random forest model (from Ranger)";



	AddOption("-RGB");

	static const COptionDef OPTIONS[] =
	{
		{ "-Thres", 3, "type B1 TCB", false, "Set trigger threshold for B1 and TCB. The trigger threshold identify suspect pixels to be evaluated by Random Forest. Type 1 is for primary and 2 is for secondary. Primary test pixel with previous, next and median. Secondary will also test pixel in absolute value. Default value are -175 600 for primary and -5 100 for secondary." },
		{ "-FillClouds", 0, "", false, "Fill clouds with next or previous valid scenes (+1,-1,+2,-2,...). Only reset by default. Use NoResult to avoid output result." },
		{ "-FillMissing", 0, "", false, "Fill also missing pixels when -FillClouds is activated." },
		{ "-Scenes", 2, "first last", false, "Select a first and the last scene (1..nbScenes) to clean cloud. All scenes are selected by default." },
		{ "-PeriodTreated", 2, "Begin End", false, "Same as scenes. Select the period (YYYY-MM-DD YYYY-MM-DD) to clean cloud. The entire period are selected by default. Cannot be used with -scenes." },
		{ "-Buffer", 1, "nbPixels", false, "Set maximum buffer distance around primary suspicious pixels to keep secondary suspicious pixels. All secondary pixel farther thant tnis maximum buffer will be reset. 7 by default." },
		{ "-BufferEx", 2, "primary secondary", false, "Set all suspicious pixels arround cloud pixels as cloud. 0 by default." },
		//{ "-MedianFile", 1, "refImage", false, "Use median image file instead of computing medain on the fly." },
		{ "-Sieve", 1, "nbPixel", false, "Set the minimum number of contigious pixel to consider it as suspicious. 5 by default." },
		{ "-OutputCode", 0, "", false, "Output random forest result code." },
		//{ "-OutputJD", 0, "", false, "Output JD layer with cloud and shadow replace by no data." },
		{ "-Debug",0,"",false,"Output debug information."},
		{ "-Rename", 1, "format", false, "Add at the end of output file, the mean image date. See strftime for option. %F for YYYY-MM-DD. Use %J for julian day since 1970 and %P for path/row." },//overide scene size defenition
		{ "Model", 0, "", false, "Random forest cloud model file path." },
		{ "srcfile", 0, "", false, "Input LANDSAT scenes image file path." },
		{ "dstfile", 0, "", false, "Output LANDSAT scenes image file path." }
	};

	for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
		AddOption(OPTIONS[i]);

	AddOption("-Period");
	static const CIOFileInfoDef IO_FILE_INFO[] =
	{
		{ "Input Model", "Model", "", "", "", "Random forest model file generated by Ranger." },
		{ "LANDSAT Image", "src1file", "NbScenes", "ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Date(Julian day 1970)|... for each scene" },
		{ "Output Image", "dstfile", "Nb scenes processed", "ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Date(Julian day 1970)" },
		{ "Optionnal Reference Image", "refImage", "Nb references", "ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Date(Julian day 1970)" },
		{ "Optional Code Image", "dstfile_code","Nb scenes processed","1","random forest result"},
		{ "Optional Debug Image", "dstfile_debug", "Nb scenes processed", "6", "Bit fields [secondary(4)|TCB(2)|B1(1)]||Nb scene|scene selected to fill cloud (relative to the filled scene)"}

	};

	for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
		AddIOFileInfo(IO_FILE_INFO[i]);

}

ERMsg CCloudCleanerOption::ProcessOption(int& i, int argc, char* argv[])
{
	ERMsg msg;


	if (IsEqual(argv[i], "-Thres"))
	{
		size_t type = atoi(argv[++i]) - 1;
		if (type < m_B1threshold.size())
		{
			m_B1threshold[type] = atoi(argv[++i]);
			m_TCBthreshold[type] = atoi(argv[++i]);
		}
		else
		{
			msg.ajoute("invalide threshold type. type must be 1, 2 or 3");
		}
	}
	else if (IsEqual(argv[i], "-FillClouds"))
	{
		m_bFillClouds = true;
	}
	else if (IsEqual(argv[i], "-FillMissing"))
	{
		m_bFillMissing = true;
	}
	else if (IsEqual(argv[i], "-Buffer"))
	{
		m_buffer = atoi(argv[++i]);
	}
	else if (IsEqual(argv[i], "-Sieve"))
	{
		m_sieve = atoi(argv[++i]);
	}
	else if (IsEqual(argv[i], "-BufferEx"))
	{
		m_bufferEx[0] = atoi(argv[++i]);
		m_bufferEx[1] = atoi(argv[++i]);
	}
	else if (IsEqual(argv[i], "-Scenes"))
	{
		m_scenesTreated[0] = atoi(argv[++i]) - 1;
		m_scenesTreated[1] = atoi(argv[++i]) - 1;
	}
	else if (IsEqual(argv[i], "-PeriodTreated"))
	{
		m_periodTreated.Begin().FromFormatedString(argv[++i], "", "-", 1);//in 1 base
		m_periodTreated.End().FromFormatedString(argv[++i], "", "-", 1);
	}
	else if (IsEqual(argv[i], "-OutputCode"))
	{
		m_bOutputCode = true;
	}
	/*else if (IsEqual(argv[i], "-OutputJD"))
	{
		m_bOutputJD = true;
	}*/
	else if (IsEqual(argv[i], "-Debug"))
	{
		m_bDebug = true;
	}
	else if (IsEqual(argv[i], "-UseMedian"))
	{
		m_refFilePath = argv[++i];
	}
	else if (IsEqual(argv[i], "-Rename"))
	{
		m_rename = argv[++i];
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
		msg.ajoute("ERROR: Invalid argument line. 3 files are needed: the random forest models, the LANDSAT images series and the destination image.");
		msg.ajoute("Argument found: ");
		for (size_t i = 0; i < m_filesPath.size(); i++)
			msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
	}

	if (m_outputType == GDT_Unknown)
		m_outputType = GDT_Int16;

	if (m_outputType != GDT_Int16 && m_outputType != GDT_Int32)
		msg.ajoute("Invalid -ot option. Only GDT_Int16 or GDT_Int32 are supported");

	if ((m_scenesTreated[0] != NOT_INIT || m_scenesTreated[1] != NOT_INIT) && m_periodTreated.IsInit())
	{
		msg.ajoute("-Scene option can't be use with option -PeriodTrait");
	}
	//if (m_maxScene < 1)
		//msg.ajoute("Invalid -MaxScene. -FillMaxScene must be greater than 1.");

	//m_CPU = max(1, m_CPU/m_BLOCK_THREADS);

	return msg;
}


//***********************************************************************


ERMsg CCloudCleaner::ReadModel(std::string filePath, int CPU, ForestPtr& forest, bool bQuit)
{
	ERMsg msg;

	TreeType treetype = GetTreeType(filePath);

	try
	{
		forest.reset(CreateForest(treetype));
		forest->init_predict(0, CPU, false, DEFAULT_PREDICTIONTYPE);
		forest->loadFromFile(filePath);
		if (!bQuit)
		{
			cout << "Forest name:                       " << GetFileTitle(GetFileTitle(filePath)) << std::endl;
			cout << "Forest type:                       " << GetTreeTypeStr(treetype) << std::endl;
			cout << "Number of trees:                   " << forest->getNumTrees() << std::endl;
			cout << "Dependent variable column:         " << forest->getDependentVarId() + 1 << std::endl;
			cout << "Number of input variables:         " << forest->getNumIndependentVariables() - forest->get_virtual_cols_name().size() << std::endl;
			cout << "Number of virtual variables:       " << forest->get_virtual_cols_name().size() << std::endl;
			cout << "Number of independent variables:   " << forest->getNumIndependentVariables() << std::endl;

			cout << std::endl;
		}
	}
	catch (std::exception e)
	{
		msg.ajoute(e.what());
	}

	return msg;
}

ERMsg CCloudCleaner::ReadModel(Forests3MT& forests)
{
	ERMsg msg;

	CTimer timer(true);

	if (!m_options.m_bQuiet)
		cout << "Read forest..." << endl;

	forests.resize(m_options.m_BLOCK_THREADS);
	for (size_t t = 0; t < forests.size(); t++)
	{
		static const char* EXTRA[3] = { "START","MIDDLE","END" };
		for (size_t m = 0; m < 3; m++)
		{
			string filePath = m_options.m_filesPath[CCloudCleanerOption::RF_MODEL_FILE_PATH];
			SetFileName(filePath, GetFileTitle(filePath) + "_" + EXTRA[m] + ".classification.forest");
			msg += ReadModel(filePath, m_options.m_bMulti ? m_options.BLOCK_CPU() : -1, forests[t][m], t != 0);
			//msg += ReadModel(filePath, -1, forests[t][f], t != 0);
			//msg += ReadModel(filePath, m_options.m_bMulti ? m_options.m_CPU : -1, forests[t][f], t != 0);
		}
	}

	timer.Stop();

	if (!m_options.m_bQuiet)
		cout << "Read forest time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;

	return msg;
}

ERMsg CCloudCleaner::OpenAll(CLandsatDataset& landsatDS, CGDALDatasetEx& maskDS, CLandsatDataset& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS/*, CGDALDatasetEx& JDDS*/)
{
	ERMsg msg;



	if (!m_options.m_bQuiet)
	{
		cout << endl << "Open input image..." << endl;
	}



	msg = landsatDS.OpenInputImage(m_options.m_filesPath[CCloudCleanerOption::LANDSAT_FILE_PATH], m_options);

	if (msg && landsatDS.GetNbScenes() < 3)
		msg.ajoute("CloudCleaner need at least 3 scenes");

	if (msg)
		landsatDS.UpdateOption(m_options);

	if (msg)
	{
		const std::vector<CTPeriod>& scenesPeriod = landsatDS.GetScenePeriod();
		CTPeriod p = m_options.m_period;//period to load
		set<size_t> toLoad;

		for (size_t s = 0; s < landsatDS.GetNbScenes(); s++)
		{
			if (p.IsIntersect(scenesPeriod[s]))
				toLoad.insert(s);
		}

		size_t nb_input_scenes = 0;
		if (!toLoad.empty())
		{
			m_options.m_scenesLoaded[0] = *toLoad.begin();
			m_options.m_scenesLoaded[1] = *toLoad.rbegin();
		}

		if (m_options.m_periodTreated.IsInit())
		{
			const std::vector<CTPeriod>& scenesPeriod = landsatDS.GetScenePeriod();
			CTPeriod p = m_options.m_periodTreated;

			set<size_t> selected;

			for (size_t s = 0; s < scenesPeriod.size(); s++)
			{
				if (s > 1 && (scenesPeriod[s].End() < scenesPeriod[s - 1].Begin()))
					msg.ajoute("Scenes of the input landsat images (" + GetFileName(m_options.m_filesPath[CCloudCleanerOption::LANDSAT_FILE_PATH]) + ") are not chronologically ordered.");

				if (p.IsIntersect(scenesPeriod[s]))
					selected.insert(s);
			}

			if (!selected.empty())
			{
				m_options.m_scenesTreated[0] = *selected.begin();
				m_options.m_scenesTreated[1] = *selected.rbegin();
			}
			else
			{
				msg.ajoute("No input scenes intersect -PeriodTrait (" + m_options.m_periodTreated.GetFormatedString("%1 %2", "%F") + ").");
			}
		}


		if (m_options.m_scenesTreated[0] == NOT_INIT)
			m_options.m_scenesTreated[0] = 0;

		if (m_options.m_scenesTreated[1] == NOT_INIT)
			m_options.m_scenesTreated[1] = landsatDS.GetNbScenes() - 1;

		if (m_options.m_scenesTreated[0] >= landsatDS.GetNbScenes() || m_options.m_scenesTreated[1] >= landsatDS.GetNbScenes())
			msg.ajoute("Scenes {" + to_string(m_options.m_scenesTreated[0] + 1) + ", " + to_string(m_options.m_scenesTreated[1] + 1) + "} must be in range {1, " + to_string(landsatDS.GetNbScenes()) + "}");

		if (m_options.m_scenesTreated[0] > m_options.m_scenesTreated[1])
			msg.ajoute("First scene (" + to_string(m_options.m_scenesTreated[0] + 1) + ") must be smaller or equal to the last scene (" + to_string(m_options.m_scenesTreated[1] + 1) + ")");

	}

	if (!msg)
		return msg;

	//update period from scene
	size_t nbSceneLoaded = m_options.m_scenesLoaded[1] - m_options.m_scenesLoaded[0] + 1;
	size_t nbScenedProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;



	CTPeriod loadedPeriod;
	CTPeriod processPeriod;
	const std::vector<CTPeriod>& p = landsatDS.GetScenePeriod();

	ASSERT(m_options.m_scenesTreated[0] < p.size());
	ASSERT(m_options.m_scenesTreated[1] < p.size());

	for (size_t i = 0; i < p.size(); i++)
	{
		if (i >= m_options.m_scenesLoaded[0] && i <= m_options.m_scenesLoaded[1])
			loadedPeriod += p[i];

		if (i >= m_options.m_scenesTreated[0] && i <= m_options.m_scenesTreated[1])
			processPeriod += p[i];
	}

	if (!m_options.m_bQuiet)
	{
		CGeoExtents extents = landsatDS.GetExtents();
		CProjectionPtr pPrj = landsatDS.GetPrj();
		string prjName = pPrj ? pPrj->GetName() : "Unknown";

		if (m_options.m_period.IsInit())
			cout << "    User's Input loading period  = " << m_options.m_period.GetFormatedString() << endl;

		if (m_options.m_periodTreated.IsInit())
			cout << "    User's Input treating period = " << m_options.m_periodTreated.GetFormatedString() << endl;

		cout << "    Size           = " << landsatDS->GetRasterXSize() << " cols x " << landsatDS->GetRasterYSize() << " rows x " << landsatDS.GetRasterCount() << " bands" << endl;
		cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
		cout << "    Projection     = " << prjName << endl;
		cout << "    NbBands        = " << landsatDS.GetRasterCount() << endl;
		cout << "    Scene size     = " << landsatDS.GetSceneSize() << endl;
		//		cout << "    Nb. Scenes     = " << landsatDS.GetNbScenes() << endl;
		cout << "    Entire period  = " << landsatDS.GetPeriod().GetFormatedString() << " (nb scenes = " << landsatDS.GetNbScenes() << ")" << endl;
		cout << "    Loaded period  = " << loadedPeriod.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;
		cout << "    Treated period = " << processPeriod.GetFormatedString() << " (nb scenes = " << nbScenedProcess << ")" << endl;
	}



	if (!m_options.m_period.IsIntersect(processPeriod))
		msg.ajoute("Input period and process perid does not intersect. Verify period or scenes options");


	if (msg && !m_options.m_maskName.empty())
	{
		if (!m_options.m_bQuiet)
			cout << "Open mask image..." << endl;

		msg += maskDS.OpenInputImage(m_options.m_maskName);
	}

	//if (msg && !m_options.m_refFilePath.empty())
	//{
	//	if (!m_options.m_bQuiet)
	//		cout << endl << "Open references..." << endl;

	//	msg += refDS.OpenInputImage(m_options.m_refFilePath, m_options);
	//}



	if (msg && m_options.m_bCreateImage)
	{
		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images " << " x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << m_options.m_nbBands << " bands) with " << m_options.m_CPU << " threads..." << endl;


		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		CCloudCleanerOption options = m_options;
		options.m_nbBands = nbScenedProcess * landsatDS.GetSceneSize();

		set<string> subnames;
		for (size_t zz = 0; zz < nbScenedProcess; zz++)
		{
			size_t z = m_options.m_scenesTreated[0] + zz;
			string subName = landsatDS.GetSubname(z, m_options.m_rename);

			string uniqueSubName = subName;
			size_t i = 1;
			while (subnames.find(uniqueSubName) != subnames.end())
				uniqueSubName = subName + "_" + to_string(++i);

			subnames.insert(uniqueSubName);

			for (size_t b = 0; b < SCENES_SIZE; b++)
			{
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + uniqueSubName + "_" + Landsat::GetBandName(b) + ".tif|";
			}
		}

		msg += outputDS.CreateImage(filePath, options);
	}


	if (msg && m_options.m_bOutputCode)
	{
		if (!m_options.m_bQuiet)
			cout << "Create Ranger code image..." << endl;

		CCloudCleanerOption options = m_options;
		options.m_outputType = GDT_Int16;
		options.m_nbBands = nbScenedProcess;
		options.m_dstNodata = (__int16)::GetDefaultNoData(GDT_Int16);

		string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
		SetFileTitle(filePath, GetFileTitle(filePath) + "_code");

		//replace the common part by the new name
		set<string> subnames;
		for (size_t zz = 0; zz < nbScenedProcess; zz++)
		{
			size_t z = m_options.m_scenesTreated[0] + zz;
			string subName = landsatDS.GetSubname(z, m_options.m_rename);
			string uniqueSubName = subName;
			size_t i = 1;
			while (subnames.find(uniqueSubName) != subnames.end())
				uniqueSubName = subName + "_" + to_string(++i);

			subnames.insert(uniqueSubName);
			options.m_VRTBandsName += GetFileTitle(filePath) + "_" + uniqueSubName + ".tif|";
		}

		msg += DTCodeDS.CreateImage(filePath, options);
	}

	//if (msg && m_options.m_bOutputJD && !m_options.m_bCreateImage)
	//{
	//	if (!m_options.m_bQuiet)
	//		cout << "Create output JD image..." << endl;

	//	CCloudCleanerOption options = m_options;
	//	options.m_outputType = GDT_Int16;
	//	options.m_nbBands = nbScenedProcess;
	//	options.m_dstNodata = (__int16)::GetDefaultNoData(GDT_Int16);

	//	string filePath = m_options.m_filesPath[CCloudCleanerOption::OUTPUT_FILE_PATH];
	//	

	//	//replace the common part by the new name
	//	for (size_t zz = 0; zz < nbScenedProcess; zz++)
	//	{
	//		size_t z = m_options.m_scenesTreated[0] + zz;
	//		string subName = landsatDS.GetSubname(z, m_options.m_rename);

	//		options.m_VRTBandsName += GetFileTitle(filePath) + "_" + subName + "_" + GetBandName(JD) + ".tif|";
	//	}

	//	msg += JDDS.CreateImage(filePath, options);
	//}

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
		set<string> subnames;
		for (size_t zz = 0; zz < nbScenedProcess; zz++)
		{
			size_t z = m_options.m_scenesTreated[0] + zz;
			string subName = landsatDS.GetSubname(z, m_options.m_rename);
			string uniqueSubName = subName;
			size_t i = 1;
			while (subnames.find(uniqueSubName) != subnames.end())
				uniqueSubName = subName + "_" + to_string(++i);

			subnames.insert(uniqueSubName);
			for (size_t j = 0; j < CCloudCleanerOption::NB_DBUG; j++)
			{
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + uniqueSubName + CCloudCleanerOption::DEBUG_NAME[j] + ".tif|";
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
		cout << "Using:  " << m_options.m_filesPath[CCloudCleanerOption::RF_MODEL_FILE_PATH] << endl;

		if (!m_options.m_maskName.empty())
			cout << "Mask:   " << m_options.m_maskName << endl;
	}

	GDALAllRegister();

	Forests3MT forests;
	msg += ReadModel(forests);

	if (!msg)
		return msg;

	CLandsatDataset inputDS;
	CGDALDatasetEx maskDS;
	CLandsatDataset outputDS;
	CGDALDatasetEx DTCodeDS;
	CGDALDatasetEx debugDS;
	//CGDALDatasetEx& JDDS;

	msg = OpenAll(inputDS, maskDS, outputDS, DTCodeDS, debugDS/*, JDDS*/);

	if (msg)
	{
		size_t iv = forests[0][1]->getNumIndependentVariables() - forests[0][1]->get_virtual_cols_name().size();
		if (iv == 28)
			m_options.m_bUseMedian = true;


		if (iv != 21 && iv != 28)
		{
			msg.ajoute("Bad Random Forest models. Number of independant variable (" + to_string(iv) + ") excluding virtual vars (" + to_string(forests[0][0]->get_virtual_cols_name().size()) + ") must be 21 or 28 (when median is used).");
		}

		/*if (forests[0][0]->getNumIndependentVariables() != forests[0][1]->getNumIndependentVariables() ||
			forests[0][1]->getNumIndependentVariables() != forests[0][2]->getNumIndependentVariables())
		{
			msg.ajoute("Bad Random Forest models. All model must have the same number of independant variable. Ie 21 or 28 fro model with median.");
		}*/
	}

	if (msg)
	{
		//size_t nbScenes = inputDS.GetNbScenes();
		//size_t sceneSize = inputDS.GetSceneSize();
		size_t nbScenedProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;
		size_t nbScenedLoaded = m_options.m_scenesLoaded[1] - m_options.m_scenesLoaded[0] + 1;


		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);

		if (maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if (!msg)
			return msg;

		CGeoExtents extents = bandHolder.GetExtents();
		std::vector<CTPeriod> scenesPeriod = inputDS.GetScenePeriod();
		string date_format = GetScenesDateFormat(scenesPeriod);

		SuspectBitset suspects1(nbScenedProcess);
		SuspectBitset suspects2(nbScenedProcess);
		CloudBitset clouds(nbScenedProcess);

		for (size_t i = 0; i < nbScenedProcess; i++)
		{
			for (size_t ii = 0; ii < 2; ii++)
			{
				suspects1[i][ii].resize((size_t)extents.m_xSize*extents.m_ySize);
				suspects2[i][ii].resize((size_t)extents.m_xSize*extents.m_ySize);
				clouds[i][ii].resize((size_t)extents.m_xSize*extents.m_ySize);
			}
		}

		//		cout << "random fill..." << endl;
		//		m_options.ResetBar((size_t)nbScenedProcess*extents.m_xSize*extents.m_ySize);
		//				
		//		CRandomGenerator RG;
		//		for (__int64 b = 0; b < (__int64)nbScenedProcess; ++b)
		//		{
		//			for (size_t xy = 0; xy < (size_t)extents.m_xSize*extents.m_ySize; ++xy)
		//			{
		//				suspects1[b][0].set(xy, RG.Randu(0, 1) > 0.95);
		//				suspects1[b][1].set(xy, RG.Randu(0, 1) > 0.99);
		//				suspects2[b][0].set(xy, RG.Randu(0, 1) > 0.90);
		//				suspects2[b][1].set(xy, RG.Randu(0, 1) > 0.95);
		//#pragma omp atomic		
		//				m_options.m_xx++;
		//			}
		//		
		//			m_options.UpdateBar();
		//		}

		PROCESS_MEMORY_COUNTERS memCounter;
		if (!m_options.m_bQuiet)
		{
			GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
			cout << "Memory used: " << memCounter.WorkingSetSize / (1024 * 1024) << " Mo" << endl;
		}

		vector<pair<int, int>> XYindex = extents.GetBlockList();

		if (!m_options.m_bQuiet)
			cout << "Find suspicious pixels with " << m_options.m_BLOCK_THREADS << " block threads (" << m_options.BLOCK_CPU() << " threads/block)" << endl;

		CTimer timer(true);
		size_t nbPixels = extents.m_xSize*extents.m_ySize;// min(extents.m_xSize*extents.m_ySize, extents.m_xBlockSize*extents.m_yBlockSize);
		m_options.ResetBar((size_t)nbScenedProcess*nbPixels + nbScenedLoaded * nbPixels);
		
		size_t nbPixelsTest1 = 0;
		size_t nbPixelsTest2 = 0;
		for (__int64 b = 0; b < (__int64)XYindex.size(); b++)
		{
			size_t xBlock = XYindex[b].first;
			size_t yBlock = XYindex[b].second;

			CGeoRectIndex index = extents.GetBlockRect((int)xBlock, (int)yBlock);
			CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);

			nbPixelsTest1 += index.m_xSize*index.m_ySize;
			nbPixelsTest2 += blockSize.m_x*blockSize.m_y;

		}
		cout << "nb pixels =" << nbPixels << endl;
		cout << "nb pixels =" << nbPixelsTest1 << endl;
		cout << "nb pixels =" << nbPixelsTest2 << endl;

		
		//pass 1 : find suspicious pixel
		omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(m_options.m_BLOCK_THREADS) if (m_options.m_bMulti)
		for (__int64 b = 0; b < (__int64)XYindex.size(); b++)
		{
			size_t thread = omp_get_thread_num();
			size_t xBlock = XYindex[b].first;
			size_t yBlock = XYindex[b].second;

			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			FindSuspicious(xBlock, yBlock, bandHolder[thread], suspects1, suspects2);

		}//for all blocks

		timer.Stop();
		if (!m_options.m_bQuiet)
		{
			GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
			cout << "Memory used: " << memCounter.WorkingSetSize / (1024 * 1024) << " Mo" << endl;
			cout << "Time to find suspicious: " << SecondToDHMS(timer.Elapsed()) << endl;
		}

		if (!m_options.m_bQuiet)
		{
			cout << "Suspect Clouds1 (Clouds2), Shadows1 (Shadows2) before clearing" << endl;
			for (size_t zz = 0; zz < suspects1.size(); zz++)
			{
				size_t z = m_options.m_scenesTreated[0] + zz;
				string date = scenesPeriod[z].Begin().GetFormatedString(date_format);
				cout << std::setfill(' ') << setw(6) << z + 1 << " (";
				cout << std::setfill(' ') << date << "): ";
				cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(suspects1[zz][0].count()) / suspects1[zz][0].size() * 100.0 << "% (";
				cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(suspects2[zz][0].count()) / suspects2[zz][0].size() * 100.0 << "%) ";
				cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(suspects1[zz][1].count()) / suspects1[zz][1].size() * 100.0 << "% (";
				cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(suspects2[zz][1].count()) / suspects2[zz][1].size() * 100.0 << "%)";
				cout << endl;
			}
		}

		if (m_options.m_sieve > 0)
		{
			timer.Start(true);
			if (!m_options.m_bQuiet)
				cout << "Sieve primary suspicious pixels with " << m_options.m_CPU << " CPUs" << endl;

			m_options.ResetBar((size_t)nbScenedProcess*nbPixels * 2);

#pragma omp parallel for num_threads(m_options.m_CPU) if (m_options.m_bMulti)
			for (__int64 zz = 0; zz < (__int64)nbScenedProcess * 2; zz++)
			{
				size_t z = zz / 2;
				size_t i = zz % 2;
				SieveSuspect1(m_options.m_sieve, extents, suspects1[z][i]);
			}

			timer.Stop();
			if (!m_options.m_bQuiet)
			{
				GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
				cout << "Memory used: " << memCounter.WorkingSetSize / (1024 * 1024) << " Mo" << endl;
				cout << "Time to sieve primary suspicious: " << SecondToDHMS(timer.Elapsed()) << endl;
			}
		}


		if (m_options.m_buffer > 0)
		{
			timer.Start(true);
			if (!m_options.m_bQuiet)
				cout << "Clean secondary suspicious pixels that do not touch primary suspicious pixel with " << m_options.m_CPU << " CPUs" << endl;

			m_options.ResetBar((size_t)2 * nbScenedProcess*nbPixels * 2);

#pragma omp parallel for num_threads(m_options.m_CPU) if (m_options.m_bMulti)
			for (__int64 zz = 0; zz < (__int64)nbScenedProcess * 2; zz++)
			{
				size_t z = zz / 2;
				size_t i = zz % 2;
				CleanSuspect2(extents, suspects1[z][i], suspects2[z][i]);
			}
			timer.Stop();

			if (!m_options.m_bQuiet)
			{
				GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
				cout << "Memory used: " << memCounter.WorkingSetSize / (1024 * 1024) << " Mo" << endl;
				cout << "Time to clean secondary suspicious: " << SecondToDHMS(timer.Elapsed()) << endl;
			}
		}

		if (!m_options.m_bQuiet)
		{

			cout << "Suspect Clouds1 (Clouds2), Shadows1 (Shadows2) after clearing" << endl;
			for (size_t zz = 0; zz < suspects1.size(); zz++)
			{
				size_t z = m_options.m_scenesTreated[0] + zz;
				string date = scenesPeriod[z].Begin().GetFormatedString(date_format);
				cout << std::setfill(' ') << setw(6) << z + 1 << " (";
				cout << std::setfill(' ') << date << "): ";
				cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(suspects1[zz][0].count()) / suspects1[zz][0].size() * 100.0 << "% (";
				cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(suspects2[zz][0].count()) / suspects2[zz][0].size() * 100.0 << "%) ";
				cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(suspects1[zz][1].count()) / suspects1[zz][1].size() * 100.0 << "% (";
				cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(suspects2[zz][1].count()) / suspects2[zz][1].size() * 100.0 << "%)";
				cout << endl;
			}

		}

		if (!m_options.m_bQuiet)
			cout << "Ranger all suspicious pixels with " << m_options.m_BLOCK_THREADS << " block threads (" << m_options.BLOCK_CPU() << " threads/block)" << endl;
		//pass 2 : find clouds
		timer.Start(true);
		m_options.ResetBar((size_t)nbScenedProcess*nbPixels * 3 + nbScenedLoaded * nbPixels);
#pragma omp parallel for schedule(static, 1) num_threads(m_options.m_BLOCK_THREADS) if (m_options.m_bMulti)
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int thread = omp_get_thread_num();
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			//data
			RFCodeData RFcode;
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			FindClouds(xBlock, yBlock, bandHolder[thread], forests[thread], RFcode, suspects1, suspects2, clouds);
			WriteBlock1(xBlock, yBlock, bandHolder[thread], RFcode, DTCodeDS);
		}//for all blocks

		if (DTCodeDS.IsOpen())
			DTCodeDS.FlushCache();

		forests.clear();//destroy forest end free memory

		timer.Stop();

		if (!m_options.m_bQuiet)
		{
			GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
			cout << "Memory used: " << memCounter.WorkingSetSize / (1024 * 1024) << " Mo" << endl;
			cout << "Time to identify cloud with Ranger: " << SecondToDHMS(timer.Elapsed()) << endl;
		}

		if (m_options.m_bufferEx[0] > 0 || m_options.m_bufferEx[1] > 0)
		{
			if (!m_options.m_bQuiet)
				cout << "Set suspicious pixels around clouds as clouds with " << m_options.m_CPU << " CPUs" << endl;

			timer.Start(true);
			//size_t nbPixels = min(extents.m_xSize*extents.m_ySize, extents.m_xBlockSize*extents.m_yBlockSize);
			m_options.ResetBar(clouds.size()*nbPixels * 2);

			SetBuffer(extents, suspects1, suspects2, clouds);
			timer.Stop();
			if (!m_options.m_bQuiet)
				cout << "Time to set buffer around clouds: " << SecondToDHMS(timer.Elapsed()) << endl;
		}


		if (m_options.m_bCreateImage || m_options.m_bDebug)
		{
			timer.Start(true);
			if (!m_options.m_bQuiet)
				cout << "Clean/replace clouds, output debug with " << m_options.m_BLOCK_THREADS << " block threads (" << m_options.BLOCK_CPU() << " threads/block)" << endl;

			m_options.ResetBar((size_t)nbScenedProcess*nbPixels + nbScenedLoaded * nbPixels);

			//pass 3 : reset or replace clouds
// no omp here
#pragma omp parallel for schedule(static, 1) num_threads(m_options.m_BLOCK_THREADS) if (m_options.m_bMulti)
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int thread = omp_get_thread_num();
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				//data
				LansatData data;
				DebugData debug;
				ReadBlock(xBlock, yBlock, bandHolder[thread]);
				ResetReplaceClouds(xBlock, yBlock, bandHolder[thread], data, debug, suspects1, suspects2, clouds);
				WriteBlock2(xBlock, yBlock, bandHolder[thread], data, debug, outputDS, debugDS);

			}//for all blocks

			timer.Stop();
			cout << "Time to output debug and cleaned images: " << SecondToDHMS(timer.Elapsed()) << endl;
		}

		//close inputs and outputs
		CloseAll(inputDS, maskDS, outputDS, DTCodeDS, debugDS);

		GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
		cout << "Memory used: " << memCounter.WorkingSetSize / (1024 * 1024) << " Mo" << endl;

		cout << endl;
		cout << " Scene " + string(date_format == "%Y" ? " year  " : "    date     ") + "  Clouds  Shadows" << endl;
		for (size_t zz = 0; zz < clouds.size(); zz++)
		{
			size_t z = m_options.m_scenesTreated[0] + zz;
			string date = scenesPeriod[z].Begin().GetFormatedString(date_format);

			cout << std::setfill(' ') << setw(6) << z + 1 << " (";
			cout << std::setfill(' ') << date << "): ";
			cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(clouds[zz][0].count()) / clouds[zz][0].size() * 100.0 << "% ";
			cout << std::setfill(' ') << setw(6) << std::fixed << std::setprecision(2) << (double)(clouds[zz][1].count()) / clouds[zz][1].size() * 100.0 << "%";
			cout << endl;
		}

	}

	return msg;
}


void CCloudCleaner::ReadBlock(size_t xBlock, size_t yBlock, CBandsHolder& bandHolder)
{
#pragma omp critical(ReadBlockIO)
	{

		m_options.m_timerRead.Start();

		bandHolder.LoadBlock((int)xBlock, (int)yBlock);
		//if (bandHolderRef.GetRasterCount() > 0)
		//	bandHolderRef.LoadBlock((int)xBlock, (int)yBlock);

		m_options.m_timerRead.Stop();
	}
}

size_t CCloudCleaner::GetPrevious(const CLandsatPixelVector& landsat, size_t z)
{
	size_t previous = NOT_INIT;
	for (size_t zz = z - 1; zz < landsat.size() && previous == NOT_INIT; zz--)
		if (landsat[zz].IsValid())
			previous = zz;

	return previous;
}

size_t CCloudCleaner::GetNext(const CLandsatPixelVector& landsat, size_t z)
{
	size_t next = NOT_INIT;
	for (size_t zz = z + 1; zz < landsat.size() && next == NOT_INIT; zz++)
		if (landsat[zz].IsValid())
			next = zz;

	return next;
}


size_t CCloudCleaner::get_m(size_t z1, const CLandsatPixelVector& data)
{
	size_t m = NOT_INIT;
	size_t z0 = GetPrevious(data, z1);
	size_t z2 = GetNext(data, z1);

	if (z0 != NOT_INIT && z2 != NOT_INIT)
	{
		m = 1;
	}
	else if (z0 == NOT_INIT && z2 != NOT_INIT)
	{
		m = 0;
	}
	else if (z0 != NOT_INIT && z2 == NOT_INIT)
	{
		m = 2;
	}
	else //before and after is not init
	{
		m = 1;
	}

	return m;
}

array <CLandsatPixel, 4> CCloudCleaner::GetP(size_t z1, CLandsatPixelVector& data, const CLandsatPixel& median)
{
	ASSERT(z1 < data.size());

	array <CLandsatPixel, 4> p;

	size_t z0 = GetPrevious(data, z1);
	size_t z2 = GetNext(data, z1);

	if (z0 == NOT_INIT && z2 == NOT_INIT)
	{
		p[1] = data[z1];
	}
	else if (z0 == NOT_INIT)
	{
		p[0] = data[z1];
		p[1] = data[z2];
		z2 = GetNext(data, z2);
		p[2] = z2 < data.size() ? data[z2] : NO_PIXEL;
	}
	else if (z2 == NOT_INIT)
	{
		p[2] = data[z1];
		p[1] = data[z0];
		z0 = GetPrevious(data, z0);
		p[0] = z0 < data.size() ? data[z0] : NO_PIXEL;
	}
	else
	{
		p[0] = data[z0];
		p[1] = data[z1];
		p[2] = data[z2];
	}


	//set median pixel if any
	p[3] = median;


	return p;
}

//array <CLandsatPixel, 4> CCloudCleaner::GetP(size_t z1, CLandsatPixelVector& data, const CLandsatPixel& median)
//{
//	ASSERT(z1 < data.size());
//
//	array <CLandsatPixel, 4> p;
//
//	size_t z0 = GetPrevious(data, z1);
//	size_t z2 = GetNext(data, z1);
//
//	if (z0 == NOT_INIT && z2 == NOT_INIT)
//	{
//		p[0] = median;
//		p[1] = data[z1];
//		p[2] = median;
//	}
//	else if (z0 == NOT_INIT)
//	{
//		p[0] = median;
//		p[1] = data[z1];
//		p[2] = data[z2];
//	}
//	else if (z2 == NOT_INIT)
//	{
//		p[0] = data[z0];
//		p[1] = data[z1];
//		p[2] = median;
//	}
//	else
//	{
//		p[0] = data[z0];
//		p[1] = data[z1];
//		p[2] = data[z2];
//	}
//
//
//	//set median pixel if any
//	p[3] = median;
//
//
//	return p;
//}


std::vector <CLandsatPixel> CCloudCleaner::GetR(CLandsatWindow& windowRef, size_t x, size_t y)
{
	std::vector <CLandsatPixel> r;
	if (windowRef.IsInit())
	{
		r.resize(windowRef.GetNbScenes());
		for (size_t i = 0; i < windowRef.GetNbScenes(); i++)
		{
			r[i] = windowRef.GetPixel(i, (int)x, (int)y);
		}
	}

	return r;
}
//Get input image reference
void CCloudCleaner::FindSuspicious(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, SuspectBitset& suspects1, SuspectBitset& suspects2)
{
	size_t nbScenesProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;
	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect((int)xBlock, (int)yBlock);
	CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);
	//size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;


	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += nbScenesProcess * blockSize.m_x*blockSize.m_y;
		m_options.UpdateBar();

		return;
	}


	//allocate process memory and load data
	LansatData data;
	CLandsatPixelVector median;
	LoadData(bandHolder, data, median);

	//#pragma omp critical(ProcessBlock)
#pragma omp parallel for num_threads( m_options.BLOCK_CPU()) if (m_options.m_bMulti)
	for (__int64 zz = 0; zz < (__int64)nbScenesProcess; zz++)
	{
		size_t z = m_options.m_scenesTreated[0] - m_options.m_scenesLoaded[0] + zz;

		for (size_t y = 0; y < blockSize.m_y; y++)
		{
			for (size_t x = 0; x < blockSize.m_x; x++)
			{
				size_t xy = y * blockSize.m_x + x;

				size_t fm = get_m(z, data[xy]);
				array <CLandsatPixel, 4> p = GetP(z, data[xy], (xy < median.size()) ? median[xy] : CLandsatPixel());

				size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;

				suspects1[zz][0].set(xy2, m_options.IsB1Trigged(p, CCloudCleanerOption::T_PRIMARY, fm));
				suspects1[zz][1].set(xy2, m_options.IsTCBTrigged(p, CCloudCleanerOption::T_PRIMARY, fm));

				suspects2[zz][0].set(xy2, m_options.IsB1Trigged(p, CCloudCleanerOption::T_SECONDARY, fm));
				suspects2[zz][1].set(xy2, m_options.IsTCBTrigged(p, CCloudCleanerOption::T_SECONDARY, fm));

			}//x
		}//y

#pragma omp atomic		
		m_options.m_xx += (size_t)blockSize.m_x*blockSize.m_y;

		m_options.UpdateBar();

	}//z
}

//Get input image reference
//void CCloudCleaner::FindClouds(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const Forests3& forests, RFCodeData& RFcode, SuspectBitset& suspects1, SuspectBitset& suspects2, CloudBitset& clouds)
//{
//	size_t nbScenesProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;
//	size_t nbScenes = bandHolder.GetNbScenes();
//	size_t sceneSize = bandHolder.GetSceneSize();
//	CGeoExtents extents = bandHolder.GetExtents();
//	CGeoRectIndex index = extents.GetBlockRect((int)xBlock, (int)yBlock);
//	CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);
//
//
//	if (bandHolder.IsEmpty())
//	{
//#pragma omp atomic		
//		m_options.m_xx += nbScenesProcess * blockSize.m_x*blockSize.m_y;
//		m_options.UpdateBar();
//
//		return;
//	}
//
//
//	//allocate process memory and load data
//	LansatData data;
//	CLandsatPixelVector median;
//	LoadData(bandHolder, data, median);
//
//	//forest model, 0: beg,1: mid, 2: end
//	StringVector vars("t1_B1,t1_B2,t1_B3,t1_B4,t1_B5,t1_B6,t1_B7,t2_B1,t2_B2,t2_B3,t2_B4,t2_B5,t2_B6,t2_B7,t3_B1,t3_B2,t3_B3,t3_B4,t3_B5,t3_B6,t3_B7", ",");
//	//add reference names
//	if (m_options.m_bUseMedian)
//	{
//		for (size_t b = B1; b < QA; b++)
//			vars.push_back(string("t4_") + Landsat::GetBandName(b));
//	}
//
//
//
//	if (m_options.m_bOutputCode)
//	{
//		RFcode.resize(nbScenesProcess);
//		for (size_t z = 0; z < RFcode.size(); z++)
//			RFcode[z].insert(RFcode[z].begin(), blockSize.m_x*blockSize.m_y, (__int16)GetDefaultNoData(GDT_Int16));
//	}
//
//	for (size_t m = 0; m < 3; m++)
//	{
//		boost::dynamic_bitset<size_t> suspectPixel(nbScenesProcess*blockSize.m_x*blockSize.m_y);
//
//		for (size_t zz = 0; zz < nbScenesProcess; zz++)
//		{
//			size_t z = m_options.m_scenesTreated[0] + zz;
//
//			for (size_t y = 0; y < blockSize.m_y; y++)
//			{
//				for (size_t x = 0; x < blockSize.m_x; x++)
//				{
//					size_t xy = y * blockSize.m_x + x;
//					size_t xyz = z * blockSize.m_y * blockSize.m_x + xy;
//					if (data[xy][z].IsValid())
//					{
//						size_t fm = get_m(z, data[xy]);
//						if (fm == m)
//						{
//							m_options.m_nbPixel++;
//
//							size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;
//
//							bool b1 = suspects1[zz][0].test(xy2) || suspects1[zz][1].test(xy2);
//							bool b2 = suspects2[zz][0].test(xy2) || suspects2[zz][1].test(xy2);
//							if (b1 || b2)
//							{
//								suspectPixel.set(xyz);
//								m_options.m_nbPixelDT++;
//							}
//						}
//					}
//				}
//			}
//		}
//
//		if (suspectPixel.count() > 0)
//		{
//			DataShort input;
//			input.set_virtual_cols(forests[m]->get_virtual_cols_txt(), forests[m]->get_virtual_cols_name());
//			input.resize(suspectPixel.count(), vars);
//
//			for (size_t zz = 0; zz < nbScenesProcess; zz++)
//			{
//				size_t z = m_options.m_scenesTreated[0] + zz;
//
//				size_t cur_xyz = 0;
//				for (size_t y = 0; y < blockSize.m_y; y++)
//				{
//					for (size_t x = 0; x < blockSize.m_x; x++)
//					{
//						size_t xy = y * blockSize.m_x + x;
//						if (suspectPixel.test(xy))
//						{
//							array <CLandsatPixel, 4> p = GetP(z, data[xy], (xy < median.size()) ? median[xy] : CLandsatPixel());
//
//							size_t c = 0;
//							for (size_t t = 0; t < 3 + (m_options.m_bUseMedian ? 1 : 0); t++)
//							{
//								for (size_t b = 0; b < 7; b++)
//								{
//									bool error = false;
//									input.set(c++, cur_xyz, p[t][b], error);
//								}
//							}
//
//							ASSERT(c == vars.size());
//							cur_xyz++;
//						}//if suspect
//					}//x
//				}//y
//			}//z
//
//			input.update_virtual_cols();
//			//#pragma omp critical(ProcessBlock)
//			{
//				forests[m]->run_predict(&input);
//			}
//
//			for (size_t zz = 0; zz < nbScenesProcess; zz++)
//			{
//				size_t cur_xyz = 0;
//				for (size_t y = 0; y < blockSize.m_y; y++)
//				{
//					for (size_t x = 0; x < blockSize.m_x; x++)
//					{
//						size_t xy = y * blockSize.m_x + x;
//						if (suspectPixel.test(xy))
//						{
//							int RFexit = int(forests[m]->getPredictions().at(0).at(0).at(cur_xyz));
//
//							size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;
//							if (RFexit == 116)//clouds
//							{
//								clouds[zz][0].set(xy2);
//							}//if cloud
//							else if (RFexit == 115)//shadow
//							{
//								clouds[zz][1].set(xy2);
//							}//if cloud
//
//							if (!RFcode.empty())
//								RFcode[zz][xy] = (__int16)(RFexit);
//
//							cur_xyz++;
//						}//if suspect
//					}//x
//				}//y
//			}//z 
//
//
//			forests[m]->clear();
//		}//if have suspect
//
//#pragma omp atomic		
//		m_options.m_xx += nbScenesProcess * blockSize.m_x*blockSize.m_y;
//
//
//		m_options.UpdateBar();
//
//	}//for all model
//
//	//m_options.m_timerProcess.Stop();
////}//omp
//}

void CCloudCleaner::FindClouds(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const Forests3& forests, RFCodeData& RFcode, SuspectBitset& suspects1, SuspectBitset& suspects2, CloudBitset& clouds)
{
	size_t nbScenesProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;
	//size_t nbScenes = bandHolder.GetNbScenes();
	//size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect((int)xBlock, (int)yBlock);
	CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);


	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += nbScenesProcess * blockSize.m_x*blockSize.m_y;
		m_options.UpdateBar();

		return;
	}


	//allocate process memory and load data
	LansatData data;
	CLandsatPixelVector median;
	LoadData(bandHolder, data, median);
	//CLandsatWindow window = bandHolder.GetWindow();
	//size_t nbScenesLoaded = m_options.m_scenesLoaded[1] - m_options.m_scenesLoaded[0] + 1;


	//forest model, 0: beg,1: mid, 2: end
	StringVector vars("t1_B1,t1_B2,t1_B3,t1_B4,t1_B5,t1_B6,t1_B7,t2_B1,t2_B2,t2_B3,t2_B4,t2_B5,t2_B6,t2_B7,t3_B1,t3_B2,t3_B3,t3_B4,t3_B5,t3_B6,t3_B7", ",");
	//add reference names
	if (m_options.m_bUseMedian)
	{
		for (size_t b = B1; b < QA; b++)
			vars.push_back(string("t4_") + Landsat::GetBandName(b));
	}

	if (m_options.m_bOutputCode)
	{
		RFcode.resize(nbScenesProcess);
		for (size_t z = 0; z < RFcode.size(); z++)
			RFcode[z].insert(RFcode[z].begin(), blockSize.m_x*blockSize.m_y, (__int16)GetDefaultNoData(GDT_Int16));
	}

	for (int m = 0; m < 3; m++)
	{
		for (int zz = 0; zz < (int)nbScenesProcess; zz++)
		{
			size_t z = m_options.m_scenesTreated[0] - m_options.m_scenesLoaded[0] + zz;

			boost::dynamic_bitset<size_t> suspectPixel((size_t)blockSize.m_x*blockSize.m_y);
			for (size_t y = 0; y < blockSize.m_y; y++)
			{
				for (size_t x = 0; x < blockSize.m_x; x++)
				{
					if(m==0)
						m_options.m_nbPixel++;

					size_t xy = y * blockSize.m_x + x;
					size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;
					bool b1 = suspects1[zz][0].test(xy2) || suspects1[zz][1].test(xy2);
					bool b2 = suspects2[zz][0].test(xy2) || suspects2[zz][1].test(xy2);
					if (b1 || b2)
					{
						ASSERT(data[xy][z].IsValid());
						size_t fm = get_m(z, data[xy]);
						if (fm == m)
						{
							suspectPixel.set(xy);
							m_options.m_nbPixelDT++;
						}
					}
				}
			}

			if (suspectPixel.count() > 0)
			{
				DataShort input;
				input.set_virtual_cols(forests[m]->get_virtual_cols_txt(), forests[m]->get_virtual_cols_name());
				input.resize(suspectPixel.count(), vars);

				size_t cur_xy = 0;
				for (size_t y = 0; y < blockSize.m_y; y++)
				{
					for (size_t x = 0; x < blockSize.m_x; x++)
					{
						size_t xy = y * blockSize.m_x + x;
						if (suspectPixel.test(xy))
						{
						/*	CLandsatPixelVector data(nbScenesLoaded);
							CLandsatPixel median;


							for (int zzz = 0; zzz < (int)nbScenesLoaded; zzz++)
							{
								size_t z = m_options.m_scenesLoaded[0] + zzz;
								data[zzz] = window.GetPixel(z, (int)x, (int)y);
							}

							*///if (m_options.m_bUseMedian)
							//	median = window.GetPixelMedian((int)x, (int)y);
						/*	size_t z0 = window.GetPrevious(z, (int)x, (int)y);
							size_t z2 = window.GetNext(z, (int)x, (int)y);

							array <CLandsatPixel, 4> p;
							p[3] = window.GetPixelMedian((int)x, (int)y);
							p[0] = z0!=NOT_INIT?window.GetPixel(z0, (int)x, (int)y) :p[3];
							p[1] = window.GetPixel(z, (int)x, (int)y);
							p[2] = z2 != NOT_INIT ? window.GetPixel(z2, (int)x, (int)y) :p[3];
							*/
							

							//array <CLandsatPixel, 4> p = GetP(zz, data, median);
							array <CLandsatPixel, 4> p = GetP(z, data[xy], (xy < median.size()) ? median[xy] : CLandsatPixel());

							size_t c = 0;
							for (size_t t = 0; t < 3 + (m_options.m_bUseMedian ? 1 : 0); t++)
							{
								for (size_t b = 0; b < 7; b++)
								{
									bool error = false;
									input.set(c++, cur_xy, p[t][b], error);
								}
							}

							ASSERT(c == vars.size());
							cur_xy++;
						}//if suspect
					}//x
				}//y

				input.update_virtual_cols();
				//#pragma omp critical(ProcessBlock)
				//{
				forests[m]->run_predict(&input);
				//}

				cur_xy = 0;
				for (size_t y = 0; y < blockSize.m_y; y++)
				{
					for (size_t x = 0; x < blockSize.m_x; x++)
					{
						size_t xy = y * blockSize.m_x + x;
						if (suspectPixel.test(xy))
						{
							int RFexit = int(forests[m]->getPredictions().at(0).at(0).at(cur_xy));

							size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;
							if (RFexit == 116)//clouds
							{
								clouds[zz][0].set(xy2);
							}//if cloud
							if (RFexit == 115)//shadow
							{
								clouds[zz][1].set(xy2);
							}//if cloud

							if (!RFcode.empty())
								RFcode[zz][xy] = (__int16)(RFexit);

							cur_xy++;
						}//if suspect
					}//x
				}//y
			}//if have suspect

#pragma omp atomic		
			m_options.m_xx += (size_t)blockSize.m_x*blockSize.m_y;

			m_options.UpdateBar();
		}//z

		forests[m]->clear();
	}//m

	//m_options.m_timerProcess.Stop();
//}//omp
}


void CCloudCleaner::WriteBlock1(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, RFCodeData& RFcode, CGDALDatasetEx& RFCodeDS)
{

#pragma omp critical(WriteBlockIO)
	{

		m_options.m_timerWrite.Start();

		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect((int)xBlock, (int)yBlock);

		ASSERT(outputRect.Width() == blockSize.m_x);
		ASSERT(outputRect.Height() == blockSize.m_y);

		if (m_options.m_bOutputCode)
		{

			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);

			for (size_t z = 0; z < RFCodeDS.GetRasterCount(); z++)
			{
				ASSERT(RFcode.empty() || RFcode[z].size() == outputRect.Width()*outputRect.Height());

				GDALRasterBand *pBand = RFCodeDS.GetRasterBand(z);
				if (!RFcode.empty())
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(RFcode[z][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				else
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
			}
		}
	}

	m_options.m_timerWrite.Stop();

}



bool CCloudCleaner::SieveSuspect1(size_t level, size_t nbSieve, const CGeoExtents& extents, CGeoPointIndex xy, boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& new_suspects1, vector<size_t>& pixels)
{
	ASSERT(pixels.size() < nbSieve);

	size_t xy1 = (size_t)xy.m_y * extents.m_xSize + xy.m_x;
	assert(suspects1.test(xy1));

	bool bSieve = true;
	pixels.push_back(xy1);

	//on the edge case when suspect pixel touch to cloud 
	for (size_t yy = 0; yy < 3 && pixels.size() <= nbSieve && bSieve && level < 50000; yy++)
	{
		size_t yyy = xy.m_y + yy - 1;
		if (yyy < (size_t)extents.m_ySize)
		{
			for (size_t xx = 0; xx < 3 && pixels.size() <= nbSieve && bSieve && level < 50000; xx++)
			{
				size_t xxx = xy.m_x + xx - 1;
				if (xxx < (size_t)extents.m_xSize)
				{
					size_t xy2 = yyy * extents.m_xSize + xxx;
					if (suspects1.test(xy2))
					{
						if (pixels.size() == nbSieve || new_suspects1.test(xy2))
							bSieve = false;
						else
							bSieve = SieveSuspect1(level + 1, nbSieve, extents, CGeoPointIndex((int)xxx, (int)yyy), suspects1, new_suspects1, pixels);
					}//not treated
				}//inside extent
			}//for buffer x
		}//inside extent
	}//for buffer y 



	return bSieve;
}


void CCloudCleaner::SieveSuspect1(size_t nbSieve, const CGeoExtents& extents, boost::dynamic_bitset<size_t>& suspects1)
{
	boost::dynamic_bitset<size_t> new_suspects1(suspects1.size());
	vector<size_t> pixels;
	pixels.reserve(nbSieve + 1);
	//boost::dynamic_bitset<size_t> treated1(suspects1.size());
	//ofStream file;
	//file.open("D:/Sieve.csv");
	//file << "i,j,X,Y,Pixels";

	//file << endl;
	for (size_t y = 0; y < extents.m_ySize; y++)
	{

		for (size_t x = 0; x < extents.m_xSize; x++)
		{
			size_t xy = y * extents.m_xSize + x;
			if (suspects1.test(xy) && !new_suspects1.test(xy))
			{
				pixels.resize(0);

				bool bSieve = SieveSuspect1(0, nbSieve, extents, CGeoPointIndex((int)x, (int)y), suspects1, new_suspects1, pixels);

				for (size_t i = 0; i < pixels.size(); i++)
				{
					if (bSieve)
						suspects1.reset(pixels[i]);
					else
						new_suspects1.set(pixels[i]);
				}


				//if (nbPixels > 0 && nbPixels <= nbSieve)
				//{
				//	//suspects1.reset(xy1);
				//	CleanSuspect1(extents, CGeoPointIndex((int)x, (int)y), suspects1, treated2);
				//}

			}

			//file << x << "," << y << "," << extents.m_xMin + (x+0.5)* extents.XRes() << "," << extents.m_yMax + (y + 0.5)* extents.YRes() << "," << nbPixels << endl;

#pragma omp atomic		
			m_options.m_xx++;
		}

		m_options.UpdateBar();
	}//for buffer y 

	//file.close();

	suspects1 = new_suspects1;
}

//
//
//void CCloudCleaner::SieveSuspect1(size_t level, size_t nbSieve, const CGeoExtents& extents, CGeoPointIndex xy, boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& treated, size_t& nbPixels)
//{
//	//	size_t nbPixels = 0;
//	size_t xy1 = (size_t)xy.m_y * extents.m_xSize + xy.m_x;
//	assert(suspects1.test(xy1));
//	assert(!treated.test(xy1));
//
//
//
//	treated.set(xy1);
//
//	nbPixels++;//itself
//
//	//on the edge case when suspect pixel touch to cloud 
//	for (size_t yy = 0; yy < 3 && nbPixels <= nbSieve && level < 50000; yy++)
//	{
//		size_t yyy = xy.m_y + yy - 1;
//		if (yyy < (size_t)extents.m_ySize)
//		{
//			for (size_t xx = 0; xx < 3 && nbPixels <= nbSieve && level < 50000; xx++)
//			{
//				size_t xxx = xy.m_x + xx - 1;
//				if (xxx < (size_t)extents.m_xSize)
//				{
//					size_t xy2 = yyy * extents.m_xSize + xxx;
//					if (suspects1.test(xy2) && !treated.test(xy2))
//					{
//						SieveSuspect1(level + 1, nbSieve, extents, CGeoPointIndex((int)xxx, (int)yyy), suspects1, treated, nbPixels);
//					}//not treated
//				}//inside extent
//			}//for buffer x
//		}//inside extent
//	}//for buffer y 
//
//
//
//	//return nbPixels;
//}
//
//
//void CCloudCleaner::SieveSuspect1(size_t nbSieve, const CGeoExtents& extents, boost::dynamic_bitset<size_t>& suspects1)
//{
//	boost::dynamic_bitset<size_t> new_suspects1(suspects1.size());
//	boost::dynamic_bitset<size_t> treated1(suspects1.size());
//	//ofStream file;
//	//file.open("D:/Sieve.csv");
//	//file << "i,j,X,Y,Pixels";
//
//	//file << endl;
//	for (size_t y = 0; y < extents.m_ySize; y++)
//	{
//
//		for (size_t x = 0; x < extents.m_xSize; x++)
//		{
//			//size_t nbPixels = 0;
//			size_t xy = y * extents.m_xSize + x;
//			if (suspects1.test(xy) /*&& !treated1.test(xy)*/)
//			{
//				treated1.reset();
//				//			boost::dynamic_bitset<size_t> treated2(suspects1.size());
//
//				size_t nbPixels = 0;
//				SieveSuspect1(0, nbSieve, extents, CGeoPointIndex((int)x, (int)y), suspects1, treated1, nbPixels);
//				new_suspects1.set(xy, nbPixels > nbSieve);
//
//				//if (nbPixels > 0 && nbPixels <= nbSieve)
//				//{
//				//	//suspects1.reset(xy1);
//				//	CleanSuspect1(extents, CGeoPointIndex((int)x, (int)y), suspects1, treated2);
//				//}
//
//			}
//
//			//file << x << "," << y << "," << extents.m_xMin + (x+0.5)* extents.XRes() << "," << extents.m_yMax + (y + 0.5)* extents.YRes() << "," << nbPixels << endl;
//
//#pragma omp atomic		
//			m_options.m_xx++;
//		}
//
//		m_options.UpdateBar();
//	}//for buffer y 
//
//	//file.close();
//
//	suspects1 = new_suspects1;
//}
//
//void CCloudCleaner::FastSieveSuspect1(size_t nbSieve, const CGeoExtents& extents, boost::dynamic_bitset<size_t>& suspects1)
//{
//	ASSERT(nbSieve <= 9);
//
//	boost::dynamic_bitset<size_t> new_suspects1(suspects1.size());
//
//	for (size_t y = 0; y < extents.m_ySize; y++)
//	{
//		for (size_t x = 0; x < extents.m_xSize; x++)
//		{
//			size_t xy = y * extents.m_xSize + x;
//			if (suspects1.test(xy))
//			{
//				size_t nbPixels = 0;
//
//				for (size_t yy = 0; yy < 2 * nbSieve + 1 && nbPixels <= nbSieve; yy++)
//				{
//					size_t yyy = y + yy - 1;
//					if (yyy < (size_t)extents.m_ySize)
//					{
//						for (size_t xx = 0; xx < 2 * nbSieve + 1 && nbPixels <= nbSieve; xx++)
//						{
//							size_t xxx = x + xx - 1;
//							if (xxx < (size_t)extents.m_xSize)
//							{
//								size_t xy2 = yyy * extents.m_xSize + xxx;
//								if (suspects1.test(xy2))
//									nbPixels++;
//							}//inside extent
//						}//for buffer x
//					}//inside extent
//				}//for buffer y 
//
//				if (nbPixels > nbSieve)
//					new_suspects1.set(xy);
//			}//if pixel is suspect
//			//file << x << "," << y << "," << extents.m_xMin + (x+0.5)* extents.XRes() << "," << extents.m_yMax + (y + 0.5)* extents.YRes() << "," << nbPixels << endl;
//
//#pragma omp atomic		
//			m_options.m_xx++;
//		}
//
//		m_options.UpdateBar();
//	}//for buffer y 
//
//	//file.close();
//
//	suspects1 = new_suspects1;
//}

void CCloudCleaner::TouchSuspect1(size_t level, const CGeoExtents& extents, CGeoPointIndex xy, const boost::dynamic_bitset<size_t>& suspects1, const boost::dynamic_bitset<size_t>& suspects2, boost::dynamic_bitset<size_t>& treated, boost::dynamic_bitset<size_t>& touch)
{
	ASSERT(!treated.test((size_t)xy.m_y * extents.m_xSize + xy.m_x));

	size_t xy1 = (size_t)xy.m_y * extents.m_xSize + xy.m_x;
	treated.set(xy1);
	touch.set(xy1, suspects2.test(xy1));

	if (touch.test(xy1))
	{
		//on the edge case when suspect pixel touch to cloud 
		for (size_t yy = 0; yy < 3 && level < 50000; yy++)
		{
			size_t yyy = xy.m_y + yy - 1;
			if (yyy < (size_t)extents.m_ySize)
			{
				for (size_t xx = 0; xx < 3 && level < 50000; xx++)
				{
					size_t xxx = xy.m_x + xx - 1;
					if (xxx < (size_t)extents.m_xSize)
					{
						size_t xy2 = yyy * extents.m_xSize + xxx;
						if (!treated.test(xy2) && suspects2.test(xy2))
						{
							TouchSuspect1(level + 1, extents, CGeoPointIndex((int)xxx, (int)yyy), suspects1, suspects2, treated, touch);
						}//not treated
					}//inside extent
				}//for buffer x
			}//inside extent
		}//for buffer y 

		//if (!bTouch || level >= 50000)
			//suspects2.reset(xy1);
	}//if not touch

	//return bTouch;
}
void CCloudCleaner::CleanSuspect2(const CGeoExtents& extents, const boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& suspects2)
{
	ASSERT(suspects1.size() == suspects2.size());

	//reset all suspect2 pixels that are farther than x pixels of suspect1 pixel
	for (size_t y = 0; y < extents.m_ySize; y++)
	{
		for (size_t x = 0; x < extents.m_xSize; x++)
		{
			size_t xy = y * extents.m_xSize + x;

			if (suspects2.test(xy))
			{
				//do circle cleaning
				bool bReset = true;
				size_t maxBuffer_y = m_options.m_buffer;
				for (size_t yy = 0; (yy < 2 * maxBuffer_y + 1) && bReset; yy++)
				{
					size_t maxBuffer_x = min(maxBuffer_y, yy <= maxBuffer_y ? yy : 2 * maxBuffer_y - yy + 1);
					for (size_t xx = 0; (xx < 2 * maxBuffer_x + 1) && bReset; xx++)
					{
						size_t yyy = y + yy - maxBuffer_y;
						size_t xxx = x + xx - maxBuffer_x;
						if (yyy < (size_t)extents.m_ySize && xxx < (size_t)extents.m_xSize)
						{
							size_t xy2 = yyy * extents.m_xSize + xxx;

							ASSERT(xx < (2 * maxBuffer_x + 1) && yy < (2 * maxBuffer_y + 1));

							if (suspects1.test(xy2))
								bReset = false;

						}
					}//for buffer x
				}//for buffer y 

				if (bReset)
					suspects2.reset(xy);
			}//if suspect 2
#pragma omp atomic		
			m_options.m_xx++;
		}//x

		m_options.UpdateBar();

	}//y

	//by optimization we find all suspect2 pixel that pouch suspect1

	boost::dynamic_bitset<size_t> treated(suspects1.size());
	boost::dynamic_bitset<size_t> touch(suspects2.size());


	//now find all suspect2 pixels that touch suspect1 pixel
	for (size_t y = 0; y < extents.m_ySize; y++)
	{
		for (size_t x = 0; x < extents.m_xSize; x++)
		{
			size_t xy = y * extents.m_xSize + x;
			if (suspects1.test(xy) && !treated.test(xy))
			{
				TouchSuspect1(0, extents, CGeoPointIndex((int)x, (int)y), suspects1, suspects2, treated, touch);
			}

#pragma omp atomic		
			m_options.m_xx++;
		}

		m_options.UpdateBar();

	}//for buffer y 

	 //now remove all suspect2 pixels that do not touch suspect1 pixel
	suspects2 = touch;
}


void CCloudCleaner::ResetReplaceClouds(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, LansatData& data, DebugData& debug, SuspectBitset& suspects1, SuspectBitset& suspects2, CloudBitset& clouds)
{
	size_t nbScenesProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;
	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect((int)xBlock, (int)yBlock);
	CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);
	//size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;


	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += nbScenesProcess * blockSize.m_x*blockSize.m_y;
		m_options.UpdateBar();

		return;
	}

	//allocate process memory and load data
	CLandsatPixelVector median;
	LoadData(bandHolder, data, median);

	LansatData dataCopy = data;

	if (m_options.m_bDebug)
	{
		debug.resize(nbScenesProcess*CCloudCleanerOption::NB_DBUG);
		for (size_t i = 0; i < debug.size(); i++)
			debug[i].insert(debug[i].begin(), blockSize.m_x*blockSize.m_y, (__int16)GetDefaultNoData(GDT_Int16));
	}

	vector<vector<size_t>> replacement;
	if (m_options.m_bCreateImage && m_options.m_bFillClouds)
	{
		replacement.resize(nbScenesProcess);
		for (size_t i = 0; i < replacement.size(); i++)
			replacement[i].insert(replacement[i].begin(), blockSize.m_x*blockSize.m_y, NOT_INIT);
	}

	//#pragma omp critical(ProcessBlock)
	{
		//m_options.m_timerProcess.Start();


		//process debug and find replacement pixel 
//no ompMP here
		for (size_t zz = 0; zz < nbScenesProcess; zz++)
		{
			size_t z = m_options.m_scenesTreated[0] - m_options.m_scenesLoaded[0] + zz;
			for (size_t y = 0; y < blockSize.m_y; y++)
			{
				for (size_t x = 0; x < blockSize.m_x; x++)
				{
					size_t xy = (size_t)y*blockSize.m_x + x;
					size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;

					if (!debug.empty())
					{
						if (dataCopy[xy].at(z).IsValid())
						{
							array <CLandsatPixel, 4> p = GetP(z, dataCopy[xy], (xy < median.size()) ? median[xy] : CLandsatPixel());

							size_t fm = get_m(z, dataCopy[xy]);
							ASSERT(fm < p.size());
							if (p[fm].IsInit())
							{
								if (suspects1[zz][0].test(xy2) || suspects1[zz][1].test(xy2))
									debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DEBUG_FLAG][xy] = m_options.GetDebugFlag(p, CCloudCleanerOption::T_PRIMARY, fm);
								else if (suspects2[zz][0].test(xy2) || suspects2[zz][1].test(xy2))
									debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DEBUG_FLAG][xy] = m_options.GetDebugFlag(p, CCloudCleanerOption::T_SECONDARY, fm);


								debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_NB_SCENE][xy] = (p[0].IsInit() ? 1 : 0) + (p[1].IsInit() ? 1 : 0) + (p[2].IsInit() ? 1 : 0);
								debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_MODEL][xy] = (__int16)fm;
								debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DELTA_B1][xy] = m_options.GetB1Trigger(p, fm);
								debug[zz*CCloudCleanerOption::NB_DBUG + CCloudCleanerOption::D_DELTA_TCB][xy] = m_options.GetTCBTrigger(p, fm);
							}
						}
					}

					bool bCloud = (clouds[zz][0].test(xy2) || clouds[zz][1].test(xy2));
					bool bMissing = m_options.m_bFillMissing && !dataCopy[xy].at(z).IsValid();

					if (bCloud || bMissing)
					{
						size_t z2 = NOT_INIT;
						for (size_t i = 0; i < dataCopy[xy].size() * 2 && z2 == NOT_INIT; i++)
						{
							size_t iz = size_t(z + (int)pow(-1, i)*(i / 2 + 1));
							if (iz < dataCopy[xy].size() && dataCopy[xy][iz].IsInit())
							{
								size_t izz = size_t(z + (int)pow(-1, i)*(i / 2 + 1));

								bool bReplace = (izz < clouds.size()) ? dataCopy[xy].at(iz).IsValid() && !(clouds[izz][0].test(xy2) || clouds[izz][1].test(xy2)) : true; 
								if (bReplace)
									z2 = iz;
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
#pragma omp atomic		
					m_options.m_xx++;
				}//x
			}//y
			m_options.UpdateBar();
		}// z

		//replace cloud by valid pixel for all x and y 
		for (size_t zz = 0; zz < replacement.size(); zz++)
		{
			size_t z = m_options.m_scenesTreated[0] - m_options.m_scenesLoaded[0] + zz;
			for (size_t xy = 0; xy < replacement[zz].size(); xy++)
			{
				if (replacement[zz][xy] != NOT_INIT)
				{
					data[xy][z] = dataCopy[xy][replacement[zz][xy]];
				}
			}//for xy
		}//z


		//m_options.m_timerProcess.Stop();
	}//critical
}


void CCloudCleaner::WriteBlock2(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const LansatData& data, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
{

#pragma omp critical(WriteBlockIO)
	{
		m_options.m_timerWrite.Start();

		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect((int)xBlock, (int)yBlock);

		ASSERT(outputRect.Width() == blockSize.m_x);
		ASSERT(outputRect.Height() == blockSize.m_y);

		if (m_options.m_bCreateImage)
		{
			ASSERT(outputDS.GetRasterCount() % SCENES_SIZE == 0);

			__int16 noData = (__int16)outputDS.GetNoData(0);
			vector<__int16> tmp(blockSize.m_x*blockSize.m_y, noData);

			for (size_t bb = 0; bb < outputDS.GetRasterCount(); bb++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(bb);

				if (!data.empty())
				{
					ASSERT(data.size() == blockSize.m_x*blockSize.m_y);

					size_t z = m_options.m_scenesTreated[0] - m_options.m_scenesLoaded[0] + bb / SCENES_SIZE;
					//size_t z = m_options.m_scenesTreated[0] + bb / SCENES_SIZE;
					//size_t z = bb / SCENES_SIZE;
					size_t b = bb % SCENES_SIZE;
					for (size_t y = 0; y < blockSize.m_y; y++)
					{
						for (size_t x = 0; x < blockSize.m_x; x++)
						{
							size_t xy = y * blockSize.m_x + x;
							tmp[xy] = data[xy][z][b];

							if (b == JD && tmp[xy] < 0)
							{
								tmp[xy] = noData;
							}
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


//void CCloudCleaner::SetBuffer(const CGeoExtents& extents, SuspectBitset& suspects1, SuspectBitset& suspects2, CloudBitset& clouds)
//{
//	if (m_options.m_buffer > 0 || m_options.m_bufferEx > 0)
//	{
//
//		m_options.ResetBar(clouds.size()*extents.m_xSize*extents.m_ySize);
//		//set buffer around cloud
//#pragma omp parallel for num_threads(m_options.m_CPU) if (m_options.m_bMulti)
//		for (__int64 zz = 0; zz < (__int64)clouds.size(); zz++)
//		{
//			boost::dynamic_bitset<size_t> cloudsCopy = clouds[zz];
//			//size_t z = m_options.m_scenesTreated[0] + zz;
//			for (size_t y = 0; y < extents.m_ySize; y++)
//			{
//				for (size_t x = 0; x < extents.m_xSize; x++)
//				{
//					//size_t xy = (size_t)y*blockSize.m_x + x;
//					size_t xy2 = y * extents.m_xSize + x;
//
//					//he suspicious = 
//					if (cloudsCopy.test(xy2))
//					{
//						size_t maxBuffer = max(m_options.m_buffer, m_options.m_bufferEx);
//						for (size_t yy = 0; yy < 2 * maxBuffer + 1; yy++)
//						{
//							for (size_t xx = 0; xx < 2 * maxBuffer + 1; xx++)
//							{
//								size_t yyy = y + yy - maxBuffer;
//								size_t xxx = x + xx - maxBuffer;
//								if (yyy < (size_t)extents.m_ySize && xxx < (size_t)extents.m_xSize)
//								{
//									size_t xy3 = yyy * extents.m_xSize + xxx;
//
//									if (xx < (2 * m_options.m_buffer + 1) && yy < (2 * m_options.m_buffer + 1))
//									{
//										clouds[zz].set(xy3);
//									}
//
//									if (xx < (2 * m_options.m_bufferEx + 1) && yy < (2 * m_options.m_bufferEx + 1))
//									{
//										bool bSus = !suspects2.empty() ? suspects2[zz].test(xy3) : false;
//										if (suspects1[zz].test(xy3) || bSus)
//											clouds[zz].set(xy3);
//									}
//
//								}
//							}//for buffer x
//						}//for buffer y 
//					}//if cloud
//#pragma omp atomic
//					m_options.m_xx++;
//				}//x
//				m_options.UpdateBar();
//			}//y
//		}//zz
//	}
//}

void CCloudCleaner::SetBuffer(const CGeoExtents& extents, SuspectBitset& suspects1, SuspectBitset& suspects2, CloudBitset& clouds)
{

	//set suspicious around cloud as cloud
#pragma omp parallel for num_threads(m_options.m_CPU) if (m_options.m_bMulti)
	for (__int64 zz = 0; zz < (__int64)clouds.size() * 2; zz++)
	{
		//for (size_t i = 0; i < 2; i++)

		size_t z = zz / 2;
		size_t i = zz % 2;
		boost::dynamic_bitset<size_t> cloudsCopy = clouds[z][i];

		for (size_t y = 0; y < extents.m_ySize; y++)
		{
			for (size_t x = 0; x < extents.m_xSize; x++)
			{
				size_t xy2 = y * extents.m_xSize + x;

				if (cloudsCopy.test(xy2))
				{
					//primary
					for (size_t yy = 0; yy < 2 * m_options.m_bufferEx[0] + 1; yy++)
					{
						for (size_t xx = 0; xx < 2 * m_options.m_bufferEx[0] + 1; xx++)
						{
							size_t yyy = y + yy - m_options.m_bufferEx[0];
							size_t xxx = x + xx - m_options.m_bufferEx[0];
							if (yyy < (size_t)extents.m_ySize && xxx < (size_t)extents.m_xSize)
							{
								size_t xy3 = yyy * extents.m_xSize + xxx;
								if (xx < (2 * m_options.m_bufferEx[0] + 1) && yy < (2 * m_options.m_bufferEx[0] + 1))
								{
									if (suspects1[z][i].test(xy3))
										clouds[z][i].set(xy3);
								}
							}
						}//for buffer x
					}//for buffer y 

					//secondary
					for (size_t yy = 0; yy < 2 * m_options.m_bufferEx[1] + 1; yy++)
					{
						for (size_t xx = 0; xx < 2 * m_options.m_bufferEx[1] + 1; xx++)
						{
							size_t yyy = y + yy - m_options.m_bufferEx[1];
							size_t xxx = x + xx - m_options.m_bufferEx[1];
							if (yyy < (size_t)extents.m_ySize && xxx < (size_t)extents.m_xSize)
							{
								size_t xy3 = yyy * extents.m_xSize + xxx;
								if (xx < (2 * m_options.m_bufferEx[1] + 1) && yy < (2 * m_options.m_bufferEx[1] + 1))
								{
									if (suspects2[z][i].test(xy3))
										clouds[z][i].set(xy3);
								}
							}
						}//for buffer x
					}//for buffer y 
				}//if cloud
#pragma omp atomic
				m_options.m_xx++;
			}//x
			m_options.UpdateBar();
		}//y

	}//zz
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

void CCloudCleaner::LoadData(const CBandsHolder& bandHolder, LansatData& data, CLandsatPixelVector& median)
{
	CGeoExtents extents = bandHolder.GetExtents();
	CLandsatWindow window = bandHolder.GetWindow();
	//size_t nbScenes = bandHolder.GetNbScenes();
	size_t nbScenesLoaded = m_options.m_scenesLoaded[1] - m_options.m_scenesLoaded[0] + 1;

	CGeoSize blockSize = window.GetGeoSize();
	data.resize(blockSize.m_x*blockSize.m_y);

	if (m_options.m_bUseMedian)
		median.resize(blockSize.m_x*blockSize.m_y);
	//
	//#pragma omp parallel for num_threads(m_options.m_CPU) if (m_options.m_bMulti)
	//	for (int x = 0; x < blockSize.m_x; x++)
	//	{
	//		for (size_t y = 0; y < blockSize.m_y; y++)
	//		{
	//			data[y*blockSize.m_x + x].resize(nbScenes);
	//
	//			for (size_t z = 0; z < nbScenes; z++)
	//			{
	//				data[y*blockSize.m_x + x][z] = window.GetPixel(z, (int)x, (int)y);
	//				if (m_options.m_bUseMedian)
	//					median[y*blockSize.m_x + x] = window.GetPixelMedian((int)x, (int)y);
	//			}
	//		}
	//	}

	for (size_t y = 0; y < blockSize.m_y; y++)
		for (size_t x = 0; x < blockSize.m_x; x++)
			//data[y*blockSize.m_x + x].resize(nbScenes);
			data[y*blockSize.m_x + x].resize(nbScenesLoaded);
			


#pragma omp parallel for num_threads(m_options.BLOCK_CPU()) if (m_options.m_bMulti)
	for (int zz = 0; zz < (int)nbScenesLoaded; zz++)
	{
		size_t z = m_options.m_scenesLoaded[0] + zz;
		for (size_t y = 0; y < blockSize.m_y; y++)
		{
			for (size_t x = 0; x < blockSize.m_x; x++)
			{
				data[y*blockSize.m_x + x][zz] = window.GetPixel(z, (int)x, (int)y);
				if (zz == 0 && m_options.m_bUseMedian)
					median[y*blockSize.m_x + x] = window.GetPixelMedian((int)x, (int)y);
			}
		}

#pragma omp atomic		
		m_options.m_xx += (size_t)blockSize.m_x*blockSize.m_y;
		m_options.UpdateBar();

	}
}


string CCloudCleaner::GetScenesDateFormat(const std::vector<CTPeriod>& p)
{
	string str;

	set<size_t> stat;
	for (size_t z = 0; z < p.size(); z++)
		stat.insert(p[z].size());

	if (*stat.rbegin() == 1)
		str = "%F";  //Individuall input
	else //if (stat[HIGHEST] <= 366)
		str = "%Y";	//annual input
	//else
		//str = "%1-%2", "%Y");	//multi annual input

	return str;
}



__int32 CCloudCleanerOption::GetB1Trigger(std::array <CLandsatPixel, 4>& p, size_t fm)
{
	size_t c0 = (fm == 0) ? 1 : 0;
	size_t c2 = (fm == 2) ? 1 : 2;

	if (!p[fm].IsInit())
		return -32768;

	if (!p[c0].IsInit() && !p[c2].IsInit() && !p[3].IsInit())
		return -32768;

	__int32 t1 = p[c0].IsInit() ? (p[c0][Landsat::B1] - p[fm][Landsat::B1]) : -32767;
	__int32 t2 = p[c2].IsInit() ? (p[c2][Landsat::B1] - p[fm][Landsat::B1]) : -32767;
	__int32 t3 = p[3].IsInit() ? (p[3][Landsat::B1] - p[fm][Landsat::B1]) : -32767;

	return max(max(t1, t2), t3);
}

__int32 CCloudCleanerOption::GetTCBTrigger(std::array <CLandsatPixel, 4>& p, size_t fm)
{
	size_t c0 = (fm == 0) ? 1 : 0;
	size_t c2 = (fm == 2) ? 1 : 2;

	if (!p[fm].IsInit())
		return -32768;

	if (!p[c0].IsInit() && !p[c2].IsInit() && !p[3].IsInit())
		return -32768;

	__int32 t1 = p[c0].IsInit() ? (p[c0][Landsat::I_TCB] - p[fm][Landsat::I_TCB]) : 32767;
	__int32 t2 = p[c2].IsInit() ? (p[c2][Landsat::I_TCB] - p[fm][Landsat::I_TCB]) : 32767;
	__int32 t3 = p[3].IsInit() ? (p[3][Landsat::I_TCB] - p[fm][Landsat::I_TCB]) : 32767;



	return min(min(t1, t2), t3);
}



//__int32 CCloudCleanerOption::GetB1TriggerRef(const CLandsatPixel& p, std::vector <CLandsatPixel>& r)
//{
//	if (!p.IsInit())
//		return -32768;
//
//
//	__int32 t3 = 32767;
//	for (size_t i = 0; i < r.size(); i++)
//	{
//		if (r[i].IsInit())
//			t3 = min(t3, r[i][Landsat::B1] - p[Landsat::B1]);
//	}
//
//	if (t3 == 32767)
//		t3 = -32768;
//
//	return t3;
//}
//
//__int32 CCloudCleanerOption::GetTCBTriggerRef(const CLandsatPixel& p, std::vector <CLandsatPixel>& r)
//{
//	if (!p.IsInit())
//		return -32768;
//
//	__int32 t3 = -32767;
//	for (size_t i = 0; i < r.size(); i++)
//	{
//		if (r[i].IsInit())
//			t3 = max(t3, r[i][Landsat::I_TCB] - p[Landsat::I_TCB]);
//	}
//
//	if (t3 == -32767)
//		t3 = -32768;
//
//	return t3;
//}
//
//__int32 CCloudCleanerOption::GetZSWTrigger(std::array <CLandsatPixel, 4>& p, size_t fm)
//{
//	/*if (!IsZSWTrigged(p, t, fm))
//		return -32768;
//*/
//	size_t c0 = (fm == 0) ? 1 : 0;
//	size_t c2 = (fm == 2) ? 1 : 2;
//
//	if (!p[fm].IsInit())
//		return 32767;
//
//	if (!p[c0].IsInit() && !p[c2].IsInit())
//		return 32767;
//
//	__int32 t1 = p[c0].IsInit() ? (p[c0][Landsat::I_ZSW] - p[fm][Landsat::I_ZSW]) : 32767;
//	__int32 t2 = p[c2].IsInit() ? (p[c2][Landsat::I_ZSW] - p[fm][Landsat::I_ZSW]) : 32767;
//
//	return min(t1, t2);
//}



