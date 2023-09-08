//***********************************************************************
#pragma once

//#include "boost\dynamic_bitset.hpp"
#include "Basic/UtilTime.h"
#include "Basic/Mtrx.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset1.h"


namespace WBSF
{
	
	class CSegmentationOption : public CBaseOptions
	{
	public:

		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		//enum TDebug {D_NB_NBR, D_};
		
		CSegmentationOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		

		double m_RMSEThreshold;
		size_t m_maxLayers;
		int m_firstYear;
		//bool m_bExportBreaks;
		bool m_bDebug;
	};

	typedef std::deque < std::vector< __int16>> OutputData;
	typedef std::deque < std::vector< __int16>> DebugData;
	
	typedef std::pair<double, size_t> NBRPair;
	typedef std::vector<NBRPair> NBRVector;

	class CSegmentation
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("Segmentation version ") + VERSION + " (" + __DATE__ + ")"; }

		ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData, DebugData& debugData);
		void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, OutputData& outputData, DebugData& debugData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);

		CSegmentationOption m_options;

		static const char* VERSION;
		static const size_t NB_THREAD_PROCESS;

		static std::pair<double, size_t> ComputeRMSE(const NBRVector& data, size_t i);
		static std::vector<size_t> Segmentation(const NBRVector& data, size_t max_nb_seg, double max_error);

	};
}