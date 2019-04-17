//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 1.0.1	17/04/2019	Rémi Saint-Amant	Some bug correction, add debug
// 1.0.0	31/01/2019	Rémi Saint-Amant	Creation


//-te -790800 7649400 -711300 7709400 -multi -co "tiled=YES" -co "BLOCKXSIZE=512" -co "BLOCKYSIZE=512" -co "compress=LZW" -RGB Natural -of VRT --config GDAL_CACHEMAX 4096 -stats -overview {2,4,8,16} -overwrite "C:\Lansat(Fire)\Fire\fire.vrt"  "C:\Lansat(Fire)\Input\Landsat_2010-2015.vrt" "C:\Lansat(Fire)\Output\BC.vrt"
//"U:\GIS\#documents\TestCodes\FireSeverity\Fires\Fires.vrt" "U:\GIS\#documents\TestCodes\FireSeverity\Input\Landsat_2010-2015.vrt" "U:\GIS\#documents\TestCodes\FireSeverity\Output\test.vrt"

#include "stdafx.h"
//#include "VisualLeakDetector\include\vld.h"

#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <bitset>
#include <boost/dynamic_bitset.hpp>

#include "FireSeverity.h"
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

static const char* version = "1.0.1";

std::string CFireSeverity::GetDescription()
{
	return  std::string("FireSeverity version ") + version + " (" + _T(__DATE__) + ")\n";
}


CFireSeverityOption::CFireSeverityOption()
{
	m_outputType = GDT_Int16;
	m_scenesSize = Landsat::SCENES_SIZE;
	m_scenesLoaded = { NOT_INIT ,NOT_INIT };
	m_scenesTreated = { NOT_INIT ,NOT_INIT };
	m_buffer = 8;
	m_bDebug = false;

	m_appDescription = "This software compute fire severity (delta NBR) from Landsat images series";

	static const COptionDef OPTIONS[] =
	{
			{ "-Buffer", 1, "nbPixels", false, "Set buffer size around fires to do NBR correction. 8 by default. 0 to don't use correction." },
			{ "-Debug",0,"",false,"Output debug information."},
			{ "Fires", 0, "", false, "fires zones (0=no fire, 1=fire) for each years. Bands name must finish with _year." },
			{ "srcfile", 0, "", false, "Input LANDSAT scenes image file path." },
			{ "dstfile", 0, "", false, "Output LANDSAT scenes image file path." }
	};

	for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
		AddOption(OPTIONS[i]);

	//AddOption("-Period");
	RemoveOption("-ot");
	static const CIOFileInfoDef IO_FILE_INFO[] =
	{
		{ "Fire", "f", "", "", "", "File of fires zones (0=no fire, 1=fire) for each years. Bands name must finish with _year." },
		{ "LANDSAT", "src1file", "NbScenes", "ScenesSize(9)", "B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|JD: Date(Julian day 1970)|... for each scene" },
		{ "Output", "dstfile", "Nb scenes processed", "", "DeltaNBR|zScore|FireSeverity" },
	};

	for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
		AddIOFileInfo(IO_FILE_INFO[i]);

}

ERMsg CFireSeverityOption::ProcessOption(int& i, int argc, char* argv[])
{
	ERMsg msg;


	if (IsEqual(argv[i], "-Buffer"))
	{
		m_buffer = atoi(argv[++i]);
		if (m_buffer > 100)
			msg.ajoute("Invalid buffer. Buffer must be smaller than 100.");
	}
	else if (IsEqual(argv[i], "-Debug"))
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

ERMsg CFireSeverityOption::ParseOption(int argc, char* argv[])
{
	ERMsg msg = CBaseOptions::ParseOption(argc, argv);

	ASSERT(NB_FILE_PATH == 3);
	if (msg && m_filesPath.size() != NB_FILE_PATH)
	{
		msg.ajoute("ERROR: Invalid argument line. 3 files are needed: the fire series, the LANDSAT images series and the destination image.");
		msg.ajoute("Argument found: ");
		for (size_t i = 0; i < m_filesPath.size(); i++)
			msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
	}

	
	

	
	return msg;
}


//***********************************************************************


ERMsg CFireSeverity::OpenAll(CLandsatDataset& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& fireDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
{
	ERMsg msg;

	if (!m_options.m_bQuiet)
	{
		cout << endl << "Open input image..." << endl;
	}

	msg = landsatDS.OpenInputImage(m_options.m_filesPath[CFireSeverityOption::LANDSAT_FILE_PATH], m_options);

	if (msg && landsatDS.GetNbScenes() < 3)
		msg.ajoute("Input Landsat series need at least 3 scenes");

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
					msg.ajoute("Scenes of the input Landsat images (" + GetFileName(m_options.m_filesPath[CFireSeverityOption::LANDSAT_FILE_PATH]) + ") are not chronologically ordered.");

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
		cout << "    Entire period  = " << landsatDS.GetPeriod().GetFormatedString() << " (nb scenes = " << landsatDS.GetNbScenes() << ")" << endl;
		cout << "    Loaded period  = " << loadedPeriod.GetFormatedString() << " (nb scenes = " << nbSceneLoaded << ")" << endl;
		//cout << "    Treated period = " << processPeriod.GetFormatedString() << " (nb scenes = " << nbScenedProcess << ")" << endl;
	}



	if (!m_options.m_period.IsIntersect(processPeriod))
		msg.ajoute("Input period and process period does not intersect. Verify period or scenes options");


	if (msg && !m_options.m_maskName.empty())
	{
		if (!m_options.m_bQuiet)
			cout << "Open mask image..." << endl;

		msg += maskDS.OpenInputImage(m_options.m_maskName);
	}

	if (msg)
		msg = fireDS.OpenInputImage(m_options.m_filesPath[CFireSeverityOption::FIRE_FILE_PATH], m_options);


	if (msg && m_options.m_bCreateImage)
	{
		if (!m_options.m_bQuiet)
			cout << "Open output images " << endl;
		//cout << "Open output images " << " x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << m_options.m_nbBands << " bands) with " << m_options.m_CPU << " threads..." << endl;


		string filePath = m_options.m_filesPath[CFireSeverityOption::OUTPUT_FILE_PATH];
		CFireSeverityOption options = m_options;
		options.m_nbBands = NB_OUTPUTS * fireDS.GetRasterCount();

		for (size_t zz = 0; zz < fireDS.GetRasterCount(); zz++)
		{
			string title = GetFileTitle(fireDS.GetInternalName(zz));
			if (title.length() > 4)
			{
				string sYear = title.substr(title.size() - 4);
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_dNBR.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_zScore1.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_zScore2.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_FireS.tif|";
			}
		}

		msg += outputDS.CreateImage(filePath, options);
	}

	if (msg && m_options.m_bDebug)
	{
		if (!m_options.m_bQuiet )
			cout << "Open debug images " << endl;
		//cout << "Open output images " << " x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << m_options.m_nbBands << " bands) with " << m_options.m_CPU << " threads..." << endl;


		string filePath = m_options.m_filesPath[CFireSeverityOption::OUTPUT_FILE_PATH];
		SetFileTitle(filePath, GetFileTitle(filePath) + "_debug");
		CFireSeverityOption options = m_options;
		options.m_nbBands = NB_DEBUGS * fireDS.GetRasterCount();

		for (size_t zz = 0; zz < fireDS.GetRasterCount(); zz++)
		{
			string title = GetFileTitle(fireDS.GetInternalName(zz));
			if (title.length() > 4)
			{
				string sYear = title.substr(title.size() - 4);
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_nbMissing.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_offset.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_T1_B3.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_T1_B4.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_T1_B5.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_T1_B7.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_T3_B3.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_T3_B4.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_T3_B5.tif|";
				options.m_VRTBandsName += GetFileTitle(filePath) + "_" + sYear + "_T3_B7.tif|";
			}
		}

		msg += debugDS.CreateImage(filePath, options);
	}

	return msg;
}






ERMsg CFireSeverity::Execute()
{
	ERMsg msg;

	if (!m_options.m_bQuiet)
	{
		cout << "Output: " << m_options.m_filesPath[CFireSeverityOption::OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[CFireSeverityOption::LANDSAT_FILE_PATH] << endl;
		cout << "Fires:  " << m_options.m_filesPath[CFireSeverityOption::FIRE_FILE_PATH] << endl;

		if (!m_options.m_maskName.empty())
			cout << "Mask:   " << m_options.m_maskName << endl;
	}

	GDALAllRegister();

	CLandsatDataset inputDS;
	CGDALDatasetEx maskDS;
	CGDALDatasetEx outputDS;
	CGDALDatasetEx fireDS;
	CGDALDatasetEx debugDS;

	msg = OpenAll(inputDS, maskDS, fireDS, outputDS, debugDS);


	FireBitset fires;
	vector <bool> bHaveFire;
	if (msg)
	{
		msg += LoadFires(inputDS, fireDS, fires, bHaveFire);
	}

	if (msg)
	{
		PROCESS_MEMORY_COUNTERS memCounter;
		if (!m_options.m_bQuiet)
		{
			GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
			cout << "Memory used: " << memCounter.WorkingSetSize / (1024 * 1024) << " Mo" << endl;
		}

		//size_t nbScenedProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;
		//size_t nbScenedLoaded = m_options.m_scenesLoaded[1] - m_options.m_scenesLoaded[0] + 1;

		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);

		if (maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if (!msg)
			return msg;


		CGeoExtents extents = bandHolder.GetExtents();
		vector<pair<int, int>> XYindex = extents.GetBlockList();
		size_t nbPixels = extents.m_xSize*extents.m_ySize;

		BufferStat bufferStat(fires.size());
		if (m_options.m_buffer > 0)
		{
			if (!m_options.m_bQuiet)
				cout << "Compute buffer statistics with " << m_options.m_BLOCK_THREADS << " block threads (" << m_options.BLOCK_CPU() << " threads/block)" << endl;

			m_options.ResetBar((size_t)fires.size()*nbPixels);

			//pass 1 : find dates
			omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(m_options.m_BLOCK_THREADS) if (m_options.m_bMulti)
			for (__int64 b = 0; b < (__int64)XYindex.size(); b++)
			{
				size_t thread = omp_get_thread_num();
				size_t xBlock = XYindex[b].first;
				size_t yBlock = XYindex[b].second;

				if (bHaveFire[b])//there is some fire in this block?
				{
					ReadBlock(xBlock, yBlock, bandHolder[thread]);
					ComputeBufferStat(xBlock, yBlock, bandHolder[thread], fires, bufferStat);
				}
				else
				{
					CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);
#pragma omp atomic		
					m_options.m_xx += fires.size()* blockSize.m_x*blockSize.m_y;
					m_options.UpdateBar();
				}
			}//for all blocks
		}

		//pass 2 : compute dNBR and fire severity
		if (!m_options.m_bQuiet)
			cout << "Compute fire severity with " << m_options.m_BLOCK_THREADS << " block threads (" << m_options.BLOCK_CPU() << " threads/block)" << endl;

		m_options.ResetBar((size_t)fires.size()*nbPixels);

		omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(m_options.m_BLOCK_THREADS) if (m_options.m_bMulti)
		for (__int64 b = 0; b < (__int64)XYindex.size(); b++)
		{
			size_t thread = omp_get_thread_num();
			size_t xBlock = XYindex[b].first;
			size_t yBlock = XYindex[b].second;

			if (bHaveFire[b])//there is some fire in this block?
			{
				OutputData output;
				DebugData debug;
				ReadBlock(xBlock, yBlock, bandHolder[thread]);
				ProcessBlock(xBlock, yBlock, bandHolder[thread], fires, bufferStat, output, debug);
				WriteBlock(xBlock, yBlock, output, debug, outputDS, debugDS);
			}
			else
			{
				CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);
#pragma omp atomic		
				m_options.m_xx += fires.size()* blockSize.m_x*blockSize.m_y;
				m_options.UpdateBar();
			}
		}//for all blocks

		//close inputs and outputs
		CloseAll(inputDS, maskDS, fireDS, outputDS, debugDS);

		GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
		cout << "Memory used: " << memCounter.WorkingSetSize / (1024 * 1024) << " Mo" << endl;
	}

	return msg;
}


void CFireSeverity::ReadBlock(size_t xBlock, size_t yBlock, CBandsHolder& bandHolder)
{
#pragma omp critical(ReadBlockIO)
	{
		m_options.m_timerRead.Start();
		bandHolder.LoadBlock((int)xBlock, (int)yBlock);
		m_options.m_timerRead.Stop();
	}
}

ERMsg CFireSeverity::LoadFires(CLandsatDataset& lansatDS, CGDALDatasetEx& fireDS, FireBitset& fires, std::vector <bool>& bHaveFire)
{
	ERMsg msg;



	if (msg)
	{
		CBandsHolderMT bandHolder = CBandsHolderMT(1, m_options.m_memoryLimit, m_options.m_IOCPU, 2);
		msg += bandHolder.Load(fireDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if (msg)
		{
			CGeoExtents extents = bandHolder.GetExtents();

			fires.resize(bandHolder.GetRasterCount());
			for (size_t i = 0; i < fires.size(); i++)
			{
				int year = GetYear(fireDS.GetInternalName(i));
				if (year != -999)
				{
					fires[i].first[T_JD] = CBaseOptions::GetTRefIndex(CBaseOptions::JDAY1970, CTRef(year, JANUARY, DAY_01));
					fires[i].first[T_Z] = FindLayerIndex(lansatDS, year);
					if (fires[i].first[T_Z] < lansatDS.GetNbScenes())
					{
						fires[i].second[T_FIRE].resize((size_t)extents.m_xSize*extents.m_ySize);
						if (m_options.m_buffer > 0)
							fires[i].second[T_BUFFER].resize((size_t)extents.m_xSize*extents.m_ySize);
					}
					else
					{
						msg.ajoute("Unable to find year (" + to_string(year) + ") in the input image");
					}
				}
				else
				{
					msg.ajoute("Error: file name of fire file must end with 4 digits year (YYYY)");
				}
			}

			if (msg)
			{
				vector<pair<int, int>> XYindex = extents.GetBlockList();
				bHaveFire.resize(XYindex.size(), false);
				if (!m_options.m_bQuiet)
					cout << "Load fires with " << m_options.m_CPU << " threads" << endl;

				size_t nbPixels = extents.m_xSize*extents.m_ySize;
				m_options.ResetBar((size_t)fires.size()*nbPixels * 2);

				CTimer timerRead(true);
				//pass 1 : load fire pixel
				omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(2) if (m_options.m_bMulti)
				for (__int64 b = 0; b < (__int64)XYindex.size(); b++)
				{

					size_t thread = omp_get_thread_num();
					size_t xBlock = XYindex[b].first;
					size_t yBlock = XYindex[b].second;
					CGeoRectIndex index = extents.GetBlockRect((int)xBlock, (int)yBlock);
					CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);

#pragma omp critical(LOAD_FIRES)
					{
						bandHolder[thread].LoadBlock((int)xBlock, (int)yBlock);
					}

#pragma omp critical(READ_FIRES)
					{
						if (!bandHolder[thread].IsEmpty())
						{
							bHaveFire[b] = true;
							CRasterWindow fire_window = bandHolder[thread].GetWindow();
							ASSERT(fire_window.GetNbScenes() >= fires.size());
#pragma omp parallel for num_threads(m_options.m_CPU) if (m_options.m_bMulti)
							for (__int64 zz = 0; zz < (__int64)fires.size(); zz++)
							{
								for (size_t y = 0; y < blockSize.m_y; y++)
								{
									for (size_t x = 0; x < blockSize.m_x; x++)
									{
										size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;
										if (fire_window[zz]->IsValid((int)x, (int)y))
										{
											bool bSet = fire_window[zz]->at((int)x, (int)y) > 0;
											fires[zz].second[T_FIRE].set(xy2, bSet);
										}
									}//x
								}//y
							}//zz
						}//block not empty

#pragma omp atomic
						m_options.m_xx += fires.size()*blockSize.m_x*blockSize.m_y;
						m_options.UpdateBar();
					}//critical
				}//

				//set buffer if any
				if (m_options.m_buffer > 0)
				{
#pragma omp parallel for num_threads(m_options.m_CPU) if (m_options.m_bMulti)
					for (__int64 zz = 0; zz < (__int64)fires.size(); zz++)
					{
						for (size_t y = 0; y < extents.m_ySize; y++)
						{
							for (size_t x = 0; x < extents.m_xSize; x++)
							{
								size_t xy = y * extents.m_xSize + x;
								if (fires[zz].second[T_FIRE].test(xy))
								{
									for (size_t yy = 0; (yy < 2 * m_options.m_buffer + 1); yy++)
									{
										for (size_t xx = 0; (xx < 2 * m_options.m_buffer + 1); xx++)
										{
											size_t yyy = y + yy - m_options.m_buffer;
											size_t xxx = x + xx - m_options.m_buffer;
											if (yyy < (size_t)extents.m_ySize && xxx < (size_t)extents.m_xSize)
											{
												size_t xy2 = yyy * extents.m_xSize + xxx;
												if (!fires[zz].second[T_FIRE].test(xy2))
													fires[zz].second[T_BUFFER].set(xy2, true);
											}
										}//xx
									}//yy
								}//if fire
							}//x
#pragma omp atomic
							m_options.m_xx += extents.m_xSize;
							m_options.UpdateBar();

						}//y
					}//zz
				}
				else
				{
					m_options.m_xx += fires.size()*nbPixels;
					m_options.UpdateBar();
				}

				timerRead.Stop();

				if (!m_options.m_bQuiet)
				{
					cout << endl << "Time to load fires = " << SecondToDHMS(timerRead.Elapsed()) << endl;
					cout << "Fire pixels " << endl;
					for (__int64 zz = 0; zz < (__int64)fires.size(); zz++)
						cout << zz + 1 << ":" << fires[zz].second[T_FIRE].count() << " (" << fires[zz].second[T_BUFFER].count() << ")" << endl;
				}

			}//if msg
		}//if msg
	}//if msg

	return msg;
}

size_t CFireSeverity::FindLayerIndex(CLandsatDataset& lansatDS, int year)
{
	size_t index = NOT_INIT;
	const std::vector<CTPeriod>& periods = lansatDS.GetScenePeriod();
	for (size_t z = 0; z < periods.size() && index == NOT_INIT; z++)
	{
		if (periods[z].GetFirstYear() == year)
			index = z;
	}

	return index;
}

int CFireSeverity::GetYear(const std::string& name)
{
	int year = -999;

	string title = GetFileTitle(name);
	if (title.length() > 4)
	{
		string sYear = title.substr(title.length() - 4);
		year = stoi(sYear);
	}
	
	return year;
}


size_t CFireSeverity::GetPrevious(size_t z, size_t x, size_t y, const CLandsatWindow& window)
{
	size_t previous = NOT_INIT;
	for (size_t zz = z - 1; zz < window.GetNbScenes() && previous == NOT_INIT; zz--)
		if (window.GetPixel(zz, (int)x, (int)y).IsValid())
			previous = zz;

	return previous;
}

size_t CFireSeverity::GetNext(size_t z, size_t x, size_t y, const CLandsatWindow& window)
{
	size_t next = NOT_INIT;
	for (size_t zz = z + 1; zz < window.GetNbScenes() && next == NOT_INIT; zz++)
		if (window.GetPixel(zz, (int)x, (int)y).IsValid())
			next = zz;

	return next;
}


void CFireSeverity::ComputeBufferStat(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const FireBitset& fires, BufferStat& bufferStat)
{
	size_t nbScenesLoaded = m_options.m_scenesLoaded[1] - m_options.m_scenesLoaded[0] + 1;
	size_t nbScenesProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;
	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect((int)xBlock, (int)yBlock);
	CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);

	ASSERT(!bandHolder.IsEmpty());

	CLandsatWindow window = bandHolder.GetWindow();


	//compute offset when buffer
#pragma omp parallel for num_threads( m_options.BLOCK_CPU()) if (m_options.m_bMulti)
	for (__int64 zz = 0; zz < (__int64)fires.size(); zz++)
	{
		size_t z = fires[zz].first[T_Z];
		__int16 JD_base = __int16(fires[zz].first[T_JD]);
		//find dates
		for (size_t y = 0; y < blockSize.m_y; y++)
		{
			for (size_t x = 0; x < blockSize.m_x; x++)
			{
				size_t xy = y * blockSize.m_x + x;
				size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;

				if (fires[zz].second[T_BUFFER].test(xy2))
				{
					size_t z0 = GetPrevious(z, x, y, window);
					size_t z2 = GetNext(z, x, y, window);
					if (z0 != NOT_INIT && z2 != NOT_INIT)
					{
						array <CLandsatPixel, 2> p;
						p[0] = window.GetPixel(z0, (int)x, (int)y);
						p[1] = window.GetPixel(z2, (int)x, (int)y);

						//buffer is variable that are share over many block at the same time
						//this variable must be protect when writing
#pragma omp critical(BUFFER_STAT)
						{
							pair<__int16, __int16> date = make_pair(p[0][I_JD], p[1][I_JD]);
							double dNBR = GetDeltaNBR(JD_base, p);
							bufferStat[zz][date] += dNBR;
						}
					}//if buffer
				}//is init
			}//x
		}//y


#pragma omp atomic		
		m_options.m_xx += (size_t)blockSize.m_x*blockSize.m_y;

		m_options.UpdateBar();

	}//zz

}

//Get input image reference
void CFireSeverity::ProcessBlock(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const FireBitset& fires, const BufferStat& bufferStat, OutputData& output, DebugData& debug)
{
	size_t nbScenesLoaded = m_options.m_scenesLoaded[1] - m_options.m_scenesLoaded[0] + 1;
	size_t nbScenesProcess = m_options.m_scenesTreated[1] - m_options.m_scenesTreated[0] + 1;
	size_t nbScenes = bandHolder.GetNbScenes();
	size_t sceneSize = bandHolder.GetSceneSize();
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoRectIndex index = extents.GetBlockRect((int)xBlock, (int)yBlock);
	CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);

	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += (nbScenesProcess + nbScenesLoaded)* blockSize.m_x*blockSize.m_y;
		m_options.UpdateBar();

		return;
	}

	//allocate process memory and load data
	CLandsatWindow window = bandHolder.GetWindow();

	__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);
	output.resize(fires.size());
	for (size_t i = 0; i < output.size(); i++)
	{
		for (size_t j = 0; j < output[i].size(); j++)
			output[i][j].resize(blockSize.m_x*blockSize.m_y, noData);
	}

	if (m_options.m_bDebug)
	{
		
		__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);
		debug.resize(fires.size());
		for (size_t i = 0; i < debug.size(); i++)
		{
			for (size_t j = 0; j < debug[i].size(); j++)
				debug[i][j].resize(blockSize.m_x*blockSize.m_y, noData);
		}
	}
	
	//compute dNBR and apply offset when buffer
#pragma omp parallel for num_threads( m_options.BLOCK_CPU()) if (m_options.m_bMulti)
	for (__int64 zz = 0; zz < (__int64)fires.size(); zz++)
	{
		size_t z = fires[zz].first[T_Z];
		__int16 JD_base = __int16(fires[zz].first[T_JD]);

		//find dates
		for (size_t y = 0; y < blockSize.m_y; y++)
		{
			for (size_t x = 0; x < blockSize.m_x; x++)
			{
				size_t xy = y * blockSize.m_x + x;
				size_t xy2 = ((size_t)index.m_y + y)* extents.m_xSize + index.m_x + x;

				if (fires[zz].second[T_FIRE].test(xy2))
				{
					size_t z0 = GetPrevious(z, x, y, window);
					size_t z2 = GetNext(z, x, y, window);
					if (z0 != NOT_INIT && z2 != NOT_INIT)
					{
						array <CLandsatPixel, 2> p;
						p[0] = window.GetPixel(z0, (int)x, (int)y);
						p[1] = window.GetPixel(z2, (int)x, (int)y);

						pair<__int16, __int16> date = make_pair(p[0][I_JD], p[1][I_JD]);
						auto it = bufferStat[zz].find(date);
						__int16 offset = 0;
						if (it != bufferStat[zz].end())
							offset = __int16(Round(it->second[MEAN]));

						__int16 dNBR = GetDeltaNBR(JD_base, p);
						array<__int16, 2> Zscore = GetZscore(p);
						output[zz][O_DNBR][xy] = dNBR - offset;
						output[zz][O_ZSCORE1][xy] = Zscore[0];
						output[zz][O_ZSCORE2][xy] = Zscore[1];
						output[zz][O_FIRE_SEV][xy] = GetFireSeverity(dNBR - offset);

						if (!debug.empty())
						{
							debug[zz][D_NB_MISSING][xy] = GetNbMissing(JD_base, p);
							debug[zz][D_OFFSET][xy] = offset;
							debug[zz][D_T1_B3][xy] = p[0][I_B3];
							debug[zz][D_T1_B4][xy] = p[0][I_B4];
							debug[zz][D_T1_B5][xy] = p[0][I_B5];
							debug[zz][D_T1_B7][xy] = p[0][I_B7];
							debug[zz][D_T3_B3][xy] = p[1][I_B3];
							debug[zz][D_T3_B4][xy] = p[1][I_B4];
							debug[zz][D_T3_B5][xy] = p[1][I_B5];
							debug[zz][D_T3_B7][xy] = p[1][I_B7];
						}
						
					}
				}
			}//x
		}//y


#pragma omp atomic		
		m_options.m_xx += (size_t)blockSize.m_x*blockSize.m_y;

		m_options.UpdateBar();

	}//zz
}

__int16 CFireSeverity::GetFireSeverity(__int16 dNBR)
{
	static const double F[2][2] =
	{
		{ 0.22, 0.311 },//Ron
		{ 0.09, 0.019 }//Jo
	};

	double fs = (double)Round(100.0 * ((double)dNBR / (F[1][0] * dNBR + F[1][1])));
	return __int16(max(-32767.0, min(32767.0, fs)));
}

array<__int16, 2> CFireSeverity::GetZscore(const array <CLandsatPixel, 2>& p)
{
	static const double F[2][4][2] =
	{
		{ {278.4841,121.1584},{1839.0676,365.3447},{920.9541,332.3372},{435.8769,195.7674 } },//pre-salvage
		{ {676.3447,676.3447},{1456.5572,253.4981},{2316.8939,2316.8939},{1855.5038,349.1989 } }//salvage
	};

	static const size_t B[4] = { I_B3, I_B4, I_B5, I_B7 };
	

	double Zsore[2] = { 0 };
	for (int f = 0; f < 2; f++)
	{
		for (int i = 0; i < 4; i++)
		{
			Zsore[f] += 10*Square((p[1][B[i]] - F[f][i][0]) / F[f][i][1]) / 4.0;
		}
	}

	//array<__int16, 2>
	return { {__int16(Zsore[0]), __int16(Zsore[1])} };
	//return (Zsore[0]<15&& Zsore[1]<10)?1:0;
}

__int16 CFireSeverity::GetNbMissing(__int16 JD_base, array <CLandsatPixel, 2> p)
{
	int nbDays = p[1][I_JD] - JD_base;
	ASSERT(nbDays >= 365); // at least the next year

	__int16 nbMissing = __int16(max(0, nbDays-365)/365);

	return nbMissing;
}

__int16 CFireSeverity::GetDeltaNBR(__int16 JD_base, array <CLandsatPixel, 2> p)
{
	//CTRef test1 = CBaseOptions::GetTRef(CBaseOptions::JDAY1970, JD_base);
	//CTRef test2 = CBaseOptions::GetTRef(CBaseOptions::JDAY1970, p[0][I_JD]);
	//CTRef test3 = CBaseOptions::GetTRef(CBaseOptions::JDAY1970, p[1][I_JD]);
	//int nbDays = p[1][I_JD] - JD_base;
	//ASSERT(nbDays >= 365); // at least the next year

	static const double F[2][4][2] =
	{
		//B3 B4 B5 B7
		{ {0.66, 138.87}, { 0.65,163.13 }, { 0.66,453.22 }, { 0.70,434.00 }},//T3 is missing
		{{0.60,156.54},{0.57,195.84},{0.60,506.01},{0.71,447.77}}//T3 and T4 is missing
	};

	__int16 nbMissing = GetNbMissing(JD_base, p);

	//make correction if missing year
	//if (nbDays > 2 * 365)
	if (nbMissing>0)
	{
		static const size_t B[4] = { B3, B4, B5, B7 };
		//size_t f = (nbDays < 3 * 365) ? 0 : 1;
		size_t f = nbMissing == 1 ? 0 : 1;

		for (int i = 0; i < 4; i++)
		{
			p[1][B[i]] = __int16(p[1][B[i]] * F[f][i][0] + F[f][i][1]);
		}
	}

	return __int16(p[0][I_NBR] - p[1][I_NBR]);
}



void CFireSeverity::WriteBlock(size_t xBlock, size_t yBlock, OutputData& output, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
{

#pragma omp critical(WriteBlockIO)
	{

		m_options.m_timerWrite.Start();

		CGeoExtents extents = outputDS.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize((int)xBlock, (int)yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect((int)xBlock, (int)yBlock);

		ASSERT(outputRect.Width() == blockSize.m_x);
		ASSERT(outputRect.Height() == blockSize.m_y);

		if (m_options.m_bCreateImage)
		{
			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);
			for (size_t z = 0; z < outputDS.GetRasterCount() / NB_OUTPUTS; z++)
			{
				for (size_t i = 0; i < NB_OUTPUTS; i++)
				{
					size_t b = z * NB_OUTPUTS + i;
					ASSERT(output.empty() || output[z][i].size() == outputRect.Width()*outputRect.Height());

					GDALRasterBand *pBand = outputDS.GetRasterBand(b);
					if (!output.empty())
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[z][i][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
					else
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
				}
			}
		}

		if (m_options.m_bDebug)
		{
			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);
			for (size_t z = 0; z < debugDS.GetRasterCount() / NB_DEBUGS; z++)
			{
				for (size_t i = 0; i < NB_DEBUGS; i++)
				{
					size_t b = z * NB_DEBUGS + i;
					ASSERT(debug.empty() || debug[z][i].size() == outputRect.Width()*outputRect.Height());

					GDALRasterBand *pBand = debugDS.GetRasterBand(b);
					if (!debug.empty())
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(debug[z][i][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
					else
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noData), 1, 1, GDT_Int16, 0, 0);
				}
			}
		}

		m_options.m_timerWrite.Stop();
	}

	

}

void CFireSeverity::CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& fireDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS)
{
	if (!m_options.m_bQuiet)
		_tprintf("\nClose all files...\n");

	landsatDS.Close();
	maskDS.Close();
	fireDS.Close();

	m_options.m_timerWrite.Start();
	outputDS.Close(m_options);
	debugDS.Close(m_options);


	m_options.m_timerWrite.Stop();
	m_options.PrintTime();
}

void CFireSeverity::LoadData(const CBandsHolder& bandHolder, LansatData& data)
{
	CGeoExtents extents = bandHolder.GetExtents();
	CLandsatWindow window = bandHolder.GetWindow();
	size_t nbScenesLoaded = m_options.m_scenesLoaded[1] - m_options.m_scenesLoaded[0] + 1;

	CGeoSize blockSize = window.GetGeoSize();
	data.resize(blockSize.m_x*blockSize.m_y);

	for (size_t y = 0; y < blockSize.m_y; y++)
		for (size_t x = 0; x < blockSize.m_x; x++)
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
			}
		}

		//#pragma omp atomic		
			//	m_options.m_xx += (size_t)blockSize.m_x*blockSize.m_y;
				//m_options.UpdateBar();

	}
}


string CFireSeverity::GetScenesDateFormat(const std::vector<CTPeriod>& p)
{
	string str;

	set<size_t> stat;
	for (size_t z = 0; z < p.size(); z++)
		stat.insert(p[z].size());

	if (*stat.rbegin() == 1)
		str = "%F";  //Individual input
	else //if (stat[HIGHEST] <= 366)
		str = "%Y";	//annual input
	//else
		//str = "%1-%2", "%Y");	//multi annual input

	return str;
}


