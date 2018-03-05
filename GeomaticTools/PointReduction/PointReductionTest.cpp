//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************

//
#include "stdafx.h" 
#include <iostream>
#include "UnitTest++.h"
//#include <boost\archive\binary_oarchive.hpp>
//#include <boost\archive\binary_iarchive.hpp>
//#include <boost\serialization\vector.hpp>

#include "PointsExtractor.h"
#include "GDALBasic.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "ogr_spatialref.h"


//TEST Image


using namespace std;
using namespace CFL;
using namespace stdString;
using namespace GeoBasic;

 

SUITE(GeomaticBase)
{
	static const char* VRTFilePath = "c:\\temp\\test1.vrt";
	static const char* locFilePath = "c:\\temp\\test1.csv";
	static const char* outFilePath = "c:\\temp\\test1out.csv";
	
	static const char* filePath[5] = { "c:\\temp\\1.tif", "c:\\temp\\2.tif", "c:\\temp\\3.tif", "c:\\temp\\4.tif", "c:\\temp\\5.tif" };

	TEST(VerifyCoordinate)
	{
		CGeoPoint pt1(0, 0);
		CGeoPoint pt2(10, 10);
		CGeoRect rect1(0, 0, 10, 10);

		CHECK_EQUAL(true, rect1.IsInside(pt1));
		CHECK_EQUAL(true, rect1.IsInside(pt2));
		CHECK_EQUAL(false, rect1.IsRectEmpty());
		CHECK_EQUAL(false, rect1.IsRectNull());

		CGeoRect rect2(20, 20, 20, 30);
		CHECK_EQUAL(true, rect2.IsRectEmpty());
		CHECK_EQUAL(false, rect2.IsRectNull());
	}

	TEST(VerifyIndex)
	{
		CGeoPointIndex pt1(0, 0);
		CGeoPointIndex pt2(10, 10);
		CGeoRectIndex rect1(0, 0, 10, 10);


		CHECK_EQUAL(true, rect1.IsInside(pt1));
		CHECK_EQUAL(false, rect1.IsInside(pt2));
		CHECK_EQUAL(false, rect1.IsRectEmpty());
		CHECK_EQUAL(false, rect1.IsRectNull());

		CGeoRectIndex rect2(20, 20, 0, 10);
		CHECK_EQUAL(true, rect2.IsRectEmpty());
		CHECK_EQUAL(false, rect2.IsRectNull());
	}

	TEST(SaveData)
	{
		// -------------------------------------------------------------------- 
		//      Register standard GDAL drivers, and process generic GDAL        
		//      command options.                                                
		// -------------------------------------------------------------------- 
		GDALAllRegister();
		
		static const char* PRJ_TEST =
			"PROJCS[\"WGS 84 / TM\","
			"GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\", 6378137, 298.257223563,AUTHORITY[\"EPSG\", \"7030\"]],AUTHORITY[\"EPSG\", \"6326\"]],PRIMEM[\"Greenwich\", 0,AUTHORITY[\"EPSG\", \"8901\"]],UNIT[\"degree\", 0.0174532925199433,AUTHORITY[\"EPSG\", \"9122\"]],AUTHORITY[\"EPSG\", \"4326\"]],"
			"PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\", 0],PARAMETER[\"central_meridian\", 0],PARAMETER[\"scale_factor\", 0.9996],PARAMETER[\"false_easting\", 0],PARAMETER[\"false_northing\", 0],UNIT[\"metre\", 1,AUTHORITY[\"EPSG\", \"9001\"]],"
			"]";
			

		ERMsg msg = CProjectionManager::CreateProjection(PRJ_TEST);
		CHECK(msg);
		size_t prjID = CProjectionManager::GetPrjID(PRJ_TEST);
		
		
		CGeoExtents extents[5] = { CGeoExtents(0, 0, 100, 100, 100, 100, 16, 16, prjID), CGeoExtents(100, 0, 200, 100, 100, 100, 16, 16, prjID), CGeoExtents(0, 100, 100, 200, 100, 100, 16, 16, prjID), CGeoExtents(100, 100, 200, 200, 100, 100, 16, 16, prjID), CGeoExtents(50, 50, 150, 150, 100, 100, 16, 16, prjID) };
		int FACTOR[5] = { 10000, 20000, 30000, 40000, 50000 };

		CBaseOptions option;
		
		option.m_createOptions.push_back("COMPRESS=LZW");
		option.m_createOptions.push_back("TILED=YES");
		option.m_createOptions.push_back("BLOCKXSIZE=16");
		option.m_createOptions.push_back("BLOCKYSIZE=16");
		option.m_nbBands = 1;
		option.m_dstNodata = -9999;
		option.m_prj = PRJ_TEST;
		option.m_format = "GTIFF";
		option.m_outputType = GDT_Int32;
		option.m_bUseDefaultNoData = true; 
		option.m_bOverwrite = true;

		for (int i = 0; i < 5; i++)
		{
			option.m_extents = extents[i];
			CGDALDatasetEx dataset;
			msg = dataset.CreateImage(filePath[i], option);
			CHECK(msg);

			vector<int> output(100*100);
			//fill images
			for (int y = 0; y < 100; y++)
				for (int x = 0; x < 100; x++)
					output[y * 100 + x] = FACTOR[i] + y * 100 + x;

			dataset.GetRasterBand(0)->RasterIO(GF_Write, 0, 0, 100, 100, &(output[0]), 100, 100, GDT_Int32,0,0);
			dataset.Close();
		}

		//Create VRT file
		string command = string("gdalbuildvrt.exe -hidenodata -separate \"") + VRTFilePath + "\" \"c:\\temp\\*.tif\"";

		msg = WinExecWait(command.c_str());
		CHECK(msg);

		//CHECK(!CFL::FileExists(m_filePath));

		//ofStream file;
		//CHECK(file.open(m_filePath));
		//boost::archive::binary_oarchive ar(file);

		//ar << ANN;
		//file.close();

		ofStream file;
		msg = file.open(locFilePath);
		CHECK(msg);

		file.write("X,Y\n");
		//Create location file list
		for (int y = 0; y < 200; y++)
		{
			for (int x = 0; x < 200; x++)
			{
				file.write(to_string(x + 0.5f) + "," + to_string(y + 0.5f) + "\n");
			}
		}

		file.close();

	}

	

	TEST(ExecutePointExtractor)
	{

		
		static const char* argV[] = { __FILE__, "-multi", "-BlockSize", "3","3", "-dstNoData", "-9999", "-overwrite", "-q", VRTFilePath, locFilePath, outFilePath};
		static const int argC = sizeof(argV) / sizeof(char*);

		CTimer timer(true);

		CPointsExtractor pointsExtractor;
		ERMsg msg = pointsExtractor.m_options.ParseOption(argC, const_cast<char**>(argV) );

		if( !msg || !pointsExtractor.m_options.m_bQuiet )
			cout << pointsExtractor.GetDescription();


		if( msg )  
			msg = pointsExtractor.Execute();

		CHECK(msg);
		if( !msg)  
		{
			PrintMessage(msg);
		}

		timer.Stop();

		if( !pointsExtractor.m_options.m_bQuiet )
			cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;




		//ifStream file;
		//CHECK(file.open(m_filePath));

		//boost::archive::binary_iarchive ar(file);

		//ar >> ANN;
		//file.close();

		//CHECK(ANN.size() == 4);

		//CSearchResultVector result;
		//CHECK(ANN2.Search(pt, 4, result));
		//CHECK_EQUAL(1, result[0].m_index);
		//CHECK_EQUAL(3, result[1].m_index);
		//CHECK_EQUAL(4, result[2].m_index);
		//CHECK_EQUAL(2, result[3].m_index);
	}

	TEST(VerifyPointExtractorResults)
	{
		CGeoCoordFile file;
		ERMsg msg = file.Load(outFilePath);
		CHECK(msg);
		CHECK_EQUAL(40000, file.m_xy.size());
		
		
		for (size_t i = 0; i < file.size(); i++)
		{
			//CHECK_EQUAL(x, file.m_xy[y * 100 + x].m_x);
			//CHECK_EQUAL(y, file.m_xy[y * 100 + x].m_y);
				
			StringVector tmp(file[i], ",");
			CHECK_EQUAL(7, tmp.size());
			for (size_t j = 0; j < tmp.size(); j++)
			{
				int x = int(file.m_xy[i].m_x);
				int y = int(file.m_xy[i].m_y);

				string val = "-9999";
				switch (j)
				{
				case 0: val = to_string(x + 0.5f); break;
				case 1: val = to_string(y + 0.5f); break;
				case 2: if (x<100 && y<100) val = to_string(10000 + (100-y-1) * 100 + x);  break;
				case 3: if (x >= 100 && y<100) val = to_string(20000 + (100 - y - 1) * 100 + (x-100) );  break;
				case 4: if (x<100 && y >= 100) val = to_string(30000 + (200 - y - 1) * 100 + x);  break;
				case 5: if (x >= 100 && y >= 100) val = to_string(40000 + (200 - y - 1) * 100 + (x-100) );  break;
				case 6: if (x >= 50 && x<150 && y >= 50 && y<150) val = to_string(50000 + (150 - y - 1) * 100 + (x-50));  break;
				}

				CHECK_EQUAL(val, tmp[j]);
			}
		}
	
	}

	TEST(RemoveFile)
	{
//		CHECK(RemoveFile(m_filePath));
	}
}

