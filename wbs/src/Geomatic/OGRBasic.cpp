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
#include <float.h>
#include <limits.h>
#pragma warning(disable: 4275 4251)
#include "ogrsf_frmts.h"
#include "ogr_spatialref.h"
#include "ogr_p.h"


#include "Basic/Statistic.h"
#include "Basic/UtilStd.h"
#include "Basic/UtilMath.h"
#include "Geomatic/OGRBasic.h"

//#include "StdFile.h"

using namespace std;

namespace WBSF
{
	ERMsg COGRDataSource::Open(const char * pszName, int bUpdate)
	{
		ERMsg msg;

		m_poDS = OGRSFDriverRegistrar::Open(pszName, bUpdate);
		if (m_poDS == NULL)
		{
			const char* pError = CPLGetLastErrorMsg();
			if (pError && *pError)
				msg.ajoute(pError);
			else msg.ajoute("File doesn't exist");

			return msg;
		}

		return msg;
	}

	void COGRDataSource::Close()
	{
		if (m_poDS)
		{
			OGRDataSource::DestroyDataSource(m_poDS);
			m_poDS = NULL;
		}

	}

	bool COGRDataSource::FileExist(const char * filePathOut)
	{
		bool bFileExist = false;
		COGRDataSource tmp;
		if (tmp.Open(filePathOut))
		{
			tmp.Close();
			bFileExist = true;
		}

		return bFileExist;

	}



	//*******************************************************************************************
	COGRBaseOption::COGRBaseOption(UINT maskOption)
	{
		m_maskOption = maskOption;
		Reset();
	}

	void COGRBaseOption::Reset()
	{
		m_createOptions.clear();
		m_workOptions.clear();
		m_format = "ESRI Shapefile";

		m_bMulti = false;
		m_bOverwrite = false;
		m_bQuiet = false;
		m_bVersion = false;
		m_bNeedHelp = false;


		m_filesPath.clear();

	}

	ERMsg COGRBaseOption::ParseOption(int argc, char* argv[])
	{
		ERMsg msg;
		// Must process GDAL_SKIP before GDALAllRegister(), but we can't call 
		// GDALGeneralCmdLineProcessor before it needs the drivers to be registered 
		// for the --format or --formats options 
		for (int i = 1; i < argc; i++)
		{
			if (IsEqual(argv[i], "--config") && i + 2 < argc && IsEqual(argv[i + 1], "GDAL_SKIP"))
			{
				CPLSetConfigOption(argv[i + 1], argv[i + 2]);

				i += 2;
			}
		}

		// -------------------------------------------------------------------- 
		//      Register standard GDAL drivers, and process generic GDAL        
		//      command options.                                                
		// -------------------------------------------------------------------- 
		OGRRegisterAll();
		argc = OGRGeneralCmdLineProcessor(argc, &argv, 0);
		if (argc < 1)
			exit(-argc);

		// -------------------------------------------------------------------- 
		//      Parse arguments.                                                
		// -------------------------------------------------------------------- 
		for (int i = 1; i < argc; i++)
		{
			msg += ProcessOption(i, argc, argv);
		}

		if (m_bVersion)
		{
			string error = Format("%s was compiled against GDAL %s and is running against GDAL %s\n",
				argv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));

			msg.ajoute(error);
			//return msg;
		}

		if (m_bNeedHelp)
		{
			msg.ajoute(GetUsage());
			msg.ajoute(GetHelp());
			//return msg;
		}


		return msg;
	}


	ERMsg COGRBaseOption::ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;
		string error;


		if (IsUsed(BASE_OPTIONS) && IsEqual(argv[i], "--utility_version"))
		{
			m_bVersion = true;
		}
		else if (IsUsed(BASE_OPTIONS) && IsEqual(argv[i], "-co") && i < argc - 1)
		{
			m_createOptions.push_back(argv[++i]);
		}
		else if (IsUsed(BASE_OPTIONS) && IsEqual(argv[i], "-of") && i < argc - 1)
		{
			m_format = argv[++i];
		}
		else if (IsUsed(BASE_OPTIONS) && IsEqual(argv[i], "-wo") && i < argc - 1)//working option
		{
			//UNIFIED_SRC_NODATA
			m_workOptions.push_back(argv[++i]);
		}
		else if (IsUsed(BASE_OPTIONS) && IsEqual(argv[i], "-multi"))
		{
			m_bMulti = true;
		}
		else if (IsUsed(BASE_OPTIONS) && IsEqual(argv[i], "-f") && i < argc - 1)
		{
			m_format = argv[i + 1];
		}
		else if (IsUsed(BASE_OPTIONS) && IsEqual(argv[i], "-overwrite"))
		{
			m_bOverwrite = true;
		}
		else if (IsUsed(BASE_OPTIONS) && IsEqual(argv[i], "-q"))
		{
			m_bQuiet = true;
		}
		else if (IsEqual(argv[i], "-?") || IsEqual(argv[i], "-help"))
		{
			m_bNeedHelp = true;
		}
		else if (argv[i][0] == '-')
		{
			error = string("ERROR: Invalid option: ") + argv[i] + ", use - ? for more help.\n";
			msg.ajoute(error);
			//return false;
		}
		else
		{
			m_filesPath.push_back(argv[i]);
		}

		return msg;
	}

	string COGRBaseOption::GetUsage()const
	{

		string usage = "Usage: \n";
		if (IsUsed(BASE_OPTIONS))
		{
			usage +=
				"  [--help-general] [--formats]\n"
				"  [-of format] [-ot Byte/Int16/...] [-co \"NAME=VALUE\"]*\n"
				"  [-srcnodata value] [-dstnodata value] [-wo \"NAME=VALUE\"]*\n"
				"  [-q] [-multi] [-overwrite]\n";
		}

		if (IsUsed(OP_SEPARATE))
			usage += "  [-separate]\n";

		if (IsUsed(OP_EXTENTS))
		{
			usage +=
				"  [-te xmin ymin xmax ymax] -tap\n";

		}

		if (IsUsed(OP_SIZE))
		{
			usage +=
				"  [-ts xres yres] [-ts width height]\n";

		}

		if (IsUsed(OP_BANDS))
		{
			usage +=
				"  [-b band]\n";

		}

		//usage += "  srcfile*\n";


		return usage;
	}

	string COGRBaseOption::GetHelp()const
	{
		string help;
		if (IsUsed(BASE_OPTIONS))
		{
			help +=
				"  -of format: Select the output format. The default is GeoTIFF (GTiff). Use the\n"
				"      short format name.\n"
				"  -ot type: Output bands data type.\n"
				"  -co \"NAME=VALUE\": passes a creation option to the output format driver.\n"
				"      Multiple -co options may be listed. See format specific documentation\n"
				"      for legal creation options for each format.\n"
				//"  -srcnodata value [value...]: Set nodata masking values for input bands\n"
				//"      (different values can be supplied for each band). If more than one value\n"
				//"      is supplied all values should be quoted to keep them together as a single\n"
				//"      operating system argument. Masked values will not be used in\n"
				//"      interpolation. Use a value of None to ignore intrinsic nodata settings\n"
				//"      on the source dataset.\n"
				//"  -dstnodata value [value...]: Set nodata values for output bands (different\n"
				//"      values can be supplied for each band). If more than one value is\n"
				//"      supplied all values should be quoted to keep them together as a single\n"
				//"      operating system argument. New files will be initialized to this value\n"
				//"      and if possible the nodata value will be recorded in the output file.\n"
				"  -srcnodata value: Use this value only if they are missing from input bands.\n"
				"  -dstnodata value: Set nodata values for output bands.  New files will be\n"
				"      initialized to this value and if possible the nodata value will be\n"
				"      recorded in the output file.\n"
				"  -wo \"NAME=VALUE\": Set a working options. Multiple -wo options may be listed.\n"
				"      UNIFIED_SRC_NODATA=YES/[NO]: By default nodata masking values considered\n"
				"      independently for each band. However, sometimes it is desired to treat\n"
				"      all bands as nodata if and only if, all bands match the corresponding\n"
				"      nodata values. To get this behavior set this option to YES.\n"
				//		"  -dstalpha: Create an output alpha band to identify nodata (unset/\n"
				//		"      transparent) pixels.\n"
				//"  -wm memory_in_mb: Set the amount of memory (in megabytes) that API is allowed\n"
				//"      to use for caching.\n"
				"  -q: Be quiet.\n"
				"  -multi: Use multithreaded implementation. Multiple threads will be used to\n"
				"      process chunks of image and perform input/output operation\n"
				"      simultaneously.\n"
				"  -overwrite: Overwrite the target dataset if it already exists.\n";
		}
		if (IsUsed(OP_SEPARATE))
		{
			help += "  -separate: When input image is VRT, the file will be open as separate image.\n";
		}

		if (IsUsed(OP_EXTENTS))
		{
			help +=
				"  -te xmin ymin xmax ymax: set georeferenced extents of output file to be\n"
				"      created (in target SRS).\n"
				"  -tap: (target aligned pixels) align the coordinates of the extent of the\n"
				"      output file to the grid of the imput file, such that the aligned extent\n"
				"      includes the minimum extent.\n";
			//"  -ts width height:"
			//"      set output file size in pixels and lines. If width or height is set to 0, the other dimension will be guessed from the computed resolution. Note that -ts cannot be used with -tr"
		}

		if (IsUsed(OP_SIZE))
		{
			help +=
				"  -tr xres yres: set output file resolution (in target georeferenced units)\n"
				"  -ts width height: set output file size in pixels and lines. If width or\n"
				"      height is set to 0, the other dimension will be guessed from the computed\n"
				"      resolution.\n";
			//"  -ts width height:"
			//"      set output file size in pixels and lines. If width or height is set to 0, the other dimension will be guessed from the computed resolution. Note that -ts cannot be used with -tr"
		}

		if (IsUsed(OP_BANDS))
		{
			help +=
				"  -b band: Select an input band band for output. Bands are numbered from 1.\n"
				"      Multiple -b switches may be used to select a set of input bands to write\n"
				"      to the output file, or to reorder bands.\n";
		}
		//Starting with GDAL 1.8.0, band can also be set to "mask,1" (or just "mask") to mean the mask band of the 1st band of the input dataset.


		//help += "\n";

		return help;
	}


	//CGeoExtents COGRBaseOption::ComputeExtent(const CGeoExtents& inputExtents)const
	//{
	////	CGeoExtents inputExtents = inputDSVector[0].GetExtents();
	//	CGeoExtents extents;
	//	((CGeoRect2&)extents) = inputExtents;
	//
	//	//if no output extents sepcified, take the extents of the first image
	//	if( m_bExtents )
	//	{
	//		((CGeoRect2&)extents) = m_extents;
	//			
	//		//get the nearest grid cell
	//		if( m_bTap )
	//			extents.AlignTo( inputExtents );
	//
	//	}
	//
	//	if( m_bSize )
	//	{
	//		extents.m_xSize = m_xSize;
	//		extents.m_ySize = m_ySize;
	//	}
	//	else if( m_bRes)
	//	{
	//		extents.m_xSize = (int)ceil(extents.Width()/m_xRes);
	//		extents.m_ySize = (int)ceil(extents.Height()/m_yRes);
	//	}
	//	
	//		
	//	//take parent resolution
	//	if( extents.m_xSize == 0)
	//		extents.m_xSize = (int)abs(ceil(extents.Width()/inputExtents.GetXRes()));
	//	if( extents.m_ySize == 0)
	//		extents.m_ySize = (int)abs(ceil(extents.Height()/inputExtents.GetYRes()));
	//
	//
	//	return extents;
	//}
	//
	//CGeoExtents COGRBaseOption::ComputeExtent(const CGDALDatasetExVector& inputDS)const
	//{
	////by default take the minimum extent
	//	CGeoExtents extents = inputDS[0].GetExtents();
	//	for( size_t i=1; i<inputDS.size(); i++)
	//		extents.IntersectExtent( inputDS[i].GetExtents() );
	//
	//	return ComputeExtent(extents);
	//}
	//
	//
}