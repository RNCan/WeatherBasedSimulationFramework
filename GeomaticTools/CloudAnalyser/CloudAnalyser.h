#pragma once

//#include "stdafx.h"
//#include <float.h>
//#include <math.h>
//#include <array>
//#include <utility>
//#include <iostream>
//#include <bitset>
//#include <boost/dynamic_bitset.hpp>
//
//#include "Basic/UtilTime.h"
//#include "Basic/UtilMath.h"
//#include "Basic/OpenMP.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/See5hooks.h"
#include "Geomatic/LandsatDataset.h"
//
namespace WBSF
{
//	class CDecisionTreeBaseEx;
	//class CDecisionTree;
	typedef std::vector<CLandsatPixel>CLandsatPixelVector;


	class CCloudAnalyserOption : public CBaseOptions
	{
	public:

		enum TFilePath { DT_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };


		CCloudAnalyserOption();
		
		virtual ERMsg ParseOption(int argc, char* argv[])
		{
			ERMsg msg = CBaseOptions::ParseOption(argc, argv);

			ASSERT(NB_FILE_PATH == 4);
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

			/*	if (IsEqual(argv[i], "-Trigger"))
				{
				string str_domain = argv[++i];
				TDomain domain = GetIndiceDomain(str_domain);
				string str_type = argv[++i];
				TIndices type = GetIndiceType(str_type);
				string op = argv[++i];
				double threshold = atof(argv[++i]);

				if (type != I_INVALID)
				{
				if (type != I_INVALID)
				{
				if (CIndices::IsValidOp(op))
				m_trigger.push_back(CIndices(domain, type, op, threshold));
				else
				msg.ajoute(op + " is an invalid operator for -Trigger option");
				}
				else
				{
				msg.ajoute(str_type + " is an invalid type for -Trigger option");
				}
				}
				else
				{
				msg.ajoute(str_domain + " is an invalid domain for -Trigger option");
				}


				}*/
			/*else if (IsEqual(argv[i], "-Despike"))
			{
			string str = argv[++i];
			TIndices type = GetIndicesType(str);
			string op = argv[++i];
			double threshold = atof(argv[++i]);


			if (type != I_INVALID)
			{
			if (CIndices::IsValidOp(op))
			m_despike.push_back(CIndices(type, op, threshold));
			else
			msg.ajoute(op + " is an invalid operator for -Despike option");

			}
			else
			{
			msg.ajoute(str + " is an invalid type for -Despike option");
			}
			}
			else */
			if (IsEqual(argv[i], "-B1"))
			{
				m_B1threshold = atof(argv[++i]);
			}
			else if (IsEqual(argv[i], "-TCB"))
			{
				m_TCBthreshold = atof(argv[++i]);
			}
			/*else if (IsEqual(argv[i], "-Debug"))
			{
			m_bDebug=true;
			}*/
			else
			{
				//Look to see if it's a know base option
				msg = CBaseOptions::ProcessOption(i, argc, argv);
			}

			return msg;
		}


		bool IsTrigged(std::array <CLandsatPixel, 3>& p)
		{
			bool t1 = p[0][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold;
			bool t2 = p[2][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold;
			bool t3 = p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold;
			bool t4 = p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold;
			
			return t1&&t2&&t3&&t4;
		}
		//bool m_bByYear;

		//bool m_bAllBands;


		//CIndiciesVector m_trigger;
		//CIndiciesVector m_despike;

		//bool m_bExportBands;
		//bool m_bExportTimeSeries;
		//bool m_bFireSeverity;
		//bool m_bDebug;
		//int	 m_nbDisturbances;

		double m_B1threshold;
		double m_TCBthreshold;


		__int64 m_nbPixelDT;
		__int64 m_nbPixel;
	};

	class CCloudAnalyser
	{
	public:

		std::string GetDescription();
		ERMsg Execute();


		ERMsg OpenAll(CGDALDatasetEx& lansatDS, CGDALDatasetEx& maskDS, std::vector<CGDALDatasetEx>& outputDS);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, CDecisionTree& DT, std::vector< std::vector< CLandsatPixelVector >>& data, std::vector< std::vector< std::vector<short> >>& output);
		void WriteBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, const std::vector< std::vector< std::vector<short> >>& output, std::vector<CGDALDatasetEx>& outputDS);
		void CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, std::vector<CGDALDatasetEx>& outputDS);

		//void Evaluate(int x, int y, const std::vector<std::array<short, 3>>& DTCode, std::vector<std::vector<std::vector<short>>>& output);
		void LoadData(const CBandsHolder& bandHolder, std::vector< std::vector< CLandsatPixelVector >>& data);
		ERMsg ReadRules(CDecisionTree& DT);

		CCloudAnalyserOption m_options;

		//static int FindIndex(int start, const std::vector<short>& bandsvalue, int dir);
		static void LoadModel(CDecisionTreeBaseEx& DT, std::string filePath);
		//static short DTCode2ChangeCodeIndex(short DTCode);
		//static short DTCode2ChangeCode(short DTCode);

		static CDecisionTreeBlock GetDataRecord(std::array<CLandsatPixel, 3> p, CDecisionTreeBaseEx& DT);
	};

}