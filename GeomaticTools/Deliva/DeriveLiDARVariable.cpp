//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
// version
// 2021-11-24	1.0.0	Rémi Saint-Amant	Creation from old windows version

#include "stdafx.h" 
#include <float.h>
#include <math.h>
#include <algorithm>
#include <array>
#include <boost\multi_array.hpp>
#include <iostream>


#include "DeriveLiDARVariable.h"
#include "Basic/OpenMP.h"
#include "Geomatic/GDALBasic.h"
#include "Basic/UtilMath.h"
//#include "CSVFile.h"



#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "ogr_spatialref.h"



//-subset MIN+MEAN+MAX -multi -tr 30 30 -co "compress=LZW" -ot Int16 --config GDAL_CACHEMAX 1024  -overwrite test.tif test30m_new.tif




using namespace std;
namespace WBSF
{



	const char* CDeriveLiDARVariable::VERSION = "1.0.0";
	const int CDeriveLiDARVariable::NB_THREAD_PROCESS = 2;



	//template <class T>
	//std::string TestToString(T val, int pres = -1)
	//{
	//	std::string str;
	//	bool bReal = std::is_same<T, float>::value || std::is_same<T, double>::value;
	//	if (bReal)
	//	{
	//		std::ostringstream st;
	//		st.imbue(std::locale("C"));
	//		if (pres < 0)
	//		{
	//			st << val;
	//		}
	//		else
	//		{
	//			st << std::fixed << std::setprecision(pres) << val;
	//		}

	//		str = st.str();
	//		size_t pos = str.find('.');
	//		if (pos != std::string::npos)//if it's a real;
	//		{
	//			int i = (int)str.length() - 1;
	//			while (i >= 0 && str[i] == '0')
	//				i--;

	//			if (i >= 0 && str[i] == '.') i--;
	//			str = str.substr(0, i + 1);
	//			if (str.empty())
	//				str = "0";
	//		}
	//	}
	//	else
	//	{
	//		std::ostringstream st;
	//		st.imbue(std::locale("C"));
	//		if (pres < 0)
	//		{
	//			st << val;
	//		}
	//		else
	//		{
	//			st << std::setfill(' ') << std::setw(pres) << val;
	//		}

	//		str = st.str();
	//	}



	//	return str;
	//}

	CDeriveLiDARVariableOption::CDeriveLiDARVariableOption() :CBaseOptions(true)
	{
		m_precision = 4;
		m_stats.set();//select sll stats
		m_appDescription = "This software extract LIDAR statistic from LIDAR image";

		AddOption("-tr");


		static const COptionDef OPTIONS[] =
		{
			{ "-subset", 1, "type", true, "Subset only some statistics separate by +. All statistic by default. stat can be: MEAN,MIN,MAX,RGE,CAND,QDMH,STDB,MBSTD,SKEW,KURT,HPR05,HPR10,HPR25,HPR50,HPR75,HPR90,HPR95,STRA0,STRA1,STRA2,STRA3,STRA4,STRA5,STRA6,STRA7" },
			//	{ "-X", 1, "str", false, "File header title for X coordinates. \"X\" by default." },
			//{ "-Y", 1, "str", false, "File header title for Y coordinates. \"Y\" by default." },
			//{ "-prec", 1, "precision", false, "Output precision. 4 by default." },
			//{ "Image", 0, "", false, "LIDAR Image." },
			{ "srcfile", 0, "", false, "Input image file path" },
			{ "dstfile", 0, "", false, "Output image file path" }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);




		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input", "Image", "", "*", "", "" },
			{ "Input", "srcfile", "", "", "", "" },
			{ "Output", "dstfile", "", "", "", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);


		m_XHeader = "X";
		m_YHeader = "Y";
	}


	ERMsg CDeriveLiDARVariableOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		if (msg && m_filesPath.size() != 2)
		{
			msg.ajoute("Invalid argument line. 2 files are needed: LIDAR image, and output image.\n");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);

		}


		return msg;
	}

	ERMsg CDeriveLiDARVariableOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;
		if (IsEqual(argv[i], "-subset"))
		{
			msg = m_stats.FromString(argv[++i]);
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}


	//*************************************************************************************************************************************************************


	ERMsg CDeriveLiDARVariable::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		ERMsg msg;


		if (!m_optionsIn.m_bQuiet)
			cout << endl << "Open input image..." << endl;


		msg = inputDS.OpenInputImage(m_optionsIn.m_filesPath[INPUT_FILE_PATH], m_optionsIn);
		if (msg)
		{

			inputDS.UpdateOption(m_optionsIn);
			inputDS.UpdateOption(m_optionsOut);

			if (!m_optionsIn.m_bQuiet)
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


		if (msg && !m_optionsIn.m_maskName.empty())
		{
			if (!m_optionsIn.m_bQuiet)
				cout << "Open mask image..." << endl;
			msg += maskDS.OpenInputImage(m_optionsIn.m_maskName);
		}

		if (msg && m_optionsIn.m_bCreateImage)
		{

			if (!m_optionsIn.m_bQuiet)
				cout << "Create output image..." << endl;



			m_optionsOut.m_nbBands = m_optionsIn.m_stats.count();

			string filePath = m_optionsOut.m_filesPath[OUTPUT_FILE_PATH];
			msg += outputDS.CreateImage(filePath, m_optionsOut);
		}


		return msg;
	}




	ERMsg CDeriveLiDARVariable::Execute()
	{
		ERMsg msg;

		m_optionsOut = m_optionsIn;
		m_optionsIn.m_bRes = false;
		m_optionsIn.m_xRes = 0;
		m_optionsIn.m_yRes = 0;

		if (!m_optionsIn.m_bQuiet)
		{
			cout << "Output: " << m_optionsIn.m_filesPath[OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_optionsIn.m_filesPath[INPUT_FILE_PATH] << endl;

			if (!m_optionsIn.m_maskName.empty())
				cout << "Mask:   " << m_optionsIn.m_maskName << endl;
		}

		GDALAllRegister();


		if (!msg)
			return msg;



		CGDALDatasetEx inputDS;
		CGDALDatasetEx maskDS;
		CGDALDatasetEx outputDS;


		msg = OpenAll(inputDS, maskDS, outputDS);
		if (msg)
		{

			int Rx = int(round(outputDS.GetExtents().XRes() / inputDS.GetExtents().XRes()));
			int Ry = int(round(outputDS.GetExtents().YRes() / inputDS.GetExtents().YRes()));

			CBandsHolderMT bandHolder(2 * max(Rx, Ry), m_optionsIn.m_memoryLimit, m_optionsIn.m_IOCPU, m_optionsIn.m_BLOCK_THREADS);

			if (maskDS.IsOpen())
				bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_optionsIn.m_maskDataUsed);

			//load with original resolution


			msg += bandHolder.Load(inputDS, m_optionsIn.m_bQuiet, m_optionsIn.m_extents, m_optionsIn.m_period);
			if (!msg)
				return msg;

			if (!m_optionsIn.m_bQuiet && m_optionsIn.m_bCreateImage)
				cout << "Create output images x(" << m_optionsOut.m_extents.m_xSize << " C x " << m_optionsOut.m_extents.m_ySize << " R ) with " << m_optionsOut.m_CPU << " threads..." << endl;


			CGeoExtents extents = bandHolder.GetExtents();
			m_optionsIn.ResetBar((size_t)extents.m_xSize * extents.m_ySize);



			vector<pair<int, int>> XYindex = extents.GetBlockList();

			omp_set_nested(1);
#pragma omp parallel for schedule(static, 1) num_threads(m_optionsIn.m_BLOCK_THREADS) if (m_optionsIn.m_bMulti)
			for (int b = 0; b < (int)XYindex.size(); b++)
			{
				int thread = omp_get_thread_num();
				int xBlock = XYindex[b].first;
				int yBlock = XYindex[b].second;

				//inputDS.GetExtents().GetBlockExtents();
				//extents.xy
				//deque < CGeoExtents> input_extents;
				//data

				OutputData data;
				ReadBlock(xBlock, yBlock, bandHolder[thread]);
				ProcessBlock(xBlock, yBlock, bandHolder[thread], data);
				WriteBlock(xBlock, yBlock, outputDS, data);
			}//for all blocks

			//close inputs and outputs
			CloseAll(inputDS, maskDS, outputDS);

		}

		return msg;
	}

	void CDeriveLiDARVariable::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
		{

			m_optionsIn.m_timerRead.Start();
			bandHolder.LoadBlock(xBlock, yBlock);
			m_optionsIn.m_timerRead.Stop();
		}
	}

	//Get input image reference
	void CDeriveLiDARVariable::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, OutputData& data)
	{

		CGeoExtents input_extents = m_optionsIn.GetExtents();
		CGeoExtents r = input_extents.GetBlockExtents(xBlock, yBlock);
		CGeoRectIndex ri = r.GetPosRect();
		CGeoRectIndex outputRect = m_optionsOut.GetExtents().CoordToXYPos(r);
		
		double Rx = abs(m_optionsOut.m_xRes / input_extents.XRes());
		double Ry = abs(m_optionsOut.m_yRes / input_extents.YRes());


		if (bandHolder.IsEmpty())
		{
#pragma omp atomic		
			m_optionsIn.m_xx += (size_t)ri.m_xSize * ri.m_ySize;
			m_optionsIn.UpdateBar();

			return;
		}



		m_optionsIn.m_timerProcess.Start();

		//allocate process memory
		data.resize(m_optionsIn.m_stats.count());
		for (size_t i = 0; i < data.size(); i++)
			data[i].resize(outputRect.m_xSize * outputRect.m_ySize, (float)m_optionsOut.m_dstNodata);




		//process all x and y 
#pragma omp parallel for num_threads( m_optionsIn.BLOCK_CPU()) if (m_optionsIn.m_bMulti)  
		for (__int64 y = 0; y < outputRect.m_ySize; y++)
		{
			for (size_t x = 0; x < outputRect.m_xSize; x++)
			{
				CGeoPoint coord = m_optionsOut.GetExtents().XYPosToCoord(CGeoPointIndex(int(outputRect.m_x + x), int(outputRect.m_y + y)));
				coord.m_x -= Rx / 2;
				coord.m_y += Ry / 2;
				CGeoPointIndex ij = r.CoordToXYPos(coord);
				CDataWindowPtr pWindow = bandHolder.GetWindow(0, ij.m_x, ij.m_y, int(ceil(Rx)), int(ceil(Ry)));

				CLiDARStat stats;
				for (size_t yy = 0; yy < Ry; yy++)
				{
					for (size_t xx = 0; xx < Rx; xx++)
					{
						if (pWindow->IsValid(int(xx), int(yy)))
						{
							double value = pWindow->at(int(xx), int(yy));
							ASSERT(value >= 0);
							stats += value;
						}
					}
				}

				if (stats.IsInit())
				{
					size_t xy = y * outputRect.m_xSize + x;
					//size_t xy = (outputRect.m_y + y) * m_optionsOut.m_extents.m_xSize + outputRect.m_x + x;



					for (size_t i = 0, ii = 0; i < m_optionsIn.m_stats.size() && ii < m_optionsIn.m_stats.count(); i++)
					{
						if (m_optionsIn.m_stats[i])
						{
							double value = stats[i];
							ASSERT(value >= 0);
							data[ii][xy] = float(value);


							ii++;
						}
					}
				}
			}
		}



#pragma omp atomic	
		m_optionsIn.m_xx += (size_t)ri.m_xSize * ri.m_ySize;

		m_optionsIn.UpdateBar();

		m_optionsIn.m_timerProcess.Stop();
	}

	void CDeriveLiDARVariable::WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& data)
	{
		if (m_optionsIn.m_bCreateImage)
		{
#pragma omp critical(BlockIO)
			{

				m_optionsIn.m_timerWrite.Start();

				CGeoExtents input_extents = m_optionsIn.GetExtents();
				CGeoExtents r = input_extents.GetBlockExtents(xBlock, yBlock);
				CGeoRectIndex ri = r.GetPosRect();
				CGeoRectIndex outputRect = m_optionsOut.GetExtents().CoordToXYPos(r);


				ASSERT(data.size() == outputDS.GetRasterCount());
				for (size_t b = 0; b < outputDS.GetRasterCount(); b++)
				{
					ASSERT(data[b].size() == outputRect.m_xSize * outputRect.m_ySize);


					float noDataOut = (float)outputDS.GetNoData(b);
					GDALRasterBand* pBand = outputDS.GetRasterBand(b);

					if (!data[b].empty())
					{
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(data[b][0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
					}//for all output bands
					else
					{
						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &noDataOut, 1, 1, GDT_Float32, 0, 0);
					}
				}

				m_optionsIn.m_timerWrite.Stop();
			}
		}//if create image

	}

	void CDeriveLiDARVariable::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS)
	{
		if (!m_optionsIn.m_bQuiet)
			_tprintf(L"\nClose all files...\n");

		inputDS.Close();
		maskDS.Close();

		//close output
		m_optionsIn.m_timerWrite.Start();
		outputDS.Close(m_optionsOut);
		m_optionsIn.m_timerWrite.Stop();
		m_optionsIn.PrintTime();
	}

}