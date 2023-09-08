//***********************************************************************


#include "Basic/UtilTime.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset1.h"


namespace WBSF
{


	class CHorizonImageOption : public CBaseOptions
	{
	public:

		
		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		CHorizonImageOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		
		//double m_start;
		//double m_end;
		//double m_by;
		//bool m_bRadian;//in radian by default
		size_t m_nAngles;
		double m_maxDistance;
		bool m_bSVF;
		
		size_t GetNbAngles()const { return m_nAngles; }
		//size_t GetNbAngles()const { return size_t(m_by == 0 ? 1 : std::floor((m_end - m_start) / m_by)); }
		//double GetAngles(size_t a)const { return m_start + a*m_by; }
	};

	typedef float OutputDataType;
	typedef std::deque < std::vector<OutputDataType> > OutputData;
	typedef std::vector<OutputDataType> CHorizon;

	class CEllipsoidProperties
	{
	public:
		double semiMajorAxis;
		double eccentricity;
		//		double flattening;
	//	double semiMinorAxis;


		CEllipsoidProperties(std::string model = "WGS84");
		CEllipsoidProperties(double semiMajorAxis, double flattening);

		double getAzimuth(const CGeoPoint& pt1, const CGeoPoint& pt2)const { return getAzimuth(pt1.m_lat, pt1.m_lon, pt2.m_lat, pt2.m_lon); }
		double getElevationAngle(const CGeoPoint3D& pt1, const CGeoPoint3D& pt2)const { return getElevationAngle(pt1.m_z, pt2.m_z, pt1.m_lat, pt2.m_lat, pt1.m_lon, pt2.m_lon); }

		double getElevationAngle(double h_A, double h_B, double latitude_A, double latitude_B, double longitude_A, double longitude_B)const;
		double getAzimuth(double lat1, double lon1, double lat2, double lon2)const;
		double getIsometricLatitude(double latitude)const;
		CGeoPoint3D geographic2cartesian(double latitude, double longitude, double altitude)const;
		CGeoDistance dist2deg(double distance, const CGeoPoint& location)const;
		double geodesic(double lat1, double lat2, double deltaLon)const;
		double localSphereRadius(double latitude)const;
	};



	class CHorizonImage
	{
	public:



		ERMsg Execute();

		std::string GetDescription();
		void AllocateMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData);

		ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);
		
		//CHorizon ComputeHorizon(const CGeoPoint& location, CGDALDatasetEx& DEM, double distance = 50, size_t precision = 1, const std::string& ellipsoidModel = "WGS84");
		CHorizon ComputeHorizon(CGeoPoint location, CGDALDatasetEx& DEM, double distance=-1, double precision=45, const std::string& ellipsoidModel = "WGS84");


		CHorizonImageOption m_options;


//		static std::vector<size_t> histc(std::vector<double> az, std::vector<double> azimuth, int a, int b, int c, int d);
		static const char* VERSION;
	};

}