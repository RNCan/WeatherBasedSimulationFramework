//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 2.0.0	29/09/2018	Rémi Saint-Amant	Creation from DisturbanceAnalyser




#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <bitset>
#include <boost/dynamic_bitset.hpp>

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



static const char* version = "2.0.0";


enum TFilePath { SEE5_FILE_PATH, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
typedef vector<float> DTCodeVector;
typedef CDecisionTreeBlock CSee5TreeBlock;
typedef CDecisionTreeBaseEx CSee5Tree;
typedef CDecisionTree CSee5TreeMT;
typedef std::vector<float>  OutputData;



class CSee5Option : public CBaseOptions
{
public:
	CSee5Option()
	{
		m_scenesSize = 1;
		m_appDescription = "This software interpolate values (with a decision tree model from See5) from input images";
		m_BLOCK_THREADS = omp_get_num_procs();
		
		static const COptionDef OPTIONS[] =
		{
			{"-BLOCK_THREADS",1,"threads",false,"Number of threads used to process blocks. nb CPU by default. Only used when -multi is define. "},
			{ "See5Model",0,"",false,"See5 model file path."},
			{ "srcfile",0,"",false, "Input image file path."},
			{ "dstfile",0,"",false, "Output image file path."}
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		//RemoveOption("-BLOCK_THREADS");

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Model", "DTModel","","","","Decision tree model file generate by See5."},
			{ "Input Image", "srcfile","","nbBands","same number iof band as the input See5 model"},
			{ "Output Image", "dstfile","1","","See5 model result"},
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
			msg.ajoute("ERROR: Invalid argument line. 3 files are needed: See5 model, the input LANDSAT image and the destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}
		
		if(!m_bMulti)
			m_BLOCK_THREADS = 1;

		return msg;
	}

	virtual ERMsg ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		//Look to see if it's a know base option
		msg = CBaseOptions::ProcessOption(i, argc, argv);


		return msg;
	}

};

//***********************************************************************
//									 
//	Main                                                             
//									 
//***********************************************************************

class CSee5
{
public:

	string GetDescription() { return  string("See5Images version ") + version + " (" + _T(__DATE__) + ")\n"; }
	ERMsg Execute();


	ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);
	void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
	void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, CSee5Tree& DT, OutputData& output);
	void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& output);
	void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

	ERMsg LoadModel(string filePath, CSee5TreeMT& DT);

	CSee5Option m_options;

};

ERMsg CSee5::LoadModel(string filePath, CSee5TreeMT& DT)
{
	ERMsg msg;
	if (!m_options.m_bQuiet)
	{
		cout << "Read rules..." << endl;
	}

	CTimer timer(true);

	msg += DT.Load(filePath, m_options.m_BLOCK_THREADS, m_options.m_IOCPU);
	timer.Stop();

	if (!m_options.m_bQuiet)
		cout << "Read rules time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;


	return msg;
}


ERMsg CSee5::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
{
	ERMsg msg;


	if (!m_options.m_bQuiet)
		cout << endl << "Open input image..." << endl;

	msg = inputDS.OpenInputImage(m_options.m_filesPath[INPUT_FILE_PATH], m_options);
	
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

		CSee5Option option = m_options;
		option.m_nbBands = 1;

		string filePath = option.m_filesPath[OUTPUT_FILE_PATH];
		msg += outputDS.CreateImage(filePath, option);
	}


	return msg;
}



ERMsg CSee5::Execute()
{
	ERMsg msg;

	if (!m_options.m_bQuiet)
	{
		cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[SEE5_FILE_PATH] << endl;

		if (!m_options.m_maskName.empty())
			cout << "Mask:   " << m_options.m_maskName << endl;
	}

	GDALAllRegister();

	//if (m_options.m_outputType != GDT_Unknown && m_options.m_outputType != GDT_Int16 && m_options.m_outputType != GDT_Int32)
		//msg.ajoute("Invalid -ot option. Only GDT_Int16 or GDT_Int32 are supported");

	if (!msg)
		return msg;


	CSee5TreeMT DT;
	msg += LoadModel(m_options.m_filesPath[SEE5_FILE_PATH], DT);
	
	if (!msg)
		return msg;

	CGDALDatasetEx inputDS;
	CGDALDatasetEx maskDS;
	CGDALDatasetEx outputDS;
	

	msg = OpenAll(inputDS, maskDS, outputDS);
	
	if (msg && inputDS.GetRasterCount() != DT.front().NTest)
		msg.ajoute("The number of bands in the input image (" + to_string(inputDS.GetRasterCount()) + ") is not equal as the number of independants variables (" + to_string(DT.front().NTest) + ") in the model.");

	if (msg)
	{
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);

		if (maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);
		if (!msg)
			return msg;

		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R ) with " << m_options.m_BLOCK_THREADS << " threads..." << endl;


		CGeoExtents extents = bandHolder.GetExtents();
		m_options.ResetBar((size_t)extents.m_xSize*extents.m_ySize);

		vector<pair<int, int>> XYindex = extents.GetBlockList();

		omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(m_options.m_BLOCK_THREADS) if (m_options.m_bMulti)
		for (int b = 0; b < (int)XYindex.size(); b++)
		{
			int thread = omp_get_thread_num();
			int xBlock = XYindex[b].first;
			int yBlock = XYindex[b].second;

			//data
			OutputData data;
			ReadBlock(xBlock, yBlock, bandHolder[thread]);
			ProcessBlock(xBlock, yBlock, bandHolder[thread], DT[thread], data);
			WriteBlock(xBlock, yBlock, outputDS, data);
		}//for all blocks


		//  Close decision tree and free allocated memory  
		DT.FreeMemory();

		//close inputs and outputs
		CloseAll(inputDS, maskDS, outputDS);

	}

	return msg;
}

void CSee5::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
{
#pragma omp critical(BlockIORead)
	{

		m_options.m_timerRead.Start();
		bandHolder.LoadBlock(xBlock, yBlock);
		m_options.m_timerRead.Stop();
	}
}

//Get input image reference
void CSee5::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, CSee5Tree& DT, OutputData& data)
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


//#pragma omp critical(ProcessBlock)
	//{
	//	m_options.m_timerProcess.Start();

		//allocate process memory
		data.resize(nbCells);

		//process all x and y 
//#pragma omp parallel for schedule(static, 1) num_threads( m_options.BLOCK_CPU() ) if (m_options.m_bMulti)  
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				size_t xy = (size_t )y * blockSize.m_x + x;
				//int thread = ::omp_get_thread_num();
				
				//process all images
				CSee5TreeBlock block(DT.MaxAtt + 1);

				//fill the data structure for decision tree
				size_t c = 0;
				DVal(block, c++) = DT.MaxClass + 1;
				DVal(block, c++) = Continuous(DT, 1) ? DT_UNKNOWN : 0;
				
				for (size_t z = 0; z < bandHolder.size(); z++)
				{
					DataType pixel = bandHolder.GetPixel(z, x, y);
					CVal(block, c++) = (ContValue)(pixel);
				}

				ASSERT(c == (bandHolder.size() + 2));

				//fill virtual bands 
				//assuming virtual band never return no data
				for (; c <= DT.MaxAtt; c++)
				{
					ASSERT(DT.AttDef[c]);
					block[c] = DT.EvaluateDef(DT.AttDef[c], block.data());
				}

				ASSERT(!block.empty());
				int predict = (int)DT.Classify(block.data());


				ASSERT(predict >= 1 && predict <= DT.MaxClass);
				int DTCode = atoi(DT.ClassName[predict]);
				data[xy] = (float)DTCode;

#pragma omp atomic	
				m_options.m_xx++;

			}//for x
		}//for y


		m_options.UpdateBar();
	//}//for y



	//m_options.m_timerProcess.Stop();

}

void CSee5::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& data)
{
	if (m_options.m_bCreateImage)
	{
#pragma omp critical(BlockIOWrite)
		{

			m_options.m_timerWrite.Start();

			CGeoExtents extents = outputDS.GetExtents();
			CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
			CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);

			ASSERT(outputRect.Width() == blockSize.m_x);
			ASSERT(outputRect.Height() == blockSize.m_y);


			float noDataOut = (float)outputDS.GetNoData(0);
			ASSERT(data.size() == blockSize.m_x*blockSize.m_y);
			GDALRasterBand *pBand = outputDS.GetRasterBand(0);

			//for (size_t b = 0; b < outputDS.GetRasterCount(); b++)
			if(!data.empty())
			{
				pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(data[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
			}//for all output bands
			else
			{
				pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noDataOut, 1, 1, GDT_Float32, 0, 0);
			}

			m_options.m_timerWrite.Stop();
		}
	}//if create image

}

void CSee5::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
{
	if (!m_options.m_bQuiet)
		_tprintf("\nClose all files...\n");

	inputDS.Close();
	maskDS.Close();

	//close output
	m_options.m_timerWrite.Start();
	outputDS.Close(m_options);
	m_options.m_timerWrite.Stop();


	m_options.PrintTime();
}


int _tmain(int argc, _TCHAR* argv[])
{
	CTimer timer(true);

	CSee5 see5;
	ERMsg msg = see5.m_options.ParseOption(argc, argv);

	if (!msg || !see5.m_options.m_bQuiet)
		cout << see5.GetDescription() << endl;


	if (msg)
		msg = see5.Execute();

	if (!msg)
	{
		PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!see5.m_options.m_bQuiet)
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}


