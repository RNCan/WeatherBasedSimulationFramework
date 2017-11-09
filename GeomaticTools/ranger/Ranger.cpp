//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 1.0.0	07/11/2017	Rémi Saint-Amant	Creation

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <bitset>
#include <deque>
#include <boost/dynamic_bitset.hpp>

#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Basic/OpenMP.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"

#include "Utility/DataShort.h"
#include "globals.h"
#include "Forest/ForestClassification.h"
#include "Forest/ForestRegression.h"
#include "Forest/ForestSurvival.h"
#include "Forest/ForestProbability.h"


#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF;
//using namespace WBSF::Landsat;

 //exemple_train_remi.regression.forest
//-seed 1234 -co COMPRESS=LZW -stats -overwrite -multi -IOCPU 3 "D:\Travaux\Ranger\Training\exemple_train_remi.regression.forest" "D:\Travaux\Ranger\Input\L8_006028_20150717_ext.vrt" "D:\Travaux\Ranger\Output\test.tif"
//-seed 1234 -te 661190 5098890 668810 5111820 -co COMPRESS=LZW -stats -overwrite -multi -IOCPU 3 "U:\GIS\#documents\TestCodes\Ranger\Training\exemple_train_remi.regression.forest" "U:\GIS\#documents\TestCodes\Ranger\Input\L8_006028_20150717_ext.vrt" "U:\GIS\#documents\TestCodes\Ranger\Output\output.tif"

static const char* version = "1.0.0";
static const int NB_THREAD_PROCESS = 2; 


enum TFilePath { FOREST_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
//enum TOutputBand{ O_FIRST_DATE, O_DISTURBANCE, O_DATE1, O_DATE2, O_LAST_DATE, NB_OUTPUT_BANDS };//O_NB_CONFIRM,
enum TOutputBand{ O_RESULT, NB_OUTPUT_BANDS };//O_NB_CONFIRM,


class CDisterbanceAnalyserOption : public CBaseOptions
{
public:
	CDisterbanceAnalyserOption()
	{
		m_seed = 0;
		m_nbPixel=0;
		m_nbPixelDT=0;
		m_scenesSize = 7;// SCENES_SIZE;
		m_appDescription = "This software look up (with a random forest tree model) for disturbance in a any number series of LANDSAT scenes";

		static const COptionDef OPTIONS[] = 
		{
		//	{ "-Trigger", 3, "tt op th", true, "Add optimization trigger to execute decision tree when comparing T-1 with T+1. tt is the trigger type, op is the comparison operator '<' or '>' and th is the trigger threshold. Supported type are \"B1\"..\"JD\", \"NBR\",\"EUCLIDEAN\", \"NDVI\", \"NDMI\", \"TCB\" (Tasseled Cap Brightness), \"TCG\" (Tasseled Cap Greenness) or \"TCW\" (Tasseled Cap Wetness)." },
			{ "-Seed", 1, "sd", false, "Seed for Random forest." },
			//{ "-Debug",0,"",false,"Output debug information."},
			{ "ForestFile",0,"",false,"Random forest model file path."},
			{ "src1file",0,"",false, "LANDSAT scenes image file path."},
			{ "dstfile",0,"",false, "Output image file path."}
		};
		
		for(int i=0; i<sizeof(OPTIONS)/sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] = 
		{
			{ "Input Model", "DTModel","","","","Decision tree model file generate by See5."},
			{ "LANDSAT Image", "src1file","","ScenesSize(9)*nbScenes","B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|B9: Date(Julian day 1970 or YYYYMMDD format) and cloud mask(NoData)|... for each scene"},
			//{ "Geophysical Image", "src2file", "", "3", "B1: Degres-day 5°C threshold|B2: Digital Elevation Model (DEM)|B3: Slope (?)" },
			{ "Output Image", "dstfile","One file per perturbation","6","FirstDate: date of the first image analysed|DTCode: disturbance code|D1: disturbace date of the first image|D2: disturbance date of the second image|NbContirm: number of image without disturbance after the disturbance|LastDate: date of the last image analysed"},
			//{ "Optional Output Image", "dstfile_FireSeverity","1","3","Ron|Jo|Mean of Ron and Jo"},
			//{ "Optional Output Image", "dstfile_ExportBands","One file per perturbation","OutputBands(9) x NbTime(8) = 36","T-2: Scenes 2 years preciding T|...|T+5: Scenes 5 years folowing T"},
			//{ "Optional Output Image", "dstfile_TimeSeries", "One file per input years", "Nb Years", "Y1: first year|...|Yn: last year" },
			//{ "Optional Output Image", "dstfile_debug","1","9"," NbPairs: number of \"Pair\", a pair is composed of 2 valid images|NbDisturbances: number of disturbances found in the series.|Disturbance: last diturbance|D1: date of the first image of the last disturbance|D2: date of the second image of the last disturbance|MeanD: mean NBR distance|MaxD1: highest NBR distance|MaxD2: second highest NBR distance|MaxD3: third highest NBR distance"}
		};

		for(int i=0; i<sizeof(IO_FILE_INFO)/sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);

	}

	virtual ERMsg ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);
		
		ASSERT( NB_FILE_PATH==3);
		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 3 files are needed: forest model, the input LANDSAT image and the destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		if (m_outputType != GDT_Unknown && m_outputType != GDT_Int16 && m_outputType != GDT_Int32)
			msg.ajoute("Invalid -ot option. Only GDT_Int16 or GDT_Int32 are supported");


		return msg;
	}

	virtual ERMsg ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;
		
		//if (IsEqual(argv[i], "-Trigger"))
		//{
		//	string str = argv[++i];
		//	TIndices type = GetIndiceType(str);
		//	string op = argv[++i];
		//	double threshold = atof(argv[++i]);

		//	if (type != I_INVALID)
		//	{
		//		if (CIndices::IsValidOp(op))
		//			m_trigger.push_back(CIndices(type, op, threshold));
		//		else
		//			msg.ajoute(op + " is an invalid operator for -Trigger option");
		//	}
		//	else
		//	{
		//		msg.ajoute(str + " is an invalid type for -Trigger option");
		//	}
		//		
		//}
		//else if (IsEqual(argv[i], "-Despike"))
		//{
		//	string str = argv[++i];
		//	TIndices type = GetIndiceType(str);
		//	//string op = argv[++i];
		//	double threshold = atof(argv[++i]);
		//	

		//	if (type != I_INVALID)
		//	{
		//		//if (CIndices::IsValidOp(op))
		//			m_despike.push_back(CIndices(type, "<", threshold));
		//		//else
		//			//msg.ajoute(op + " is an invalid operator for -Despike option");
		//		
		//	}
		//	else
		//	{
		//		msg.ajoute(str + " is an invalid type for -Despike option");
		//	}
		//}
		//else if (IsEqual(argv[i], "-nbDisturbances"))
  //      {
		//	m_nbDisturbances = atoi(argv[++i]);
		//}
		//else if (IsEqual(argv[i], "-FireSeverity"))
  //      {
		//	m_bFireSeverity = true;
		//}
		//else if (IsEqual(argv[i], "-ExportBands"))
		//{
		//	m_bExportBands = true;
		//}	
		//else if (IsEqual(argv[i], "-ExportTimeSeries"))
		//{
		//	m_bExportTimeSeries = true;
		//}
		if (IsEqual(argv[i], "-Seed"))
		{
			m_seed = atoi(argv[++i]);
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}
		
		return msg;
	}


	int m_seed;

	__int64 m_nbPixelDT;
	__int64 m_nbPixel;
};

typedef std::auto_ptr<Forest> ForestPtr;
//typedef std::vector < ForestPtr > ForestVector;
typedef std::deque < std::vector<__int16> > OutputData;
//***********************************************************************
//									 
//	Main                                                             
//									 
//***********************************************************************
class CDisterbanceAnalyser
{
public:

	string GetDescription() { return  string("RangerImages version ") + version + " (" + _T(__DATE__) + ")\n" ; }
	ERMsg Execute();


	ERMsg OpenAll(CGDALDatasetEx& lansatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);
	void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
	void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, ForestPtr& forest, OutputData& output);
	void WriteBlock(int xBlock, int yBlock, OutputData& output, CGDALDatasetEx& outputDS);
	void CloseAll(CGDALDatasetEx& landsatDS,  CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

	void Evaluate( int x, int y, const vector<array<short, 3>>& DTCode, vector<vector<vector<short>>>& output);
		
	ERMsg ReadRules(ForestPtr& forest);

	CDisterbanceAnalyserOption m_options;
};

// Create forest object
Forest* CreateForest(TreeType treetype)
{
	Forest* forest = NULL;
	switch (treetype) {
	case TREE_CLASSIFICATION:
		forest = new ForestClassification;
		break;
	case TREE_REGRESSION:
		forest = new ForestRegression;
		break;
	case TREE_SURVIVAL:
		forest = new ForestSurvival;
		break;
	case TREE_PROBABILITY:
		forest = new ForestProbability;
		break;
	}

	return forest;
}

ERMsg CDisterbanceAnalyser::ReadRules(ForestPtr& forest)
{
	ERMsg msg;
	if(!m_options.m_bQuiet) 
	{
		cout << "Read forest..."<<endl;
	}

	CTimer timer(true);

	
	TreeType treetype = GetTreeType(m_options.m_filesPath[FOREST_FILE_PATH]);
	
	
	//forests.reserve(m_options.m_CPU);
	forest.reset( CreateForest(treetype) );
	forest->init_predict(m_options.m_seed, m_options.m_CPU, false, DEFAULT_PREDICTIONTYPE);
	forest->loadFromFile(m_options.m_filesPath[FOREST_FILE_PATH]);
	

	timer.Stop();

	if( !m_options.m_bQuiet )
		cout << "Read forest time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;

	return msg;
}	


ERMsg CDisterbanceAnalyser::OpenAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
{
	ERMsg msg;
	
	
	if(!m_options.m_bQuiet) 
		cout << endl << "Open input image..." << endl;

	msg = landsatDS.OpenInputImage(m_options.m_filesPath[LANDSAT_FILE_PATH], m_options);

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

			//if (landsatDS.GetNbScenes() <= 1)
				//msg.ajoute("DisturbanceAnalyser can't be performed over only one scene");
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

		CDisterbanceAnalyserOption option = m_options;
		option.m_nbBands = NB_OUTPUT_BANDS;

		string filePath = option.m_filesPath[OUTPUT_FILE_PATH];
		msg += outputDS.CreateImage(filePath, option);
	}

	return msg;
}





ERMsg CDisterbanceAnalyser::Execute()
{
	ERMsg msg;

	if( !m_options.m_bQuiet )		
	{
		cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[LANDSAT_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[FOREST_FILE_PATH] << endl;

		if( !m_options.m_maskName.empty() )
			cout << "Mask:   "  << m_options.m_maskName << endl;
	}

	GDALAllRegister();

	
	if(!msg)
		return msg;


	ForestPtr forest;
	msg += ReadRules(forest);
	if (!msg)
		return msg;
	
	CGDALDatasetEx inputDS;
	CGDALDatasetEx maskDS;
	CGDALDatasetEx outputDS;

	msg = OpenAll(inputDS, maskDS, outputDS);
	
	if( msg)
	{
		size_t nbScenes = inputDS.GetNbScenes();
		size_t sceneSize = inputDS.GetSceneSize();
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
	

		if( maskDS.IsOpen() )
			bandHolder.SetMask( maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed );

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if(!msg)
			return msg;


		if( !m_options.m_bQuiet && m_options.m_bCreateImage) 
			cout << "Create output images " << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << NB_OUTPUT_BANDS << " B with " << m_options.m_CPU << " threads..." << endl;
		

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
			OutputData data;
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			ProcessBlock(xBlock, yBlock, bandHolder[thread], forest, data);
			WriteBlock(xBlock, yBlock, data, outputDS);
		}//for all blocks


		//close inputs and outputs
		CloseAll(inputDS, maskDS, outputDS);

	}
	
    return msg;
}

void CDisterbanceAnalyser::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
{
	#pragma omp critical(BlockIO)
	{
	
		m_options.m_timerRead.Start();
	
		bandHolder.LoadBlock(xBlock, yBlock);

		m_options.m_timerRead.Stop();
	}
}

//Get input image reference
void CDisterbanceAnalyser::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, ForestPtr& forest, OutputData& output)
{
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

	CRasterWindow window = bandHolder.GetWindow();
	if (m_options.m_bCreateImage)
	{
		output.resize(NB_OUTPUT_BANDS);
		for (size_t i = 0; i < output.size(); i++)
			output[i].insert(output[i].begin(), blockSize.m_x*blockSize.m_y, (__int16)m_options.m_dstNodata);
	}
	//

#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();

		bool bHaveData = false;
		DataShort input;
		input.resize(blockSize.m_x*blockSize.m_y, window.GetSceneSize());
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				int xy = y*blockSize.m_x + x;
				for (size_t z = 0; z < window.GetSceneSize(); z++)
				{
					if (window[z]->IsValid(x, y))
						bHaveData = true;
					bool error = false;
					input.set(z, xy, window[z]->at(x, y), error);
				}
			}
		}
		if (bHaveData)
		{
			
			forest->run_predict(&input);
			if (!output.empty())
			{
				for (int y = 0; y < blockSize.m_y; y++)
				{
					for (int x = 0; x < blockSize.m_x; x++)
					{
						int xy = y*blockSize.m_x + x;
						output[0][xy] = (__int16)(forest->getPredictions().at(0).at(0).at(xy));
					}
				}
			}
		}

		m_options.m_timerProcess.Stop();

		//#pragma omp atomic	
		m_options.m_xx += blockSize.m_x*blockSize.m_y;

		m_options.UpdateBar();
	}
}

void CDisterbanceAnalyser::WriteBlock(int xBlock, int yBlock, OutputData& output, CGDALDatasetEx& outputDS)
{

#pragma omp critical(BlockIO)
	{

		m_options.m_timerWrite.Start();
		//size_t nbScenes = data[0][0].size();
		CGeoExtents extents = outputDS.GetExtents();
		//CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

		if (m_options.m_bCreateImage)
		{
			__int16 noDataOut = (__int16)outputDS.GetNoData(0);
			for (size_t b = 0; b < outputDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(b);
				if (!output.empty())
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0][0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
				else
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noDataOut), 1, 1, GDT_Int16, 0, 0);


			}//for all output bands
		}//if create image

		m_options.m_timerWrite.Stop();

	}
}

void CDisterbanceAnalyser::CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
{
	if( !m_options.m_bQuiet )		
		_tprintf("\nClose all files...\n");

	landsatDS.Close();
	maskDS.Close(); 

	//close output
	m_options.m_timerWrite.Start();
	if( m_options.m_bComputeStats )
		outputDS.ComputeStats(m_options.m_bQuiet);
	if( !m_options.m_overviewLevels.empty() )
		outputDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);
	outputDS.Close();

		
	m_options.m_timerWrite.Stop();
		
	if( !m_options.m_bQuiet )
	{
		double percent = m_options.m_nbPixel>0?(double)m_options.m_nbPixelDT/m_options.m_nbPixel*100:0;

		_tprintf ("\n");
		_tprintf ("Percentage of pixel treated by Ranger: %0.3lf %%\n\n", percent);
	}

	m_options.PrintTime();
}


int _tmain(int argc, _TCHAR* argv[])
{
	CTimer timer(true);
	
	CDisterbanceAnalyser RandomForest;
	ERMsg msg = RandomForest.m_options.ParseOption(argc, argv);

	if (!msg || !RandomForest.m_options.m_bQuiet)
		cout << RandomForest.GetDescription() << endl;


	if (msg)
	{
		try {
			msg = RandomForest.Execute();
		}
		catch (std::exception& e) {
			std::cerr << "Error: " << e.what() << " Ranger will EXIT now." << std::endl;
			return -1;
		}
	}
		

	if( !msg)  
	{
		PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!RandomForest.m_options.m_bQuiet)
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}


