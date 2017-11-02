//***********************************************************************
// program to analyze bands and report some information on changing
//									 
//***********************************************************************
// version 
// 2.3.0	30/10/2017	Rémi Saint-Amant	Compile with GDAL 2.0 and add cloud improvement. Remove second disturbance of the same type. trigger have change
// 2.2.4	11/07/2016	Rémi Saint-Amant	Add export series
// 2.2.3	05/07/2016	Rémi Saint-Amant	Bug correction in reading despike and trigger
// 2.2.2	17/06/2016	Rémi Saint-Amant	negative julian date 1970 is missing
// 2.2.1	17/06/2016	Rémi Saint-Amant	remove 3e argument from -trigger and -despike
// 2.2.0	09/05/2016	Rémi Saint-Amant	Add missing value in the tree
// 2.1.1    08/07/2015	Rémi Saint-Amant	correct bug in load data block
// 2.1.0    29/06/2015	Rémi Saint-Amant	Use model with 6 temporal steps, add of physical layers
// 2.0.2	27/05/2015	Rémi Saint-Amant	Separate export bands, remove validation, add delta B5 option
// 2.0.1	27/03/2015	Rémi Saint-Amant	Add more export time
// 2.0.0	11/03/2015	Rémi Saint-Amant	rename BandAnalyser by DisturbanceAnalayser
// 1.7.0	05/03/2015	Rémi Saint-Amant	Change euclédienne distance by NBR difference, add QA and JDAy as export
// 1.6.9	30/01/2015	Rémi Saint-Amant	don't modify input VRT file. Bug correction in IntersectRect
// 1.6.8    27/01/2015	Rémi Saint-Amant	Compile with GDAL 1.11.1
// 1.6.7	31/12/2014	Rémi Saint-Amant	Include all values < 99.
// 1.6.6    22/12/2014	Rémi Saint-Amant	Exclude to map value >= 99. Output the number of disturbances
// 1.6.5	26/11/4014	Rémi Saint-Amant	some correction in help, add all band in export
// 1.6.4	31/10/4014	Rémi Saint-Amant	Add fire severity model and add B7 to export bands
// 1.6.3    25/10/2014	Rémi Saint-Amant	bug correction in help
// 1.6.2	26/09/2014	Rémi Saint-Amant	bugs corrections
// 1.6.1	24/07/2014	Rémi Saint-Amant	bugs corrections
// 1.6.0	26/06/2014	Rémi Saint-Amant	GDAL 1.11, UNICODE, VC 2013
// 1.5.0	25/10/2013	Rémi Saint-Amant	Output only B3-B4-B5 in export and stats
// 1.4.8	22/07/2013	Rémi Saint-Amant	Change -stats by -exportStats. Add -stats option.
// 1.4.7	20/06/2013	Rémi Saint-Amant	Bug correction in mask
// 1.4.6    28/05/2013	Rémi Saint-Amant	Remove multiple built overview message
// 1.4.5    24/05/2013	Rémi Saint-Amant	Bug correction
// 1.4.4    24/05/2013	Rémi Saint-Amant	Add statistics
// 1.4.3    24/05/2013	Rémi Saint-Amant	add Overview, add confirmation
// 1.4.2    14/05/2013	Rémi Saint-Amant	multiple correction, manage input map with cloud or Julian day
// 1.4.1    08/05/2013	Rémi Saint-Amant	Return disturbance code instead of type(index)
// 1.4.0    29/04/2013  Rémi Saint-Amant	New version with clouds mask and quality (9 bands per images)
// 1.3.2	01/03/2012	Rémi Saint-Amant	statistic compute only on change
// 1.3.2	26/02/2012	Rémi Saint-Amant	Fixe problem with export bands
// 1.3.1	25/02/2012	Rémi Saint-Amant	Fixe problem with statistics
// 1.3.0    07/02/2013	Rémi Saint-Amant	Operation by block instead of by row.
//                                             Add of band definition and virtual bands 
// 1.2.2    24/12/2012	Rémi Saint-Amant	Add export bands
// 1.2.1    18/12/2012	Rémi Saint-Amant	Add new export of band
// 1.2.0    18/12/2012	Rémi Saint-Amant	Add possibility of export individual DT result
// 1.1.0	25/09/2012	Rémi Saint-Amant	Add decision tree 
// 1.0.0	20/09/2012	Rémi Saint-Amant	Creation



#include "stdafx.h"
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
#include "Geomatic/See5hooks.h"
#include "Geomatic/LandsatDataset.h"


#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF;
using namespace WBSF::Landsat;

 

static const char* version = "2.3.0";
static const int NB_THREAD_PROCESS = 2; 
static const int FIRE_CODE = 1;
//static const int OTHER_CODE = 100;
static const int SPIKING_CODE = 110;
static const int NO_DISTERBANCE = 99;
static const int NOT_TRIGGED_CODE = 100;
static const int DT_CODE_NOT_INIT = 0;
static const float NO_IMAGE_NO_DATA = -99999;



//GDALWarp -co "COMPRESS=LZW" -overwrite -te 2122545 7061956 2129235 7068496 "U:\GIS1\LANDSAT_SR\mos\20141219_mergeiexe\step3_cloudfree_v2\mystack9914.vrt" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\input\lansat.tif"
//GDALWarp -co "COMPRESS=LZW" -overwrite -te 2122545 7061956 2129235 7068496 info.vrt "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\input\physics.tif"


//-stats -ExportBands -debug --config GDAL_CACHEMAX 512 -blockSize 128 128 -multi -te -55300 6786400 -35300 6797400 -IOCPU 2 -ot Int16 -co "compress=LZW" -overwrite -mask "U:\GIS\#projets\LAQ\DATA\mask\Harv_mask123_buff50km.tif" -maskValue 1 U:\GIS\#projets\LAQ\ANALYSE_CA\20130416_SR_run1\V4_SR_DTD1_v1 U:\GIS1\LANDSAT_SR\mos\20130327_mergeiexe\Pilote_region\t2.tif U:\GIS\#documents\TestCodes\BandsAnalyser\Test1\test.tif
//-Source MERGED -stats -ExportBands -debug --config GDAL_CACHEMAX 512 -blockSize 128 128 -co "tiled=YES" -co "BLOCKXSIZE=128" -co "BLOCKYSIZE=128"  -IOCPU 2 -ot Int16 -co "compress=LZW" -overwrite -mask "U:\GIS\#projets\LAQ\DATA\mask\Harv_mask123_buff50km.tif" -maskValue 1 "U:\GIS\#projets\LAQ\ANALYSE_CA\20130416_SR_run1\V4_SR_DTD1_v1" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test2\Input\inputByYears.tif" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test2\Output\OutputByYears.tif"
//-B9 JDay1970 -stats -ExportBands -debug -multi --config GDAL_CACHEMAX 512 -blockSize 128 128 -co "tiled=YES" -co "BLOCKXSIZE=128" -co "BLOCKYSIZE=128"  -IOCPU 2 -ot Int16 -co "compress=LZW" -overview {4,8} -overwrite -mask "U:\GIS\#projets\LAQ\DATA\mask\Harv_mask123_buff50km.tif" -maskValue 1 "U:\GIS\#projets\LAQ\ANALYSE_CA\20130416_SR_run1\V4_SR_DTD1_v1" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test2\Input\inputByYears.tif" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test2\Output\OutputByYears.tif"
//-te 1541000 6872800 1591000 6922800 --config GDAL_CACHEMAX 512 -co "tiled=YES" -co "BLOCKXSIZE=128" -co "BLOCKYSIZE=128" -blocksize 128 128 -DistanceMin 500 -ot Int16 -multi -dstnodata -32768 -IOCPU 2 -co "compress=LZW" -co "BIGTIFF=YES"  -mask "U:\GIS\#projets\LAQ\DATA\mask\Harv_mask123_buff50km.tif" -maskValue 1 -NbDisturbances 3 -stats -ExportBands -Debug -B9 JDay1970 "U:\GIS\#projets\LAQ\ANALYSE_CA\20130416_SR_run1\V4_SR_DTD1_v1" "U:\GIS1\LANDSAT_SR\mos\20130327_mergeiexe\test3\##mos_9911.vrt" "U:\GIS\#projets\LAQ\ANALYSE_CA\20130521_SR_run2\Testdd44.tif"

//-te 1702375 7105975 1703650 7106850 -FireSeverity -multi -IOCPU 2 -ot Int16 -co "compress=LZW" -overview {4,8} -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test3\Model\V4_SR_DTD1_v1" "U:\GIS1\LANDSAT_SR\mos\20140924_mergeiexe_30m\##_mystack9914_local.vrt" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test3\Output\Fire 1999-2014.tif"
//-te 1702375 7105975 1703650 7106850 -FireSeverity -multi -IOCPU 2 -ot Int16 -co "compress=LZW" -overview {4,8} -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test3\Model\V4_SR_DTD1_v1" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test3\Input\Fire 1999-2014.tif" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test3\Output\Fire 1999-2014 from TIFF.tif"
// -FireSeverity -DistanceMin 50 --config GDAL_CACHEMAX 512 -blockSize 128 128 -co "tiled=YES" -co "BLOCKXSIZE=128" -co "BLOCKYSIZE=128"  -multi -IOCPU 2 -ot Int16 -co "compress=LZW" -overview {4,8} -overwrite "U:\GIS\#documents\TestCodes\BandsAnalyser\Test2\Model\V4_SR_DTD1_v1" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test2\Input\AnnualScenes.tif" "U:\GIS\#documents\TestCodes\BandsAnalyser\Test2\Output\AnnualScenes.tif" 

//-co "tiled=YES" -co "BLOCKXSIZE=1024" -co "BLOCKYSIZE=1024" -ot Int16 -multi -CPU 16 -dstnodata -32768 -co "compress=LZW" -co "BIGTIFF=YES" --config GDAL_CACHEMAX 2048 -of VRT -Trigger NBR 0.1 -Despike TCB 0.9 -stats -overview {2,4,8,16} -overwrite "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test5\Model\DTv5" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test5\inputs\VRT_L578_8415.tif" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test5\inputs\physics.vrt" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test5\Output\zone.tif"
//-Trigger NBR 0.1 -Despike TCB 0.9 -Debug -IOCPU 4 -multi -ot Int16 -co "compress=LZW" -stats -overview {2,4,8,16} -overwrite "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\Model\DTv4" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\input\lansat.tif" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\input\physics.tif" "U:\GIS\#documents\TestCodes\DisturbanceAnalyser\Test4\Output\zone2.tif"



//-te -45300 6791400 -35300 6797400
//-te 1708000 6832000 1733600 6857600
//-multi -te -55300 6786400 -35300 6797400



enum TFilePath { DT_FILE_PATH, LANDSAT_FILE_PATH, PHYSICAL_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
enum TOutputBand{ O_FIRST_DATE, O_DISTURBANCE, O_DATE1, O_DATE2, O_LAST_DATE, NB_OUTPUT_BANDS };//O_NB_CONFIRM,

enum TPhysical { PHYSICAL_DD5, PHYSICAL_DEM, PHYSICAL_SLOPE, NB_PHYSICAL_BANDS };
enum { DT_SCENES_SIZE = SCENES_SIZE + NB_PHYSICAL_BANDS };

enum TDebug { D_NB_PAIRS, D_NB_DISTURBANCES, D_FIRST_DISTURBANCE, D_F_DATE1, D_F_DATE2, D_LAST_DISTURBANCE, D_L_DATE1, D_L_DATE2, NB_DEBUGS };//, D_MEAN_D, D_MAX1, D_MAX2, D_MAX3
enum TExportTemporal { E_Tm3, E_Tm2, E_Tm1, E_Tp1, E_Tp2, E_Tp3, NB_EXPORT_TEMPORAL };
enum TExportBands { E_B1, E_B2, E_B3, E_B4, E_B5, E_B6, E_B7, E_QA, E_JD, NB_EXPORT_BANDS };
static const int EXPORT_BANDS[NB_EXPORT_BANDS] = { B1, B2, B3, B4, B5, B6, B7, QA, JD };


enum TStat{ B_LOWEST, B_MEAN, B_STD_DEV, B_HIGHEST, NB_STATS};
static const short BANDS_STATS[NB_STATS] = { LOWEST, MEAN, STD_DEV, HIGHEST};
static const char* DEBUG_NAME[NB_DEBUGS] = { "size", "dist", "first", "dateF1", "dateF2", "last", "dateL1", "dateL2" };//, "mean", "max1", "max2", "max3"


typedef vector<__int16> DTCodeVector;



enum TFireSeverity{FS_NO_FIRE_SEVERITY=-1, FS_RON, FS_JO, FS_MEAN, NB_FIRE_SEVERITY};
static const char * FIRE_SEVERITY_MODEL_NAME[NB_FIRE_SEVERITY] = { "Ron", "Jo", "Mean" };

//DAD = Disturbance Analyser Data
class CDADVector : public CLandsatPixelVector
{
public:

	array<double, NB_PHYSICAL_BANDS> m_physical;
	CDADVector(size_t size = 0) :CLandsatPixelVector(size)
	{}

	bool IsInit()const { return size() > 1; }

	size_t GetNbDTCode()const{ return m_DTCode.size(); }
	size_t GetNbDisturbances()const
	{
		size_t n = 0;
		for (size_t z = 0; z < m_DTCode.size(); z++)
		{
			if (IsDisturbed(m_DTCode[z]))
				n++;
		}

		return n;
	}

	size_t GetDisturbanceIndex(size_t pos)const
	{
		ASSERT(pos < m_DTCode.size());

		size_t index = UNKNOWN_POS;
		for (size_t z = 0; z < m_DTCode.size() && index >= m_DTCode.size(); z++)
		{
			//look up for change
			if (IsDisturbed(m_DTCode[z]))
			{
				if (pos == 0)
					index = z;

				pos--;
			}
		}

		return index;
	}

	size_t GetIndex(size_t index, int shift)const
	{
		ASSERT(index >= 0);

		size_t indexShift = (size_t)((int)index + shift);
		if (indexShift >= size() || !at(indexShift).IsInit())
			indexShift = -1;

		return indexShift;
	}

	size_t GetFirstDisturbanceIndex()const
	{
		size_t index = UNKNOWN_POS;
		for (DTCodeVector::const_iterator it = m_DTCode.begin(); it != m_DTCode.end() && index == UNKNOWN_POS; it++)
		{
			//look up for change
			if (IsDisturbed(*it))
				index = std::distance(m_DTCode.begin(), it);
		}
		return index;
	}

	size_t GetLastDisturbanceIndex()const
	{
		ASSERT(m_DTCode.size() == size() - 1);
		size_t index = UNKNOWN_POS;
		for (DTCodeVector::const_reverse_iterator it = m_DTCode.rbegin(); it != m_DTCode.rend() && index == UNKNOWN_POS; it++)
		{
			//look up for change
			if (IsDisturbed(*it))
				index = m_DTCode.size() - std::distance(m_DTCode.rbegin(), it) - 1;
		}

		ASSERT(index == UNKNOWN_POS || index < size() - 1);
		return index;
	}

	CTRef GetFirstTRef()const
	{
		CTRef TRef;
		for (const_iterator it = begin(); it != end() && !TRef.IsInit(); it++)
			TRef = it->GetTRef();

		return TRef;
	}

	CTRef GetLastTRef()const
	{
		CTRef TRef;
		for (const_reverse_iterator it = rbegin(); it != rend() && !TRef.IsInit(); it++)
			TRef = it->GetTRef();

		return TRef;
	}

	size_t FindYear(int year)const
	{
		size_t z = NOT_INIT;
		if (!empty())
		{
			size_t zz = begin()->GetTRef().GetYear() - year;
			if (zz < size() && at(zz).GetTRef().GetYear() == year)
			{
				z = zz;
			}
			else
			{
				//for (const_iterator it = begin(); it != end() && z == NOT_INIT; it++)
				for (size_t i = 0; i < size() && z == NOT_INIT; i++)
					if (at(i).GetTRef().GetYear() == year)
						z = i;
			}
		}

		return z;
	}

	void clear()
	{
		CLandsatPixelVector::clear();
		m_DTCode.clear();
	}


	static bool IsDisturbed(__int16 DTCode){ return DTCode > DT_CODE_NOT_INIT && DTCode < NO_DISTERBANCE; }

	CDecisionTreeBlock GetDataRecord(size_t z1,/* size_t z2,*/ CDecisionTreeBaseEx& DT)
	{
		ASSERT(z1 < size());
		//ASSERT(z2 < size() );
		ASSERT(IsInit());

		CDecisionTreeBlock block(DT.MaxAtt + 1);


		//fill the data structure for decision tree
		size_t c = 0;
		DVal(block, c++) = DT.MaxClass + 1;
		DVal(block, c++) = Continuous(DT, 1) ? DT_UNKNOWN : 0;
		for (size_t i = 0; i < m_physical.size(); i++)
			CVal(block, c++) = (ContValue)m_physical[i];


		//__int64 first_image = (__int64)z1 - 2;
		//__int64 last_image = (__int64)z1 + 3;// load 6 pixels for DT
		//
		//for (__int64 z = first_image; z <= last_image; z++)
		//{
		//	bool bValid = (z >= 0 && z<(__int64)size()) && at(z).IsValid() && at(z).IsInit();
		//	for (size_t j = 0; j < Landsat::SCENES_SIZE; j++)
		//		CVal(block, c++) = (ContValue)(bValid ? at(z).at(j) : NO_IMAGE_NO_DATA);
		//	
		//}//for

		ASSERT(at(z1).IsValid() && !IsSpiking(z1));

		__int64 first_image = (__int64)z1;
		__int64 last_image = (__int64)z1;
		size_t nbz1 = 0;
		while (nbz1 != 2)
		{
			ASSERT(at(first_image).IsValid());

			first_image--;

			if (first_image < 0 || !IsSpiking(first_image) )
				nbz1++;
		}

		size_t nbz2 = 0;
		while (nbz2 != 3)
		{
			ASSERT(at(last_image).IsValid());
			last_image++;

			if (last_image >= (__int64)size() || !IsSpiking(last_image))
				nbz2++;
		}

		for (__int64 z = first_image; z <= last_image; z++)
		{
			if (z >= 0 && z < (__int64)size())
			{
				bool bValid = at(z).IsValid() && !IsSpiking(z);
				if (bValid)
				{
					for (size_t j = 0; j < Landsat::SCENES_SIZE; j++)
						CVal(block, c++) = (ContValue)(at(z).at(j));
				}
				else
				{
					ASSERT(last_image - first_image>=6);
					int g;
					g = 1;
				}
			}
			else
			{
				for (size_t j = 0; j < Landsat::SCENES_SIZE; j++)
					CVal(block, c++) = (ContValue)(NO_IMAGE_NO_DATA);
			}
		}//for


		ASSERT(c == (6 * 9 + 3 + 2));
		//fill virtual bands 
		for (; c <= DT.MaxAtt; c++)
		{
			ASSERT(DT.AttDef[c]);
			//assuming virtual band never return no data
			block[c] = DT.EvaluateDef(DT.AttDef[c], block.data());
		}

		return block;
	}


	__int16 GetDTCode(size_t z)const{ return m_DTCode[z]; }
	void SetDTCode(size_t z, __int16 DTCode){ m_DTCode[z] = DTCode; }

	void Despike(const CIndiciesVector& despike)
	{
		if (!empty())
		{
			m_despike.resize(size());
			m_despike.reset();

			//for (iterator it = begin(); it != end() && it + 1 != end() && it + 2 != end(); it++)
			for (size_t i = 0; i < size() - 2; i++)
				m_despike.set(i + 1, despike.IsSpiking(at(i), at(i + 1), at(i + 2)));
		}

		ASSERT(m_despike.size() == size());
	}

	void InitDTCode()
	{
		m_DTCode.clear();
		if (IsInit())
			m_DTCode.insert(m_DTCode.begin(), size() - 1, DT_CODE_NOT_INIT);
	}

	bool IsSpiking(size_t i)const{ return m_despike[i]; }

protected:


	DTCodeVector m_DTCode;
	boost::dynamic_bitset<size_t> m_despike;
};


class CDisterbanceAnalyserOption : public CBaseOptions
{
public:
	CDisterbanceAnalyserOption()
	{
		m_bAllBands=false;
	
		m_bExportBands=false;
		m_bExportTimeSeries = false;
		m_nbDisturbances=1;
		m_bDebug=false;
		m_nbPixel=0;
		m_nbPixelDT=0;
		m_scenesSize = SCENES_SIZE;
		m_bFireSeverity = false;

		m_appDescription = "This software look up (with a decision tree model) for disturbance in a any number series of LANDSAT scenes";

		AddOption("-TTF");
		AddOption("-Period");

		static const COptionDef OPTIONS[] = 
		{
			{ "-Trigger", 3, "tt op th", true, "Add optimization trigger to execute decision tree when comparing T-1 with T+1. tt is the trigger type, op is the comparison operator '<' or '>' and th is the trigger threshold. Supported type are \"B1\"..\"JD\", \"NBR\",\"EUCLIDEAN\", \"NDVI\", \"NDMI\", \"TCB\" (Tasseled Cap Brightness), \"TCG\" (Tasseled Cap Greenness) or \"TCW\" (Tasseled Cap Wetness)." },
			{ "-Despike", 3, "dt op dh", true, "Despike to remove invalid pixel. dt is the despike type, op is the comparison operator '<' or '>', th is the despike threshold. Supported type are \"B1\"..\"JD\", \"NBR\",\"EUCLIDEAN\", \"NDVI\", \"NDMI\", \"TCB\" (Tasseled Cap Brightness), \"TCG\" (Tasseled Cap Greenness) or \"TCW\" (Tasseled Cap Wetness)." },
			{ "-NbDisturbances", 1, "nb", false, "Number of disturbance to output. 1 by default." },
			{ "-FireSeverity", 1, "model", false, "Compute fire severity for \"Ron\", \"Jo\" and \"Mean\" model." },
			{ "-ExportBands",0,"",false,"Export disturbances scenes."},
			{ "-ExportTimeSeries", 0, "", false, "Export informations over all period." },
			//{ "-ExportCloud", 0, "", false, "Export clouds and shawdows informations for all years." },
			{ "-Debug",0,"",false,"Output debug information."},
			{ "DTModel",0,"",false,"Decision tree model file path."},
			{ "src1file",0,"",false, "LANDSAT scenes image file path."},
			{ "src2file", 0, "", false, "Geophysical (DD5, DEM, slope) input image file path." },
			{ "dstfile",0,"",false, "Output image file path."}
		};
		
		for(int i=0; i<sizeof(OPTIONS)/sizeof(COptionDef); i++)
			AddOption(OPTIONS[i]);

		static const CIOFileInfoDef IO_FILE_INFO[] = 
		{
			{ "Input Model", "DTModel","","","","Decision tree model file generate by See5."},
			{ "LANDSAT Image", "src1file","","ScenesSize(9)*nbScenes","B1: Landsat band 1|B2: Landsat band 2|B3: Landsat band 3|B4: Landsat band 4|B5: Landsat band 5|B6: Landsat band 6|B7: Landsat band 7|QA: Image quality|B9: Date(Julian day 1970 or YYYYMMDD format) and cloud mask(NoData)|... for each scene"},
			{ "Geophysical Image", "src2file", "", "3", "B1: Degres-day 5°C threshold|B2: Digital Elevation Model (DEM)|B3: Slope (?)" },
			{ "Output Image", "dstfile","One file per perturbation","6","FirstDate: date of the first image analysed|DTCode: disturbance code|D1: disturbace date of the first image|D2: disturbance date of the second image|NbContirm: number of image without disturbance after the disturbance|LastDate: date of the last image analysed"},
			{ "Optional Output Image", "dstfile_FireSeverity","1","3","Ron|Jo|Mean of Ron and Jo"},
			{ "Optional Output Image", "dstfile_ExportBands","One file per perturbation","OutputBands(9) x NbTime(8) = 36","T-2: Scenes 2 years preciding T|...|T+5: Scenes 5 years folowing T"},
			{ "Optional Output Image", "dstfile_TimeSeries", "One file per input years", "Nb Years", "Y1: first year|...|Yn: last year" },
			{ "Optional Output Image", "dstfile_debug","1","9"," NbPairs: number of \"Pair\", a pair is composed of 2 valid images|NbDisturbances: number of disturbances found in the series.|Disturbance: last diturbance|D1: date of the first image of the last disturbance|D2: date of the second image of the last disturbance|MeanD: mean NBR distance|MaxD1: highest NBR distance|MaxD2: second highest NBR distance|MaxD3: third highest NBR distance"}
		};

		for(int i=0; i<sizeof(IO_FILE_INFO)/sizeof(CIOFileInfoDef); i++)
			AddIOFileInfo(IO_FILE_INFO[i]);

	}

	virtual ERMsg ParseOption(int argc, char* argv[])
	{
		ERMsg msg = CBaseOptions::ParseOption(argc, argv);
		
		ASSERT( NB_FILE_PATH==4);
		if (msg && m_filesPath.size() != NB_FILE_PATH)
		{
			msg.ajoute("ERROR: Invalid argument line. 4 files are needed: decision tree model, the geophysical image, the LANDSAT image and the destination image.");
			msg.ajoute("Argument found: ");
			for (size_t i = 0; i < m_filesPath.size(); i++)
				msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
		}

		return msg;
	}

	virtual ERMsg ProcessOption(int& i, int argc, char* argv[])
	{
		ERMsg msg;
		
		if (IsEqual(argv[i], "-Trigger"))
		{
			string str = argv[++i];
			TIndices type = GetIndiceType(str);
			string op = argv[++i];
			double threshold = atof(argv[++i]);

			if (type != I_INVALID)
			{
				if (CIndices::IsValidOp(op))
					m_trigger.push_back(CIndices(type, op, threshold));
				else
					msg.ajoute(op + " is an invalid operator for -Trigger option");
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
			//string op = argv[++i];
			double threshold = atof(argv[++i]);
			

			if (type != I_INVALID)
			{
				//if (CIndices::IsValidOp(op))
					m_despike.push_back(CIndices(type, "<", threshold));
				//else
					//msg.ajoute(op + " is an invalid operator for -Despike option");
				
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
			m_bDebug=true;
		}
		else
		{
			//Look to see if it's a know base option
			msg = CBaseOptions::ProcessOption(i, argc, argv);
		}
		
		return msg;
	}

	bool m_bByYear;
	bool m_bAllBands;
	
	
	CIndiciesVector m_trigger;
	CIndiciesVector m_despike;

	bool m_bExportBands;
	bool m_bExportTimeSeries;
	bool m_bFireSeverity;
	bool m_bDebug;
	int	 m_nbDisturbances;
	

	__int64 m_nbPixelDT;
	__int64 m_nbPixel;
};

//***********************************************************************
//									 
//	Main                                                             
//									 
//***********************************************************************
class CDisterbanceAnalyser
{
public:

	string GetDescription() { return  string("BandsAnalyser version ") + version + " (" + _T(__DATE__) + ")\n" ; }
	ERMsg Execute();


	ERMsg OpenAll(CGDALDatasetEx& lansatDS, CGDALDatasetEx& physicalDS, CGDALDatasetEx& maskDS, vector<CGDALDatasetEx>& outputDS, CGDALDatasetEx& fireSeverityDS, vector<CGDALDatasetEx>& exportBandsDS, CGDALDatasetEx& exportTSDS, CGDALDatasetEx& debugDS);
	void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder1, CBandsHolder& bandHolder2);
	void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder1, const CBandsHolder& bandHolder2, CDecisionTree& DT, vector< vector< CDADVector >>& disturbances);
	void WriteBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, const vector< vector< CDADVector >>& disturbances, vector<CGDALDatasetEx>& outputDS, CGDALDatasetEx& fireSeverityDS, vector<CGDALDatasetEx>& exportBandsDS, CGDALDatasetEx& exportTSDS, CGDALDatasetEx& debugDS);
	void CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& physicalDS, CGDALDatasetEx& maskDS, vector<CGDALDatasetEx>& outputDS, CGDALDatasetEx& fireSeverityDS, vector<CGDALDatasetEx>& exportBandsDS, CGDALDatasetEx& exportTSDS, CGDALDatasetEx& debugDS);

	void Evaluate( int x, int y, const vector<array<short, 3>>& DTCode, vector<vector<vector<short>>>& output);
	void LoadData(const CBandsHolder& bandHolder1, const CBandsHolder& bandHolder2, vector< vector< CDADVector >>& data);
	ERMsg ReadRules(CDecisionTree& DT);

	CDisterbanceAnalyserOption m_options;

	static int FindIndex(int start, const vector<short>& bandsvalue, int dir);
	static void LoadModel(CDecisionTreeBaseEx& DT, string filePath);
	static short DTCode2ChangeCodeIndex(short DTCode);
	static short DTCode2ChangeCode(short DTCode);

};

ERMsg CDisterbanceAnalyser::ReadRules(CDecisionTree& DT)
{
	ERMsg msg;
	if(!m_options.m_bQuiet) 
	{
		cout << "Read rules..."<<endl;
	}

	CTimer timer(true);

	msg += DT.Load(m_options.m_filesPath[DT_FILE_PATH], m_options.m_CPU, m_options.m_IOCPU);
	timer.Stop();

	if( !m_options.m_bQuiet )
		cout << "Read rules time = " << SecondToDHMS(timer.Elapsed()).c_str() << endl << endl;


	return msg;
}	


ERMsg CDisterbanceAnalyser::OpenAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& physicalDS, CGDALDatasetEx& maskDS, vector<CGDALDatasetEx>& outputDS, CGDALDatasetEx& fireSeverityDS, vector<CGDALDatasetEx>& exportBandsDS, CGDALDatasetEx& exportTSDS, CGDALDatasetEx& debugDS)
{
	ERMsg msg;
	
	
	if(!m_options.m_bQuiet) 
		cout << endl << "Open input image..." << endl;

	msg = landsatDS.OpenInputImage(m_options.m_filesPath[LANDSAT_FILE_PATH], m_options);
	msg += physicalDS.OpenInputImage(m_options.m_filesPath[PHYSICAL_FILE_PATH], m_options);
	if (msg && physicalDS.GetRasterCount() != NB_PHYSICAL_BANDS)
		msg.ajoute("Geophysical layers must be composed of 3 bands: DD5, DEM, slope");

	if(msg)
	{
	
		landsatDS.UpdateOption(m_options);
			
		if(!m_options.m_bQuiet) 
		{
			CGeoExtents extents = landsatDS.GetExtents();
			CProjectionPtr pPrj = landsatDS.GetPrj();
			string prjName = pPrj ? pPrj->GetName() : "Unknown";

			cout << "    Size           = " << landsatDS->GetRasterXSize() << " cols x " << landsatDS->GetRasterYSize() << " rows x " << landsatDS.GetRasterCount() << " bands" << endl;
			cout << "    Extents        = X:{" << ToString(extents.m_xMin) << ", " << ToString(extents.m_xMax) << "}  Y:{" << ToString(extents.m_yMin) << ", " << ToString(extents.m_yMax) << "}" << endl;
			cout << "    Projection     = " << prjName << endl;
			cout << "    NbBands        = " << landsatDS.GetRasterCount() << endl;
			cout << "    Scene size     = " << landsatDS.GetSceneSize() << endl;
			cout << "    Nb. Scenes     = " << landsatDS.GetNbScenes() << endl;
			cout << "    First image    = " << landsatDS.GetPeriod().Begin().GetFormatedString() << endl;
			cout << "    Last image     = " << landsatDS.GetPeriod().End().GetFormatedString() << endl;
			cout << "    Input period   = " << m_options.m_period.GetFormatedString() << endl;

			if (landsatDS.GetNbScenes() <= 1)
				msg.ajoute("DisturbanceAnalyser can't be performed over only one scene");
		}
	}


	if( msg && !m_options.m_maskName.empty() )
	{
		if(!m_options.m_bQuiet) 
			cout << "Open mask image..." << endl;
		msg += maskDS.OpenInputImage( m_options.m_maskName );
	}

	if(msg && m_options.m_bCreateImage)
	{

		if(!m_options.m_bQuiet) 
			cout << "Create output image..." << endl;

		CDisterbanceAnalyserOption option = m_options;
		option.m_nbBands = NB_OUTPUT_BANDS;

		outputDS.resize(m_options.m_nbDisturbances);
		for(size_t i=0; i<outputDS.size()&&msg; i++)
		{
			string filePath = option.m_filesPath[OUTPUT_FILE_PATH];
			if (i>0)
				SetFileTitle(filePath, GetFileTitle(filePath) + ToString(i + 1));
			
			msg += outputDS[i].CreateImage(filePath, option);
		}
	}
	

	//open exportStats files
	if (msg && m_options.m_bFireSeverity)
	{
		if(!m_options.m_bQuiet) 
			_tprintf("Create fire severity image...\n");

		CDisterbanceAnalyserOption options = m_options;
		options.m_nbBands = NB_FIRE_SEVERITY;
		options.m_outputType = GDT_Float32;
		options.m_dstNodata=::GetDefaultNoData(GDT_Int32);
		string exportPath = options.m_filesPath[OUTPUT_FILE_PATH];
		SetFileTitle(exportPath, GetFileTitle(exportPath) + "_fireSeverity");
		for (size_t m = 0; m < NB_FIRE_SEVERITY; m++)
			options.m_VRTBandsName += GetFileTitle(exportPath) + "_" + FIRE_SEVERITY_MODEL_NAME[m] + ".tif|";

		msg += fireSeverityDS.CreateImage(exportPath, options);
	}
	
	if( msg && m_options.m_bExportBands )
	{
		if(!m_options.m_bQuiet) 
			cout << "Create export bands images..." << endl;
			

		CDisterbanceAnalyserOption options = m_options;
		options.m_nbBands = NB_EXPORT_BANDS*NB_EXPORT_TEMPORAL;//T-2 à T+3
		options.m_outputType = GDT_Int16;
		options.m_dstNodata=::GetDefaultNoData(GDT_Int16);

		exportBandsDS.resize(options.m_nbDisturbances);
		for(size_t i=0; i<exportBandsDS.size(); i++)
		{
			string exportPath = options.m_filesPath[OUTPUT_FILE_PATH];
			if (i>0)
				SetFileTitle(exportPath, GetFileTitle(exportPath) + ToString(i + 1));

			SetFileTitle(exportPath, GetFileTitle(exportPath) +"_exportBands");

			assert(NB_EXPORT_BANDS == SCENES_SIZE);
			for (size_t s = 0; s < NB_EXPORT_TEMPORAL; s++)
			{
				
				for (size_t b = 0; b < SCENES_SIZE; b++)
					options.m_VRTBandsName += GetFileTitle(exportPath) + string("_T") + FormatA("%+d", s - 2) + string("_") + CLandsatDataset::SCENE_NAME[b] + ".tif|";
			}

			msg += exportBandsDS[i].CreateImage(exportPath, options);
		}
	}
	
	if (msg && m_options.m_bExportTimeSeries)
	{
		if (!m_options.m_bQuiet)
			cout << "Create export time series images..." << endl;


		ASSERT(landsatDS.GetNbScenes() == landsatDS.GetPeriod().GetNbYears());
		CDisterbanceAnalyserOption options = m_options;
		options.m_nbBands = landsatDS.GetNbScenes() - 1;
		options.m_outputType = GDT_Byte;
		options.m_dstNodata = ::GetDefaultNoData(GDT_Byte);
		
		string exportPath = options.m_filesPath[OUTPUT_FILE_PATH];
		SetFileTitle(exportPath, GetFileTitle(exportPath) + "_timeSeries");
		
		for (size_t y = 0; y < landsatDS.GetScenePeriod().size()-1; y++)
		{
			ASSERT(landsatDS.GetScenePeriod().at(y).Begin().GetYear() == landsatDS.GetScenePeriod().at(y).End().GetYear());
			int year1 = landsatDS.GetScenePeriod().at(y).Begin().GetYear();
			//int year2 = landsatDS.GetScenePeriod().at(y+1).Begin().GetYear();
			options.m_VRTBandsName += GetFileTitle(exportPath) + string("_") + FormatA("%d", year1) + ".tif|";
		}

		msg += exportTSDS.CreateImage(exportPath, options);
	}

	

	
	if( msg && m_options.m_bDebug )
	{
		if(!m_options.m_bQuiet) 
			cout << "Create debug image..." << endl;

		CDisterbanceAnalyserOption options = m_options;
		options.m_nbBands = NB_DEBUGS;
		options.m_outputType = GDT_Int32;
		options.m_dstNodata= ::GetDefaultNoData(GDT_Int32);
		string filePath = options.m_filesPath[OUTPUT_FILE_PATH];
		SetFileTitle(filePath, GetFileTitle(filePath) + "_debug");
		for (size_t d = 0; d < NB_DEBUGS; d++)
		{
			options.m_VRTBandsName += GetFileTitle(filePath) + "_" + DEBUG_NAME[d] + ".tif|";
		}

		msg += debugDS.CreateImage(filePath, options);
	}

	return msg;
}



ERMsg CDisterbanceAnalyser::Execute()
{
	ERMsg msg;

	if( !m_options.m_bQuiet )		
	{
		cout << "Output: " << m_options.m_filesPath[OUTPUT_FILE_PATH] << endl;
		cout << "From:   " << m_options.m_filesPath[LANDSAT_FILE_PATH] << endl;
		cout << "        " << m_options.m_filesPath[PHYSICAL_FILE_PATH] << endl;
		cout << "Using:  " << m_options.m_filesPath[DT_FILE_PATH] << endl;
		
		//for(size_t m=0; m<m_options.m_optionalDT.size(); m++)
			//cout << "        " << m_options.m_optionalDT[m].m_filePath << endl;

		if( !m_options.m_maskName.empty() )
			cout << "Mask:   "  << m_options.m_maskName << endl;
	}

	GDALAllRegister();

	if( m_options.m_outputType != GDT_Unknown && m_options.m_outputType != GDT_Int16 && m_options.m_outputType != GDT_Int32)
		msg.ajoute("Invalid -ot option. Only GDT_Int16 or GDT_Int32 are supported");
	
	if(!msg)
		return msg;


	CDecisionTree DT;
	msg += ReadRules(DT);
	if (!msg)
		return msg;
	
	CLandsatDataset lansatDS;
	CGDALDatasetEx physicalDS;
	CGDALDatasetEx maskDS;

	vector<CGDALDatasetEx> outputDS;
	CGDALDatasetEx fireSeverityDS;
	vector<CGDALDatasetEx> exportBandsDS;
	CGDALDatasetEx debugDS;
	CGDALDatasetEx exportTSDS;

	msg = OpenAll(lansatDS, physicalDS, maskDS, outputDS, fireSeverityDS, exportBandsDS, exportTSDS, debugDS);
	
	if( msg)
	{
		size_t nbScenes = lansatDS.GetNbScenes();
		size_t sceneSize = lansatDS.GetSceneSize();
		CBandsHolderMT bandHolder1(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);
		CBandsHolderMT bandHolder2(1, m_options.m_memoryLimit, m_options.m_IOCPU, NB_THREAD_PROCESS);

		if( maskDS.IsOpen() )
			bandHolder1.SetMask( maskDS.GetSingleBandHolder(), m_options.m_maskDataUsed );

		msg += bandHolder1.Load(lansatDS, m_options.m_bQuiet, m_options.m_extents, m_options.m_period);
		msg += bandHolder2.Load(physicalDS, true, m_options.m_extents);
		

		if(!msg)
			return msg;


		if( !m_options.m_bQuiet && m_options.m_bCreateImage) 
			cout << "Create output images " << m_options.m_nbDisturbances << " x(" << m_options.m_extents.m_xSize << " C x " << m_options.m_extents.m_ySize << " R x " << NB_OUTPUT_BANDS << " B) with " << m_options.m_CPU << " threads..." << endl;


		CGeoExtents extents = bandHolder1.GetExtents();
		m_options.ResetBar(extents.m_xSize*extents.m_ySize);

		vector<pair<int,int>> XYindex = extents.GetBlockList();
		
		omp_set_nested(1);
		#pragma omp parallel for schedule(static, 1) num_threads(NB_THREAD_PROCESS) if (m_options.m_bMulti)
		for(int b=0; b<(int)XYindex.size(); b++)
		{
			int threadBlockNo = omp_get_thread_num();
			int xBlock=XYindex[b].first;
			int yBlock=XYindex[b].second;

			//data
			vector< vector< CDADVector >> data;
			ReadBlock(xBlock, yBlock, bandHolder1[threadBlockNo], bandHolder2[threadBlockNo]);
			ProcessBlock(xBlock, yBlock, bandHolder1[threadBlockNo], bandHolder2[threadBlockNo], DT, data);
			WriteBlock(xBlock, yBlock, bandHolder1[threadBlockNo], data, outputDS, fireSeverityDS, exportBandsDS, exportTSDS, debugDS);
		}//for all blocks


		//  Close decision tree and free allocated memory  
		DT.FreeMemory();

		//close inputs and outputs
		CloseAll(lansatDS, physicalDS, maskDS, outputDS, fireSeverityDS, exportBandsDS, exportTSDS, debugDS);

	}
	
    return msg;
}

void CDisterbanceAnalyser::ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder1, CBandsHolder& bandHolder2)
{
	#pragma omp critical(BlockIO)
	{
	
		m_options.m_timerRead.Start();
	
		bandHolder1.LoadBlock(xBlock, yBlock);
		bandHolder2.LoadBlock(xBlock, yBlock);

		m_options.m_timerRead.Stop();
	}
}

//Get input image reference
void CDisterbanceAnalyser::ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder1, const CBandsHolder& bandHolder2, CDecisionTree& DT, vector< vector< CDADVector >>& data)
{
	//CGDALDatasetEx& inputDS, 

	size_t nbScenes = bandHolder1.GetNbScenes();
	size_t sceneSize = bandHolder1.GetSceneSize();
	CGeoExtents extents = bandHolder1.GetExtents();
	CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
	int nbCells = extents.m_xSize*extents.m_ySize;

	if (bandHolder1.IsEmpty())
	{
#pragma omp atomic		
		m_options.m_xx += (std::min(nbCells, blockSize.m_x*blockSize.m_y));
		m_options.UpdateBar();

		return;
	}


#pragma omp critical(ProcessBlock)
	{
		m_options.m_timerProcess.Start();

		//allocate process memory
		LoadData(bandHolder1, bandHolder2, data);

		//process all x and y 
#pragma omp parallel for schedule(static, 1) num_threads( m_options.m_CPU ) if (m_options.m_bMulti)  
		for (int y = 0; y < blockSize.m_y; y++)
		{
			for (int x = 0; x < blockSize.m_x; x++)
			{
				int threadNo = ::omp_get_thread_num();

				if (!data[x][y].empty())
				{
					//process all scenes 
					for (size_t z1 = 0; z1 < data[x][y].GetNbDTCode(); z1++)
					{
							if (!data[x][y].IsSpiking(z1))
							{
								size_t z2 = z1 + 1;
								while (z2<data[x][y].size() && data[x][y].IsSpiking(z2))
									z2++;

								if (z2 < data[x][y].size())
								{
#pragma omp atomic
									m_options.m_nbPixel++;

									if (m_options.m_trigger.IsTrigged(data[x][y][z1], data[x][y][z2]))
									{
#pragma omp atomic
										m_options.m_nbPixelDT++;

										vector <AttValue> block = data[x][y].GetDataRecord(z1, DT[threadNo]);
										ASSERT(!block.empty());
										int predict = (int)DT[threadNo].Classify(block.data());
										ASSERT(predict >= 1 && predict <= DT[threadNo].MaxClass);
										int DTCode = atoi(DT[threadNo].ClassName[predict]);
										//add exception, if the last DTcode is the same, we don't add it, RSA 26-10-2017 
										if (z1 - 1 >= data[x][y].size() || DTCode != data[x][y].GetDTCode(z1-1) )
											data[x][y].SetDTCode(z1, DTCode);
									}
									else //if not trigger
									{
										data[x][y].SetDTCode(z1, NOT_TRIGGED_CODE);
									}
									//}
								}
							}
							else//z1 spiking
							{
								data[x][y].SetDTCode(z1, SPIKING_CODE);
							}
						//}//if z1 init
					}//for z1
				}
#pragma omp atomic	
				m_options.m_xx++;
			}//for x


			m_options.UpdateBar();
		}//for y



		m_options.m_timerProcess.Stop();
	}
}

void CDisterbanceAnalyser::WriteBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, const vector< vector< CDADVector >>& data, vector<CGDALDatasetEx>& outputDS, CGDALDatasetEx& fireSeverityDS, vector<CGDALDatasetEx>& exportBandsDS, CGDALDatasetEx& exportTSDS, CGDALDatasetEx& debugDS)
{
	if( data.empty() )
		return;

#pragma omp critical(BlockIO)
	{

		m_options.m_timerWrite.Start();


		size_t nbScenes = data[0][0].size();
		CGeoExtents extents = bandHolder.GetExtents();
		CGeoSize blockSize = extents.GetBlockSize(xBlock, yBlock);
		CGeoRectIndex outputRect = extents.GetBlockRect(xBlock, yBlock);
		assert(bandHolder.GetNbScenes() == bandHolder.GetPeriod().GetNbYears());


		ASSERT(outputRect.Width() == blockSize.m_x);
		ASSERT(outputRect.Height() == blockSize.m_y);

		if (m_options.m_bCreateImage)
		{
			__int32 noDataOut = (__int32)outputDS[0].GetNoData(0);
			for (size_t i = 0; i < outputDS.size(); i++)
			{
				ASSERT(data.size() == blockSize.m_x);
				ASSERT(data[0].size() == blockSize.m_y);
				ASSERT(outputDS[i].GetRasterCount() == NB_OUTPUT_BANDS);

				for (size_t b = 0; b < outputDS[i].GetRasterCount(); b++)
				{
					GDALRasterBand *pBand = outputDS[i].GetRasterBand(b);

					vector<__int32> output(blockSize.m_x*blockSize.m_y, noDataOut);
					for (int y = 0; y < blockSize.m_y; y++)
					{
						for (int x = 0; x < blockSize.m_x; x++)
						{
							int xy = int(y*blockSize.m_x + x);
							size_t z = data[x][y].GetDisturbanceIndex(i);

							switch (b)
							{
							case O_FIRST_DATE:	output[xy] = m_options.GetTRefIndex(data[x][y].GetFirstTRef()); break;
							case O_DISTURBANCE:	if (z != UNKNOWN_POS) output[xy] = data[x][y].GetDTCode(z); break;
							case O_DATE1:		if (z != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][z].GetTRef()); break;
							case O_DATE2:		if (z != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][z + 1].GetTRef()); break;
							case O_LAST_DATE:	output[xy] = m_options.GetTRefIndex(data[x][y].GetLastTRef()); break;
							default: ASSERT(FALSE);
							}

						}//x
					}//y

					pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Int32, 0, 0);
					//pBand->FlushCache();
				}//for all output bands
			}//for all disturbance
		}//if create image


		if (m_options.m_bFireSeverity)
		{
			__int16 noDataIn = (__int16)::GetDefaultNoData(GDT_Int16);
			float noDataOut = (float)::GetDefaultNoData(GDT_Int32);


			for (size_t b = 0; b < NB_FIRE_SEVERITY; b++)
			{
				GDALRasterBand *pBand = fireSeverityDS.GetRasterBand(b);
				vector<float> output(blockSize.m_x*blockSize.m_y, noDataOut);

				for (int y = 0; y < blockSize.m_y; y++)
				{
					for (int x = 0; x < blockSize.m_x; x++)
					{
						if (data[x][y].IsInit())
						{
							for (size_t z = 0; z < data[x][y].GetNbDTCode(); z++)
							{

								if (data[x][y][z].IsInit() &&
									data[x][y].GetDTCode(z) == FIRE_CODE &&
									data[x][y][z][E_B4] != noDataIn &&
									data[x][y][z][E_B7] != noDataIn &&
									data[x][y][z + 1][E_B4] != noDataIn &&
									data[x][y][z + 1][E_B7] != noDataIn)
								{
									static const double _a_[2] = { 0.22, 0.311 };
									static const double _b_[2] = { 0.09, 0.019 };

									double NBR1 = data[x][y][z].NBR();
									double NBR2 = data[x][y][z + 1].NBR();
									double dNBR = NBR1 - NBR2;

									//Genreal Saturated Growth
									double CBI_Ron = dNBR / (_a_[FS_RON] * dNBR + _b_[FS_RON]);
									double CBI_Jo = dNBR / (_a_[FS_JO] * dNBR + _b_[FS_JO]);

									double fireSeverity = 0;
									switch (b)
									{
									case FS_RON: fireSeverity = CBI_Ron; break;
									case FS_JO:	fireSeverity = CBI_Jo; break;
									case FS_MEAN:	fireSeverity = (CBI_Ron + CBI_Jo) / 2; break;
									default: ASSERT(false);
									}

									output[y*blockSize.m_x + x] = (float)fireSeverity;
								}//il all is presnet????
							}//for all disterbance
						}//if disturbance
					}//x
				}//y

				pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Float32, 0, 0);
				//pBand->FlushCache();
			}//all model (3)

		}//stats

		if (m_options.m_bExportBands)
		{
			__int16 noData = (__int16)::GetDefaultNoData(GDT_Int16);
			for (size_t i = 0; i < exportBandsDS.size(); i++)
			{
				ASSERT(exportBandsDS[i].GetRasterCount() == NB_EXPORT_BANDS*NB_EXPORT_TEMPORAL);

				for (size_t t = 0; t < NB_EXPORT_TEMPORAL; t++)
				{
					for (size_t b = 0; b < NB_EXPORT_BANDS; b++)
					{
						GDALRasterBand *pBand = exportBandsDS[i].GetRasterBand(t*NB_EXPORT_BANDS + b);

						vector<__int16> exportBands(blockSize.m_x*blockSize.m_y, noData);
						for (int y = 0; y < blockSize.m_y; y++)
						{
							for (int x = 0; x < blockSize.m_x; x++)
							{
								size_t z = data[x][y].GetDisturbanceIndex(i);
								if (z < data[x][y].size())
								{
									int tt = int(t) - 2;
									size_t Tindex = data[x][y].GetIndex(z, tt);

									if (Tindex < data[x][y].size())
									{
										exportBands[y*blockSize.m_x + x] = data[x][y][Tindex][EXPORT_BANDS[b]];
									}
								}
							}
						}

						pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(exportBands[0]), outputRect.Width(), outputRect.Height(), GDT_Int16, 0, 0);
						//pBand->FlushCache();
					}//for all bands in a scene
				}//for T1 and T2
			}//for all disturbance
		}//export bands


		if (m_options.m_bExportTimeSeries)
		{
			char noDataOut = (char)::GetDefaultNoData(GDT_Byte);

			for (size_t b = 0; b < exportTSDS.GetRasterCount(); b++)
			{
				GDALRasterBand *pBand = exportTSDS.GetRasterBand(b);
				int year = bandHolder.GetPeriod().Begin().GetYear() + int(b);

				vector<byte> output(blockSize.m_x*blockSize.m_y, noDataOut);
				for (int y = 0; y < blockSize.m_y; y++)
				{
					for (int x = 0; x < blockSize.m_x; x++)
					{
						size_t z = data[x][y].FindYear(year);
						if (z != NOT_INIT)
						{
							int xy = y*blockSize.m_x + x;
							output[xy] = (char)data[x][y].GetDTCode(z);
							if (output[xy] == 0)
								output[xy] = noDataOut;
						}
					}//x
				}//y

				pBand->RasterIO(GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Byte, 0, 0);
			}//for all debug variable
		}//time series

		if( m_options.m_bDebug )
		{
			__int32 noDataOut = (__int32)::GetDefaultNoData(GDT_Int32);

			for(size_t b=0; b<NB_DEBUGS; b++)
			{
				GDALRasterBand *pBand = debugDS.GetRasterBand(b);

				vector<__int32> output(blockSize.m_x*blockSize.m_y, noDataOut);
				for(int y=0; y<blockSize.m_y; y++)
				{
					for(int x=0; x<blockSize.m_x; x++)
					{
						if( data[x][y].IsInit() )
						{
							size_t z° = data[x][y].GetFirstDisturbanceIndex();
							size_t zⁿ = data[x][y].GetLastDisturbanceIndex();
							int xy = y*blockSize.m_x+x;
							switch(b)
							{
							case D_NB_PAIRS:			output[xy] = (int)data[x][y].size(); break;
							case D_NB_DISTURBANCES:		output[xy] = (int)data[x][y].GetNbDisturbances(); break;
							case D_FIRST_DISTURBANCE:	if (z° != UNKNOWN_POS) output[xy] = data[x][y].GetDTCode(z°); break;
							case D_F_DATE1:				if (z° != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][z°].GetTRef()); break;
							case D_F_DATE2:				if (z° != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][z° + 1].GetTRef()); break;
							case D_LAST_DISTURBANCE:	if (zⁿ != UNKNOWN_POS) output[xy] = data[x][y].GetDTCode(zⁿ); break;
							case D_L_DATE1:				if (zⁿ != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][zⁿ].GetTRef()); break;
							case D_L_DATE2:				if (zⁿ != UNKNOWN_POS) output[xy] = m_options.GetTRefIndex(data[x][y][zⁿ + 1].GetTRef()); break;
							//case D_MEAN_D:				output[xy] = (int)(data[x][y].GetMeanDistance()*m_options.m_NBRFactor); break;
							//case D_MAX1:				output[xy] = (int)(data[x][y].GetMaxDistance(0)*m_options.m_NBRFactor); break;
							//case D_MAX2:				output[xy] = (int)(data[x][y].GetMaxDistance(1)*m_options.m_NBRFactor); break;
							//case D_MAX3:				output[xy] = (int)(data[x][y].GetMaxDistance(2)*m_options.m_NBRFactor); break;
							default: ASSERT(FALSE);
							}
						}
					}//x
				}//y
				
				pBand->RasterIO( GF_Write, outputRect.m_x, outputRect.m_y, outputRect.Width(), outputRect.Height(), &(output[0]), outputRect.Width(), outputRect.Height(), GDT_Int32, 0, 0  );
				//pBand->FlushCache();
			}//for all debug variable
		}//debug
	}
	
	m_options.m_timerWrite.Stop(); 
	
}

void CDisterbanceAnalyser::CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& physicalDS, CGDALDatasetEx& maskDS, vector<CGDALDatasetEx>& outputDS, CGDALDatasetEx& fireSeverityDS, vector<CGDALDatasetEx>& exportBandsDS, CGDALDatasetEx& exportTSDS, CGDALDatasetEx& debugDS)
{
	if( !m_options.m_bQuiet )		
		_tprintf("\nClose all files...\n");

	landsatDS.Close();
	physicalDS.Close();
	maskDS.Close(); 

	//close output
	m_options.m_timerWrite.Start();
	for(size_t i=0; i<outputDS.size(); i++)
	{
		if( m_options.m_bComputeStats )
			outputDS[i].ComputeStats(i==0?m_options.m_bQuiet:true);
		if( !m_options.m_overviewLevels.empty() )
			outputDS[i].BuildOverviews(m_options.m_overviewLevels, i==0?m_options.m_bQuiet:true);
		outputDS[i].Close();
	}

	if( m_options.m_bComputeStats )
		fireSeverityDS.ComputeStats(true);
	if( !m_options.m_overviewLevels.empty() )
		fireSeverityDS.BuildOverviews(m_options.m_overviewLevels, true);
	fireSeverityDS.Close();

	for(size_t i=0; i<exportBandsDS.size(); i++)
	{
		if( m_options.m_bComputeStats )
			exportBandsDS[i].ComputeStats(true); 
		if( !m_options.m_overviewLevels.empty() )
			exportBandsDS[i].BuildOverviews(m_options.m_overviewLevels, true);
		exportBandsDS[i].Close();
	}


	if (m_options.m_bComputeStats)
		exportTSDS.ComputeStats(true);
	if (!m_options.m_overviewLevels.empty())
		exportTSDS.BuildOverviews(m_options.m_overviewLevels, true);
	exportTSDS.Close();

	
	if (m_options.m_bComputeStats)
		debugDS.ComputeStats(true);
	if (!m_options.m_overviewLevels.empty())
		debugDS.BuildOverviews(m_options.m_overviewLevels, true);
	debugDS.Close();
	

		
	m_options.m_timerWrite.Stop();
		
	if( !m_options.m_bQuiet )
	{
		double percent = m_options.m_nbPixel>0?(double)m_options.m_nbPixelDT/m_options.m_nbPixel*100:0;

		_tprintf ("\n");
		_tprintf ("Percentage of pixel treated by DecisionTree: %0.3lf %%\n\n", percent);
	}

	m_options.PrintTime();
}

void CDisterbanceAnalyser::LoadData(const CBandsHolder& bandHolder1, const CBandsHolder& bandHolder2, vector< vector< CDADVector>>& data)
{
	CRasterWindow landsat = bandHolder1.GetWindow();
	CRasterWindow physical = bandHolder2.GetWindow();

	CGeoSize blockSize = landsat.GetGeoSize();
	data.resize(blockSize.m_x);
	for (int x = 0; x<blockSize.m_x; x++)
	{
		data[x].resize(blockSize.m_y);
		for (int y = 0; y < blockSize.m_y; y++)
		{
			//physic
			for (size_t z = 0; z < physical.size(); z++)
				data[x][y].m_physical[z] = physical[z]->at(x, y);

			size_t nbScenes = landsat.GetNbScenes();
			data[x][y].reserve(nbScenes);
			for (size_t z = 0; z < landsat.GetNbScenes(); z++)
			{
				CLandsatPixel pixel = ((CLandsatWindow&)landsat).GetPixel(z, x, y);
				//if (pixel.IsInit())
				if (pixel.IsValid())
				{
					data[x][y].push_back(pixel);
				}
			}

			data[x][y].Despike(m_options.m_despike);
			data[x][y].InitDTCode();

		}
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	CTimer timer(true);
	
	CDisterbanceAnalyser regressionTree;
	ERMsg msg = regressionTree.m_options.ParseOption(argc, argv);

	if( !msg || !regressionTree.m_options.m_bQuiet )
		cout << regressionTree.GetDescription() << endl ;


	if( msg )  
		msg = regressionTree.Execute();

	if( !msg)  
	{
		PrintMessage(msg);
		return -1;
	}

	timer.Stop();

	if( !regressionTree.m_options.m_bQuiet )
		cout << endl << "Total time = " << SecondToDHMS(timer.Elapsed()) << endl;

	int nRetCode = 0;
}


