//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
// version
// 1.0.1	22/05/2018	Rémi Saint-Amant	Compile with VS 2017
// 1.0.0	21/12/2017	Rémi Saint-Amant	Creation 

#include "stdafx.h" 
#include <float.h>
#include <math.h>
#include <algorithm>
#include <array>
#include <boost\multi_array.hpp>
#include <iostream>


#include "PointsReduction.h"
#include "Basic/OpenMP.h"
#include "Basic/ApproximateNearestNeighbor.h"
#include "Basic/UtilMath.h"
#include "Geomatic/GDALBasic.h"

//#include "CSVFile.h"

//#pragma warning(disable: 4275 4251)
//#include "gdal_priv.h"
//#include "ogr_spatialref.h"


//TEST Image
//=ALEA.ENTRE.BORNES(-2341000,3010750), =ALEA.ENTRE.BORNES(5860250,8500000)

//-wm 15000
//-blocksize 1024 1024 -iocpu 1 -multi -dstNoData -32768 --config GDAL_CACHEMAX 1024 -overwrite "U:\GIS\#projets\LAM\ANALYSE\VALIDATION\HARVESTING_inventory_all\v3\25m\all_yr.tif" "U:\GIS\#documents\TestCodes\PointsReduction\Test2\Input\Test2.csv" "U:\GIS\#documents\TestCodes\PointsReduction\Test2\Output\Test2.csv"
//-dstnodata -999 -multi -overwrite -blocksize 5000 5000 "K:/#projets/LAM/ANALYSE/VALIDATION/flooding/TEST.VRT" "U:\GIS\#documents\TestCodes\PointsReduction\Test1\Input\Test1.csv" "U:\GIS\#documents\TestCodes\PointsReduction\Test1\Output\Test2.csv"
//-multi -ot INT16 -dstNoData -32768 --config GDAL_CACHEMAX 8096 -overwrite -stats -overview {2,4,8,16} "U:\GIS1\LANDSAT_SR\LCC\2012\#57_12.vrt" "U:\GIS\#documents\TestCodes\PointsReduction\Test3\Input\Test3.csv" "U:\GIS\#documents\TestCodes\PointsReduction\Test3\Output\Test3(new).csv"

using namespace std;
namespace WBSF
{

	const char* CPointsReduction::VERSION = "1.0.1";
	const int CPointsReduction::NB_THREAD_PROCESS = 2;


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

	CPointsReductionOption::CPointsReductionOption() :
		CBaseOptions(false)
	{

		m_appDescription = "This software reduce the number of points of a locations file base on distance";

		static const char* DEFAULT_OPTIONS[] = { "-srcnodata", "-dstnodata", "-q", "-overwrite", "-te", "-mask", "-maskValue", "-multi", "-CPU", "-IOCPU", "-wm", "-BlockSize", "-?", "-??", "-???", "-help" };
		for (int i = 0; i < sizeof(DEFAULT_OPTIONS) / sizeof(char*); i++)
			AddOption(DEFAULT_OPTIONS[i]);


		m_distanceMin = 5000;
		static const COptionDef OPTIONS[] =
		{
			{ "-d", 1, "distance [km]", false, "Minimum distance between points. 5 km by default." },
			{ "-X", 1, "str", false, "File header title for X coordinates. \"X\" by default." },
			{ "-Y", 1, "str", false, "File header title for Y coordinates. \"Y\" by default." },
			//			{ "-prec", 1, "precision", false, "Output precision. 4 by default." },
			{ "srcfile", 0, "", false, "Input coordinate file path (CSV)" },
			{ "dstfile", 0, "", false, "Output information file path (CSV)." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input", "srcfile", "", "3 or more", "ID|X|Y|other information...", "The columns order is not important. The coordinates must have an column header \"X\" and \"Y\". A line's ID is recommended because line order is not kept in extraction" },
			{ "Output", "dstfile", "", "", "ID|X|Y|others information...|all bands values...", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);


		m_XHeader = "X";
		m_YHeader = "Y";
	}


	ERMsg CPointsReductionOption::ParseOption(int argc, char* argv[])
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

	ERMsg CPointsReductionOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;

		if (IsEqual(argv[i], "-X"))
		{
			m_XHeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-Y"))
		{
			m_YHeader = argv[++i];
		}
		else if (IsEqual(argv[i], "-d"))
		{
			m_distanceMin = ToDouble(argv[++i]) * 1000;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}



	//*************************************************************************************************************************************************************

	//#include <boost/format.hpp>

	ERMsg CPointsReduction::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << endl;
			cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		//GDALAllRegister();

		CLocationVector locations;
		

		msg = OpenAll(locations);

		if (msg)
		{
			if (!m_options.m_bQuiet && m_options.m_bCreateImage)
				printf("Reduce %I64u points with %d threads using distance min of %0.1lf...", locations.size(), m_options.m_bMulti ? m_options.m_CPU : 1, m_options.m_distanceMin);


			CGeoExtents extents = m_options.GetExtents();
			m_options.m_xxFinal = int(extents.YNbBlocks()*extents.XNbBlocks()*locations.size());

			//**************************************

			omp_set_nested(1);//for IOCPU
			boost::dynamic_bitset<size_t> treated(locations.size());

			//callback.PushTask("Generate Well Distributed Station", c);

			CApproximateNearestNeighbor ann;
			ann.set(locations, false, false);


			boost::dynamic_bitset<size_t> status(locations.size());
			//status.resize(locations.size());
			status.set();

			//callback.PushTask("Eliminate points: " + ToString(locations.size()), locations.size());

			//#pragma omp parallel for num_threads( m_optionss.m_CPU ) if (m_optionss.m_bMulti)
			for (__int64 i = 0; i < (__int64)locations.size(); i++)
			{


				bool bStatus = true;
#pragma omp critical(UpdateStatus)
				{
#pragma omp flush
					bStatus = status[i];
				}
				

				if (bStatus)
				{
					CSearchResultVector result;
					SearchD(ann, result, locations[i], m_options.m_distanceMin);

					//#pragma critical ELIMINATION
					//			if (status[i])
					//		{
					//#pragma critical ELIMINATION
#pragma omp critical(UpdateStatus)
					{
						for (size_t ii = 0; ii < result.size(); ii++)
						{

							status.reset(ii);
						}
						//}

#pragma omp flush
					}
					//msg += callback.StepIt();
				}
			}

			//callback.PopTask();

			//add noData to untreated line
			//std::ostringstream st;
			//st << std::fixed << std::setprecision(m_options.m_precision) << m_options.m_dstNodata;
			/*string strNodata = TestToString(m_options.m_dstNodata, m_options.m_precision);
			for (size_t i = 0; i < ioFile.size(); i++)
			{
			if (!treated[i])
			{
			for (size_t z = 0; z < bandHolder[0].GetRasterCount(); z++)
			ioFile[i] += ',' + strNodata;
			}
			}*/

			m_options.m_timerWrite.Start();
			//msg = ioFileIn.Save(m_options.m_filesPath[OUTPUT_FILE_PATH]);
			m_options.m_timerWrite.Stop();

			//CloseAll();
		}


		return msg;

	}

	//bool CPointsReduction::AtLeastOnePointIn(const CGeoExtents& blockExtents, const CGeoCoordFile& ioFile)
	//{
	//	bool bAtLeastOne = false;
	//	for (int i = 0; i < ioFile.m_xy.size() && !bAtLeastOne; i++)
	//		bAtLeastOne = blockExtents.IsInside(ioFile.m_xy[i]);

	//	return bAtLeastOne;
	//}

	ERMsg CPointsReduction::OpenAll(CLocationVector& locations)
	{
		ERMsg msg;


		if (!m_options.m_bQuiet)
			cout << endl << "Load coordinates..." << endl;

		CTimer timeLoadCSV(true);
		msg = locations.Load(m_options.m_filesPath[INPUT_FILE_PATH]);
		timeLoadCSV.Stop();

		if (msg)
		{
			if (!m_options.m_bQuiet)
			{
				//CProjectionPtr pPrj = CProjectionManager::GetPrj(ioFile.m_xy.GetPrjID());
				//string prjName = pPrj ? pPrj->GetName() : "Unknown";

				cout << "    Size           = " << to_string(locations.size()) << " points" << endl;
				//cout << "    Projection     = " << prjName << endl;
				cout << "    Time to load   = " << SecondToDHMS(timeLoadCSV.Elapsed()) << endl;
				cout << endl;
			}

			//msg = ioFile.ManageProjection(inputDS.GetPrjID());
		}


		if (msg)
		{

			if (!m_options.m_bOverwrite && FileExists(m_options.m_filesPath[OUTPUT_FILE_PATH]))
			{
				msg.ajoute("ERROR: Output file already exist. Delete the file or use option -overwrite.");
				return msg;
			}
		}

		return msg;
	}




	
	void CPointsReduction::SearchD(CApproximateNearestNeighbor& ann, CSearchResultVector& searchResultArray, const CLocation& location, double d)
	{
		searchResultArray.clear();

		static const size_t NB_MATCH_MAX = 3;

		CSearchResultVector tmp;
		ann.search(location, NB_MATCH_MAX, tmp);

		//if no stations is farther than the distance with try to fin more stations
		for (size_t f = 2; !tmp.empty() && tmp.back().m_distance < d && tmp.size() < ann.size(); f *= 2)
			ann.search(location, f*NB_MATCH_MAX, tmp);

		searchResultArray.reserve(tmp.size());
		for (size_t i = 0; i < tmp.size(); i++)
		{
			if (tmp[i].m_distance < d)
				searchResultArray.push_back(tmp[i]);
		}


		//m_nbStationStat += searchResultArray.size();
	}

	
	}