/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Utilities
 * Purpose:  Commandline application to list info about a given CRS.
 *           Outputs a number of formats (WKT, PROJ.4, etc.).
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *           Etienne Tourigny, etourigny.dev-at-gmail-dot-com       
 *
 * ****************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/
#include "stdafx.h"

#pragma warning(disable: 4275 4251 4996)
#include "gdal_priv.h"
#include "cpl_string.h"
#include "ogr_spatialref.h"
#include "ogr_api.h"
#include "ogrsf_frmts.h"

CPL_CVSID("$Id$");

namespace WBSF
{

	int FindSRS(const char *pszInput, OGRSpatialReference &oSRS, int bDebug);
	//CPLErr PrintSRS( const OGRSpatialReference &oSRS, 
	//                 const char * pszOutputType, 
	//                 int bPretty, int bPrintSep );
	//void PrintSRSOutputTypes( const OGRSpatialReference &oSRS, 
	//                          const char ** papszOutputTypes );
	int FindEPSG(const OGRSpatialReference &oSRS);
	int SearchCSVForWKT(const char *pszFileCSV, const char *pszTarget);

	/************************************************************************/
	/*                      FindSRS()                                       */
	/*                                                                      */
	/*      Search for SRS from pszInput, update oSRS.                      */
	/************************************************************************/
	int FindSRS(const char *pszInput, OGRSpatialReference &oSRS, int bDebug)

	{
		int            bGotSRS = FALSE;
		VSILFILE      *fp = NULL;
		GDALDataset	  *poGDALDS = NULL;
		OGRDataSource *poOGRDS = NULL;
		OGRLayer      *poLayer = NULL;
		char           *pszProjection = NULL;
		CPLErrorHandler oErrorHandler = NULL;
		int bIsFile = FALSE;
		OGRErr eErr = CE_None;

		/* temporarily supress error messages we may get from xOpen() */
		if (!bDebug)
			oErrorHandler = CPLSetErrorHandler(CPLQuietErrorHandler);

		/* Test if argument is a file */
		fp = VSIFOpenL(pszInput, "r");
		if (fp)  {
			bIsFile = TRUE;
			VSIFCloseL(fp);
			CPLDebug("gdalsrsinfo", "argument is a file");
		}

		/* try to open with GDAL */
		CPLDebug("gdalsrsinfo", "trying to open with GDAL");
		poGDALDS = (GDALDataset *)GDALOpen(pszInput, GA_ReadOnly);
		if (poGDALDS != NULL && poGDALDS->GetProjectionRef() != NULL) {
			pszProjection = (char *)poGDALDS->GetProjectionRef();
			if (oSRS.importFromWkt(&pszProjection) == CE_None) {
				CPLDebug("gdalsrsinfo", "got SRS from GDAL");
				bGotSRS = TRUE;
				oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
			}
			GDALClose((GDALDatasetH)poGDALDS);
			if (!bGotSRS)
				CPLDebug("gdalsrsinfo", "did not open with GDAL");
		}

#ifdef OGR_ENABLED
		/* if unsuccessful, try to open with OGR */
		if ( ! bGotSRS ) {
			CPLDebug( "gdalsrsinfo", "trying to open with OGR" );
			poOGRDS = OGRSFDriverRegistrar::Open( pszInput, FALSE, NULL );
			if( poOGRDS != NULL ) {
				poLayer = poOGRDS->GetLayer( 0 );
				if ( poLayer != NULL ) {
					OGRSpatialReference *poSRS = poLayer->GetSpatialRef( );
					if ( poSRS != NULL ) {
						CPLDebug( "gdalsrsinfo", "got SRS from OGR" );
						bGotSRS = TRUE;
						OGRSpatialReference* poSRSClone = poSRS->Clone();
						oSRS = *poSRSClone;
						OGRSpatialReference::DestroySpatialReference( poSRSClone );
					}
				}
				OGRDataSource::DestroyDataSource( poOGRDS );
				poOGRDS = NULL;
			} 
			if ( ! bGotSRS ) 
				CPLDebug( "gdalsrsinfo", "did not open with OGR" );
		}
#endif // OGR_ENABLED

		/* Try ESRI file */
		if (!bGotSRS && bIsFile && (strstr(pszInput, ".prj") != NULL)) {
			CPLDebug("gdalsrsinfo",
				"trying to get SRS from ESRI .prj file [%s]", pszInput);

			char **pszTemp;
			if (strstr(pszInput, "ESRI::") != NULL)
				pszTemp = CSLLoad(pszInput + 6);
			else
				pszTemp = CSLLoad(pszInput);

			if (pszTemp) {
				eErr = oSRS.importFromESRI(pszTemp);
				CSLDestroy(pszTemp);
			}
			else
				eErr = OGRERR_UNSUPPORTED_SRS;

			if (eErr != OGRERR_NONE) {
				CPLDebug("gdalsrsinfo", "did not get SRS from ESRI .prj file");
			}
			else {
				CPLDebug("gdalsrsinfo", "got SRS from ESRI .prj file");
				bGotSRS = TRUE;
				oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
			}
		}

		/* Last resort, try OSRSetFromUserInput() */
		if (!bGotSRS) {
			CPLDebug("gdalsrsinfo",
				"trying to get SRS from user input [%s]", pszInput);

			eErr = oSRS.SetFromUserInput(pszInput);

			if (eErr != OGRERR_NONE) {
				CPLDebug("gdalsrsinfo", "did not get SRS from user input");
			}
			else {
				CPLDebug("gdalsrsinfo", "got SRS from user input");
				bGotSRS = TRUE;
				oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
			}
		}

		/* restore error messages */
		if (!bDebug)
			CPLSetErrorHandler(oErrorHandler);


		return bGotSRS;
	}


	/************************************************************************/
	/*                      PrintSRS()                                      */
	/*                                                                      */
	/*      Print spatial reference in specified format.                    */
	/************************************************************************/
	/*CPLErr PrintSRS( const OGRSpatialReference &oSRS,
					 const char * pszOutputType,
					 int bPretty, int bPrintSep )

					 {
					 if ( ! pszOutputType || EQUAL(pszOutputType,""))
					 return CE_None;

					 CPLDebug( "gdalsrsinfo", "PrintSRS( oSRS, %s, %d, %d )\n",
					 pszOutputType, bPretty, bPrintSep );

					 char *pszOutput = NULL;

					 if ( EQUAL("proj4", pszOutputType ) ) {
					 if ( bPrintSep ) printf( "PROJ.4 : ");
					 oSRS.exportToProj4( &pszOutput );
					 printf( "\'%s\'\n", pszOutput );
					 }

					 else if ( EQUAL("wkt", pszOutputType ) ) {
					 if ( bPrintSep ) printf("OGC WKT :\n");
					 if ( bPretty )
					 oSRS.exportToPrettyWkt( &pszOutput, FALSE );
					 else
					 oSRS.exportToWkt( &pszOutput );
					 printf("%s\n",pszOutput);
					 }

					 else if (  EQUAL("wkt_simple", pszOutputType ) ) {
					 if ( bPrintSep ) printf("OGC WKT (simple) :\n");
					 oSRS.exportToPrettyWkt( &pszOutput, TRUE );
					 printf("%s\n",pszOutput);
					 }

					 else if ( EQUAL("wkt_noct", pszOutputType ) ) {
					 if (  bPrintSep ) printf("OGC WKT (no CT) :\n");
					 OGRSpatialReference *poSRS = oSRS.Clone();
					 poSRS->StripCTParms( );
					 if ( bPretty )
					 poSRS->exportToPrettyWkt( &pszOutput, FALSE );
					 else
					 poSRS->exportToWkt( &pszOutput );
					 OGRSpatialReference::DestroySpatialReference( poSRS );
					 printf("%s\n",pszOutput);
					 }

					 else if ( EQUAL("wkt_esri", pszOutputType ) ) {
					 if ( bPrintSep ) printf("ESRI WKT :\n");
					 OGRSpatialReference *poSRS = oSRS.Clone();
					 poSRS->morphToESRI( );
					 if ( bPretty )
					 poSRS->exportToPrettyWkt( &pszOutput, FALSE );
					 else
					 poSRS->exportToWkt( &pszOutput );
					 OGRSpatialReference::DestroySpatialReference( poSRS );
					 printf("%s\n",pszOutput);
					 }

					 else if ( EQUAL("mapinfo", pszOutputType ) ) {
					 if ( bPrintSep ) printf("MAPINFO : ");
					 oSRS.exportToMICoordSys( &pszOutput );
					 printf("\'%s\'\n",pszOutput);
					 }

					 else if ( EQUAL("xml", pszOutputType ) ) {
					 if ( bPrintSep ) printf("XML :\n");
					 oSRS.exportToXML( &pszOutput, NULL );
					 printf("%s\n",pszOutput);
					 }

					 else {
					 CPLError( CE_Failure, CPLE_AppDefined,
					 "ERROR - %s output not supported",
					 pszOutputType );
					 return CE_Failure;
					 }

					 CPLFree( pszOutput );

					 return CE_None;
					 }
					 */
	/************************************************************************/
	/*                      PrintSRSOutputTypes()                           */
	/*                                                                      */
	/*      Print spatial reference in specified formats.                   */
	/************************************************************************/
	//void PrintSRSOutputTypes( const OGRSpatialReference &oSRS, 
	//                          const char ** papszOutputTypes )
	//    
	//{
	//    int nOutputTypes = CSLCount((char**)papszOutputTypes);
	//    printf( "\n" );
	//    for ( int i=0; i<nOutputTypes; i++ ) {
	//        PrintSRS( oSRS, papszOutputTypes[i], TRUE, TRUE );
	//        printf( "\n" );        
	//    }
	//}

	/************************************************************************/
	/*                      SearchCSVForWKT()                               */
	/*                                                                      */
	/*      Search CSV file for target WKT, return EPSG code (or -1).       */
	/*      For saving space, the file can be compressed (gz)               */
	/*      If CSV file is absent are absent the function silently exits    */
	/************************************************************************/
	int SearchCSVForWKT(const char *pszFileCSV, const char *pszTarget)
	{
		const char *pszFilename = NULL;
		const char *pszWKT = NULL;
		char szTemp[1024];
		int nPos = 0;
		const char *pszTemp = NULL;

		VSILFILE *fp = NULL;
		OGRSpatialReference oSRS;
		int nCode = 0;
		int nFound = -1;

		CPLDebug("gdalsrsinfo",
			"SearchCSVForWKT()\nfile=%s\nWKT=%s\n",
			pszFileCSV, pszTarget);

		/* -------------------------------------------------------------------- */
		/*      Find and open file.                                             */
		/* -------------------------------------------------------------------- */
		// pszFilename = pszFileCSV;
		pszFilename = CPLFindFile("gdal", pszFileCSV);
		if (pszFilename == NULL)
		{
			CPLDebug("gdalsrsinfo", "could not find support file %s",
				pszFileCSV);
			// return OGRERR_UNSUPPORTED_SRS;
			return -1;
		}

		/* support gzipped file */
		if (strstr(pszFileCSV, ".gz") != NULL)
			sprintf(szTemp, "/vsigzip/%s", pszFilename);
		else
			sprintf(szTemp, "%s", pszFilename);

		CPLDebug("gdalsrsinfo", "SearchCSVForWKT() using file %s",
			szTemp);

		fp = VSIFOpenL(szTemp, "r");
		if (fp == NULL)
		{
			CPLDebug("gdalsrsinfo", "could not open support file %s",
				pszFilename);

			// return OGRERR_UNSUPPORTED_SRS;
			return -1;
		}

		/* -------------------------------------------------------------------- */
		/*      Process lines.                                                  */
		/* -------------------------------------------------------------------- */
		const char *pszLine;

		while ((pszLine = CPLReadLine2L(fp, -1, NULL)) != NULL)

		{
			// CPLDebug( "gdalsrsinfo", "read line %s", pszLine );

			if (pszLine[0] == '#')
				continue;
			/* do nothing */;

			// else if( EQUALN(pszLine,"include ",8) )
			// {
			//     eErr = importFromDict( pszLine + 8, pszCode );
			//     if( eErr != OGRERR_UNSUPPORTED_SRS )
			//         break;
			// }

			// else if( strstr(pszLine,",") == NULL )
			//     /* do nothing */;

			pszTemp = strstr(pszLine, ",");
			if (pszTemp)
			{
				nPos = int(pszTemp - pszLine);

				if (nPos == 0)
					continue;

				strncpy(szTemp, pszLine, nPos);
				szTemp[nPos] = '\0';
				nCode = atoi(szTemp);

				pszWKT = (char *)pszLine + nPos + 1;

				// CPLDebug( "gdalsrsinfo", 
				//           "code=%d\nWKT=\n[%s]\ntarget=\n[%s]\n",
				//           nCode,pszWKT, pszTarget );

				if (EQUAL(pszTarget, pszWKT))
				{
					nFound = nCode;
					CPLDebug("gdalsrsinfo", "found EPSG:%d\n"
						"current=%s\ntarget= %s\n",
						nCode, pszWKT, pszTarget);
					break;
				}
			}
		}

		VSIFCloseL(fp);

		return nFound;

	}

	/* TODO
	   - search for well-known values (AutoIdentifyEPSG())

	   - should we search .override.csv files?

	   - fix precision differences (namely in degree: 17 vs 15) so we can use epsg_ogc_simple
	   target:
	   orig: GEOGCS["SAD69",DATUM["D_South_American_1969",SPHEROID["GRS_1967_Modified",6378160,298.25]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]
	   ESRI: GEOGCS["SAD69",DATUM["D_South_American_1969",SPHEROID["GRS_1967_Truncated",6378160,298.25]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]
	   OGC:  GEOGCS["SAD69",DATUM["South_American_Datum_1969",SPHEROID["GRS_1967_Modified",6378160,298.25]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]
	   database:
	   ESRI: GEOGCS["SAD69",DATUM["D_South_American_1969",SPHEROID["GRS_1967_Truncated",6378160,298.25]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]
	   OGC:  GEOGCS["SAD69",DATUM["South_American_Datum_1969",SPHEROID["GRS 1967 Modified",6378160,298.25]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]]
	   */

	/************************************************************************/
	/*                      FindEPSG()                                      */
	/*                                                                      */
	/*      Return EPSG code corresponding to spatial reference (or -1)     */
	/************************************************************************/
	int FindEPSG(const OGRSpatialReference &oSRS)
	{
		char *pszWKT = NULL;
		char *pszESRI = NULL;
		int nFound = -1;
		OGRSpatialReference *poSRS = NULL;

		poSRS = oSRS.Clone();
		//>StripCTParms();
		poSRS->StripTOWGS84IfKnownDatumAndAllowed();
		poSRS->exportToWkt(&pszWKT);
		OGRSpatialReference::DestroySpatialReference(poSRS);

		poSRS = oSRS.Clone();
		poSRS->morphToESRI();
		poSRS->exportToWkt(&pszESRI);
		OGRSpatialReference::DestroySpatialReference(poSRS);

		CPLDebug("gdalsrsinfo", "FindEPSG()\nWKT (OGC)= %s\nWKT (ESRI)=%s",
			pszWKT, pszESRI);

		/* search for EPSG code in epsg_*.wkt.gz files */
		/* using ESRI WKT for now, as it seems to work best */
		nFound = SearchCSVForWKT("epsg_esri.wkt.gz", pszESRI);
		if (nFound == -1)
			nFound = SearchCSVForWKT("epsg_ogc_simple.wkt.gz", pszESRI);
		if (nFound == -1)
			nFound = SearchCSVForWKT("epsg_ogc.wkt.gz", pszESRI);


		CPLFree(pszWKT);
		CPLFree(pszESRI);

		return nFound;
	}
}
