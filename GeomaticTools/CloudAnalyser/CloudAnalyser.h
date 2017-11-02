#pragma once


#include "Geomatic/GDALBasic.h"
#include "Geomatic/See5hooks.h"
#include "Geomatic/LandsatDataset.h"
//
namespace WBSF
{
	typedef std::vector<CLandsatPixel>CLandsatPixelVector;


	class CCloudAnalyserOption : public CBaseOptions
	{
	public:

		enum TFilePath { DT_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };


		CCloudAnalyserOption();
		
		virtual ERMsg ParseOption(int argc, char* argv[])
		{
			ERMsg msg = CBaseOptions::ParseOption(argc, argv);

			ASSERT(NB_FILE_PATH == 3);
			if (msg && m_filesPath.size() != NB_FILE_PATH)
			{
				msg.ajoute("ERROR: Invalid argument line. 3 files are needed: decision tree model, the LANDSAT images series and the destination image.");
				msg.ajoute("Argument found: ");
				for (size_t i = 0; i < m_filesPath.size(); i++)
					msg.ajoute("   " + to_string(i + 1) + "- " + m_filesPath[i]);
			}

			return msg;
		}

		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		
		bool IsTrigged(std::array <CLandsatPixel, 3>& p)
		{
			bool t1 = p[0][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold;
			bool t2 = p[2][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold;
			bool t3 = p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold;
			bool t4 = p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold;
			
			return t1&&t2&&t3&&t4;
		}
	
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

		void LoadData(const CBandsHolder& bandHolder, std::vector< std::vector< CLandsatPixelVector >>& data);
		ERMsg ReadRules(CDecisionTree& DT);

		CCloudAnalyserOption m_options;
		
		static void LoadModel(CDecisionTreeBaseEx& DT, std::string filePath);
		
		static CDecisionTreeBlock GetDataRecord(std::array<CLandsatPixel, 3> p, CDecisionTreeBaseEx& DT);
	};

}