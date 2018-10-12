//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 2.0.0	29/09/2018	Rémi Saint-Amant	Creation from DisturbanceAnalyser




#define NOMINMAX 
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>


//#include "stdafx.h"
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
#include "Cubistdefns.h"
#include "Geomatic/LandsatDataset.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF;
using namespace WBSF::Landsat;

extern CaseNo		MaxCase;
extern String		FileStem;
extern RRuleSet	*	CubistModel;
extern int			MaxAtt;
extern DataRec		*Case;
extern Definition	*AttDef;
extern char		*SpecialStatus;
extern void Error(int ErrNo, String S1, String S2);
extern String		*AttName; 
extern String		**AttValName;
extern DiscrValue	*MaxAttVal;


static const char* version = "2.0.0";


enum TFilePath { CubistImage_FILE_PATH, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
typedef vector<float> DTCodeVector;
typedef std::vector<float>  OutputData;



class CCubistImageOption : public CBaseOptions
{
public:
	CCubistImageOption()
	{
		m_nbPixel = 0;
		m_nbPixelDT = 0;
		m_scenesSize = 1;

		m_appDescription = "This software execute Cubist over an image";


		static const COptionDef OPTIONS[] =
		{
			//{ "-Debug",0,"",false,"Output debug information."},
			{ "CubistImageModel",0,"",false,"Cubist rules file path."},
			{ "srcfile",0,"",false, "Input image file path."},
			{ "dstfile",0,"",false, "Output image file path."}
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input Model", "DTModel","","","","Decision tree model file generate by CubistImage."},
			{ "Input Image", "srcfile","","nbBands","same number iof band as the input CubistImage model"},
			{ "Output Image", "dstfile","1","","CubistImage model result"},
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
			msg.ajoute("ERROR: Invalid argument line. 3 files are needed: Cubist model, the intput image and the destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		return msg;
	}

	virtual ERMsg ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		/*	if (IsEqual(argv[i], "-Trigger"))
			{
				string str = argv[++i];
				ReplaceString(str, "<", " < ");
				ReplaceString(str, ">", " > ");
				StringVector p(str, " ");
				if (p.size() == 3)
				{
					TIndices type = GetIndiceType(p[0]);
					string op = p[1];
					double threshold = atof(p[2].c_str());

					if (type != I_INVALID)
					{
						if (CIndices::IsValidOp(op))
							m_trigger.push_back(CIndices(type, op, threshold, 0));
						else
							msg.ajoute(op + " is an invalid operator for -Trigger option");
					}
					else
					{
						msg.ajoute(str + " is an invalid type for -Trigger option");
					}
				}
				else
				{
					msg.ajoute(str + " is an invalid type for -Trigger option");
				}

			}
			else if (IsEqual(argv[i], "-Despike"))
			{
				string str = argv[++i];
				TIndices type = GetIndiceType(str);
				double threshold = atof(argv[++i]);
				double min_trigger = atof(argv[++i]);

				if (type != I_INVALID)
				{
					m_despike.push_back(CIndices(type, "<", threshold, min_trigger));
				}
				else
				{
					msg.ajoute(str + " is an invalid type for -Despike option");
				}
			}
			else if (IsEqual(argv[i], "-nbDisturbances"))
			{
				m_nbDisturbances = atoi(argv[++i]);
			}
			else if (IsEqual(argv[i], "-FireSeverity"))
			{
				m_bFireSeverity = true;
			}
			else if (IsEqual(argv[i], "-ExportBands"))
			{
				m_bExportBands = true;
			}
			else if (IsEqual(argv[i], "-ExportTimeSeries"))
			{
				m_bExportTimeSeries = true;
			}
			else if (IsEqual(argv[i], "-Debug"))
			{
				m_bDebug = true;
			}
			else
			{*/
			//Look to see if it's a know base option
		msg = CBaseOptions::ProcessOption(i, argc, argv);
		//}

		return msg;
	}

	__int64 m_nbPixelDT;
	__int64 m_nbPixel;
};

//***********************************************************************
//									 
//	Main                                                             
//									 
//***********************************************************************

class CCubistImage
{
public:

	string GetDescription() { return  string("CubistImageImages version ") + version + " (" + _T(__DATE__) + ")\n"; }
	ERMsg Execute();


	ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);
	void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
	void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, RRuleSet * model, OutputData& output);
	void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& output);
	void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

	//void Evaluate(int x, int y, const vector<array<short, 3>>& DTCode, vector<vector<vector<short>>>& output);
	//void LoadData(const CBandsHolder& bandHolder1, const CBandsHolder& bandHolder2, vector< vector< CDADVector >>& data);
	//void LoadData(int x, int y, const CBandsHolder& bandHolder, CCubistImageTreeBlock& block);
	ERMsg LoadModel(string filePath, RRuleSet ** model);
	void GetDataRecGDAL(int xBlock, int yBlock, const CBandsHolder &bandHolder, Boolean Train, DataRec* pBl);

	CCubistImageOption m_options;

	//static int FindIndex(int start, const vector<short>& bandsvalue, int dir);
	//static void LoadModel(CCubistImageTree& DT, string filePath);
	//static short DTCode2ChangeCodeIndex(short DTCode);
	//static short DTCode2ChangeCode(short DTCode);

};

ERMsg CCubistImage::LoadModel(string filePath, RRuleSet ** model)
{
	ERMsg msg;

	CTimer timer(true);


	if (!m_options.m_bQuiet)
		_tprintf("Read model...\n");

	//  Read information on attribute names and values  
	string fileName = m_options.m_filesPath[0] + ".names";
	FILE* F = fopen(fileName.c_str(), "r");

	if (!F)
		Error(0, (String)fileName.c_str(), "");

	GetNames(F);

	fclose(F);

	// Read the model file that defines the ruleset and sets values
	// for various global variables such as USEINSTANCES  
	FileStem = (String)m_options.m_filesPath[0].c_str();
	*model = CubistModel = GetCommittee(".model");

	fileName = m_options.m_filesPath[0] + ".data";
	F = fopen(fileName.c_str(), "r");


	if (!F)
		Error(0, (String)fileName.c_str(), "");

	GetData(F, true, false);

	fclose(F);

	//  Prepare the file of instances and the kd-tree index  
	if (!m_options.m_bQuiet)
		_tprintf("Init model...\n");
	InitialiseInstances(CubistModel);


	//  Reorder instances to improve caching  
	try
	{
		if (!m_options.m_bQuiet)
			_tprintf("Copy instance...\n");

		CopyInstances();
		for (int i = 0; i <= MaxCase; i++)
		{
			Free(Case[i]);
		}
		Free(Case);
	}
	catch (...)
	{
		msg.ajoute("Exception throw when compiling model");
	}


	timer.Stop();

	if (!m_options.m_bQuiet)
		cout << "Read rules time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;


	return msg;
}


ERMsg CCubistImage::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
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

		CCubistImageOption option = m_options;
		option.m_nbBands = 1;

		string filePath = option.m_filesPath[OUTPUT_FILE_PATH];
		msg += outputDS.CreateImage(filePath, option);
	}


	return msg;
}



ERMsg CCubistImage::Execute()
{
	ERMsg msg;

	if (!m_options.m_bQuiet)
	{
		cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[CubistImage_FILE_PATH] << endl;

		if (!m_options.m_maskName.empty())
			cout << "Mask:   " << m_options.m_maskName << endl;
	}

	GDALAllRegister();

	//if (m_options.m_outputType != GDT_Unknown && m_options.m_outputType != GDT_Int16 && m_options.m_outputType != GDT_Int32)
		//msg.ajoute("Invalid -ot option. Only GDT_Int16 or GDT_Int32 are supported");

	if (!msg)
		return msg;


	RRuleSet	* model = NULL;
	msg += LoadModel(m_options.m_filesPath[CubistImage_FILE_PATH], &model);
	if (!msg)
		return msg;

	CGDALDatasetEx inputDS;
	CGDALDatasetEx maskDS;
	CGDALDatasetEx outputDS;


	msg = OpenAll(inputDS, maskDS, outputDS);

	if (msg && inputDS.GetRasterCount() != (MaxAtt - 1))
		msg.ajoute("The number of bands in the input image (" + to_string(inputDS.GetRasterCount()) + ") is not equal as the number of independants variables (" + to_string(MaxAtt - 1) + ") in the model.");

	if (msg)
	{
		CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, m_options.m_BLOCK_THREADS);

		if (maskDS.IsOpen())
			bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

		msg += bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);
		if (!msg)
			return msg;

		if (!m_options.m_bQuiet && m_options.m_bCreateImage)
			cout << "Create output images x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R ) with " << m_options.m_CPU << " threads..." << endl;


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
			ProcessBlock(xBlock, yBlock, bandHolder[thread], model, data);
			WriteBlock(xBlock, yBlock, outputDS, data);
		}//for all blocks


		//  Close decision tree and free allocated memory  
		FreeCttee(model);
		FreeNamesData();


		//close inputs and outputs
		CloseAll(inputDS, maskDS, outputDS);

	}

	return msg;
}

void CCubistImage::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
{
#pragma omp critical(BlockIO)
	{

		m_options.m_timerRead.Start();
		bandHolder.LoadBlock(xBlock, yBlock);
		m_options.m_timerRead.Stop();
	}
}

//Get input image reference
void CCubistImage::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, RRuleSet* model, OutputData& data)
{
	CGeoExtents extents = bandHolder.GetExtents();
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);

	if (bandHolder.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += (size_t)blockSize.m_x*blockSize.m_y;
		m_options.UpdateBar();

		return;
	}


	data.resize(blockSize.m_x*blockSize.m_y);

	//#pragma omp critical(ProcessBlock)
		//{
	m_options.m_timerProcess.Start();

	//allocate process memory

	DataRec* Block = NULL;
	Block = AllocZero(blockSize.m_x*blockSize.m_y, DataRec);
	float* Err = AllocZero(blockSize.m_x*blockSize.m_y, float);

	for (size_t xy = 0; xy < blockSize.m_x*blockSize.m_y; xy++)
		Block[xy] = AllocZero(MaxAtt + 3, AttValue);

	GetDataRecGDAL(xBlock, yBlock, bandHolder, false, Block);

	//process all x and y 
#pragma omp parallel for num_threads( m_options.BLOCK_CPU()) if (m_options.m_bMulti)  
	for (int xy = 0; xy < blockSize.m_y; xy++)
	{
		data[xy] = PredictValue(CubistModel, Block[xy], &Err[xy]);
	}//for x


#pragma omp atomic	
	m_options.m_xx += (size_t)blockSize.m_x*blockSize.m_y;

	m_options.UpdateBar();

	Free(Block);					Block = Nil;
	Free(Err);						Err = Nil;



	m_options.m_timerProcess.Stop();
}

void CCubistImage::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& data)
{
	if (m_options.m_bCreateImage)
	{
#pragma omp critical(BlockIO)
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
			if (!data.empty())
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

void CCubistImage::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
{
	if (!m_options.m_bQuiet)
		_tprintf("\nClose all files...\n");

	inputDS.Close();
	maskDS.Close();

	//close output
	m_options.m_timerWrite.Start();
	outputDS.Close(m_options);
	m_options.m_timerWrite.Stop();

	/*if (!m_options.m_bQuiet)
	{
		double percent = m_options.m_nbPixel > 0 ? (double)m_options.m_nbPixelDT / m_options.m_nbPixel * 100 : 0;

		_tprintf("\n");
		_tprintf("Percentage of pixel treated by DecisionTree: %0.3lf %%\n\n", percent);
	}*/

	m_options.PrintTime();
}

void CCubistImage::GetDataRecGDAL(int xBlock, int yBlock, const CBandsHolder &bandHolder, Boolean Train, DataRec* pBl)
{
	ASSERT(bandHolder.GetRasterCount() == MaxAtt - 1);

	CGeoExtents extents = bandHolder.GetExtents();
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);


	for (Attribute Att = 1; Att <= MaxAtt; Att++)
	{
		if (AttDef[Att])
		{
			for (int xy = 0; xy < blockSize.m_x*blockSize.m_y; xy++)
			{
				pBl[xy][Att] = EvaluateDef(AttDef[Att], pBl[xy]);
				//if (Continuous(Att))
				//{
				CheckValue(pBl[xy], Att);
				//}
			}

			continue;
		}

		if (Att == 1)
		{
			for (int xy = 0; xy < blockSize.m_x*blockSize.m_y; xy++)
			{
				//if (Continuous(Att))
				CVal(pBl[xy], Att) = UNKNOWN;
				//else
					//DVal(pBl[x], Att) = 0;
			}
			continue;
		}

		//  Get the attribute values
		CDataWindowPtr pWindow = bandHolder.GetWindow(Att - 1 - 1);
		for (size_t y = 0; y < blockSize.m_y; y++)
		{
			for (size_t x = 0; x < blockSize.m_x; x++)
			{
				double test = pWindow->at((int)x, (int)y);
				CVal(pBl[y*blockSize.m_x + x], Att) = pWindow->at( (int)x, (int)y);
				CheckValue(pBl[y*blockSize.m_x + x], Att);
			}
		}

		//for (size_t y = 0; y < blockSize.m_y; y++)
		//{
		//	for (size_t x = 0; x < blockSize.m_x; x++)
		//	{
		//		int xy = y * blockSize.m_x + x;
		//		if (Continuous(Att))
		//		{
		//			CVal(pBl[xy], Att) = pWindow->at((int)x, (int)y);
		//			CheckValue(pBl[xy], Att);
		//		}
		//		else
		//		{
		//			char	Name[1000] = { 0 };
		//			_gcvt_s(Name, 1000, pWindow->at((int)x, (int)y), 5);

		//			int Dv = Which(Name, AttValName[Att], 1, MaxAttVal[Att]);
		//			if (!Dv)
		//			{
		//				if (StatBit(Att, DISCRETE))
		//				{
		//					if (Train)
		//					{
		//						//  Add value to list  
		//						if (MaxAttVal[Att] >= (long)AttValName[Att][0])
		//						{
		//							Error(TOOMANYVALS, AttName[Att], (char *)AttValName[Att][0] - 1);
		//							Dv = MaxAttVal[Att];
		//						}
		//						else
		//						{
		//							Dv = ++MaxAttVal[Att];
		//							AttValName[Att][Dv] = _strdup(Name);
		//							AttValName[Att][Dv + 1] = "<other>"; // no free 
		//						}
		//					}
		//					else
		//					{
		//						//  Set value to "<other>"  
		//						Dv = MaxAttVal[Att] + 1;
		//					}
		//				}
		//				else
		//				{
		//					Error(BADATTVAL, AttName[Att], Name);
		//				}
		//			}
		//			DVal(pBl[x], Att) = Dv;
		//		}
		//	}
		//}
	}


	//  Preserve original case number  
	for (int xy = 0; xy < blockSize.m_x*blockSize.m_y; xy++)
		DVal(pBl[xy], 0) = MaxCase + 1;
}


int _tmain(int argc, _TCHAR* argv[])
{
	CTimer timer(true);

	CCubistImage regressionTree;
	ERMsg msg = regressionTree.m_options.ParseOption(argc, argv);

	if (!msg || !regressionTree.m_options.m_bQuiet)
		cout << regressionTree.GetDescription() << endl;


	if (msg)
		msg = regressionTree.Execute();

	if (!msg)
	{
		PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if (!regressionTree.m_options.m_bQuiet)
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}


