//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <iostream>
#include <boost/scoped_array.hpp>
#include <tchar.h>

#pragma warning(disable: 4275 4251)
#include "GDAL_priv.h"
#include "ogr_api.h"
#include "ogr_srs_api.h"
#include "gdal.h"



#include "Basic/OpenMP.h"
#include "Geomatic/UtilGDAL.h"
#include "Geomatic/Projection.h"
//#include "CSV.h"



using namespace std;

namespace WBSF
{

void RegisterGDAL()
{
	char test[255] = { 0 };
	const char* pTest = CPLGetConfigOption("GDAL_DRIVER_PATH", test);
	
	const char* pTest2 = CPLGetConfigOption("GDAL_DATA", test);


	string path = GetApplicationPath() + "External";
	CPLSetConfigOption("GDAL_DRIVER_PATH", path.c_str());

	path += "\\gdal-data";
	CPLSetConfigOption("GDAL_DATA", path.c_str());

	GDALAllRegister();
}

ERMsg OpenInputImage(const string& filePath, GDALDataset** poInputDS, double srcNodata, bool bUseDefaultNoData)
{
	ERMsg msg;
	
	//GDAL_OF_SHARED|
	*poInputDS = (GDALDataset *)GDALOpenEx(filePath.c_str(), GDAL_OF_READONLY | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);

	if( *poInputDS == NULL)
		msg.ajoute(CPLGetLastErrorMsg());

	

	//if( msg )
	//{
	//	const char* desc = (*poInputDS)->GetDriver()->GetDescription();
	//	bool bVRT = strcmp(desc, "VRT") == 0;
	//	if (!bVRT)
	//	{//dont set no data if VRT
	//		for (int i = 0; i < (*poInputDS)->GetRasterCount() && msg; i++)
	//		{
	//			double noData = srcNodata;
	//			if (noData == MISSING_NO_DATA)
	//			{
	//				noData = ::GetNoData((*poInputDS)->GetRasterBand(i+1));
	//				if (noData == MISSING_NO_DATA && bUseDefaultNoData)
	//					noData = ::GetDefaultNoData((*poInputDS)->GetRasterBand(i + 1)->GetRasterDataType());
	//			}
	//			else
	//			{
	//				msg += VerifyNoData(noData, (*poInputDS)->GetRasterBand(i + 1)->GetRasterDataType());
	//			}

	//			(*poInputDS)->GetRasterBand(i + 1)->SetNoDataValue(noData);
	//		}
	//	}
	//}

	return msg;
}


ERMsg VerifyNoData(double nodata, short eType)
{
	ERMsg msg;

	
	if( nodata < GetTypeLimit(eType, true) || nodata > GetTypeLimit(eType, false) )
	{
		
		if( eType != GDT_Byte)
		{
			string error = string("ERROR: Nodata (" + ToString(nodata) + ") is out of range[" + ToString(GetTypeLimit(eType, true)) + ", " + ToString(GetTypeLimit(eType, false)) + "]\n");
			msg.ajoute(error);
		}
		else
		{
			string warning = string("ERROR: Nodata (" + ToString(nodata) + ") is out of range[" + ToString(GetTypeLimit(eType, true)) + ", " + ToString(GetTypeLimit(eType, false)) + "]\n");
			_tprintf(convert(warning).c_str());
		}
	}

	return msg;
}

double GetNoData(GDALRasterBand* pBand)
{
	double noData = MISSING_NO_DATA;

	int bSuccess = 0;
	noData = pBand->GetNoDataValue(&bSuccess);
	if (!bSuccess)
	{
		short eType = pBand->GetRasterDataType();
		noData = GetDefaultNoData(eType);
	}

	return noData;
}

ERMsg VerifyInputSize(GDALDataset* poInputDS1, GDALDataset* poInputDS2 )
{
	ASSERT( poInputDS1 );
	ASSERT( poInputDS2 );
	
	ERMsg msg;

	if( poInputDS1->GetRasterXSize() != poInputDS2->GetRasterXSize() ||
		poInputDS1->GetRasterYSize() != poInputDS2->GetRasterYSize() )
	{
		msg.ajoute( "ERROR: At least one input image don't have the same extent");
	}

	return msg;
}


double GetTypeLimit(short eType, bool bLow)
{
	double limit = 0;
	switch (eType)
	{
	case GDT_Unknown: limit = 0; break;
	case GDT_Byte: limit = bLow ? 0 : UCHAR_MAX; break;
	case GDT_UInt16: limit = bLow ? 0 : USHRT_MAX; break;
	case GDT_Int16: limit = bLow ? SHRT_MIN : SHRT_MAX; break;
	case GDT_UInt32: limit = bLow ? 0 : UINT_MAX; break;
	case GDT_Int32: limit = bLow ? INT_MIN : INT_MAX; break;
	case GDT_Float32: limit = bLow ? -FLT_MAX : FLT_MAX; break;
	case GDT_Float64: limit = bLow ? -DBL_MAX : DBL_MAX; break;
	case GDT_CInt16: limit = bLow ? SHRT_MIN : SHRT_MAX; break;
	case GDT_CInt32: limit = bLow ? INT_MIN : INT_MAX; break;
	case GDT_CFloat32: limit = bLow ? -FLT_MAX : FLT_MAX; break;
	case GDT_CFloat64: limit = bLow ? -DBL_MAX : DBL_MAX; break;
	default: ASSERT(false);
	}

	return limit;
}

double LimitToBound(double v, short cellType, double shiftedBy, bool bRound)
{
	if (v>-FLT_MAX && v<FLT_MAX && v != MISSING_DATA && !_isnan(v) && ::_finite(v))
	{
		if (bRound && cellType < GDT_Float32)
			v = (double)Round(v);

		if (v < GetTypeLimit(cellType, true))
			v = GetTypeLimit(cellType, true) + shiftedBy;
		else if (v > GetTypeLimit(cellType, false))
			v = GetTypeLimit(cellType, false);// -shiftedBy;
	}
	else
	{
		v = GetTypeLimit(cellType, true);
	}

	return v;
}

double GetDefaultNoData(short eType)
{
	return GetTypeLimit(eType, eType != GDT_Byte && eType != GDT_UInt16 && eType != GDT_UInt32);
}


//open output image
ERMsg OpenOutputImage(const string& filePath, GDALDataset* poInputDS, GDALDataset** poOutputDS, const char* outDriverName, short cellType, int nbBand, double dstNodata, const StringVector& createOptions, bool bOverwrite, const CGeoExtents& extentsIn, bool bUseDefaultNoData)
{
	
	ERMsg msg;


	CGeoExtents extents(extentsIn);

	if( !bOverwrite )
	{
		//verify that the file doesn't exist
		FILE* pFile = fopen( filePath.c_str(), "r");
		if( pFile)
		{
			fclose(pFile);
			msg.ajoute("ERROR: Output file already exist. Delete the file or use option -overwrite.");
			return msg;
		}
	}

	ASSERT( poInputDS->GetRasterCount() > 0);

	GDALDriver* poDriverOut = poInputDS->GetDriver();
	if( outDriverName && strlen(outDriverName)>0)
		poDriverOut = (GDALDriver*)GDALGetDriverByName( outDriverName );

    
	//short 
	if( cellType == GDT_Unknown)
		cellType = poInputDS->GetRasterBand(1)->GetRasterDataType();

	if( nbBand<=0)
		nbBand = poInputDS->GetRasterCount();

	char **papszOptions = NULL;
	for(size_t i=0; i<createOptions.size(); i++)
		papszOptions = CSLAddString( papszOptions, createOptions[i].c_str() );

	
	if( !extents.IsInit() )
	{
		CGeoTransform GT;
		poInputDS->GetGeoTransform(GT);
		extents.SetGeoTransform(GT, poInputDS->GetRasterXSize(), poInputDS->GetRasterYSize());
	}

	*poOutputDS = poDriverOut->Create(filePath.c_str(), extents.m_xSize, extents.m_ySize, nbBand, (GDALDataType)cellType, papszOptions);
	

	CPLFree(papszOptions);
	papszOptions=NULL;

	if( *poOutputDS == NULL)
	{
		msg.ajoute(CPLGetLastErrorMsg());
		return msg;
	}
	
	
	//init output image with input image
	//double adfGeoTransform[6] = {0};
	//poInputDS->GetGeoTransform(adfGeoTransform);
	CGeoTransform GT;
	extents.GetGeoTransform(GT);
	(*poOutputDS)->SetGeoTransform(GT);
	(*poOutputDS)->SetProjection( poInputDS->GetProjectionRef() );

	bool bUseBandByBand = nbBand == poInputDS->GetRasterCount();
	for(int i=0; i<nbBand&&msg; i++)
	{
		double nodata = dstNodata;
		//if no data isn't supply, take the input no data (if we have the same number of band) if present, if not take smaller value of the type
		if( nodata == MISSING_NO_DATA )
		{
			if (bUseDefaultNoData)
			//if there is no output nodata and there is no image input nodata then
			//All types excep Byte will used the minimum value of the type
//			if( cellType != GDT_Byte)
				nodata = GetDefaultNoData(cellType);
			//else nodata = MISSING_NO_DATA;

		}
		else
		{
			msg += VerifyNoData(nodata, (*poOutputDS)->GetRasterBand(i+1)->GetRasterDataType());
		}

		if( msg && nodata != MISSING_NO_DATA )
		{
			(*poOutputDS)->GetRasterBand(i+1)->SetNoDataValue(nodata);
		}
	}

	return msg;
}


void Close(GDALDataset* poDS)
{
	// Once we're done, close properly the dataset 
	if( poDS!=NULL)
		GDALClose( (GDALDatasetH) poDS );
}

void PosToCoord(double* GT, int Xpixel, int Yline, double& Xgeo, double& Ygeo)
{
	Xgeo = GT[0] + (Xpixel+0.5)*GT[1] + (Yline+0.5)*GT[2];
    Ygeo = GT[3] + (Xpixel+0.5)*GT[4] + (Yline+0.5)*GT[5];
}

void CoordToPos(double* GT, double Xgeo, double Ygeo, int& Xpixel, int& Yline)
{
	_ASSERTE( GT[2]==0 && GT[4]==0);

	Xpixel = (int)((Xgeo-GT[0])/GT[1]-0.5);
    Yline = (int)((Ygeo-GT[3])/GT[5]-0.5);
}

CStatistic GetWindowMean(GDALRasterBand* pBand, int x1, int y1, int x2, int y2)
{
	ASSERT( x1<=x2&&y1<=y2);

	CStatistic stat;
	
	double noData = GetNoData(pBand);

	//int bSuccess=0;
	////double noData= pBand->GetNoDataValue(&bSuccess);
	//if( !bSuccess )
		//noData = ::GetDefaultNoData(pBand->GetRasterDataType());
	
	CStatistic::SetVMiss(noData);

	//std::vector<float> dataIn(abs(x1-x2)*abs(y1-y2)+1);
	for(int x=x1; x<=x2; x++)
	{
		for(int y=y1; y<=y2; y++)
		{
			if( x>=0 && x<pBand->GetXSize() &&
				y>=0 && y<pBand->GetYSize() )
			{
				float v=0;
				pBand->RasterIO( GF_Read, x, y, 1, 1, &v, 1, 1, GDT_Float32, 0, 0 );
				if( v>noData)
					stat+=v;
			}
		}
	}
	
	return stat;
}

double GetWindowMean(GDALRasterBand* pBand, int nbNeighbor, double T, const CGeoPointIndexVector& pts, const std::vector<double>& d)
{
	ASSERT( pts.size() > 0);
	ASSERT( pts.size() == d.size() );

	/*CStatistic X;
	CStatistic Y;
	for(size_t i=0; i<pts.size(); i++) 
	{
		ASSERT( pts[i].m_x>=0 && pts[i].m_x<pBand->GetXSize() );
		ASSERT( pts[i].m_y>=0 && pts[i].m_y<pBand->GetYSize() );
		X += pts[i].m_x;
		Y += pts[i].m_y;
	}*/

	//int x1, int y1, int x2, int y2

	
	double noData = GetNoData(pBand);

	//int bSuccess=0;
	//double noData= pBand->GetNoDataValue(&bSuccess);
	//if( !bSuccess )
		//noData = ::GetDefaultNoData(pBand->GetRasterDataType());
	
	//int nbX = int(X[HIGHEST]-X[LOWEST]+1);
	//int nbY = int(Y[HIGHEST]-Y[LOWEST]+1);
	//vector<float> v(nbX*nbY);
	//pBand->RasterIO( GF_Read, int(X[LOWEST]), int(Y[LOWEST]), nbX, nbY, &(v[0]), nbX, nbY, GDT_Float32, 0, 0 );


	CStatistic W;
	CStatistic H;
	
	for(size_t i=0; i<pts.size()&&W[NB_VALUE]<nbNeighbor; i++)
	{
		float Fi=(float)noData;
		pBand->RasterIO( GF_Read, pts[i].m_x, pts[i].m_y, 1, 1, &Fi, 1, 1, GDT_Float32, 0, 0 );
		//float Fi = v[int((pts[i].m_y-Y[LOWEST])*nbX + (pts[i].m_x-X[LOWEST]))];
		
		if( fabs(Fi-noData) > EPSILON_NODATA )
		{
			//if( d[i]<=maxDistance )
			double Wi = 0;
			Wi = pow(d[i],-T);

			H += Wi*Fi;
			W += Wi;
		}
	}
	//assert(false);//a remplacer pas MISSING?
	return W[SUM]>0 ? float(H[SUM] / W[SUM]) : noData;
}

CGeoExtents GetExtents(GDALDataset* poDataset)
{
	CGeoExtents extents;

	if( poDataset )
	{
		//Set projection
		if (CProjectionManager::CreateProjection(poDataset->GetProjectionRef()))
			extents.SetPrjID(CProjectionManager::GetPrjID(poDataset->GetProjectionRef()));


		CGeoTransform GT;
		poDataset->GetGeoTransform(GT);

		extents.SetGeoTransform(GT, poDataset->GetRasterXSize(), poDataset->GetRasterYSize());
		extents.SetPrjID(CProjectionManager::GetPrjID(poDataset->GetProjectionRef()));
		extents.NormalizeRect();
		
		//take block size of the first layer
		if( poDataset && poDataset->GetRasterCount()>0)
		{
			GDALRasterBand* pBand = poDataset->GetRasterBand(1);
			pBand->GetBlockSize(&extents.m_xBlockSize, &extents.m_yBlockSize);
		}
	}

	return extents;
}

ERMsg GetFilePathList(const char* fileName, int filePathCol, StringVector& filePathList)
{
	ERMsg msg;

	ifStream file;
	msg = file.open(fileName);
	if( !msg )
		return msg;

	
	string path = GetPath(fileName);
	string line;
	std::getline(file, line);
	

	while (std::getline(file, line))
	{
		Trim(line);
		if( !line.empty() )
		{
			string layerFilePath;
			string::size_type pos = 0;
			for (int i = 0; i<filePathCol&&pos != string::npos; i++)
				layerFilePath = Tokenize(line, ",;", pos);

			if( !layerFilePath.empty() )
			{
				ReplaceString(layerFilePath, "/", "\\");
				if (layerFilePath[0] == '\\' )
					layerFilePath = "." + layerFilePath;
			
				if (layerFilePath[0] == '.')
					ReplaceString(layerFilePath, ".\\", path);

				filePathList.push_back(layerFilePath);
			}
		}
	}

	file.close();

	return msg;
}

void PrintMessage(ERMsg msg)
{
	for(size_t i=0; i<msg.dimension(); i++)
	{
		_tprintf(convert(msg[(int)i]).c_str());
		_tprintf(_T("\n"));
	}
}


const double GDAL_CELL_LIMIT[7][2] = 
{
	{0, 256},
	{-128, 127},
	{0, 65535},
	{-32768, 32767},
	{0, 4294967295},
	{-2147483648.0, 2147483647.0},
	{-3.4e38, 3.4e38}
};

string GetGDALFilter(bool bMustSupportCreate)
{
	string theFileFiltersString;
	// first get the GDAL driver manager
	//GDALAllRegister();

	GDALDriverManager *myGdalDriverManager = GetGDALDriverManager();

	if (!myGdalDriverManager)
	{
		return theFileFiltersString;
		// XXX good place to throw exception if we
	}	// XXX decide to do exceptions

	// Grind through all the drivers and their respective metadata.
	// We'll add a file filter for those drivers that have a file
	// extension defined for them; the others, welll, even though
	// theoreticaly we can open those files because there exists a
	// driver for them, the user will have to use the "All Files" to
	// open datasets with no explicitly defined file name extension.
	// Note that file name extension strings are of the form
	// "DMD_EXTENSION=.*".  We'll also store the long name of the
	// driver, which will be found in DMD_LONGNAME, which will have the
	// same form.


	for (int i = 0; i < myGdalDriverManager->GetDriverCount(); ++i)
	{

		// then iterate through all of the supported drivers, adding the
		// corresponding file filter
		GDALDriver *myGdalDriver = myGdalDriverManager->GetDriver(i);
		ASSERT(myGdalDriver);

		if (!myGdalDriver )
		{
	//      qWarning("unable to get driver %d", i);
			continue;
		}

		//if bMustSupportCreate flag is set, then skip driver that does'nt support it
		string test = myGdalDriver->GetMetadataItem( GDAL_DCAP_CREATE );
		int bSupportCreate = CSLFetchBoolean( myGdalDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE );
		if( bMustSupportCreate && !bSupportCreate)
			continue;

		// string wrapper of GDAL driver description
		string myGdalDriverDescription = myGdalDriver->GetDescription();
		// long name for the given driver
		string myGdalDriverLongName = myGdalDriver->GetMetadataItem( GDAL_DMD_LONGNAME );
		// file name extension for given driver
		string myGdalDriverExtension = myGdalDriver->GetMetadataItem( GDAL_DMD_EXTENSION );
		
		if( myGdalDriverDescription == "EHdr" )
			myGdalDriverExtension = "bil";

		if (!(myGdalDriverExtension.empty() || myGdalDriverLongName.empty()))
		{
			// if we have both the file name extension and the long name,
			// then we've all the information we need for the current
			// driver; therefore emit a file filter string and move to
			// the next driver
			// XXX add check for SDTS; in that case we want (*CATD.DDF)
			string glob = "*." + myGdalDriverExtension;
			theFileFiltersString += myGdalDriverLongName + " (" + glob + ")|" + glob + "|";
		}

		if (myGdalDriverExtension.empty() && !myGdalDriverLongName.empty())
		{
			// Then what we have here is a driver with no corresponding
			// file extension; e.g., GRASS.  In which case we append the
			// string to the "catch-all" which will match all file types.
			// (I.e., "*.*") We use the driver description intead of the
			// long time to prevent the catch-all line from getting too
			// large.

			// ... OTOH, there are some drivers with missing
			// DMD_EXTENSION; so let's check for them here and handle
			// them appropriately

			// USGS DEMs use "*.dem"
			if (myGdalDriverDescription.substr(0,7) == "USGSDEM" )
			{
				string glob = "*.dem";
				theFileFiltersString += myGdalDriverLongName + " (" + glob + ")|" + glob + "|";
			}
			else if (myGdalDriverDescription.substr(0,4) == "DTED")
			{
				// DTED use "*.dt0"
				string glob = "*.dt0";
				theFileFiltersString += myGdalDriverLongName + " (" + glob + ")|" + glob + "|";
			}
			else if (myGdalDriverDescription.substr(0,5) == "MrSID")
			{
				// MrSID use "*.sid"
				string glob = "*.sid";
				theFileFiltersString += myGdalDriverLongName + " (" + glob + ")|" + glob + "|";
			}
		}

		// A number of drivers support JPEG 2000. Add it in for those.
	/*    if (  myGdalDriverDescription.substr(0,5) == "MrSID"
			  || myGdalDriverDescription.substr(0,3) =="ECW"
			  || myGdalDriverDescription.substr(0,8) =="JPEG2000"
			  || myGdalDriverDescription.substr(0,6) =="JP2KAK" )
		{
		  string glob = "*.jp2 *.j2k";
		  theFileFiltersString += "JPEG 2000 (" + glob + ")|" + glob + "|";
		}
	*/
	}                           // each loaded GDAL driver

	// can't forget the default case
	theFileFiltersString += "All other files (*.*)|*.*||";

	return theFileFiltersString;
}

void GetGDALDriverList(StringVector& GDALList, bool bMustSupportCreate)
{

	//string theFileFiltersString;
	// first get the GDAL driver manager
	//GDALAllRegister();

	GDALDriverManager *myGdalDriverManager = GetGDALDriverManager();

	if (!myGdalDriverManager)
	{
		return;
		// XXX good place to throw exception if we
	}	// XXX decide to do exceptions

	// Grind through all the drivers and their respective metadata.
	// We'll add a file filter for those drivers that have a file
	// extension defined for them; the others, welll, even though
	// theoreticaly we can open those files because there exists a
	// driver for them, the user will have to use the "All Files" to
	// open datasets with no explicitly defined file name extension.
	// Note that file name extension strings are of the form
	// "DMD_EXTENSION=.*".  We'll also store the long name of the
	// driver, which will be found in DMD_LONGNAME, which will have the
	// same form.


	for (int i = 0; i < myGdalDriverManager->GetDriverCount(); ++i)
	{

		// then iterate through all of the supported drivers, adding the
		// corresponding file filter
		GDALDriver *myGdalDriver = myGdalDriverManager->GetDriver(i);
		ASSERT(myGdalDriver);

		if (!myGdalDriver )
		{
	//      qWarning("unable to get driver %d", i);
			continue;
		}

		//if bMustSupportCreate flag is set, then skip driver that does'nt support it
		string test = myGdalDriver->GetMetadataItem( GDAL_DCAP_CREATE );
		int bSupportCreate = CSLFetchBoolean( myGdalDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE );
		if( bMustSupportCreate && !bSupportCreate)
			continue;

		// string wrapper of GDAL driver description
		string myGdalDriverDescription = myGdalDriver->GetDescription();
		// long name for the given driver
		//string myGdalDriverLongName = myGdalDriver->GetMetadataItem( GDAL_DMD_LONGNAME );
		//+ " ("myGdalDriverLongName+")"

		GDALList.push_back(myGdalDriverDescription);
	}
}

GDALDriver* GetDriverFromExtension(const std::string& extIn, bool bMustSupportCreate)
{
	string ext(extIn);
	MakeLower(ext);
	// first get the GDAL driver manager
	//GDALAllRegister();

	GDALDriverManager *myGdalDriverManager = GetGDALDriverManager();
	ASSERT( myGdalDriverManager );

	//if( ext == ".bil" )
		//return (GDALDriver* )GDALGetDriverByName("EHrd");

	for (int i = 0; i < myGdalDriverManager->GetDriverCount(); ++i)
	{
		// then iterate through all of the supported drivers, adding the
		// corresponding file filter
		GDALDriver *myGdalDriver = myGdalDriverManager->GetDriver(i);
		ASSERT(myGdalDriver);

		if (!myGdalDriver )
			continue;

		//if bMustSupportCreate flag is set, then skip driver that does'nt support it
		// string wrapper of GDAL driver description
		string myGdalDriverDescription = myGdalDriver->GetDescription();
		// long name for the given driver
		string myGdalDriverLongName = myGdalDriver->GetMetadataItem( GDAL_DMD_LONGNAME );
		// file name extension for given driver
		string myGdalDriverExtension = myGdalDriver->GetMetadataItem( GDAL_DMD_EXTENSION );

		if( myGdalDriverDescription == "EHdr" )
			myGdalDriverExtension = "bil";

		string test = myGdalDriver->GetMetadataItem( GDAL_DCAP_CREATE );
		int bSupportCreate = CSLFetchBoolean( myGdalDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE );
		if( bMustSupportCreate && !bSupportCreate)
			continue;

		
		if (!(myGdalDriverExtension.empty() || myGdalDriverLongName.empty()))
		{
			// if we have both the file name extension and the long name,
			// then we've all the information we need for the current
			// driver; therefore emit a file filter string and move to
			// the next driver
			// XXX add check for SDTS; in that case we want (*CATD.DDF)
			myGdalDriverExtension.insert(myGdalDriverExtension.begin(), '.');
			MakeLower(myGdalDriverExtension);
			if( ext == myGdalDriverExtension)
				return myGdalDriver;
		}
	}                           // each loaded GDAL driver

	
	return NULL;
}


std::string GetDriverExtension(const std::string& formatName)
{
	string GdalDriverExtension;

	GDALDriver* poDriverOut = (GDALDriver*)GDALGetDriverByName(formatName.c_str());
	if (poDriverOut != NULL)
	{
		const char* pExt = poDriverOut->GetMetadataItem(GDAL_DMD_EXTENSION);
		// file name extension for given driver
		if (pExt!=NULL)
			GdalDriverExtension = string(".") + pExt;
	}

	return GdalDriverExtension;
}

/************************************************************************/
/*                        GDALInfoReportCorner()                        */
/************************************************************************/

static std::string GDALInfoReportCorner(GDALDatasetH hDataset, OGRCoordinateTransformationH hTransform, const char * corner_name, double x, double y)
{
	double	dfGeoX, dfGeoY;
	double	adfGeoTransform[6];
	std::string tmp;
	std::string info;

	tmp = FormatA("%-11s ", corner_name);
	info += tmp;

	/* -------------------------------------------------------------------- */
	/*      Transform the point into georeferenced coordinates.             */
	/* -------------------------------------------------------------------- */
	if (GDALGetGeoTransform(hDataset, adfGeoTransform) == CE_None)
	{
		dfGeoX = adfGeoTransform[0] + adfGeoTransform[1] * x
			+ adfGeoTransform[2] * y;
		dfGeoY = adfGeoTransform[3] + adfGeoTransform[4] * x
			+ adfGeoTransform[5] * y;
	}

	else
	{
		tmp = FormatA("(%7.1f,%7.1f)\n", x, y);
		info += tmp;
		return info;
	}

	/* -------------------------------------------------------------------- */
	/*      Report the georeferenced coordinates.                           */
	/* -------------------------------------------------------------------- */
	if (ABS(dfGeoX) < 181 && ABS(dfGeoY) < 91)
	{
		tmp = FormatA("(%12.7f,%12.7f) ", dfGeoX, dfGeoY);
		info += tmp;
	}
	else
	{
		tmp = FormatA("(%12.3f,%12.3f) ", dfGeoX, dfGeoY);
		info += tmp;
	}

	/* -------------------------------------------------------------------- */
	/*      Transform to latlong and report.                                */
	/* -------------------------------------------------------------------- */
	if (hTransform != NULL
		&& OCTTransform(hTransform, 1, &dfGeoX, &dfGeoY, NULL))
	{

		tmp = FormatA("(%s,", GDALDecToDMS(dfGeoX, "Long", 2));
		info += tmp;
		tmp = FormatA("%s)", GDALDecToDMS(dfGeoY, "Lat", 2));
		info += tmp;
	}

	tmp = FormatA("\n");
	info += tmp;

	return info;
}

ERMsg GetGDALInfo(const std::string& filePath, CNewGeoFileInfo& info)
{
	ERMsg msg;
	std::string tmp;

	//GDALAllRegister();
	info.m_filePath = filePath;

	/* -------------------------------------------------------------------- */
	/*      Open dataset.                                                   */
	/* -------------------------------------------------------------------- */
	GDALDatasetH hDataset = GDALOpenEx(filePath.c_str(), GDAL_OF_READONLY | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);
	//GDALDatasetH hDataset = GDALOpen(filePath.c_str(), GA_ReadOnly);

	if (hDataset == NULL)
	{
		msg.ajoute(CPLGetLastErrorMsg());
		return msg;
	}

	/* -------------------------------------------------------------------- */
	/*      Report general info.                                            */
	/* -------------------------------------------------------------------- */
	GDALDriverH	hDriver = GDALGetDatasetDriver(hDataset);
	tmp = FormatA("Driver: %s/%s\n",GDALGetDriverShortName(hDriver), GDALGetDriverLongName(hDriver));

	info.m_info += tmp;

	char** papszFileList = GDALGetFileList(hDataset);
	if (CSLCount(papszFileList) == 0)
	{
		tmp = FormatA("Files: none associated\n");
		info.m_info += tmp;
	}
	else
	{
		tmp = FormatA("Files: %s\n", papszFileList[0]);
		info.m_info += tmp;
		for (int i = 1; papszFileList[i] != NULL; i++)
		{
			tmp = FormatA("       %s\n", papszFileList[i]);
			info.m_info += tmp;
		}
	}
	CSLDestroy(papszFileList);

	tmp = FormatA("Size is %d, %d\n",GDALGetRasterXSize(hDataset),GDALGetRasterYSize(hDataset));

	info.m_info += tmp;
	/* -------------------------------------------------------------------- */
	/*      Report projection.                                              */
	/* -------------------------------------------------------------------- */
	if (GDALGetProjectionRef(hDataset) != NULL)
	{
		OGRSpatialReferenceH  hSRS;
		char		      *pszProjection;

		pszProjection = (char *)GDALGetProjectionRef(hDataset);

		hSRS = OSRNewSpatialReference(NULL);
		if (OSRImportFromWkt(hSRS, &pszProjection) == CE_None)
		{
			char	*pszPrettyWkt = NULL;

			OSRExportToPrettyWkt(hSRS, &pszPrettyWkt, FALSE);
			tmp = FormatA("Coordinate System is:\n%s\n", pszPrettyWkt);
			info.m_info += tmp;
			CPLFree(pszPrettyWkt);
		}
		else
		{
			tmp = FormatA("Coordinate System is `%s'\n",
				GDALGetProjectionRef(hDataset));
			info.m_info += tmp;
		}

		OSRDestroySpatialReference(hSRS);
	}

	/* -------------------------------------------------------------------- */
	/*      Report Geotransform.                                            */
	/* -------------------------------------------------------------------- */
	double		adfGeoTransform[6];
	if (GDALGetGeoTransform(hDataset, adfGeoTransform) == CE_None)
	{
		if (adfGeoTransform[2] == 0.0 && adfGeoTransform[4] == 0.0)
		{
			tmp = FormatA("Origin = (%.15f,%.15f)\n", adfGeoTransform[0], adfGeoTransform[3]);
			info.m_info += tmp;

			tmp = FormatA("Pixel Size = (%.15f,%.15f)\n", adfGeoTransform[1], adfGeoTransform[5]);
			info.m_info += tmp;
		}
		else
		{
			tmp = FormatA("GeoTransform =\n"
				"  %.16g, %.16g, %.16g\n"
				"  %.16g, %.16g, %.16g\n",
				adfGeoTransform[0],
				adfGeoTransform[1],
				adfGeoTransform[2],
				adfGeoTransform[3],
				adfGeoTransform[4],
				adfGeoTransform[5]);
			info.m_info += tmp;
		}
	}

	/* -------------------------------------------------------------------- */
	/*      Report GCPs.                                                    */
	/* -------------------------------------------------------------------- */
	if (GDALGetGCPCount(hDataset) > 0)
	{
		if (GDALGetGCPProjection(hDataset) != NULL)
		{
			char *pszProjection = (char *)GDALGetGCPProjection(hDataset);
			OGRSpatialReferenceH hSRS = OSRNewSpatialReference(NULL);

			if (OSRImportFromWkt(hSRS, &pszProjection) == CE_None)
			{
				char	*pszPrettyWkt = NULL;

				OSRExportToPrettyWkt(hSRS, &pszPrettyWkt, FALSE);
				printf("GCP Projection = \n%s\n", pszPrettyWkt);
				CPLFree(pszPrettyWkt);
			}
			else
			{
				printf("GCP Projection = %s\n", GDALGetGCPProjection(hDataset));
			}

			OSRDestroySpatialReference(hSRS);
		}

		for (int i = 0; i < GDALGetGCPCount(hDataset); i++)
		{
			const GDAL_GCP	*psGCP;

			psGCP = GDALGetGCPs(hDataset) + i;

			printf("GCP[%3d]: Id=%s, Info=%s\n"
				"          (%.15g,%.15g) -> (%.15g,%.15g,%.15g)\n",
				i, psGCP->pszId, psGCP->pszInfo,
				psGCP->dfGCPPixel, psGCP->dfGCPLine,
				psGCP->dfGCPX, psGCP->dfGCPY, psGCP->dfGCPZ);
		}
	}
	
	/* -------------------------------------------------------------------- */
	/*      Report metadata.                                                */
	/* -------------------------------------------------------------------- */
	/*    papszMetadata = (bShowMetadata) ? GDALGetMetadata( hDataset, NULL ) : NULL;
	if( bShowMetadata && CSLCount(papszMetadata) > 0 )
	{
	printf( "Metadata:\n" );
	for( i = 0; papszMetadata[i] != NULL; i++ )
	{
	printf( "  %s\n", papszMetadata[i] );
	}
	}

	for( iMDD = 0; bShowMetadata && iMDD < CSLCount(papszExtraMDDomains); iMDD++ )
	{
	papszMetadata = GDALGetMetadata( hDataset, papszExtraMDDomains[iMDD] );
	if( CSLCount(papszMetadata) > 0 )
	{
	printf( "Metadata (%s):\n", papszExtraMDDomains[iMDD]);
	for( i = 0; papszMetadata[i] != NULL; i++ )
	{
	printf( "  %s\n", papszMetadata[i] );
	}
	}
	}
	*/
	/* -------------------------------------------------------------------- */
	/*      Report "IMAGE_STRUCTURE" metadata.                              */
	/* -------------------------------------------------------------------- */
	/*    papszMetadata = (bShowMetadata) ? GDALGetMetadata( hDataset, "IMAGE_STRUCTURE" ) : NULL;
	if( bShowMetadata && CSLCount(papszMetadata) > 0 )
	{
	printf( "Image Structure Metadata:\n" );
	for( i = 0; papszMetadata[i] != NULL; i++ )
	{
	printf( "  %s\n", papszMetadata[i] );
	}
	}
	*/
	/* -------------------------------------------------------------------- */
	/*      Report subdatasets.                                             */
	/* -------------------------------------------------------------------- */
	/*    papszMetadata = GDALGetMetadata( hDataset, "SUBDATASETS" );
	if( CSLCount(papszMetadata) > 0 )
	{
	printf( "Subdatasets:\n" );
	for( i = 0; papszMetadata[i] != NULL; i++ )
	{
	printf( "  %s\n", papszMetadata[i] );
	}
	}
	*/
	/* -------------------------------------------------------------------- */
	/*      Report geolocation.                                             */
	/* -------------------------------------------------------------------- */
	/*    papszMetadata = (bShowMetadata) ? GDALGetMetadata( hDataset, "GEOLOCATION" ) : NULL;
	if( bShowMetadata && CSLCount(papszMetadata) > 0 )
	{
	printf( "Geolocation:\n" );
	for( i = 0; papszMetadata[i] != NULL; i++ )
	{
	printf( "  %s\n", papszMetadata[i] );
	}
	}
	*/
	/* -------------------------------------------------------------------- */
	/*      Report RPCs                                                     */
	/* -------------------------------------------------------------------- */
	/*    papszMetadata = (bShowMetadata) ? GDALGetMetadata( hDataset, "RPC" ) : NULL;
	if( bShowMetadata && CSLCount(papszMetadata) > 0 )
	{
	printf( "RPC Metadata:\n" );
	for( i = 0; papszMetadata[i] != NULL; i++ )
	{
	printf( "  %s\n", papszMetadata[i] );
	}
	}
	*/
	/* -------------------------------------------------------------------- */
	/*      Setup projected to lat/long transform if appropriate.           */
	/* -------------------------------------------------------------------- */
	const char  *pszProjection = NULL;
	OGRCoordinateTransformationH hTransform = NULL;

	if (GDALGetGeoTransform(hDataset, adfGeoTransform) == CE_None)
		pszProjection = GDALGetProjectionRef(hDataset);

	if (pszProjection != NULL && strlen(pszProjection) > 0)
	{
		OGRSpatialReferenceH hProj, hLatLong = NULL;


		hProj = OSRNewSpatialReference(pszProjection);
		if (hProj != NULL)
			hLatLong = OSRCloneGeogCS(hProj);

		if (hLatLong != NULL)
		{
			CPLPushErrorHandler(CPLQuietErrorHandler);
			hTransform = OCTNewCoordinateTransformation(hProj, hLatLong);
			CPLPopErrorHandler();

			OSRDestroySpatialReference(hLatLong);
		}

		if (hProj != NULL)
			OSRDestroySpatialReference(hProj);
	}

	/* -------------------------------------------------------------------- */
	/*      Report corners.                                                 */
	/* -------------------------------------------------------------------- */
	tmp = FormatA("Corner Coordinates:\n");
	info.m_info += tmp;
	info.m_info += GDALInfoReportCorner(hDataset, hTransform, "Upper Left", 0.0, 0.0);
	info.m_info += GDALInfoReportCorner(hDataset, hTransform, "Lower Left", 0.0, GDALGetRasterYSize(hDataset));
	info.m_info += GDALInfoReportCorner(hDataset, hTransform, "Upper Right", GDALGetRasterXSize(hDataset), 0.0);
	info.m_info += GDALInfoReportCorner(hDataset, hTransform, "Lower Right", GDALGetRasterXSize(hDataset), GDALGetRasterYSize(hDataset));
	info.m_info += GDALInfoReportCorner(hDataset, hTransform, "Center", GDALGetRasterXSize(hDataset) / 2.0, GDALGetRasterYSize(hDataset) / 2.0);

	if (hTransform != NULL)
	{
		OCTDestroyCoordinateTransformation(hTransform);
		hTransform = NULL;
	}

	/* ==================================================================== */
	/*      Loop over bands.                                                */
	/* ==================================================================== */
	for (int iBand = 0; iBand<GDALGetRasterCount(hDataset); iBand++)
	{
		double      dfMin, dfMax, dfNoData;
		int         bGotMin, bGotMax, bGotNodata, bSuccess;
		int         nBlockXSize, nBlockYSize;
		double      dfMean, dfStdDev;
		//GDALColorTableH	hTable;
		CPLErr      eErr;

		GDALRasterBandH	hBand = GDALGetRasterBand(hDataset, iBand + 1);

		/*       if( bSample )
		{
		float afSample[10000];
		int   nCount;

		nCount = GDALGetRandomRasterSample( hBand, 10000, afSample );
		tmp.Format( "Got %d samples.\n", nCount );
		info.m_info+=tmp;
		}
		*/
		GDALGetBlockSize(hBand, &nBlockXSize, &nBlockYSize);
		tmp = FormatA("Band %d Block=%dx%d Type=%s, ColorInterp=%s\n", iBand + 1,
			nBlockXSize, nBlockYSize,
			GDALGetDataTypeName(
			GDALGetRasterDataType(hBand)),
			GDALGetColorInterpretationName(
			GDALGetRasterColorInterpretation(hBand)));
		info.m_info += tmp;
		if (GDALGetDescription(hBand) != NULL
			&& strlen(GDALGetDescription(hBand)) > 0)
		{
			tmp = FormatA("  Description = %s\n", GDALGetDescription(hBand));
			info.m_info += tmp;
		}

		dfMin = GDALGetRasterMinimum(hBand, &bGotMin);
		dfMax = GDALGetRasterMaximum(hBand, &bGotMax);
		if (bGotMin || bGotMax)
		{
			tmp = FormatA("  ");
			info.m_info += tmp;
			if (bGotMin)
			{
				tmp = FormatA("Min=%.3f ", dfMin);
				info.m_info += tmp;
			}
			if (bGotMax)
			{
				tmp = FormatA("Max=%.3f ", dfMax);
				info.m_info += tmp;
			}

			/*            if( bComputeMinMax )
			{
			CPLErrorReset();
			GDALComputeRasterMinMax( hBand, FALSE, adfCMinMax );
			if (CPLGetLastErrorType() == CE_None)
			{
			printf( "  Computed Min/Max=%.3f,%.3f",
			adfCMinMax[0], adfCMinMax[1] );
			}
			}
			*/
			tmp = FormatA("\n");
			info.m_info += tmp;
		}

		eErr = GDALGetRasterStatistics(hBand, true, false,
			&dfMin, &dfMax, &dfMean, &dfStdDev);
		if (eErr == CE_None)
		{
			tmp = FormatA("  Minimum=%.3f, Maximum=%.3f, Mean=%.3f, StdDev=%.3f\n", dfMin, dfMax, dfMean, dfStdDev);
			info.m_info += tmp;
		}

		/*        if( bReportHistograms )
		{
		int nBucketCount, *panHistogram = NULL;

		eErr = GDALGetDefaultHistogram( hBand, &dfMin, &dfMax,
		&nBucketCount, &panHistogram,
		TRUE, GDALTermProgress, NULL );
		if( eErr == CE_None )
		{
		int iBucket;

		tmp.Format( "  %d buckets from %g to %g:\n  ",
		nBucketCount, dfMin, dfMax );
		for( iBucket = 0; iBucket < nBucketCount; iBucket++ )
		{
		tmp.Format( "%d ", panHistogram[iBucket] );
		}
		printf( "\n" );
		CPLFree( panHistogram );
		}
		}

		if ( bComputeChecksum)
		{
		printf( "  Checksum=%d\n",
		GDALChecksumImage(hBand, 0, 0,
		GDALGetRasterXSize(hDataset),
		GDALGetRasterYSize(hDataset)));
		}
		*/

		dfNoData = GDALGetRasterNoDataValue(hBand, &bGotNodata);
		if (bGotNodata)
		{
			tmp = FormatA("  NoData Value=%.18g\n", dfNoData);
			info.m_info += tmp;

		}

		/*        if( GDALGetOverviewCount(hBand) > 0 )
		{
		int		iOverview;

		printf( "  Overviews: " );
		for( iOverview = 0;
		iOverview < GDALGetOverviewCount(hBand);
		iOverview++ )
		{
		GDALRasterBandH	hOverview;
		const char *pszResampling = NULL;

		if( iOverview != 0 )
		printf( ", " );

		hOverview = GDALGetOverview( hBand, iOverview );
		printf( "%dx%d",
		GDALGetRasterBandXSize( hOverview ),
		GDALGetRasterBandYSize( hOverview ) );

		pszResampling =
		GDALGetMetadataItem( hOverview, "RESAMPLING", "" );

		if( pszResampling != NULL
		&& EQUALN(pszResampling,"AVERAGE_BIT2",12) )
		printf( "*" );
		}
		printf( "\n" );

		if ( bComputeChecksum)
		{
		printf( "  Overviews checksum: " );
		for( iOverview = 0;
		iOverview < GDALGetOverviewCount(hBand);
		iOverview++ )
		{
		GDALRasterBandH	hOverview;

		if( iOverview != 0 )
		printf( ", " );

		hOverview = GDALGetOverview( hBand, iOverview );
		printf( "%d",
		GDALChecksumImage(hOverview, 0, 0,
		GDALGetRasterBandXSize(hOverview),
		GDALGetRasterBandYSize(hOverview)));
		}
		printf( "\n" );
		}
		}

		if( GDALHasArbitraryOverviews( hBand ) )
		{
		printf( "  Overviews: arbitrary\n" );
		}

		nMaskFlags = GDALGetMaskFlags( hBand );
		if( (nMaskFlags & (GMF_NODATA|GMF_ALL_VALID)) == 0 )
		{
		GDALRasterBandH hMaskBand = GDALGetMaskBand(hBand) ;

		printf( "  Mask Flags: " );
		if( nMaskFlags & GMF_PER_DATASET )
		printf( "PER_DATASET " );
		if( nMaskFlags & GMF_ALPHA )
		printf( "ALPHA " );
		if( nMaskFlags & GMF_NODATA )
		printf( "NODATA " );
		if( nMaskFlags & GMF_ALL_VALID )
		printf( "ALL_VALID " );
		printf( "\n" );

		if( hMaskBand != NULL &&
		GDALGetOverviewCount(hMaskBand) > 0 )
		{
		int		iOverview;

		printf( "  Overviews of mask band: " );
		for( iOverview = 0;
		iOverview < GDALGetOverviewCount(hMaskBand);
		iOverview++ )
		{
		GDALRasterBandH	hOverview;

		if( iOverview != 0 )
		printf( ", " );

		hOverview = GDALGetOverview( hMaskBand, iOverview );
		printf( "%dx%d",
		GDALGetRasterBandXSize( hOverview ),
		GDALGetRasterBandYSize( hOverview ) );
		}
		printf( "\n" );
		}
		}
		*/
		if (strlen(GDALGetRasterUnitType(hBand)) > 0)
		{
			tmp = FormatA("  Unit Type: %s\n", GDALGetRasterUnitType(hBand));
			info.m_info += tmp;
		}

		if (GDALGetRasterCategoryNames(hBand) != NULL)
		{
			char **papszCategories = GDALGetRasterCategoryNames(hBand);
			int i;

			tmp = FormatA("  Categories:\n");
			info.m_info += tmp;
			for (i = 0; papszCategories[i] != NULL; i++)
			{
				tmp = FormatA("    %3d: %s\n", i, papszCategories[i]);
				info.m_info += tmp;
			}
		}

		if (GDALGetRasterScale(hBand, &bSuccess) != 1.0
			|| GDALGetRasterOffset(hBand, &bSuccess) != 0.0)
		{
			tmp = FormatA("  Offset: %.15g,   Scale:%.15g\n",
				GDALGetRasterOffset(hBand, &bSuccess),
				GDALGetRasterScale(hBand, &bSuccess));
			info.m_info += tmp;
		}

		/*        papszMetadata = (bShowMetadata) ? GDALGetMetadata( hBand, NULL ) : NULL;
		if( bShowMetadata && CSLCount(papszMetadata) > 0 )
		{
		printf( "  Metadata:\n" );
		for( i = 0; papszMetadata[i] != NULL; i++ )
		{
		printf( "    %s\n", papszMetadata[i] );
		}
		}

		papszMetadata = (bShowMetadata) ? GDALGetMetadata( hBand, "IMAGE_STRUCTURE" ) : NULL;
		if( bShowMetadata && CSLCount(papszMetadata) > 0 )
		{
		printf( "  Image Structure Metadata:\n" );
		for( i = 0; papszMetadata[i] != NULL; i++ )
		{
		printf( "    %s\n", papszMetadata[i] );
		}
		}

		if( GDALGetRasterColorInterpretation(hBand) == GCI_PaletteIndex
		&& (hTable = GDALGetRasterColorTable( hBand )) != NULL )
		{
		int			i;

		printf( "  Color Table (%s with %d entries)\n",
		GDALGetPaletteInterpretationName(
		GDALGetPaletteInterpretation( hTable )),
		GDALGetColorEntryCount( hTable ) );

		if (bShowColorTable)
		{
		for( i = 0; i < GDALGetColorEntryCount( hTable ); i++ )
		{
		GDALColorEntry	sEntry;

		GDALGetColorEntryAsRGB( hTable, i, &sEntry );
		printf( "  %3d: %d,%d,%d,%d\n",
		i,
		sEntry.c1,
		sEntry.c2,
		sEntry.c3,
		sEntry.c4 );
		}
		}
		}

		if( bShowRAT && GDALGetDefaultRAT( hBand ) != NULL )
		{
		GDALRasterAttributeTableH hRAT = GDALGetDefaultRAT( hBand );

		GDALRATDumpReadable( hRAT, NULL );
		}
		*/
	}

	GDALClose(hDataset);

	//    CSLDestroy( papszExtraMDDomains );
	//   CSLDestroy( argv );

	// GDALDumpOpenDatasets( stderr );

	//    GDALDestroyDriverManager();

	//    CPLDumpSharedList( NULL );
	//    CPLCleanupTLS();

	return msg;
}


//*******************************************************************************************

size_t CBaseOptions::m_xxFinal=0;
size_t CBaseOptions::m_xx=0;
size_t CBaseOptions::m_xxx=0;
const char* CBaseOptions::TEMPORAL_REF_NAME[NB_SOURCES] = { "Jday1970", "YYYYMMDD" };
const char* CBaseOptions::TT_TYPE_NAME[NB_TT] = { "OverallYears", "ByYears", "ByMonths", "None" };
const char* CBaseOptions::RGB_NAME[NB_RGB] = { "Natural", "LandWater", "TrueColor" };

const COptionDef CBaseOptions::OPTIONS_DEF[] =
{
	{"--utility_version",0,"",false,"Print GDAL version."},
	{"-co",1, "\"NAME=VALUE\"", true,"Passes a creation option to the output format driver. Multiple -co options may be listed. See format specific documentation for legal creation options for each format."},
	{"-of",1, "format",false,"Select the output format. The default is GeoTIFF (GTiff). Use the short format name."},
	{"-wo",1,"\"NAME=VALUE\"",true,"Set a working options. Multiple -wo options may be listed. UNIFIED_SRC_NODATA=YES/[NO]: By default nodata masking values considered independently for each band. However, sometimes it is desired to treat all bands as nodata if and only if, all bands match the corresponding nodata values. To get this behavior set this option to YES."},
	{"-srcNoData",1,"value",false,"Use this value only if they are missing from input mage."},
	{"-dstNoData",1,"value",false,"Set nodata values for output Image. New files will be initialized to this value and if possible the nodata value will be recorded in the output file."},
	{"-dstNoDataEx",1,"value",false,"Set nodata values for all extra output image (error, debug...). Take -dstNoData by default."},
	{"-ot",1,"Byte/Int16/...",false,"Output bands data type."},
	{"-overwrite",0,"",false,"Overwrite the target dataset if it already exists."},
	{"-q",0,"",false,"Be quiet."},
	{"-te",4,"xmin ymin xmax ymax",false,"Set geo-referenced extents of output file to be created (in target SRS)."},
	{"-tap",0,"",false,"(target aligned pixels) align the coordinates of the extent of the output file to the grid of the input file, such that the aligned extent includes the minimum extent."},
	{"-ts",2,"",false,"set output file resolution (in target geo-referenced units)"},
	{"-tr",2,"",false,"set output file size in pixels and lines. If width or height is set to 0, the other dimension will be guessed from the computed resolution."},
	{"-b",1,"bandNo",true,"Select an input band band for output. Bands are numbered from 1. Multiple -b switches may be used to select a set of input bands to write to the output file, or to reorder bands."},
	{"-mask",1,"name",false,"Specify an input mask. Use all value different than nodata by default."},
	{"-maskValue",1,"value",false,"Select a value in the mask to treat. By default all valid data are treat."},
	{"-multi",0,"",false,"Use multithreaded implementation. Multiple threads will be used to process chunks of image and perform input/output operation simultaneously."},
	{"-CPU",1,"nbCPU",false,"Number of CPUs to used when computation. If the number is negative, then CPU define the number of free CPU. CPUs = AllCPU/BLOCK_CPU by default. Only  used when -multi is define."},
	{"-IOCPU",1,"nbCPU",false,"number of CPUs used to read files. If the number is negative, then CPU define the number of free CPU. Only one thread is assigned by default. Only  used when -multi is define."},
	{"-BLOCK_THREADS",1,"threads",false,"Number of threads used to process blocks. 2 by default. Only used when -multi is define. "},
	{"-wm",1,"size(mb)",false,"Set the amount of memory (in megabytes) that API is allowed to use for caching."},
	{"-ResetJobLog",0,"",false,"Create a log with output information. If the file already exist, the text will be append at the end of the file."},
	{"-JobLog",1,"filePath",false,"Reset the job log file. Must be call for the first job."},
	{"-NoResult",0,"",false,"Output result/image will not be created. For test purpose."},
	{"-BlockSize",2,"XSize YSize",false,"Internal processing block size. Can be used instead of -wm."},
	{"-Overview",1,"{level1,level2,...}",false,"Build overview with this list of integral overview levels to build."},
	{"-stats",0,"",false,"Build statistics inside output images."},
	{"-hist", 0, "", false, "Report histogram information for all bands." },
	{ "-TTF", 1, "type", false, "Temporal type format. Can be \"Jday1970\" or \"YYYYMMDD\". \"YYYYMMDD\" need output in Int32. \"Jday1970\" by default." },
	{ "-SceneSize", 1, "size", false, "Number of images associate per scene. " },
	{ "-TT", 1, "type", false, "The temporal transformation allow user to merge images in different time period segment. The available types are: OverallYears, ByYears, ByMonths and None. None can be use to subset part of the input image. ByYears and ByMonths merge the images by years or by months. NONE by default." },
	{ "-Period", 2, "begin end", false, "Output period image. Format of date must be \"yyyy-mm-dd\". When ByYear is specify, the beginning and ending date is apply for each year in the period [first year, last year]." },
	{ "-RGB", 1, "t", false, "Create RGB virtual layer (.VRT) file fro landsat images. Type can be Natural or LandWater. " },
	{ "-RemoveEmpty", 0, "", false, "Remove empty bands (bands without data) when building VRT. Entire Landsat scene will be remove when one band is empty. " },
	{ "-Rename", 1, "format", false, "Add at the end of output file, the mean image date. See strftime for option. %%F for YYYY-MM-DD. Use %%J for julian day since 1970 and %P for path/row." },
	{ "-iFactor", 1, "f", false, "Multiplicator for indices that need multiplication to output in integer. 1000 by default." },
	{"-?",0,"",false, "Print short usage."},
	{"-??",0,"",false, "Print full usage."},
	{"-???",0,"",false, "Print input/output files formats."},
	{"-help",0,"",false, "Print full usage. Equivalent as -??."},
};

static const int NB_OPTIONS = sizeof(CBaseOptions::OPTIONS_DEF)/sizeof(COptionDef);

int CBaseOptions::GetOptionIndex(const char* name)
{
	int index=-1;
	for(int i=0; i<NB_OPTIONS&&index==-1; i++)
	{
		if( _strcmpi(name, OPTIONS_DEF[i].m_name)==0 )
			index=i;
	}

	return index;
}


const char* CBaseOptions::DEFAULT_OPTIONS[] = {"-of","-ot","-co","-srcnodata","-dstnodata","-dstNoDataEx","-wo","-q","-overwrite","-te","-tap", "-mask","-maskValue","-multi","-CPU","-IOCPU","-BLOCK_THREADS","-BlockSize","-NoResult","-Overview","-stats", "-hist", "-?","-??","-???","-help"};//, "-stats"
static const int NB_DEFAULT_OPTIONS = sizeof(CBaseOptions::DEFAULT_OPTIONS)/sizeof(char*);
CBaseOptions::CBaseOptions(bool bAddDefaultOption)
{
	Reset(); 

	if( bAddDefaultOption )
	{
		for(int i=0; i<NB_DEFAULT_OPTIONS; i++)
		{
			ASSERT( GetOptionIndex(DEFAULT_OPTIONS[i])>= 0);
			//m_optionsDef.push_back(OPTIONS_DEF[GetOptionIndex(DEFAULT_OPTIONS[i])]);
			m_optionsDef[DEFAULT_OPTIONS[i]] = OPTIONS_DEF[GetOptionIndex(DEFAULT_OPTIONS[i])];
		}
	}
}

void CBaseOptions::AddOption(const char* name)
{
	int index = GetOptionIndex(name);
	ASSERT(index>=0);
	AddOption(OPTIONS_DEF[index]);
}

void CBaseOptions::RemoveOption(const char* name)
{
	m_optionsDef.erase(name);
}


void CBaseOptions::AddOption(const COptionDef& optionsDef)
{
	
	if (optionsDef.m_name[0] == '-')
		m_optionsDef[optionsDef.m_name] = optionsDef;
	else 
		m_mendatoryDef.push_back(optionsDef);
}

void CBaseOptions::AddIOFileInfo(const CIOFileInfoDef& fileDef)
{
	m_IOFileInfo.push_back(fileDef);
}


void CBaseOptions::Reset()
{
	m_mendatoryDef.clear();
	m_optionsDef.clear();
	m_createOptions.clear();
	m_workOptions.clear();
	m_format = "GTIFF";
	m_memoryLimit = 0;
	
	m_bUseDefaultNoData = true;
	m_srcNodata=MISSING_NO_DATA;
	m_dstNodata=MISSING_NO_DATA;
	m_dstNodataEx=MISSING_NO_DATA;
	m_outputType = GDT_Unknown;
	m_nbBands = UNKNOWN_POS;
	m_xRes=0;
	m_yRes=0;
	m_bandsToUsed.clear();
	m_maskDataUsed=DataTypeMin;
	m_CPU=0;//select all cpu by default
	m_IOCPU=1;//by default take only one thread for IO
	m_BLOCK_THREADS = 2;

	m_bMulti=false;
	m_bAlpha=false;
    m_bOverwrite=false;
	m_bQuiet=false;
	m_bVersion=false;
	m_bNeedHelp=false;
	m_bNeedUsage=false;
	m_bNeedIOFileInfo=false;
	m_bExtents=false;
	m_bRes=false;
	m_bTap=false;
	m_bResetJobLog=false;
	m_jobLogName.empty();
	m_bCreateImage = true;
	m_bComputeStats=false;
	m_bComputeHistogram = false;
	m_bRemoveEmptyBand = false;
	m_rename.clear();
		
	m_TTF = JDAY1970;
	m_scenesSize = 0; //number of image per scene
	m_TM = CTM::DAILY;
	m_bOpenBandAtCreation = true;
	m_RGBType = NO_RGB;
	m_iFactor = 1000;

	m_filesPath.empty();

	//common option callback status
	m_xxFinal=0;
	m_xx=0;
	m_xxx=0;
	
}

ERMsg CBaseOptions::ParseOptions(const string& str)
{
	StringVector tmp;
	tmp.TokenizeQuoted(str, " \t");
	tmp.insert(tmp.begin(), "InternalParser");

	int argc = int(tmp.size());
	boost::scoped_array<char*> args(new char*[argc]());

	for (int i = 0; i < argc; i++)
	{
		char * pStr = new char[tmp[i].length() + 1]();
		strcpy_s(pStr, tmp[i].size() + 1, tmp[i].c_str());
		args[i] = pStr;
	}

	return ParseOption(argc, args.get());
}

ERMsg CBaseOptions::ParseOptions(int argc, TCHAR* argv[])
{
	boost::scoped_array<char*> args(new char*[argc]());

	for (int i = 0; i < argc; i++)
	{
		string str = WBSF::UTF8(argv[i]);
		char * pStr = new char[str.length()+1]();
		strcpy_s(pStr, str.size() + 1, str.c_str());
		args[i] = pStr;
	}

	return ParseOption(argc, args.get() );
}

ERMsg CBaseOptions::ParseOption(int argc, char* argv[])
{
	ERMsg msg;
    // Must process GDAL_SKIP before GDALAllRegister(), but we can't call 
    // GDALGeneralCmdLineProcessor before it needs the drivers to be registered 
    // for the --format or --formats options 
    for( int i=1; i<argc; i++ )
    {
        if( IsEqual(argv[i],"--config") && i + 2 < argc && IsEqual(argv[i + 1], "GDAL_SKIP") )
        {
            CPLSetConfigOption( argv[i+1], argv[i+2] );
            i += 2;
        }
    }

// -------------------------------------------------------------------- 
//      Register standard GDAL drivers, and process generic GDAL        
//      command options.                                                
// -------------------------------------------------------------------- 
    //GDALAllRegister();
    argc = GDALGeneralCmdLineProcessor( argc, &argv, 0 );
    if( argc < 1 )
        exit( -argc );

	//GDALAddDerivedBandPixelFunc("ImageCalculator", VirtualBandFunction);
// -------------------------------------------------------------------- 
//      Parse arguments.                                                
// -------------------------------------------------------------------- 
    for( int i=1; i<argc; i++ )
    {
		char* optionName = argv[i];
		
		
		if( optionName[0] == '-' ) 
		{
			//option argument
			COptionDefMap::const_iterator it = std::find_if(m_optionsDef.begin(), m_optionsDef.end(), [optionName](COptionDefMap::const_reference od){return _strcmpi(od.second.m_name, optionName) == 0; });
			if( it!=m_optionsDef.end() )
			{
				if(i<argc-it->second.m_nbArgs)
				{
					msg += ProcessOption(i, argc, argv);
				}
				else
				{
					string error = FormatA("ERROR: option %s don't have the good number of paramters, use -? for more help.\n", argv[i]);
					msg.ajoute(error);
				}
			}
			else
			{
				string error = FormatA("ERROR: Invalid option: %s, use -? for more help.\n", argv[i]);
				msg.ajoute(error);
			}
		}
		else
		{
			//mendatory argument
			msg += ProcessOption(i, argc, argv);
		}

	}

	if( m_bVersion )
	{
		string error = FormatA("%s was compiled against GDAL %s and is running against GDAL %s\n",
               argv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
		
		msg.ajoute(error);
		//return msg;
	}

	if( m_bNeedUsage || m_bNeedHelp)
		msg.ajoute( GetUsage() );
	
	if( m_bNeedHelp )
		msg.ajoute( GetHelp() );

	if( m_bNeedIOFileInfo )
		msg.ajoute( GetIOFileInfo() );
	
	int CPU = m_bMulti ? omp_get_num_procs() : 1;
	m_CPU = min(2*CPU, max(1, m_CPU>0?m_CPU:CPU+m_CPU) );
	
	if (!m_bMulti)
	{
		m_BLOCK_THREADS = 1;
		m_IOCPU = 1;
	}

	return msg;
}

ERMsg CBaseOptions::ProcessOption(int& i, int argc, char* argv[])
{
	ERMsg msg;
	
	
	if( IsEqual(argv[i], "--utility_version") )
	{
		m_bVersion=true;
	}
	else if( IsEqual(argv[i],"-co") )
	{
		m_createOptions.push_back( argv[++i] );
	}   
	else if( IsEqual(argv[i],"-of") )
	{
		m_format = argv[++i];
	}
	else if( IsEqual(argv[i],"-wo") )//working option
	{
		m_workOptions.push_back( argv[++i] );
	}   
	else if( IsEqual(argv[i],"-srcnodata") )
	{
		m_srcNodata = LimitToBound( atof(argv[++i]), GDT_Float64, 0, false);
	}
	else if( IsEqual(argv[i],"-dstnodata") )
	{
		m_dstNodata = LimitToBound( atof(argv[++i]), GDT_Float64, 0, false);
	}
	else if( IsEqual(argv[i],"-dstNoDataEx") )
	{
		m_dstNodataEx = LimitToBound( atof(argv[++i]), GDT_Float64, 0, false);
	}
	else if( IsEqual(argv[i],"-dstalpha")  )
	{
		m_bAlpha=true;
	}
	else if( IsEqual(argv[i],"-ot") )
	{
		for( int iType=1; iType<GDT_TypeCount; iType++ )
		{
			ASSERT(GDALGetDataTypeName((GDALDataType)iType) != NULL);
			if (IsEqual(GDALGetDataTypeName((GDALDataType)iType), argv[i + 1]))
			{
				m_outputType = (short) iType;
				break;
			}
		}

		if( m_outputType == GDT_Unknown )
		{
			string error = FormatA( "ERROR: Unknown output pixel type: %s\n", argv[i+1] );
			msg.ajoute(error);
		}
		i++;
	}
	else if( IsEqual(argv[i],"-overwrite") )
	{
		m_bOverwrite = true;
	}
	else if( IsEqual(argv[i],"-q") )
	{
		m_bQuiet = true;
	}
	else if( IsEqual(argv[i],"-te") )
	{
		m_bExtents=true;
		m_extents.m_xMin = CPLAtofM(argv[i+1]);i++;
		m_extents.m_yMin = CPLAtofM(argv[i+1]);i++;
		m_extents.m_xMax = CPLAtofM(argv[i+1]);i++;
		m_extents.m_yMax = CPLAtofM(argv[i+1]);i++;
		m_extents.NormalizeRect();
	}
	else if( IsEqual(argv[i],"-tap") )
	{
		m_bTap = true;
	}
	else if( IsEqual(argv[i],"-ts") )
	{
		//m_bSize=true;
		m_extents.m_xSize = atoi(argv[i + 1]); i++;
		m_extents.m_ySize = atoi(argv[i + 1]); i++;
	}
	else if( IsEqual(argv[i],"-tr") )
	{
		m_bRes=true;
		m_xRes= CPLAtofM(argv[i+1]);i++;
		m_yRes= CPLAtofM(argv[i+1]);i++;
	}
	else if( IsEqual(argv[i],"-BlockSize") )
	{
		m_extents.m_xBlockSize = atoi(argv[i + 1]); i++;
		m_extents.m_yBlockSize = atoi(argv[i + 1]); i++;
	}
	else if( IsEqual(argv[i],"-b") )
	{
		m_bandsToUsed.push_back(atoi(argv[i+1]) ); i++;
	}
	else if( IsEqual(argv[i],"-mask") )
	{
		m_maskName = argv[++i];
	}   
	else if( IsEqual(argv[i],"-maskValue") )
	{
		m_maskDataUsed = (float)atof(argv[++i]);
	}
	else if( IsEqual(argv[i],"-multi") )
	{
		m_bMulti = true;
	}   
	else if( IsEqual(argv[i],"-CPU") )
	{
		m_CPU = min(2*omp_get_num_procs(), atoi(argv[i + 1])); i++;
		if( m_CPU <= 0)
			m_CPU = max(1, omp_get_num_procs() + m_CPU);
	}   
	else if( IsEqual(argv[i],"-IOCPU") )
	{
		m_IOCPU = atoi(argv[i+1]);i++;
		if( m_IOCPU <= 0)
			m_IOCPU = max(1, omp_get_num_procs() + m_IOCPU);
	}   
	else if (IsEqual(argv[i], "-BLOCK_THREADS"))
	{
		m_BLOCK_THREADS = atoi(argv[++i]);
	}
	else if( IsEqual(argv[i],"-wm") )
	{
		m_memoryLimit = CPLAtofM(argv[i+1]) * 1024 * 1024;
		i++;
	}
	else if( IsEqual(argv[i],"-ResetJobLog") )
	{
		m_bResetJobLog = true;
	}   
	else if( IsEqual(argv[i],"-JobLog") )
	{
		m_jobLogName = argv[++i];
	}   
	else if( IsEqual(argv[i],"-Overview") )
	{
		string tmp = argv[++i];
		StringVector levels(tmp, "{,}");

		for (StringVector::const_iterator it = levels.begin(); it != levels.end(); it++)
		{
			if( !it->empty() )
				m_overviewLevels.push_back(ToInt(*it));
		}
	}
	else if( IsEqual(argv[i],"-Stats") )
	{
		m_bComputeStats = true;
	}   
	else if (IsEqual(argv[i], "-Hist"))
	{
		m_bComputeHistogram = true;
	}
	else if (IsEqual(argv[i], "-RemoveEmpty"))
	{
		m_bRemoveEmptyBand = true;
	}
	else if (IsEqual(argv[i], "-Rename"))
	{
		m_rename = argv[++i];
	}
	else if( IsEqual(argv[i],"-NoResult") )
	{
		m_bCreateImage = false;
	}
	else if (IsEqual(argv[i], "-TTF"))
	{
		string str(argv[++i]);

		if (IsEqualNoCase(str, TEMPORAL_REF_NAME[JDAY1970]))
			m_TTF = JDAY1970;
		else if (IsEqualNoCase(str, TEMPORAL_REF_NAME[YYYYMMDD]))
			m_TTF = YYYYMMDD;
		else msg.ajoute("Bad temporal type format. temporal type format must be \"JDay1970\" or \"YYYYMMDD\"");
	}
	else if (IsEqual(argv[i], "-TTF"))
	{
		m_scenesSize = stoi(argv[++i]);
		ASSERT(m_scenesSize >= 1);
	}
	else if (IsEqual(argv[i], "-TT"))
	{
		switch (GetTTType(argv[++i]))
		{
		case TT_OVERALL_YEARS: m_TM = CTM(CTM::ANNUAL, CTM::OVERALL_YEARS); break;
		case TT_BY_YEARS: m_TM = CTM::ANNUAL; break;
		case TT_BY_MONTHS: m_TM = CTM::MONTHLY; break;
		case TT_NONE: m_TM = CTM::DAILY; break;
		default: msg.ajoute("ERROR: Invalid -TT option: valide type are \"OverallYears\", \"ByYears\", \"ByMonths\" or \"None\".");
		}
	}
	else if (IsEqual(argv[i], "-Period") )
	{
		m_period.Begin().FromFormatedString(argv[++i], "", "-", 1);//in 1 base
		m_period.End().FromFormatedString(argv[++i], "", "-", 1);
	}
	else if (IsEqual(argv[i], "-RGB"))
	{
		string str = argv[++i];
		
		m_RGBType = NO_RGB;
		for (size_t i = 0; i < NB_RGB && m_RGBType == NO_RGB; i++)
			if (IsEqualNoCase(str, RGB_NAME[i]))
				m_RGBType = static_cast<TRGBTye>(i);
		
		
		if(m_RGBType == NO_RGB)
			msg.ajoute("Bad RGB type format. RGB type format must be \"Natural\", \"LandWater\" or \"TrueColor\"");
		
	}
	else if (IsEqual(argv[i], "-iFactor"))
	{ 
		m_iFactor = stof(argv[++i]);
	}
	else if( IsEqual(argv[i],"-?") )
	{
		m_bNeedUsage=true;
	}
	else if(IsEqual(argv[i],"-??") || IsEqual(argv[i],"-help") )
	{
		m_bNeedHelp=true;
	}
	else if( IsEqual(argv[i],"-???") )//|| IsEqual(argv[i],"-IOFileInfo") 
	{
		m_bNeedIOFileInfo=true;
	}
	else if( argv[i][0] == '-' )
	{
		string error = FormatA("ERROR: Invalid option: %s, use -? or -help for more usage/help.\n", argv[i]);
		msg.ajoute(error);
	}
	else 
	{
		m_filesPath.push_back( argv[i] );
	}

	return msg;
}

string CBaseOptions::GetUsage()const
{
	string usage = m_appDescription + "\n\n";
	usage += "Usage: \n  [--help-general] [--formats]\n";

	string cmdLine(" "); 
	for( COptionDefMap::const_iterator it=m_optionsDef.begin(); it!=m_optionsDef.end(); it++)
	{
		string strOption = FormatA("[%s%s%s]%s", it->second.m_name, it->second.m_nbArgs>0 ? " " : "", it->second.m_args, it->second.m_bMulti ? "*" : "");
		if( cmdLine.length() + strOption.length() + 1 < 65)
		{
			cmdLine+=" "+strOption;
		}
		else 
		{
			usage += cmdLine + "\n";
			cmdLine="  "+strOption;//begin a new line
		}
	}

	usage += cmdLine + "\n";
	cmdLine=" ";//begin a new line
	for( COptionDefVector::const_iterator it=m_mendatoryDef.begin(); it!=m_mendatoryDef.end(); it++)
	{
		string strOption = FormatA("%s%s", it->m_name, it->m_bMulti ? "*" : "");

		if( cmdLine.length() + strOption.length() + 1 < 65)
		{
			cmdLine+=" "+strOption;
		}
		else 
		{
			usage += cmdLine + "\n";
			cmdLine="  "+strOption;//begin a new line
		}
	}

	usage += cmdLine + "\n";

	return usage;
}

string CBaseOptions::GetHelp()const
{
	string help;
	
	for( COptionDefMap::const_iterator it=m_optionsDef.begin(); it!=m_optionsDef.end(); it++)
		help += it->second.GetHelp() + "\n";

	for( COptionDefVector::const_iterator it=m_mendatoryDef.begin(); it!=m_mendatoryDef.end(); it++)
		help += it->GetHelp() + "\n";

	return help;
}

string CBaseOptions::GetIOFileInfo()const
{
	string info;
	

	for( CIOFileInfoDefVector::const_iterator it=m_IOFileInfo.begin(); it!=m_IOFileInfo.end(); it++)
		info+=it->GetHelp() + "\n\n";

	

	return info;
}

void CBaseOptions::UpdateBar()
{
	if (!m_bQuiet)
	{
#pragma omp critical(PrintF)
		{
#pragma omp flush(m_xx)
#pragma omp flush(m_xxx) 

			size_t nbX = (size_t)Round(80.0*m_xx / m_xxFinal);

			while (m_xxx<nbX)
			{
				cout << ".";
				m_xxx++;
			}

#pragma omp flush(m_xxx)
		}
	}
}

void CBaseOptions::PrintTime()
{
	if( !m_bQuiet )
	{
		cout << "\n";
		cout << "Time to read images = " << SecondToDHMS(m_timerRead.Elapsed()) << endl;
		cout << "Time to process = " << SecondToDHMS(m_timerProcess.Elapsed()) << endl;
		cout << "Time to write image = " << SecondToDHMS(m_timerWrite.Elapsed()) << endl;
	}
}

int CBaseOptions::GetTRefIndex(int type, CTRef Tref)
{
	static CTRef BASE_1970(1970, JANUARY, FIRST_DAY);
	

	int date = 0;
	Tref.Transform(CTM(CTM::DAILY));

	if (type == JDAY1970)
	{
		date = Tref.GetRef() - BASE_1970.GetRef() + 1;
	}
	else if (type == YYYYMMDD)
	{
		date = int(Tref.GetYear() * 10000 + (Tref.GetMonth() + 1) * 100 + (Tref.GetDay() + 1));
	}

	return date;
}


CTRef CBaseOptions::GetTRef(int type, int index)
{
	static CTRef BASE_1970(1970, JANUARY, FIRST_DAY);

	CTRef TRef;

	if (type == JDAY1970)
	{
		if (index>=0)
			TRef = BASE_1970 + index - 1;
	}
	else if (type == YYYYMMDD)
	{
		int year = index / 10000;
		size_t m = size_t((index - year * 10000) / 100);
		size_t d = size_t(index - year * 10000 - m * 100);
		TRef = CTRef(year, m-1, d-1);
	}
		

	return TRef;
}

short CBaseOptions::GetTTType(const char* str)
{
	short type = TT_UNKNOWN;

	string tmp(str);
	for (int i = 0; i<NB_TT; i++)
	{
		if (IsEqualNoCase(tmp, TT_TYPE_NAME[i]))
		{
			type = i;
			break;
		}
	}

	return type;
}



CTPeriod CBaseOptions::GetTTPeriod()const
{
	CTPeriod period = m_period;
	period.Transform(m_TM);

	return period;
}


CTPeriod CBaseOptions::GetTTSegment(size_t ss)
{
	CTPeriod p = m_period;
	if (m_TM.Mode() == CTM::FOR_EACH_YEAR)
	{
		if (m_TM.Type() == CTM::DAILY)
		{
			p = m_period[ss];
		}
		else if (m_TM.Type() == CTM::MONTHLY)
		{
			CTRef TRef = p.Begin();
			TRef.Transform(CTM(CTM::MONTHLY));
			TRef += int(ss);
			TRef.Transform(CTM(CTM::DAILY));

			p.Begin() = TRef;
			p.End() = TRef + p.Begin().GetNbDayPerMonth();
		}
		else if (m_TM.Type() == CTM::ANNUAL)
		{
			int year = p.Begin().GetYear();
			p.Begin() = CTRef(year + int(ss), FIRST_MONTH, FIRST_DAY);
			p.End() = CTRef(year + int(ss), LAST_MONTH, LAST_DAY);
		}
	}


	return p;
}

}