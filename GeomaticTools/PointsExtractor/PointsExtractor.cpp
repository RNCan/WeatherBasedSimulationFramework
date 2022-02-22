//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
// version
// 2.1.0	20/12/2021	Rémi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
// 2.0.1	22/05/2018	Rémi Saint-Amant	Compile with VS 2017
// 2.0.0    03/11/2017  Rémi Saint-Amant	Compile with GDAL 2.02
// 1.4.1	22/11/2016	Rémi Saint-Amant	Add RegisterAll
// 1.4.0	09/04/2015	Rémi Saint-Amant	Compile with GDAL 1.11.3 and WBSF
// 1.3.0	08/03/2015	Rémi Saint-Amant	Correction of VRT without -separate options
// 1.2.4	30/01/2015	Rémi Saint-Amant	don't modify input VRT file. Bug correction in IntersectRect
// 1.2.3    27/01/2015	Rémi Saint-Amant	Compile with GDAL 1.11.1
// 1.2.2	24/07/2014	Rémi Saint-Amant	Bugs corrections
// 1.2.1	19/06/2014	Rémi Saint-Amant	some bugs correction
// 1.2.0	19/06/2014	Rémi Saint-Amant	Compile with GDAL 1.11 and VS 2013, UNICODE 
// 1.1.0    24/02/2014	Rémi Saint-Amant	Bug correction in nb digits. Compile with GDAL 1.10
// 1.0.5	25/07/2013	Rémi Saint-Amant	Bug correction in coordinate.
// 1.0.4	07/07/2013	Rémi Saint-Amant	Add options. Add re-projection on the fly
// 1.0.3    29/06/2013	Rémi Saint-Amant	Don't remove external points
// 1.0.2    06/06/2013	Rémi Saint-Amant	Find name of file
// 1.0.1	05/06/2013	Rémi Saint-Amant	All in memory, column with file title
// 1.0.0	05/06/2013	Rémi Saint-Amant	Creation from windows version

#include "stdafx.h" 
#include <float.h>
#include <math.h>
#include <algorithm>
#include <array>
#include <boost\multi_array.hpp>
#include <iostream>


#include "PointsExtractor.h"
#include "Basic/OpenMP.h"
#include "Geomatic/GDALBasic.h"
#include "Basic/UtilMath.h"
//#include "CSVFile.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "ogr_spatialref.h"


//TEST Image
//=ALEA.ENTRE.BORNES(-2341000,3010750), =ALEA.ENTRE.BORNES(5860250,8500000)

//-wm 15000
//-blocksize 1024 1024 -iocpu 1 -multi -dstNoData -32768 --config GDAL_CACHEMAX 1024 -overwrite "U:\GIS\#projets\LAM\ANALYSE\VALIDATION\HARVESTING_inventory_all\v3\25m\all_yr.tif" "U:\GIS\#documents\TestCodes\PointsExtractor\Test2\Input\Test2.csv" "U:\GIS\#documents\TestCodes\PointsExtractor\Test2\Output\Test2.csv"
//-dstnodata -999 -multi -overwrite -blocksize 5000 5000 "K:/#projets/LAM/ANALYSE/VALIDATION/flooding/TEST.VRT" "U:\GIS\#documents\TestCodes\PointsExtractor\Test1\Input\Test1.csv" "U:\GIS\#documents\TestCodes\PointsExtractor\Test1\Output\Test2.csv"
//-multi -ot INT16 -dstNoData -32768 --config GDAL_CACHEMAX 8096 -overwrite -stats -overview {2,4,8,16} "U:\GIS1\LANDSAT_SR\LCC\2012\#57_12.vrt" "U:\GIS\#documents\TestCodes\PointsExtractor\Test3\Input\Test3.csv" "U:\GIS\#documents\TestCodes\PointsExtractor\Test3\Output\Test3(new).csv"
//-dstnodata -9999 -overwrite "U:\GIS\#projets\LAQ\ANALYSE_CA\20161125_SR_run14\See5_Dem_T101234_mean_val_1299_v8_dspkOff_nbDstrb6\Final_fix_20161216_bw1_s12\Ch_19842015__Dis1_YRt2.tif.tif" "U:\GIS\#documents\TestCodes\PointsExtractor\Test5\PP2005_lcc_v2_test_remi.csv" "U:\GIS\#documents\TestCodes\PointsExtractor\Test5\PP2005_lcc_v2_test_remi_out.csv"



using namespace std;
namespace WBSF
{

	const char* CPointsExtractor::VERSION = "2.1.0";
	const int CPointsExtractor::NB_THREAD_PROCESS = 2;
	const char * CPointsExtractor::CONDITION_NAME[NB_CONDITION] = { "AllValid", "AtLeastOneValid", "AtLeastOneMissing", "AllMissing" };



	template <class T>
	std::string TestToString(T val, int pres = -1)
	{
		std::string str;
		bool bReal = std::is_same<T, float>::value || std::is_same<T, double>::value;
		if (bReal)
		{
			std::ostringstream st;
			st.imbue(std::locale("C"));
			if (pres < 0)
			{
				st << val;
			}
			else
			{
				st << std::fixed << std::setprecision(pres) << val;
			}

			str = st.str();
			size_t pos = str.find('.');
			if (pos != std::string::npos)//if it's a real;
			{
				int i = (int)str.length() - 1;
				while (i >= 0 && str[i] == '0')
					i--;

				if (i >= 0 && str[i] == '.') i--;
				str = str.substr(0, i + 1);
				if (str.empty())
					str = "0";
			}
		}
		else
		{
			std::ostringstream st;
			st.imbue(std::locale("C"));
			if (pres < 0)
			{
				st << val;
			}
			else
			{
				st << std::setfill(' ') << std::setw(pres) << val;
			}

			str = st.str();
		}



		return str;
	}

	CPointsExtractorOption::CPointsExtractorOption() :CBaseOptions(false)
	{
		m_precision = 4;
		m_appDescription = "This software extract bands information from input image and coordinates's file";

		static const char* DEFAULT_OPTIONS[] = { "-srcnodata", "-dstnodata", "-q", "-overwrite", "-te", "-mask", "-maskValue", "-multi", "-CPU", "-IOCPU", "-BlockSize", "-?", "-??", "-???", "-help" };
		for (int i = 0; i < sizeof(DEFAULT_OPTIONS) / sizeof(char*); i++)
			AddOption(DEFAULT_OPTIONS[i]);


		static const COptionDef OPTIONS[] =
		{
			{ "-Condition", 1, "type", true, "Add conditions to the extraction. 4 possibility of condition can be define: \"AllValid\", \"AtLeastOneValid\", \"AtLeastOneMissing\", \"AllMissing\". No conditions are define by default (all will be output)." },
			{ "-X", 1, "str", false, "File header title for X coordinates. \"X\" by default." },
			{ "-Y", 1, "str", false, "File header title for Y coordinates. \"Y\" by default." },
			{ "-prec", 1, "precision", false, "Output precision. 4 by default." },
			{ "Image", 0, "", false, "Image to extract information." },
			{ "srcfile", 0, "", false, "Input coordinate file path (CSV)" },
			{ "dstfile", 0, "", false, "Output information file path (CSV)." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input", "Image", "", "*", "", "" },
			{ "Input", "srcfile", "", "3 or more", "ID|X|Y|other information...", "The columns order is not important. The coordinates must have a column header \"X\" and \"Y\". A line's ID is recommended because line order is not kept in extraction" },
			{ "Output", "dstfile", "", "", "ID|X|Y|others information...|all bands values...", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);


		m_XHeader = "X";
		m_YHeader = "Y";
	}


	ERMsg CPointsExtractorOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);

		if (msg && m_filesPath.size() != 3)
		{
			msg.ajoute("Invalid argument line. 3 files are needed: image file, the coordinates (CSV) and destination (CSV).\n");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);

		}


		return msg;
	}

	ERMsg CPointsExtractorOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;
		if (IsEqual(argv[i], "-Condition"))
		{
			string conditionName = TrimConst(argv[++i]);

			int condition = -1;
			for (int j = 0; j < NB_CONDITION; j++)
			{
				if (IsEqualNoCase(conditionName, CONDITION_NAME[j]))
				{
					condition = j;
					break;
				}
			}

			if (condition >= 0)
				m_condition.push_back(condition);
			else
				msg.ajoute(conditionName + " is an invalid Condition. See documentation.");
		}
		else if (IsEqual(argv[i], "-X"))
		{
			m_XHeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-Y"))
		{
			m_YHeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-prec"))
		{
			m_precision = ToInt(argv[++i]);
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}

	bool CPointsExtractorOption::IsRemoved(const string& str)const
	{
		ASSERT(!m_condition.empty());

		bool bRemoved = false;

		string strNodata = ToString(m_dstNodata, m_precision);
		StringVector input(str, ",");

		for (size_t i = 0; i < m_condition.size() && !bRemoved; i++)
		{
			switch (m_condition[i])
			{
			case ALL_VALID:
				for (size_t z = 0; z < input.size() && !bRemoved; z++)
					bRemoved = input[z] == strNodata;
				break;
			case AT_LEAST_ONE_VALID:
				bRemoved = true;
				for (size_t z = 0; z < input.size() && bRemoved; z++)
					bRemoved = input[z] == strNodata;
				break;
			case AT_LEAST_ONE_MISSING:
				bRemoved = true;
				for (size_t z = 0; z < input.size() && bRemoved; z++)
					bRemoved = input[z] != strNodata;
				break;
			case ALL_MISSING:
				for (size_t z = 0; z < input.size() && !bRemoved; z++)
					bRemoved = input[z] != strNodata;
				break;
			default: ASSERT(false);
			}
		}



		return bRemoved;
	}


	//*************************************************************************************************************************************************************

	//#include <boost/format.hpp>

	ERMsg CPointsExtractor::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << endl;
			cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;
			cout << "Using:  " << m_options.m_filesPath[IMAGE_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		GDALAllRegister();

		CGDALDatasetEx inputDS;
		CGDALDatasetEx maskDS;
		CGeoCoordFile ioFile;


		msg = OpenAll(inputDS, maskDS, ioFile);

		if (msg)
		{

			CBandsHolderMT bandHolder(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

			if (maskDS.IsOpen())
				bandHolder.SetMask(maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed);

			msg = bandHolder.Load(inputDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);

			if (!msg)
				return msg;


			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
				printf("Extract %I64u points with %d threads...\n", ioFile.size(), m_options.m_bMulti ? m_options.m_CPU : 1);


			CGeoExtents extents = m_options.GetExtents();
			m_options.m_xxFinal = extents.YNbBlocks()*extents.XNbBlocks()*ioFile.size();

			//**************************************

			omp_set_nested(1);//for IOCPU
			boost::dynamic_bitset<size_t> treated(ioFile.size());

			for (int yBlock = 0; yBlock < extents.YNbBlocks(); yBlock++)
			{
#pragma omp parallel for schedule(static, 1) num_threads( NB_THREAD_PROCESS ) if (m_options.m_bMulti)	
				for (int xBlock = 0; xBlock < extents.XNbBlocks(); xBlock++)
				{
					int blockThreadNo = ::omp_get_thread_num();

					//CGeoExtents extents = bandHolder[blockThreadNo].GetExtents();
					CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
					if (AtLeastOnePointIn(blockExtents, ioFile))
					{
						ReadBlock(xBlock, yBlock, bandHolder[blockThreadNo]);
						ProcessBlock(xBlock, yBlock, bandHolder[blockThreadNo], ioFile, treated);
					}
					else
					{
#pragma omp atomic
						m_options.m_xx += (int)ioFile.m_xy.size();

						m_options.UpdateBar();
					}

				}//for xBloxk
			}//for yblock



			//add noData to untreated line
			//std::ostringstream st;
			//st << std::fixed << std::setprecision(m_options.m_precision) << m_options.m_dstNodata;
			string strNodata = TestToString(m_options.m_dstNodata, m_options.m_precision);
			for (size_t i = 0; i < ioFile.size(); i++)
			{
				if (!treated[i])
				{
					for (size_t z = 0; z < bandHolder[0].GetRasterCount(); z++)
						ioFile[i] += ',' + strNodata;
				}
			}


			//apply condition
			if (!m_options.m_condition.empty())
			{
				for (CGeoCoordFile::iterator it = ioFile.begin(); it != ioFile.end();)
				{
					if (m_options.IsRemoved(*it))
					{
						//remove xy coordinate
						size_t pos = std::distance(ioFile.begin(), it);
						ioFile.m_xy.erase(ioFile.m_xy.begin() + pos);

						it = ioFile.erase(it);
					}
					else
					{
						it++;
					}
						
				}
			}

			m_options.m_timerWrite.Start();
			msg = ioFile.Save(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			m_options.m_timerWrite.Stop();

			CloseAll(inputDS, maskDS);
		}


		return msg;

	}

	bool CPointsExtractor::AtLeastOnePointIn(const CGeoExtents& blockExtents, const CGeoCoordFile& ioFile)
	{
		bool bAtLeastOne = false;
		for (int i = 0; i < ioFile.m_xy.size() && !bAtLeastOne; i++)
			bAtLeastOne = blockExtents.IsInside(ioFile.m_xy[i]);

		return bAtLeastOne;
	}

	ERMsg CPointsExtractor::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGeoCoordFile& ioFile)
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
			cout << endl << "Open input image..." << endl;

		msg = inputDS.OpenInputImage(m_options.m_filesPath[IMAGE_FILE_PATH], m_options);
		if (msg)
			inputDS.UpdateOption(m_options);


		if (msg && !m_options.m_bQuiet)
		{
			CGeoExtents extents = inputDS.GetExtents();
			CProjectionPtr pPrj = inputDS.GetPrj();
			string prjName = pPrj.get() ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << inputDS.GetRasterXSize() << " cols x " << inputDS.GetRasterYSize() << " rows x " << inputDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    NbBands        = " << inputDS.GetRasterCount() << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NoData         = " << inputDS.GetNoData(0) << endl;

		}


		if (msg && !m_options.m_maskName.empty())
		{
			if (!m_options.m_bQuiet)
				cout << endl << "Open mask..." << endl;

			msg += maskDS.OpenInputImage(m_options.m_maskName);
		}

		if (msg)
		{
			if (!m_options.m_bQuiet)
				cout << endl << "Load coordinates..." << endl;

			CTimer timeLoadCSV(true);
			msg = ioFile.Load(m_options.m_filesPath[INPUT_FILE_PATH], m_options.m_XHeader, m_options.m_YHeader);
			timeLoadCSV.Stop();

			if (msg)
			{
				if (!m_options.m_bQuiet)
				{
					CProjectionPtr pPrj = CProjectionManager::GetPrj(ioFile.m_xy.GetPrjID());
					string prjName = pPrj ? pPrj->GetName() : "Unknown";

					cout << "    Size           = " << to_string(ioFile.m_xy.size()) << " points" << endl;
					cout << "    Projection     = " << prjName << endl;
					cout << "    Time to load   = " << SecondToDHMS(timeLoadCSV.Elapsed()) << endl;
					cout << endl;
				}

				msg = ioFile.ManageProjection(inputDS.GetPrjID());
			}
		}


		if (msg)
		{

			if (!m_options.m_bOverwrite && FileExists(m_options.m_filesPath[OUTPUT_FILE_PATH]))
			{
				msg.ajoute("ERROR: Output file already exist. Delete the file or use option -overwrite.");
				return msg;
			}

			ofStream fileOut;
			msg = fileOut.open(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			if (msg)
			{
				fileOut.close();

				for (size_t i = 0; i < inputDS.GetRasterCount(); i++)
				{
					string title = GetFileTitle(inputDS.GetFilePath()) + "_" + std::to_string(i + 1);

					if (!inputDS.GetInternalName((int)i).empty())
						title = GetFileTitle(inputDS.GetInternalName((int)i));

					ioFile.m_header += "," + title;
				}
			}
		}

		return msg;
	}

	void CPointsExtractor::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder)
	{
#pragma omp critical(BlockIO)
	{
		m_options.m_timerRead.Start();
		bandHolder.LoadBlock(xBlock, yBlock);
		m_options.m_timerRead.Stop();
	}
	}


	void CPointsExtractor::ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGeoCoordFile& ioFile, boost::dynamic_bitset<size_t>& treated)
	{
		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		//int nbCells = (size_t)extents.m_xSize*extents.m_ySize;
		CGeoExtents blockExtents = extents.GetBlockExtents(xBlock, yBlock);
		CGeoRectIndex blockRect = blockExtents.GetPosRect();
		string strNodata = ToString(m_options.m_dstNodata, m_options.m_precision);

		if (bandHolder.IsEmpty())
		{
#pragma omp atomic		
			m_options.m_xx += (int)ioFile.m_xy.size();

			m_options.UpdateBar();

			return;
		}


#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();

		vector<CDataWindowPtr> input;
		bandHolder.GetWindow(input);


		//process all point
		for (int i = 0; i < ioFile.m_xy.size(); i++)
		{
			//for all point in the table
			CGeoPoint coordinate = ioFile.m_xy[i];
			CGeoPointIndex xy = blockExtents.CoordToXYPos(coordinate);

			//find position in the block
			if (blockRect.IsInside(xy))
			{
				ASSERT(!treated[i]);
				//CGeoSize block0 = extents.GetBlockSize(0, 0);
				treated.set(i);
				for (size_t z = 0; z < input.size(); z++)
				{
					DataType v = input[z]->at(xy);
					if (input[z]->IsValid(v))
						ioFile[i] += ',' + ToString(v, m_options.m_precision);
					else
						ioFile[i] += ',' + strNodata;
				}
			}

#pragma omp atomic 
			m_options.m_xx++;

			m_options.UpdateBar();
		}
	}//for batch 
	}


	void CPointsExtractor::CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS)
	{
		inputDS.Close();
		maskDS.Close();

		m_options.PrintTime();
	}
}