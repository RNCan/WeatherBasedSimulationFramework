//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <float.h>
#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"
#include "Basic/Statistic.h"
#include "Basic/Timer.h"
#include "Basic/GeoBasic.h"
#include "Basic/UtilTime.h"

class GDALDataset;
class GDALRasterBand;
class GDALDriver;

namespace WBSF
{
	
	class CGDALDatasetEx;
	class CGDALDatasetExVector;

	//General function
	void RegisterGDAL();
	ERMsg OpenInputImage(const std::string& filePath, GDALDataset** poInputDS, double srcNodata = MISSING_NO_DATA, bool bUseDefaultNoData = true, bool bReadOnly= true);
	ERMsg OpenOutputImage(const std::string& filePath, GDALDataset* poInputDS, GDALDataset** poOutputDS, const char* outDriverName = "GTiff", short cellType = 0, int nbBand = -1, double dstNodata = MISSING_NO_DATA, const StringVector& createOptions = StringVector(), bool bOverwrite = true, const CGeoExtents& extentsIn = CGeoExtents(), bool useDefaultNoData = true);
	bool GoodGeotiff(const std::string& filePath);
	
	ERMsg GetFilePathList(const char* fileName, int filePathCol, StringVector& filePathList);
	ERMsg VerifyInputSize(GDALDataset* poInputDS1, GDALDataset* poInputDS2);
	CGeoExtents GetExtents(GDALDataset* poDataset);

	ERMsg VerifyNoData(double nodata, short eType);
	double GetNoData(GDALRasterBand* pBand);
	double GetDefaultNoData(short eType);
	double GetTypeLimit(short eType, bool bLow);
	double LimitToBound(double v, short cellType, double shiftedBy = 0, bool bRound = true);
	void PosToCoord(double* GT, int Xpixel, int Yline, double& Xgeo, double& Ygeo);
	void CoordToPos(double* GT, double Xgeo, double Ygeo, int& Xpixel, int& Yline);
	CStatistic GetWindowMean(GDALRasterBand* pBand, int x1, int y1, int x2, int y2);
	double GetWindowMean(GDALRasterBand* pBand, int nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d);

	std::string GetGDALFilter(bool bMustSupportCreate);
	void GetGDALDriverList(StringVector& GDALList, bool bMustSupportCreate);
	GDALDriver* GetDriverFromExtension(const std::string& extIn, bool bMustSupportCreate);
	std::string GetDriverExtension(const std::string& formatName);
	ERMsg GetGDALInfo(const std::string& filePath, CNewGeoFileInfo& info);

	void Close(GDALDataset* poDS);
	void PrintMessage(ERMsg msg);

	typedef float DataType;
	static const float DataTypeMin = -FLT_MAX;
	typedef std::vector<DataType> DataVector;
	typedef int MaskDataType;
	typedef std::vector<MaskDataType> MaskDataVector;
	typedef DataVector::const_iterator DataIteratorConst;



	//******************************************************************************************************
	class COptionDef
	{
	public:

		const char* m_name;
		int m_nbArgs;
		const char* m_args;
		bool m_bMulti;
		const char* m_help;

		std::string GetHelp()const
		{
			std::string help;

			std::string cmdLine = std::string("  ") + m_name + (m_bMulti ? "*" : "") + (m_nbArgs > 0 ? " " : "") + m_args + ":";

			std::string strHelp = m_help;
			std::string::size_type pos = 0;
			while (pos != std::string::npos)
			{
				std::string word = Tokenize(strHelp, " ", pos);

				if (cmdLine.length() + word.length() + 1 < 80)
				{
					cmdLine += " " + word;
				}
				else
				{
					help += cmdLine + "\n";
					cmdLine = "      " + word;//begin a new line
				}
			}

			help += cmdLine;

			return help;
		}


	};
	//****************************************************************************************************

	typedef std::vector< COptionDef > COptionDefVector;
	typedef std::map< std::string, COptionDef > COptionDefMap;

	class CIOFileInfoDef
	{
	public:

		const char* m_description;
		const char* m_name;
		const char* m_nFiles;
		const char* m_nbBands;
		const char* m_help;//help of bands separate by |
		const char* m_note;//

		std::string GetHelp()const
		{
			std::string help;


			help = std::string("  ") + m_description + ": " + m_name + "\n";

			if (m_nFiles && strlen(m_nFiles) > 0)
			{
				std::string cmdLine = std::string("    Number of files: ") + m_nFiles + "\n";
				help += cmdLine;
			}

			if (m_nbBands && strlen(m_nbBands) > 0)
			{
				std::string cmdLine = std::string("    Number of bands (columns): ") + m_nbBands + "\n";
				help += cmdLine;
			}


			if (m_help && strlen(m_help) > 0)
			{
				std::string cmdLine;
				std::string strHelp = m_help;



				int bandNo = 0;
				std::string::size_type bandInfoPos = 0;
				while (bandInfoPos != std::string::npos)
				{
					std::string bandInfo = Tokenize(strHelp, "|", bandInfoPos);
					Trim(bandInfo);

					if (!bandInfo.empty())
					{
						bandNo++;
						cmdLine = std::string("    ") + ToString(bandNo) + "-";


						std::string::size_type pos = 0;
						while (pos != std::string::npos)
						{
							std::string word = Tokenize(bandInfo, " ", pos);

							if (cmdLine.length() + word.length() + 1 < 80)
							{
								cmdLine += " " + word;
							}
							else
							{
								help += cmdLine + "\n";
								cmdLine = "          " + word;//begin a new line
							}
						}

						help += cmdLine + "\n";
					}
				}
			}

			if (m_note && strlen(m_note) > 0)
			{
				std::string cmdLine;
				std::string note = m_note;
				if (!note.empty())
				{
					cmdLine += "    Note:";
					std::string::size_type pos = 0;
					while (pos != std::string::npos)
					{
						std::string word = Tokenize(note, " ", pos);

						if (cmdLine.length() + word.length() + 1 < 80)
						{
							cmdLine += " " + word;
						}
						else
						{
							help += cmdLine + "\n";
							cmdLine = "          " + word;//begin a new line
						}
					}

					help += cmdLine + "\n";
				}
			}

			return help;
		}


	};

	typedef std::vector< CIOFileInfoDef > CIOFileInfoDefVector;

	//*****************************************************************************************************
	//CBaseOptions : parse base applications option 
	class CBaseOptions
	{
	public:

		enum TTemporalRef { JDAY1970, YYYYMMDD, NB_SOURCES };
		static const char* TEMPORAL_REF_NAME[NB_SOURCES];
		static int GetTRefIndex(int type, CTRef Tref);
		static CTRef GetTRef(int type, int index);
		
		enum TRGBTye{ NO_RGB=-1, NATURAL, LANDWATER, TRUE_COLOR, NB_RGB };
		static const char* RGB_NAME[NB_RGB];

		enum TTT			{ TT_UNKNOWN = -1, TT_OVERALL_YEARS, TT_BY_YEARS, TT_BY_MONTHS, TT_NONE, NB_TT };
		static const char* TT_TYPE_NAME[NB_TT];
		static short GetTTType(const char* str);

		static const COptionDef OPTIONS_DEF[];
		static const char* DEFAULT_OPTIONS[];
		static int GetOptionIndex(const char* name);

		CBaseOptions(bool bAddDefaultOption = true);
		//CBaseOptions(const CGDALDatasetEx& inputDS){UpdateOption(inputDS);}

		void Reset();
		void AddOption(const char* name);
		void AddOption(const COptionDef& optionsDef);
		void AddIOFileInfo(const CIOFileInfoDef& fileDef);
		void RemoveOption(const char* name);

		ERMsg ParseOptions(int argc, TCHAR* argv[]);
		ERMsg ParseOptions(const std::string& str);

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		virtual std::string GetUsage()const;
		virtual std::string GetHelp()const;
		virtual std::string GetIOFileInfo()const;
		//virtual void UpdateOption(const CGDALDatasetEx& inputDS);
		int GetTRefIndex(CTRef TRef)const{ return TRef.IsInit() ? GetTRefIndex(m_TTF, TRef) : int(m_dstNodata); }
		CTRef GetTRef(int index)const{ return GetTRef(m_TTF, index); }
		bool IsVRT()const { return IsEqualNoCase(m_format, "VRT"); }
		int BLOCK_CPU()const { return std::max(1, int(m_CPU / m_BLOCK_THREADS) ); }

		bool m_bReadOnly;
		std::string m_format;
		short m_outputType;
		StringVector m_createOptions;
		StringVector m_workOptions;
		bool m_bUseDefaultNoData;
		double m_srcNodata;
		double m_dstNodata;
		double m_dstNodataEx;
		double m_memoryLimit;
		CGeoExtents m_extents;
		double m_xRes;
		double m_yRes;
		CTPeriod m_period;
		CTM m_TM;
		TRGBTye m_RGBType; 
		double m_iFactor;

		StringVector m_filesPath;
		std::vector<int> m_bandsToUsed;
		size_t m_nbBands;
		std::string m_prj;
		std::string m_maskName;
		float m_maskDataUsed;
		int m_CPU;
		int m_IOCPU;
		int m_BLOCK_THREADS;

		bool m_bMulti;
		bool m_bOverwrite;
		bool m_bQuiet;
		bool m_bNeedHelp;
		bool m_bNeedUsage;
		bool m_bNeedIOFileInfo;
		bool m_bVersion;
		bool m_bExtents;
		bool m_bRes;
		bool m_bTap;
		bool m_bResetJobLog;
		std::string m_jobLogName;
		bool m_bCreateImage;
		std::vector<int> m_overviewLevels;
		bool m_bComputeStats;
		bool m_bComputeHistogram;
		bool m_bRemoveEmptyBand;
		std::string m_rename;
			
		int m_TTF; //temporal type format for temporal dataset
		int m_scenesSize;
		bool m_bOpenBandAtCreation;
		std::string m_VRTBandsName;


		CGeoExtents GetExtents()const{ return m_extents; }
		void SetExtents(const CGeoExtents& extents)	{ m_extents = extents; }

		CTPeriod GetTTPeriod()const;
		CTPeriod GetTTSegment(size_t s);

		//common timer used
		CTimer m_timerRead;
		CTimer m_timerProcess;
		CTimer m_timerWrite;

		static size_t m_xxFinal;
		static size_t m_xx;
		static size_t m_xxx;



		void PrintTime();

		void ResetBar(size_t xxFinal)
		{
			m_xxFinal = xxFinal;
			m_xx = 0;
			m_xxx = 0;

		}

		void UpdateBar();



	protected:

		std::string m_appDescription;
		COptionDefVector m_mendatoryDef;
		COptionDefMap m_optionsDef;
		CIOFileInfoDefVector m_IOFileInfo;
	};

}