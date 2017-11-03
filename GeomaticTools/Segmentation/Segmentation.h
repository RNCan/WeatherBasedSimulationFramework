//***********************************************************************
#pragma once

//#include "boost\dynamic_bitset.hpp"
#include "Basic/UtilTime.h"
#include "Basic/Mtrx.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"


namespace WBSF
{
	
	class CSegmentationOption : public CBaseOptions
	{
	public:

		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		
		CSegmentationOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		

		double m_RMSEThreshold;
		size_t m_maxBreaks;
	};

	typedef std::deque < std::vector< __int16>> OutputData;
	typedef std::pair<__int16, size_t> NBRPair;
	typedef std::vector<NBRPair> NBRVector;

	class CSegmentation
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("Segmentation version ") + VERSION + " (" + __DATE__ + ")"; }

		ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

		CSegmentationOption m_options;

		static const char* VERSION;
		static const size_t NB_THREAD_PROCESS;

		static std::pair<double, size_t> ComputeRMSE(const NBRVector& data, size_t i);
		static std::vector<size_t> Segmentation(const NBRVector& data, size_t max_nb_seg, double max_error);

	};
}