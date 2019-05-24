//***********************************************************************


#include "Geomatic/GDALBasic.h"
#include "Basic/UtilTime.h"
#include "Simulation/ATM.h"

namespace WBSF
{
	class CLandsatCloudCleaner;

	class CConvertNetCDF2GeoTIFFOption : public CBaseOptions
	{
	public:

		//enum { ATM_WNDS = NB_ATM_VARIABLES_EX, ATM_WNDD, NB_ATM_VARIABLES_EX2 };
		//enum TOutputType  { O_IMAGES, O_CSV, NB_OUTPUT_TYPE};
		enum TFilePath	  { INPUT_PATH, OUTPUT_PATH, NB_FILE_PATH };
		
		CConvertNetCDF2GeoTIFFOption();
		CConvertNetCDF2GeoTIFFOption(const CConvertNetCDF2GeoTIFFOption& in):
			CBaseOptions(in)
		{
		}

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);


		//std::vector<double> m_levels;
		//std::vector<size_t> m_variables;
		//std::vector<__int64> m_times;
		//std::string m_locations_file_path;
		//size_t m_time_step;
		
		CProjectionTransformation toWea;
		CProjectionTransformation toGeo;
	};


	class CConvertNetCDF2GeoTIFF
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("ConvertNetCDF2GeoTIFF version ") + VERSION + " (" + __DATE__ + ")"; }
		
		CConvertNetCDF2GeoTIFFOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}