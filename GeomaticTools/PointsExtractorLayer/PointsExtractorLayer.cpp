//***********************************************************************
// program to extract points from image
//									 
//***********************************************************************
// version
// 1.1.0	13/09/2023	Rémi Saint-Amant	Compile with GDAL 3.7.1
// 1.0.0	08/12/2020	Rémi Saint-Amant	Creation

#include "stdafx.h" 
#include <float.h>
#include <math.h>
#include <algorithm>
#include <array>
#include <boost\multi_array.hpp>
#include <iostream>


#include "PointsExtractorLayer.h"
#include "Basic/OpenMP.h"
#include "Geomatic/GDALBasic.h"
#include "Basic/UtilMath.h"
//#include "CSVFile.h"

#pragma warning(disable: 4275 4251)
//#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "ogr_spatialref.h"



using namespace std;
namespace WBSF
{

	const char* CPointsExtractorLayer::VERSION = "1.1.0";
	const int CPointsExtractorLayer::NB_THREAD_PROCESS = 2;


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

	CPointsExtractorLayerOption::CPointsExtractorLayerOption() :CBaseOptions(false)
	{
		//m_precision = 4;
		m_appDescription = "This software extract bands information from input image and coordinates's file";

		static const char* DEFAULT_OPTIONS[] = { "-srcnodata", "-dstnodata", "-q", "-overwrite", "-te", "-mask", "-maskValue", "-multi", "-CPU", "-IOCPU", "-BlockSize", "-?", "-??", "-???", "-help" };
		for (int i = 0; i < sizeof(DEFAULT_OPTIONS) / sizeof(char*); i++)
			AddOption(DEFAULT_OPTIONS[i]);


		static const COptionDef OPTIONS[] =
		{
			//{ "-Condition", 1, "type", true, "Add conditions to the extraction. 4 possibility of condition can be define: \"AllValid\", \"AtLeastOneValid\", \"AtLeastOneMissing\", \"AllMissing\". No conditions are define by default (all will be output)." },
			{ "-X", 1, "str", false, "File header title for X coordinates. \"X\" by default." },
			{ "-Y", 1, "str", false, "File header title for Y coordinates. \"Y\" by default." },
			//{ "-prec", 1, "precision", false, "Output precision. 4 by default." },
			{ "layer", 0, "", false, "Layer to extract information." },
			{ "srcfile", 0, "", false, "Input coordinate file path (CSV)" },
			{ "dstfile", 0, "", false, "Output information file path (CSV)." }
		};

		for (int i = 0; i < sizeof(OPTIONS) / sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);


		static const CIOFileInfoDef IO_FILE_INFO[] =
		{
			{ "Input", "Layer", "", "*", "", "" },
			{ "Input", "srcfile", "", "3 or more", "ID|X|Y|other information...", "The columns order is not important. The coordinates must have a column header \"X\" and \"Y\". A line's ID is recommended because line order is not kept in extraction" },
			{ "Output", "dstfile", "", "", "ID|X|Y|others information...|all bands values...", "" },
		};

		for (int i = 0; i < sizeof(IO_FILE_INFO) / sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);


		m_XHeader = "X";
		m_YHeader = "Y";
	}


	ERMsg CPointsExtractorLayerOption::ParseOption(int argc, char* argv[])
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

	ERMsg CPointsExtractorLayerOption::ProcessOption(int& i, int argc, char* argv[])
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
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}

		return msg;
	}

	

	//*************************************************************************************************************************************************************

	//#include <boost/format.hpp>

	ERMsg CPointsExtractorLayer::Execute()
	{
		ERMsg msg;

		if (!m_options.m_bQuiet)
		{
			cout << endl;
			cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
			cout << "From:   " << m_options.m_filesPath[INPUT_FILE_PATH] << endl;
			cout << "Using:  " << m_options.m_filesPath[LAYER_FILE_PATH] << endl;

			if (!m_options.m_maskName.empty())
				cout << "Mask:   " << m_options.m_maskName << endl;
		}

		//GDALAllRegister();
		OGRRegisterAll();


		//GDALDriver* pDriver = OGRSFDriverRegistrar::GetDriverByName(m_options.m_filesPath[LAYER_FILE_PATH].c_str());

		GDALDataset *poDS = (GDALDataset*)GDALOpenEx(m_options.m_filesPath[LAYER_FILE_PATH].c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
		if (poDS == NULL)
		{
			printf("Open failed.\n");
			exit(1);
		}
		
		
		OGRLayer  *poLayer;

		poLayer = poDS->GetLayerByName("point");

		OGRFeature *poFeature;

		poLayer->ResetReading();
		while ((poFeature = poLayer->GetNextFeature()) != NULL)
		{
			OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
			int iField;

			for (iField = 0; iField < poFDefn->GetFieldCount(); iField++)
			{
				OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn(iField);

				if (poFieldDefn->GetType() == OFTInteger)
					printf("%d,", poFeature->GetFieldAsInteger(iField));
				else if (poFieldDefn->GetType() == OFTReal)
					printf("%.3f,", poFeature->GetFieldAsDouble(iField));
				else if (poFieldDefn->GetType() == OFTString)
					printf("%s,", poFeature->GetFieldAsString(iField));
				else
					printf("%s,", poFeature->GetFieldAsString(iField));
			}

			OGRGeometry *poGeometry;

			poGeometry = poFeature->GetGeometryRef();
			if (poGeometry != NULL
				&& wkbFlatten(poGeometry->getGeometryType()) == wkbPoint)
			{
				OGRPoint *poPoint = (OGRPoint *)poGeometry;

				printf("%.3f,%3.f\n", poPoint->getX(), poPoint->getY());
			}
			else
			{
				printf("no point geometry\n");
			}
			OGRFeature::DestroyFeature(poFeature);
		}

		
		GDALClose(poDS);
		
	
		m_options.PrintTime();

		return msg;
	}

	
	/*ERMsg CPointsExtractorLayer::OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGeoCoordFile& ioFile)
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






void OpenShapeFile(char* filename)
{
OGRErr error;
OGRDataSource *poDataSource;
poDataSource = OGRSFDriverRegistrar::Open(filename,false);
OGRLayer *poLayer;
poLayer = poDataSource ->GetLayer(0);
OGREnvelope Envelope;
error = poLayer ->GetExtent(&Envelope,true);
sBoundingBox.fMaxX = Envelope.MaxX;
sBoundingBox.fMaxY = Envelope.MaxY;
sBoundingBox.fMinX = Envelope.MinX;
sBoundingBox.fMinY = Envelope.MinY;

OGRwkbGeometryType LayerGeometryType = poLayer ->GetGeomType();
int NumberOfFeatures = poLayer ->GetFeatureCount(true);
poLayer ->ResetReading();

//Point Shapefile
if ( wkbFlatten ( LayerGeometryType ) == wkbPoint )
{
   OGRFeature *poFeature;
   for ( int i = 0; i < NumberOfFeatures; i++ )
   {
	   poFeature = poLayer ->GetNextFeature();
	   OGRGeometry *poGeometry;
	   poGeometry = poFeature ->GetGeometryRef();
	   if ( poGeometry != NULL && wkbFlatten ( poGeometry ->getGeometryType() ) == wkbPoint )
	   {
		   OGRPoint *poPoint = ( OGRPoint * )poGeometry;
		   MyPoint2D pt;
		   pt.dX = poPoint ->getX();
		   pt.dY = poPoint ->getY();
		   PointLayer.push_back(pt);
	   }
	   OGRFeature::DestroyFeature(poFeature);
   }
}

//Multipoint Shapefile
if ( wkbFlatten ( LayerGeometryType ) == wkbMultiPoint )
{
   OGRFeature *poFeature;
   MultipointFeature MultiPoint;
   for ( int i = 0; i < NumberOfFeatures; i++ )
   {
	   poFeature = poLayer ->GetNextFeature();
	   OGRGeometry *poGeometry;
	   poGeometry = poFeature ->GetGeometryRef();
	   if ( poGeometry != NULL && wkbFlatten ( poGeometry ->getGeometryType() ) == wkbMultiPoint )
	   {
		   OGRMultiPoint *poMultipoint = ( OGRMultiPoint * )poGeometry;
		   int NumberOfGeometries = poMultipoint ->getNumGeometries();
		   MultiPoint.PointsOfFeature.resize( NumberOfGeometries );
		   for ( int j = 0; j < NumberOfGeometries; j++ )
		   {
			   OGRGeometry *poPointGeometry = poMultipoint ->getGeometryRef(j);
			   OGRPoint *poPoint = ( OGRPoint * )poPointGeometry;
			   MyPoint2D pt;
			   pt.dX = poPoint ->getX();
			   pt.dY = poPoint ->getY();
			   MultiPoint.PointsOfFeature.at(j) = pt;
		   }
		   MultipointLayer.push_back(MultiPoint);
	   }
	   OGRFeature::DestroyFeature(poFeature);
   }
}

//Polyline Shapefile
if ( wkbFlatten ( LayerGeometryType ) == wkbLineString )
{
   OGRFeature *poFeature;
   LineFeature Polyline;
   OGRPoint ptTemp;
   for ( int i = 0; i < NumberOfFeatures; i++ )
   {
	   poFeature = poLayer ->GetNextFeature();
	   OGRGeometry *poGeometry;
	   poGeometry = poFeature ->GetGeometryRef();
	   if ( poGeometry != NULL && wkbFlatten ( poGeometry ->getGeometryType() ) == wkbLineString  )
	   {
		   OGRLineString *poLineString = ( OGRLineString * )poGeometry;
		   Polyline.LinesOfFeature.resize(1);
		   int NumberOfVertices = poLineString ->getNumPoints();
		   Polyline.LinesOfFeature.at(0).LineString.resize(NumberOfVertices);
		   for ( int k = 0; k < NumberOfVertices; k++ )
		   {
			   poLineString ->getPoint(k,&ptTemp);
			   MyPoint2D pt;
			   pt.dX = ptTemp.getX();
			   pt.dY = ptTemp.getY();
			   Polyline.LinesOfFeature.at(0).LineString.at(k) = pt;
		   }
		   LineLayer.push_back(Polyline);
	   }
	   else if ( poGeometry != NULL && wkbFlatten ( poGeometry ->getGeometryType() ) == wkbMultiLineString )
	   {
		   OGRMultiLineString *poMultiLineString = ( OGRMultiLineString * )poGeometry;
		   int NumberOfGeometries = poMultiLineString ->getNumGeometries();
		   Polyline.LinesOfFeature.resize(NumberOfGeometries);
		   for ( int j = 0; j < NumberOfGeometries; j++ )
		   {
			   OGRGeometry *poLineGeometry = poMultiLineString ->getGeometryRef(j);
			   OGRLineString *poLineString = ( OGRLineString * )poLineGeometry;
			   int NumberOfVertices = poLineString ->getNumPoints();
			   Polyline.LinesOfFeature.at(j).LineString.resize(NumberOfVertices);
			   for ( int k = 0; k < NumberOfVertices; k++ )
			   {
				   poLineString ->getPoint(k,&ptTemp);
				   MyPoint2D pt;
				   pt.dX = ptTemp.getX();
				   pt.dY = ptTemp.getY();
				   Polyline.LinesOfFeature.at(j).LineString.at(k) = pt;
			   }
		   }
		   LineLayer.push_back(Polyline);
	   }
	   OGRFeature::DestroyFeature(poFeature);
   }
}

//Polygon Shapefile
if ( wkbFlatten ( LayerGeometryType ) == wkbPolygon )
{
   OGRFeature *poFeature;
   PolygonFeature Polygon;
   OGRPoint ptTemp;
   for ( int i = 0; i < NumberOfFeatures; i++ )
   {
	   poFeature = poLayer ->GetNextFeature();
	   OGRGeometry *poGeometry;
	   poGeometry = poFeature ->GetGeometryRef();
	   if ( poGeometry != NULL && wkbFlatten ( poGeometry ->getGeometryType() ) == wkbPolygon )
	   {
		   OGRPolygon *poPolygon = ( OGRPolygon * )poGeometry;
		   Polygon.PolygonsOfFeature.resize(1);
		   int NumberOfInnerRings = poPolygon ->getNumInteriorRings();
		   OGRLinearRing *poExteriorRing = poPolygon ->getExteriorRing();
		   Polygon.PolygonsOfFeature.at(0).Polygon.resize(NumberOfInnerRings+1);
		   Polygon.PolygonsOfFeature.at(0).Polygon.at(0).IsClockwised = poExteriorRing ->isClockwise();
		   int NumberOfExteriorRingVertices = poExteriorRing ->getNumPoints();
		   Polygon.PolygonsOfFeature.at(0).Polygon.at(0).RingString.resize(NumberOfExteriorRingVertices);
		   for ( int k = 0; k < NumberOfExteriorRingVertices; k++ )
		   {
			   poExteriorRing ->getPoint(k,&ptTemp);
			   MyPoint2D pt;
			   pt.dX = ptTemp.getX();
			   pt.dY = ptTemp.getY();
			   Polygon.PolygonsOfFeature.at(0).Polygon.at(0).RingString.at(k) = pt;
		   }
		   for ( int h = 1; h <= NumberOfInnerRings; h++ )
		   {
			   OGRLinearRing *poInteriorRing = poPolygon ->getInteriorRing(h-1);
			   Polygon.PolygonsOfFeature.at(0).Polygon.at(h).IsClockwised = poInteriorRing ->isClockwise();
			   int NumberOfInteriorRingVertices = poInteriorRing ->getNumPoints();
			   Polygon.PolygonsOfFeature.at(0).Polygon.at(h).RingString.resize(NumberOfInteriorRingVertices);
			   for ( int k = 0; k < NumberOfInteriorRingVertices; k++ )
			   {
				   poInteriorRing ->getPoint(k,&ptTemp);
				   MyPoint2D pt;
				   pt.dX = ptTemp.getX();
				   pt.dY = ptTemp.getY();
				   Polygon.PolygonsOfFeature.at(0).Polygon.at(h).RingString.at(k) = pt;
			   }
		   }
			   PolygonLayer.push_back(Polygon);
	   }
	   else if ( poGeometry != NULL && wkbFlatten ( poGeometry ->getGeometryType() ) == wkbMultiPolygon )
	   {
		   OGRMultiPolygon *poMultiPolygon = ( OGRMultiPolygon * )poGeometry;
		   int NumberOfGeometries = poMultiPolygon ->getNumGeometries();
		   Polygon.PolygonsOfFeature.resize(NumberOfGeometries);
		   for ( int j = 0; j < NumberOfGeometries; j++ )
		   {
			   OGRGeometry *poPolygonGeometry = poMultiPolygon ->getGeometryRef(j);
			   OGRPolygon *poPolygon = ( OGRPolygon * )poPolygonGeometry;
			   int NumberOfInnerRings = poPolygon ->getNumInteriorRings();
			   OGRLinearRing *poExteriorRing = poPolygon ->getExteriorRing();
			   Polygon.PolygonsOfFeature.at(j).Polygon.resize(NumberOfInnerRings+1);
			   Polygon.PolygonsOfFeature.at(j).Polygon.at(0).IsClockwised = poExteriorRing ->isClockwise();
			   int NumberOfExteriorRingVertices = poExteriorRing ->getNumPoints();
			   Polygon.PolygonsOfFeature.at(j).Polygon.at(0).RingString.resize(NumberOfExteriorRingVertices);
			   for ( int k = 0; k < NumberOfExteriorRingVertices; k++ )
			   {
				   poExteriorRing ->getPoint(k,&ptTemp);
				   MyPoint2D pt;
				   pt.dX = ptTemp.getX();
				   pt.dY = ptTemp.getY();
				   Polygon.PolygonsOfFeature.at(j).Polygon.at(0).RingString.at(k) = pt;
			   }
			   for ( int h = 1; h <= NumberOfInnerRings; h++ )
			   {
				   OGRLinearRing *poInteriorRing = poPolygon ->getInteriorRing(h-1);
				   Polygon.PolygonsOfFeature.at(j).Polygon.at(h).IsClockwised = poInteriorRing ->isClockwise();
				   int NumberOfInteriorRingVertices = poInteriorRing ->getNumPoints();
				   Polygon.PolygonsOfFeature.at(j).Polygon.at(h).RingString.resize(NumberOfInteriorRingVertices);
				   for ( int k = 0; k < NumberOfInteriorRingVertices; k++ )
				   {
					   poInteriorRing ->getPoint(k,&ptTemp);
					   MyPoint2D pt;
					   pt.dX = ptTemp.getX();
					   pt.dY = ptTemp.getY();
					   Polygon.PolygonsOfFeature.at(j).Polygon.at(h).RingString.at(k) = pt;
				   }
			   }
		   }
		   PolygonLayer.push_back(Polygon);
	   }
   }
   OGRFeature::DestroyFeature(poFeature);
}

OGRDataSource::DestroyDataSource(poDataSource);
}
*/
	
}