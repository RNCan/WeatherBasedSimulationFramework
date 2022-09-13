//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
// version
// 1.1.0	20/12/2021	Rémi Saint-Amant	Compile with VS 2019 and GDAL 3.0.3
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

	const char* CPointsReduction::VERSION = "1.1.0";
	const int CPointsReduction::NB_THREAD_PROCESS = 2;



	CPointsReductionOption::CPointsReductionOption() :
		CBaseOptions(false)
	{

		m_appDescription = "This software reduce the number of points of a locations file base on distance";

		static const char* DEFAULT_OPTIONS[] = { "-srcnodata", "-dstnodata", "-q", "-overwrite", "-te", "-mask", "-maskValue", "-multi", "-CPU", "-IOCPU", "-wm", "-BlockSize", "-?", "-??", "-???", "-help" };
		for (int i = 0; i < sizeof(DEFAULT_OPTIONS) / sizeof(char*); i++)
			AddOption(DEFAULT_OPTIONS[i]);

		m_maxN = NOT_INIT;
		m_sort = 0;
		m_distanceMin = 5000;
		static const COptionDef OPTIONS[] =
		{
			{ "-d", 1, "distance [km]", false, "Minimum distance between points. 5 km by default." },
			{ "-s", 1, "sort by", false, "sort by 1=latitude, 2=longitude 3=elevation. Use negative sign(-) to revert sort" },
			{ "-maxN", 1, "points", false, "maximum number of points. If the reexamining points is greater than maxN, extra elimination at random" },
			
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

		if (msg && m_filesPath.size() != 2)
		{
			msg.ajoute("Invalid argument line. 2 files are needed: input and output coordinates file (CSV).\n");
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
		else if (IsEqual(argv[i], "-s"))
		{
			m_sort = ToInt(argv[++i]);
			if (m_sort < -3 || m_sort>3)
				msg.ajoute("Bad sort");
		}
		else if (IsEqual(argv[i], "-maxN"))
		{
			m_maxN = ToInt(argv[++i]);
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
				printf("Reduce %I64u points using distance min of %0.1lf km ...\n", locations.size(),  m_options.m_distanceMin/1000);


			CGeoExtents extents = m_options.GetExtents();
			m_options.m_xxFinal = int(extents.YNbBlocks() * extents.XNbBlocks() * locations.size());

			//**************************************

			omp_set_nested(1);//for IOCPU
			switch (m_options.m_sort)
			{
			case -3: std::sort(locations.begin(), locations.end(), [](const CLocation& s1, const CLocation& s2) { return s1.m_lat > s2.m_lat; }); break;
			case -2: std::sort(locations.begin(), locations.end(), [](const CLocation& s1, const CLocation& s2) { return s1.m_lon > s2.m_lon; }); break;
			case -1: std::sort(locations.begin(), locations.end(), [](const CLocation& s1, const CLocation& s2) { return s1.m_elev > s2.m_elev; }); break;
			case 0:break;
			case 1: std::sort(locations.begin(), locations.end(), [](const CLocation& s1, const CLocation& s2) { return s1.m_lat < s2.m_lat; }); break;
			case 2: std::sort(locations.begin(), locations.end(), [](const CLocation& s1, const CLocation& s2) { return s1.m_lon < s2.m_lon; }); break;
			case 3: std::sort(locations.begin(), locations.end(), [](const CLocation& s1, const CLocation& s2) { return s1.m_elev < s2.m_elev; }); break;
			}
			

			CApproximateNearestNeighbor ann;
			ann.set(locations, false, false);


			boost::dynamic_bitset<size_t> status(locations.size());
			status.set();

			for (size_t i = 0; i < locations.size(); i++)
			{
				if (status[i])
				{
					CSearchResultVector result;
					SearchD(ann, result, locations[i], m_options.m_distanceMin);

					for (size_t ii = 0; ii < result.size(); ii++)
					{
						if (result[ii].m_index > i)
							status.reset(result[ii].m_index);
					}
				}
			}//for all locations



			//save location
			
			CLocationVector locations_out(status.count());
			for (size_t i = 0, ii = 0; i < locations.size(); i++)
				if (status.test(i))
					locations_out[ii++] = locations[i];

			printf("Number of remaining points %I64u\n", locations_out.size());
			if (m_options.m_maxN != NOT_INIT&& locations_out.size() > m_options.m_maxN)
			{
				while (locations_out.size() > m_options.m_maxN)
					locations_out.erase(locations_out.begin() + size_t(WBSF::Rand(0, locations_out.size() - 1)));

				printf("Number of remaining points after limiting to maxN %I64u\n", locations_out.size());
			}
			

			
			msg = locations_out.Save(m_options.m_filesPath[OUTPUT_FILE_PATH]);

			
			m_options.m_timerWrite.Start();
			m_options.m_timerWrite.Stop();
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
			ann.search(location, f * NB_MATCH_MAX, tmp);

		searchResultArray.reserve(tmp.size());
		for (size_t i = 0; i < tmp.size(); i++)
		{
			if (tmp[i].m_distance < d)
				searchResultArray.push_back(tmp[i]);
		}


		//m_nbStationStat += searchResultArray.size();
	}


}