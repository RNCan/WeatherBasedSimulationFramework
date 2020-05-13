//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 1.1.3	12/05/2020	Rémi Saint-Amant	Add bands_name
// 1.1.2	06/05/2020	Rémi Saint-Amant	Bug correction in probability 
// 1.1.1	14/02/2019	Rémi Saint-Amant	Update
// 1.1.0	28/09/2018	Rémi Saint-Amant	Add virtual variables
// 1.0.2	22/05/2018	Rémi Saint-Amant	Compile with VS 2017
// 1.0.1	09/01/2018	Rémi Saint-Amant	bug correction with no end loop
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

#include "RangerLib/RangerLib.h"


#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF;
//using namespace WBSF::Landsat;

 //exemple_train_remi.regression.forest
//-seed 1234 -co COMPRESS=LZW -stats -overwrite -multi -IOCPU 3 "D:\Travaux\Ranger\Training\exemple_train_remi.regression.forest" "D:\Travaux\Ranger\Input\L8_006028_20150717_ext.vrt" "D:\Travaux\Ranger\Output\test.tif"
//-te 661190 5098890 668810 5111820 -seed 1234 -co COMPRESS=LZW -stats -overwrite -multi -IOCPU 3 -mask U:\GIS\#documents\TestCodes\Ranger\Input\Ocean_CB_30_WGS84_UTM20_K.tif -maskValue 1 "U:\GIS\#documents\TestCodes\Ranger\Training\exemple_train_remi.regression.forest" "U:\GIS\#documents\TestCodes\Ranger\Input\L8_006028_20150717_ext.vrt" "U:\GIS\#documents\TestCodes\Ranger\Output\output.tif"
//-te 2025000 6952000 2226000 7154000 -multi -co compress=LZW -multi -blocksize 1024 1024 -stats -hist -co tiled=YES -co BLOCKXSIZE=1024 -co BLOCKYSIZE=1024 --config GDAL_CACHEMAX 1024 -ot int16 -dstnodata 255  -seed %seed% -overview {16} -overwrite -mask U:\GIS\#projets\LAQ\LAI\ANALYSE\20170815_Map_demo\test_code_remi_v1\BKP_9616_050_S7\RUN_RF\LOSSmsk_BK2_BK1.tif -maskvalue 1 -iocpu 3 "U:\GIS\#projets\LAQ\LAI\ANALYSE\20170815_Map_demo\test_code_remi_v1\BKP_9616_050_S7\RUN_RF\test2_pv.classification.forest" "U:\GIS\#projets\LAQ\LAI\ANALYSE\20170815_Map_demo\test_code_remi_v1\BKP_9616_050_S7\RUN_RF\BK2_BK1_B123457.vrt" "U:\GIS\#documents\TestCodes\Ranger\Output\TestDeadLock.tif"
//-te 2156430 7096950 2162160 7101150 

static const char* version = "1.1.2";
static const int NB_THREAD_PROCESS = 2;


enum TFilePath { FOREST_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
enum TOutputBand { O_RESULT, NB_OUTPUT_BANDS };


class CRangerImageOption : public CBaseOptions
{
public:
	CRangerImageOption()
	{
		m_seed = 0;
		m_bUncertainty = false;
		m_appDescription = "This software look up (with a random forest tree model) for disturbance in a any number series of LANDSAT scenes";

		static const COptionDef OPTIONS[] =
		{
			{ "-Seed", 1, "sd", false, "Seed for Random forest." },
			{ "-Uncertainty",0,"",false,"Output uncertainty of prediction classification. Standard error for regression."},
			//{ "-bands_name",0,"",false,"Rename input image bands to fit with forest columns name."},
			{ "ForestFile",0,"",false,"Random forest model file path."},
			{ "srcfile",0,"",false, "LANDSAT scenes image file path."},
			{ "dstfile",0,"",false, "Output image file path."}
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Model", "Model","","","","Decision tree model file generate by Ranger."},
			{ "Input Image", "src1file","","Input image. The number of independant variables mnust be the same as the ranger model (without virtual variables)",""},
			{ "Output Image", "dstfile","Ranger estimation"},
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);

	}

	virtual ERMsg ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		ASSERT(NB_FILE_PATH == 3);
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

		if (IsEqual(argv[i], "-Uncertainty"))
		{
			m_bUncertainty = true;
		}
		else if (IsEqual(argv[i], "-Seed"))
		{
			m_seed = atoi(argv[++i]);
		}
		else if (IsEqual(argv[i], "-bands_name"))
		{
			bands_name = argv[++i];
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}

	bool m_bUncertainty;
	int m_seed;
	vector<string> m_cols_name;
	string bands_name;
};

typedef std::auto_ptr<Forest> ForestPtr;
typedef std::deque < std::vector<float> > OutputData;
typedef std::vector<float>  UncertaintyData;

//***********************************************************************
//									 
//	Main                                                             
//									 
//***********************************************************************
class CRangerImage
{
public:

	string GetDescription() { return  string("RangerImages version ") + version + " (" + _T(__DATE__) + ")\n"; }
	ERMsg Execute();


	ERMsg OpenAll(CGDALDatasetEx& lansatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& uncertaintyDS);
	void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
	void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, ForestPtr& forest, OutputData& output, UncertaintyData& uncertainty);
	void WriteBlock(int xBlock, int yBlock, OutputData& output, UncertaintyData& uncertainty, CGDALDatasetEx& outputDS, CGDALDatasetEx& uncertaintyDS);
	void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& uncertaintyDS);

	//void Evaluate( int x, int y, const vector<array<short, 3>>& DTCode, vector<vector<vector<short>>>& output);

	ERMsg ReadForest(ForestPtr& forest);

	CRangerImageOption m_options;
};

ERMsg CRangerImage::ReadForest(ForestPtr& forest)
{
	ERMsg msg;
	if (!m_options.m_bQuiet)
	{
		cout << "Read forest..." << endl;
	}

	TreeType treetype = GetTreeType(m_options.m_filesPath[FOREST_FILE_PATH]);
	if (treetype != TREE_UNKNOW)
	{
		CTimer timer(true);

		forest.reset(CreateForest(treetype));
		forest->init_predict(m_options.m_seed, m_options.m_bMulti ? m_options.m_CPU : -1, false, DEFAULT_PREDICTIONTYPE);
		//forest->init_predict(m_options.m_seed, 1, false, DEFAULT_PREDICTIONTYPE);
		forest->loadFromFile(m_options.m_filesPath[FOREST_FILE_PATH]);

		cout << "Forest name:                       " << GetFileTitle(m_options.m_filesPath[FOREST_FILE_PATH]) << std::endl;
		cout << "Forest type                        " << GetTreeTypeStr(treetype) << std::endl;
		cout << "Number of trees:                   " << forest->getNumTrees() << std::endl;
		cout << "Dependent variable ID:             " << forest->getDependentVarId() + 1 << std::endl;
		cout << "Number of input variables:         " << forest->getNumIndependentVariables() - forest->get_virtual_cols_name().size() << std::endl;
		cout << "Number of virtual variables:       " << forest->get_virtual_cols_name().size() << std::endl;
		cout << "Number of independent variables:   " << forest->getNumIndependentVariables() << std::endl;
		cout << "Seed:                              " << m_options.m_seed << std::endl;


		timer.Stop();

		if (!m_options.m_bQuiet)
			cout << "Read forest time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;

		if (forest->getNumIndependentVariables() == 0)
			msg.ajoute("Invalid tree:" + m_options.m_filesPath[FOREST_FILE_PATH]);
	}
	else
	{
		msg.ajoute("Unable to determine tree type of " + m_options.m_filesPath[FOREST_FILE_PATH]);
		if (!FileExists(m_options.m_filesPath[FOREST_FILE_PATH]))
			msg.ajoute("File doesn't exist");

		return msg;
	}

	return msg;
}


ERMsg CRangerImage::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& uncertaintyDS)
{
	ERMsg msg;


	if (!m_options.m_bQuiet)
		cout << endl << "Open input image..." << endl;

	msg = inputDS.OpenInputImage(m_options.m_filesPath[LANDSAT_FILE_PATH], m_options);

	if (msg)
	{
		inputDS.UpdateOption(m_options);

		if (!m_options.m_bQuiet)
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << inputDS->GetRasterXSize() << " cols x " << inputDS->GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
		}


		//for (size_t i = 0; i < inputDS.GetRasterCount(); i++)
		//{
		//	string name = GetFileTitle(inputDS.GetInternalName(i));
		//	if (!name.empty())
		//		//name = "B" + to_string(i + 1);
		//	m_options.m_cols_name.push_back(name);
		//}

		//if (!m_options.bands_name.empty())
		//{
		//	StringVector tmp(m_options.bands_name, ",");
		//	if (tmp.size() == inputDS.GetRasterCount())
		//		m_options.m_cols_name = tmp;
		//	else
		//		msg.ajoute("Invalid option -bands_name: the number of columns found "+to_string(tmp.size())+" is not equal the number of images bands "+to_string(inputDS.GetRasterCount()));
		//}
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

		CRangerImageOption option = m_options;
		option.m_nbBands = NB_OUTPUT_BANDS;

		string filePath = option.m_filesPath[OUTPUT_FILE_PATH];
		msg += outputDS.CreateImage(filePath, option);
	}

	if (msg && m_options.m_bUncertainty)
	{

		if (!m_options.m_bQuiet)
			cout << "Create uncertainty image..." << endl;

		CRangerImageOption option = m_options;
		option.m_nbBands = 1;
		option.m_outputType = GDT_Float32;
		option.m_dstNodata = GetDefaultNoData(GDT_Float32);

		string filePath = option.m_filesPath[OUTPUT_FILE_PATH];
		SetFileTitle(filePath, GetFileTitle(filePath) + "_uncertainty");
		msg += uncertaintyDS.CreateImage(filePath, option);
	}
	return msg;
}





ERMsg CRangerImage::Execute()
{
	ERMsg msg;

	if (!m_options.m_bQuiet)
	{
		cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[LANDSAT_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[FOREST_FILE_PATH] << endl;

		if (!m_options.m_maskName.empty())
			cout << "Mask:   " << m_options.m_maskName << endl;
	}

	GDALAllRegister();


	if (!msg)
		return msg;


	ForestPtr forest;
	msg += ReadForest(forest);
	if (!msg)
		return msg;


	CGDALDatasetEx inputDS;
	CGDALDatasetEx maskDS;
	CGDALDatasetEx outputDS;
	CGDALDatasetEx uncertaintyDS;

	msg = OpenAll(inputDS, maskDS, outputDS, uncertaintyDS);

	if (msg)
	{
		std::vector<std::string> var_name = forest->getinitial_input_cols_name();

		//if (m_options.m_cols_name.empty())//case with .geotiff
		//{
		if (inputDS.GetRasterCount() == var_name.size())
		{
			m_options.m_cols_name = var_name;
			cout << "Number of bands in input image equal the number of input forest variables. Dependant variable assume to be present." << endl;
		}
		else
		{
			//remove dependant and status
			auto it = find(var_name.begin(), var_name.end(), forest->get_dependent_variable_name());
			if (it != var_name.end())
				var_name.erase(it);
			
			//remove status variable
			if (!forest->get_status_var_name().empty())
			{
				auto it = find(var_name.begin(), var_name.end(), forest->get_status_var_name());
				if (it != var_name.end())
					var_name.erase(it);
			}

			if (inputDS.GetRasterCount() == var_name.size())
			{
				m_options.m_cols_name = var_name;
				cout << "Number of bands in input image equal the number of input forest variables "<< to_string(int(var_name.size()-forest->getinitial_input_cols_name().size())) << endl;
			}
			else
			{
				msg.ajoute("The number of input raster bands (" + ToString(inputDS.GetRasterCount()) + ") in the input image is inconsistent with the number of the input variables (" + ToString(var_name.size()) + ") in the forest model.");
				//Virtual variables (" + to_string(forest->get_virtual_cols_name().size()) + ") are excluded
				return msg;
			}
		}
		//}
		//else//case with .vrt
		//{
		//	ASSERT(inputDS.GetRasterCount() == m_options.m_cols_name.size());
		//	if (var_name.size() - 1 == m_options.m_cols_name.size())
		//	{
		//		cout << "Number of band in input image equal the number of forest variables-1. " << endl;
		//	}
		//	else if (var_name.size() == m_options.m_cols_name.size())
		//	{
		//		cout << "Number of band in input image equal the number of forest variables. Dependant variable assume to be presnet." << endl;
		//	}
		//	else
		//	{
		//		msg.ajoute("The number of input raster bands (" + ToString(inputDS.GetRasterCount()) + ") in the input image is not equal to the number of the input variables (" + ToString(var_name.size()) + ") in the forest model.");
		//		return msg;
		//	}
		//}

		//forest->getNumIndependentVariables() - forest->get_virtual_cols_name().size();
		//if (inputDS.GetRasterCount() == var_name.size())
		//{
		//	//var_name;
		//	//verify that band ane are identical to csv col name
		//	//the variable name semm to not be available in the model
		//	m_options.m_cols_name;
		//	//forest->>data().getVariableNames();
		//}
		//else
		//{
		//	msg.ajoute("The number of input raster bands (" + ToString(inputDS.GetRasterCount()) + ") in the input image is not equal to the number of the input variables (" + ToString(var_name.size()) + ") in the forest model. Virtual variables ("+ to_string(forest->get_virtual_cols_name().size()) +") are excluded");
		//	return msg;
		//}

		//size_t nbScenes = inputDS.GetNbScenes();
		//size_t sceneSize = inputDS.GetSceneSize();
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);


		if (maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

		if (!msg)
			return msg;


		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images " << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << NB_OUTPUT_BANDS << " B with " << m_options.m_CPU << " threads..." << endl;


		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);

		vector<pair<int, int>> XYindex = extents.GetBlockList();

		omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int thread = omp_get_thread_num();
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			//data
			OutputData data;
			UncertaintyData uncertainty;
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			ProcessBlock(xBlock, yBlock, bandHolder[thread], forest, data, uncertainty);
			WriteBlock(xBlock, yBlock, data, uncertainty, outputDS, uncertaintyDS);
		}//for all blocks


		//close inputs and outputs
		CloseAll(inputDS, maskDS, outputDS, uncertaintyDS);

	}

	return msg;
}

void CRangerImage::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
{
#pragma omp critical(BlockIORead)
	{

		m_options.m_timerRead.Start();

		bandHolder.LoadBlock(xBlock, yBlock);

		m_options.m_timerRead.Stop();
	}
}

//Get input image reference
void CRangerImage::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, ForestPtr& forest, OutputData& output, UncertaintyData& uncertainty)
{
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
	size_t nbCells = (size_t)extents.m_xSize*extents.m_ySize;

	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += (std::min(nbCells, (size_t)blockSize.m_x*blockSize.m_y));
		m_options.UpdateBar();

		return;
	}

	CRasterWindow window = bandHolder.GetWindow();

	//

#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();

		boost::dynamic_bitset<size_t> validPixel(blockSize.m_x*blockSize.m_y);
		for (size_t y = 0; y < blockSize.m_y; y++)
		{
			for (size_t x = 0; x < blockSize.m_x; x++)
			{
				bool bValid = true;
				for (size_t z = 0; z < window.size() && bValid; z++)//for all bands
				{
					if (!window[z]->IsValid((int)x, (int)y))
						bValid = false;
				}

				if (bValid)
				{
					size_t xy = y * blockSize.m_x + x;
					validPixel.set(xy);
				}
			}
		}

		if (validPixel.count() > 0)
		{
			if (m_options.m_bUncertainty)
				uncertainty.insert(uncertainty.begin(), blockSize.m_x*blockSize.m_y, (float)GetDefaultNoData(GDT_Float32));

			if (m_options.m_bCreateImage)
			{
				output.resize(NB_OUTPUT_BANDS);
				for (size_t i = 0; i < output.size(); i++)
					output[i].resize(blockSize.m_x*blockSize.m_y, (float)m_options.m_dstNodata);
			}


			DataShort input;
			//vector<string> names = m_options.m_cols_name;
			//names.insert(names.end(), forest->get_virtual_cols_name().begin(), forest->get_virtual_cols_name().end());
			//input.set_virtual_cols(forest->get_virtual_cols_txt(), forest->get_virtual_cols_name());
			//input.resize(validPixel.count(), names);
			input.resize(validPixel.count(), m_options.m_cols_name);

			size_t cur_xy = 0;
			for (size_t y = 0; y < blockSize.m_y; y++)
			{
				for (size_t x = 0; x < blockSize.m_x; x++)
				{
					size_t xy = y * blockSize.m_x + x;
					if (validPixel.test(xy))
					{
						for (size_t z = 0; z < window.size(); z++)//for all bands
						{
							bool error = false;
							input.set(z, cur_xy, window[z]->at((int)x, (int)y), error);
						}
						cur_xy++;
					}
				}
			}

			//a optimiser pour eviter de copier les donnees
			if (!input.update_virtual_cols(forest->get_virtual_cols_txt(), forest->get_virtual_cols_name()))
				return;


			//remove cols to ignore
			input.reshape(forest->get_independent_variable_names());
			assert(input.getNumCols() == forest->get_independent_variable_names().size());

			if (input.getNumCols() == forest->get_independent_variable_names().size())
			{

				forest->run_predict(&input);

				cur_xy = 0;
				for (size_t y = 0; y < blockSize.m_y; y++)
				{
					for (size_t x = 0; x < blockSize.m_x; x++)
					{
						size_t xy = y * blockSize.m_x + x;
						if (validPixel.test(xy))
						{
							if (!output.empty())
								output[0][xy] = (float)(forest->getPredictions(cur_xy));

							if (!uncertainty.empty())
								uncertainty[xy] = (float)(forest->getUncertainty(cur_xy));

							cur_xy++;
						}
					}
				}
			}
			else
			{

			}
		}


		m_options.m_xx += blockSize.m_x*blockSize.m_y;
		m_options.UpdateBar();

		m_options.m_timerProcess.Stop();

	}
}

void CRangerImage::WriteBlock(int xBlock, int yBlock, OutputData& output, UncertaintyData& uncertainty, CGDALDatasetEx& outputDS, CGDALDatasetEx& uncertaintyDS)
{

#pragma omp critical(BlockIOWrite)
	{
		m_options.m_timerWrite.Start();

		CGeoExtents extents = outputDS.GetExtents();
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

		if (m_options.m_bCreateImage)
		{
			float noDataOut = (float)outputDS.GetNoData(0);
			for (size_t b = 0; b < outputDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = outputDS.GetRasterBand(b);
				if (!output.empty())
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0][0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
				else
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noDataOut), 1, 1, GDT_Float32, 0, 0);


			}//for all output bands
		}//if create image

		if (m_options.m_bUncertainty)
		{
			float noDataOut = (float)uncertaintyDS.GetNoData(0);
			for (size_t b = 0; b < uncertaintyDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = uncertaintyDS.GetRasterBand(b);
				if (!output.empty())
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(uncertainty[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
				else
					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(noDataOut), 1, 1, GDT_Float32, 0, 0);
			}//for all output bands
		}//if create image

		m_options.m_timerWrite.Stop();

	}
}

void CRangerImage::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& uncertaintyDS)
{
	if (!m_options.m_bQuiet)
		_tprintf("\nClose all files...\n");

	inputDS.Close();
	maskDS.Close();

	//close output
	m_options.m_timerWrite.Start();
	//if( m_options.m_bComputeStats )
	//	outputDS.ComputeStats(m_options.m_bQuiet);
	//if( !m_options.m_overviewLevels.empty() )
	//	outputDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);
	outputDS.Close(m_options);


	//if (m_options.m_bComputeStats)
		//uncertaintyDS.ComputeStats(m_options.m_bQuiet);
	//if (!m_options.m_overviewLevels.empty())
		//uncertaintyDS.BuildOverviews(m_options.m_overviewLevels, m_options.m_bQuiet);
	uncertaintyDS.Close(m_options);



	m_options.m_timerWrite.Stop();

	/*if( !m_options.m_bQuiet )
	{
		double percent = m_options.m_nbPixel>0?(double)m_options.m_nbPixelDT/m_options.m_nbPixel*100:0;

		_tprintf ("\n");
		_tprintf ("Percentage of pixel treated by Ranger: %0.3lf %%\n\n", percent);
	}*/

	m_options.PrintTime();
}


int _tmain(int argc, _TCHAR* argv[])
{
	CTimer timer(true);

	CRangerImage RandomForest;
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


	if (!msg)
	{
		PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!RandomForest.m_options.m_bQuiet)
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}


