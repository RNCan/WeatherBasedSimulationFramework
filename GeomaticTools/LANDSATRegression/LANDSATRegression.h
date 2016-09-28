#pragma once

#include "stdafx.h"
#include <float.h>
#include <math.h>
#include <array>
#include <utility>
#include <iostream>
#include <bitset>

#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Basic/OpenMP.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"


namespace WBSF
{
	//**********************************************************************************************************************************************
	enum TRegression{ O_FIVE, O_BEGIN_END, O_ALL, NB_OUTPUT_REGRESSION };
	enum TRegressionStat{ S_N, S_SLOPE, S_RMSE, S_R², NB_REGRESSION_STATS };

	//static char* REG_OUTPUT_NAME[NB_OUTPUT_REGRESSION];
	//static size_t REG_STATS[NB_REGRESSION_STATS];

	typedef std::array<std::array<std::vector<float>, NB_REGRESSION_STATS>, NB_OUTPUT_REGRESSION> RegressionVector;

	class CLANDSATRegressionOptions : public CBaseOptions
	{
	public:



		CIndiciesVector m_despike;

		double m_RFactor;
		std::string m_beginFilePath;
		std::string m_endFilePath;

		CLANDSATRegressionOptions();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
	};



	//**********************************************************************************************************************************************
	class CLANDSATRegression
	{
	public:

		enum TFilePath { INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };



		std::string GetDescription();
		ERMsg Execute();


		ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& beginDS, CGDALDatasetEx& endDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, CGDALDatasetEx& beginDS, CGDALDatasetEx& endDS, RegressionVector& data);
		void WriteBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, RegressionVector& data, CGDALDatasetEx& outputDS);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& beginDS, CGDALDatasetEx& endDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);



		CLANDSATRegressionOptions m_options;
	};

}