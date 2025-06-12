// Horizon.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "horizon.h"

//#include <GeographicLib/Geocentric.hpp>


#pragma warning( disable : 4251)
#include "gdal/Include/gdal.h"
#include "gdal/Include/gdal_priv.h"
#include "gdal/Include/ogr_spatialref.h"

typedef double ELEVATION_TYPE;

using namespace GeographicLib;
using namespace geodetic_converter;



std::string get_path()
{
	return "G:\\TravauxModels\\Horizon\\";
}






//Load SRTM digital elevation model data.
//Load SRTM digital elevation model data from single GeoTIFF file.

//Parameters
//----------
//file_dem : str
//Name of SRTM tile
//domain : dict
//Dictionary with domain boundaries(lon_min, lon_max, lat_min, lat_max)
//[degree]
//engine : str
//Backend for loading GeoTIFF file(either 'gdal' or 'pillow')

//Returns
//------ -
//lon : ndarray
//Array(one - dimensional) with longitude[degree]
//lat : ndarray
//Array(one - dimensional) with latitude[degree]
//elevation : ndarray
//Array(two - dimensional) with elevation[metre]

//Notes
//---- -
//Data source : https://srtm.csi.cgiar.org"""
matrix<pt3> load_dem(const std::string& DEM_file_path, const Domain& domain_outer)
{
	std::vector<double> dem_x;
	std::vector<double> dem_y;
	matrix<double> dem_z;

	std::cout << "Read GeoTIFF with GDAL";

	bool bReadOnly = true;
	unsigned int flag = bReadOnly ? GDAL_OF_READONLY | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR : GDAL_OF_UPDATE | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR;
	GDALDataset* poDataset = (GDALDataset*)GDALOpenEx(DEM_file_path.c_str(), flag, NULL, NULL, NULL);

	assert(poDataset->GetRasterCount() == 1);

	int raster_size_x = poDataset->GetRasterXSize();
	int raster_size_y = poDataset->GetRasterYSize();

	std::array<double, 6> gt;
	poDataset->GetGeoTransform(gt.data());

	//elevation = ds.GetRasterBand(1).ReadAsArray();  // 16 - bit integer

	double x_ulc = gt[0];
	double y_ulc = gt[3];
	double d_x = gt[1];
	double d_y = gt[5];

	// Warning: unclear where sign of n - s pixel resolution is stored!
	std::vector<double> x_edge(raster_size_x);
	for (size_t i = 0; i < x_edge.size(); i++)
	{
		x_edge[i] = x_ulc + (i + 0.5) * (d_x * raster_size_x) / x_edge.size();
	}

	//std::vector<double> y_edge(raster_size_y + 1);
	std::vector<double> y_edge(raster_size_y);
	for (size_t j = 0; j < y_edge.size(); j++)
	{
		y_edge[j] = y_ulc + (j + 0.5) * (d_y * raster_size_y) / y_edge.size();
	}

	// Crop relevant domain
	std::array<size_t, 2> i_min_max = { std::numeric_limits<size_t>::max(), 0 };

	dem_x.reserve(size_t((domain_outer.x_max - domain_outer.x_min) / d_x));
	for (size_t i = 0; i < x_edge.size(); i++)
	{
		if (x_edge[i] >= domain_outer.x_min && x_edge[i] <= domain_outer.x_max)
		{
			i_min_max[0] = std::min(i_min_max[0], i);
			i_min_max[1] = std::max(i_min_max[1], i);

			dem_x.push_back(x_edge[i]);
		}

		//slice_lon[i] = lon_ulc + i * (d_lon * raster_size_x) / lon_edge.size();
	}

	std::array<size_t, 2> j_min_max = { std::numeric_limits<size_t>::max(), 0 };
	//std::vector<double> lat;
	dem_y.reserve(size_t((domain_outer.y_max - domain_outer.y_min) / std::abs(d_y)));
	for (size_t j = 0; j < y_edge.size(); j++)
	{
		if (y_edge[j] >= domain_outer.y_min && y_edge[j] <= domain_outer.y_max)
		{
			j_min_max[0] = std::min(j_min_max[0], j);
			j_min_max[1] = std::max(j_min_max[1], j);

			dem_y.push_back(y_edge[j]);
		}

		//slice_lon[i] = lon_ulc + i * (d_lon * raster_size_x) / lon_edge.size();
	}

	assert(i_min_max[0] + dem_x.size() - 1 == i_min_max[1]);
	assert(j_min_max[0] + dem_y.size() - 1 == j_min_max[1]);

	//elevation = elevation[slice_lat, slice_lon].astype(np.float32)
	//lon, lat = lon[slice_lon], lat[slice_lat]




	GDALRasterBand* poBand = poDataset->GetRasterBand(1);
	dem_z.resize(dem_x.size(), dem_y.size());
	poBand->RasterIO(GF_Read, (int)i_min_max[0], (int)j_min_max[0], (int)dem_x.size(), (int)dem_y.size(), dem_z.data(), (int)dem_x.size(), (int)dem_y.size(), GDT_Float64, 0, NULL);

	matrix<pt3> DEM(dem_x.size(), dem_y.size());
	for (size_t j = 0; j < dem_y.size(); j++)
	{
		for (size_t i = 0; i < dem_x.size(); i++)
		{
			size_t ij = j * dem_x.size() + i;
			{
				DEM[ij].x = dem_x[i];
				DEM[ij].y = dem_y[j];
				DEM[ij].z = dem_z[ij];
			}
		}
	}

	return DEM;
}


//matrix<double> RectBivariateSpline(std::vector<double> lat, std::vector<double> lon, matrix<double> data, size_t kx = 1, size_t ky = 1)


//Compute surface normal unit vectors (for 1-dimensional data).

//Sources
//------ -
//-https ://en.wikipedia.org/wiki/N-vector

//pt3 surf_norm(double lon, double lat)
//{
//
//	// Compute surface normals
//	//for i in prange(len_0, nogil = True, schedule = "static") :
//	double sin_lon = sin(deg2rad(lon));
//	double cos_lon = cos(deg2rad(lon));
//	double sin_lat = sin(deg2rad(lat));
//	double cos_lat = cos(deg2rad(lat));
//
//
//	//pt3 norm_ecef;
//	pt3 norm_ecef;
//	norm_ecef[0] = cos_lat * cos_lon;
//	norm_ecef[1] = cos_lat * sin_lon;
//	norm_ecef[2] = sin_lat;
//
//	return norm_ecef;
//
//}
//
//
//pt3 surf_norm(const pt3& lla)
//{
//	return surf_norm(lla.x, lla.y);
//
//}
//
//pt3 north_dir(const pt3& ecef, const pt3& vec_norm_ecef, const char* ellps = "WGS84")
//{
//	//"""Compute unit vectors pointing towards North (for 1-dimensional data).
//
//	//Sources
//	//------ -
//	//-Geoid parameters r, a and f : PROJ"""
//
//	// Coordinates of North pole
//	double np_x = 0.0;
//	double np_y = 0.0;
//	double np_z = 0.0;
//	if (ellps == "sphere")
//	{
//		double r = 6370997.0;  // earth radius[m]
//		np_z = r;
//	}
//	else
//	{
//		double a = 6378137.0;  // equatorial radius(semi - major axis)[m]
//		//if ellps == "GRS80" :
//		//f = (1.0 / 298.257222101)  // flattening[-]
//		//else :  // WGS84
//		//f = (1.0 / 298.257223563)  // flattening[-]
//		double f = (ellps == "GRS80") ? (1.0 / 298.257222101) : (1.0 / 298.257223563);  // GRS80:WGS84
//		double b = a * (1.0 - f);  // polar radius(semi - minor axis)[m]
//		np_z = b;
//	}
//
//	// Coordinate transformation
//	//for i in prange(len_0, nogil = True, schedule = "static") :
//
//		// Vector to North Pole
//	double vec_nor_x = (np_x - ecef.x);
//	double vec_nor_y = (np_y - ecef.y);
//	double vec_nor_z = (np_z - ecef.z);
//
//	// Project vector to North Pole on surface normal plane
//	double dot_pr = ((vec_nor_x * vec_norm_ecef[0]) + (vec_nor_y * vec_norm_ecef[1]) + (vec_nor_z * vec_norm_ecef[2]));
//	double vec_proj_x = vec_nor_x - dot_pr * vec_norm_ecef[0];
//	double vec_proj_y = vec_nor_y - dot_pr * vec_norm_ecef[1];
//	double vec_proj_z = vec_nor_z - dot_pr * vec_norm_ecef[2];
//
//	// Normalise vector
//	double norm = sqrt(vec_proj_x * vec_proj_x + vec_proj_y * vec_proj_y + vec_proj_z * vec_proj_z);
//
//	pt3 vec_north_ecef = { 0 };
//	vec_north_ecef[0] = vec_proj_x / norm;
//	vec_north_ecef[1] = vec_proj_y / norm;
//	vec_north_ecef[2] = vec_proj_z / norm;
//
//	return vec_north_ecef;
//}


//Compute surface normal unit vectors.
//
//Computation of surface normal unit vectors in earth - centered, earth - fixed
//(pt3) coordinates.
//
//Parameters
//----------
//lon : vector of geographic longitude[degree]
//lat : vecotr of geographic latitudes[degree]
//
//Returns
//------
//vector of surface normal components in pt3 coordinates [metre]
//template <typename Tin, typename Tout = Tin>
//std::vector<Tout> surf_norm(const std::vector<Tin>& lla)
//{
//	std::vector<Tout> vec_norm_ecef(lla.size());
//
//	for (size_t i = 0; i < lla.size(); i++)
//	{
//		vec_norm_ecef[i] = surf_norm(lla[i]);
//	}
//
//	return vec_norm_ecef;
//}
//
////template <typename T>
//matrix<pt3> surf_norm(const matrix<pt3>& lla)
//{
//	matrix<pt3> norm_ecef(lla.xy_size());
//
//	for (size_t ij = 0; ij < lla.xy_size(); ij++)
//	{
//		norm_ecef[ij] = surf_norm(lla[ij]);
//	}
//
//	return norm_ecef;
//}

// -----------------------------------------------------------------------------



//	Compute unit vectors pointing towards North.
//
//	Computation unit vectors pointing towards North in earth - centered,
//	earth - fixed(pt3) coordinates.These vectors are perpendicular to surface
//	normal unit vectors.
//
//	Parameters
//	----------
//	x_ecef : ndarray of double
//	Array(with arbitrary dimensions) with pt3 x - coordinates[metre]
//	y_ecef : ndarray of double
//	Array(with arbitrary dimensions) with pt3 y - coordinates[metre]
//	z_ecef : ndarray of double
//	Array(with arbitrary dimensions) with pt3 z - coordinates[metre]
//	vec_norm_ecef : ndarray of float
//	Array(at least two - dimensional; vector components must be stored in
//		last dimension) with surface normal components in pt3 coordinates
//	[metre]
//	ellps : str
//	Earth's surface approximation (sphere, GRS80 or WGS84)
//
//	Returns
//	------ -
//	vec_north_ecef : ndarray of float
//	Array(dimensions according to input; vector components are stored in
//		last dimension) with north vector components in pt3 coordinates
//	[metre]"""
//
//std::vector<pt3> north_dir(const std::vector <pt3>& ecef, const std::vector<pt3>& vec_norm_ecef, const char* ellps = "WGS84")
//{
//	assert(ecef.size() == vec_norm_ecef.size());
//
//	std::vector<pt3> north_dir_ecef(ecef.size());
//
//	for (size_t i = 0; i < ecef.size(); i++)
//		north_dir_ecef[i] = north_dir(ecef[i], vec_norm_ecef[i], ellps);
//
//	return north_dir_ecef;
//
//}
//
//matrix<pt3> north_dir(const matrix<pt3>& dem_ecef, const matrix<pt3>& vec_norm_ecef, const char* ellps = "WGS84")
//{
//	matrix<pt3> north_dir_ecef(dem_ecef.xy_size());
//
//	//Geocentric earth = get_earth< Geocentric>(ellps);
//	for (size_t ij = 0; ij < dem_ecef.xy_size(); ij++)
//		north_dir_ecef[ij] = north_dir(dem_ecef[ij], vec_norm_ecef[ij], ellps);
//
//	return north_dir_ecef;
//
//}


//pt3 lla2enu(const pt3& pt0, const pt3& pt, const char* ellps = "WGS84")
//{
//	pt3 ecef;
//
//	LocalCartesian localCartesian(pt0.y, pt0.x, pt0.z);
//	localCartesian.Forward(pt.y, pt.x, pt.z, ecef.x, ecef.y, ecef.z);
//
//	return ecef;
//}

//pt3 lonlat2ecef(double lon, double lat, double alt, const char* ellps = "WGS84")
//{
//
//
//
//
//	//"""Coordinate transformation from lon/lat to pt3 (for 1-dimensional data).
//
//	//Sources
//	//------ -
//	//-https ://en.wikipedia.org/wiki/Geographic_coordinate_conversion
//	//-Geoid parameters r, a and f : PROJ"""
//
//	//cdef int len_0 = lon.shape[0]
//	//cdef int i
//	//cdef double r, f, a, b, e_2, n
//	//cdef double[:] x_ecef = np.empty(len_0, dtype = np.float64)
//	//cdef double[:] y_ecef = np.empty(len_0, dtype = np.float64)
//	//cdef double[:] z_ecef = np.empty(len_0, dtype = np.float64)
//
//	//pt3 ecef4;
//
//	//// Spherical coordinates
//	//if (ellps == "sphere")
//	//{
//	//double r = 6370997.0;  // earth radius[m]
//
//	//pt3 ecef;
//	//ecef.x = (r + h) * cos(deg2rad(lat)) * cos(deg2rad(lon));
//	//ecef.y = (r + h) * cos(deg2rad(lat)) * sin(deg2rad(lon));
//	//ecef.z = (r + h) * sin(deg2rad(lat));
//
//
//	//	// Elliptic(geodetic) coordinates
//	//}
//	//else
//	//{
//
//	//	double a = 6378137.0;  // equatorial radius(semi - major axis)[m]
//	//	double f = (ellps == "GRS80") ? (1.0 / 298.257222101) : (1.0 / 298.257223563);  // GRS80:WGS84
//	//	double b = a * (1.0 - f);  // polar radius(semi - minor axis)[m]
//	//	//double e_2 = 1.0 - (b * b / a * a); // squared num.eccentricity[-]
//	//	double e_2 = 2 * f - square(f);// 1.0 - (b * b / a * a);  // squared num.eccentricity[-]
//
//	//	pt3 ecef;
//	//	double n = a / sqrt(1.0 - e_2 * square(sin(deg2rad(lat))));
//	//	ecef.x = (n + h) * cos(deg2rad(lat)) * cos(deg2rad(lon));
//	//	ecef.y = (n + h) * cos(deg2rad(lat)) * sin(deg2rad(lon));
//	//	ecef.z = (b * b / a * a * n + h) * sin(deg2rad(lat));
//
//	//	ecef4 = ecef;
//	//}
//
//
//
//	pt3 ecef;
//
//	Geocentric earth = get_earth< Geocentric>(ellps);
//	earth.Forward(lat, lon, alt, ecef.x, ecef.y, ecef.z);
//
//	return ecef;
//
//}

//void lla2ecef(double lon, double lat, double alt, double& x, double& y, double& z, const char* ellps = "WGS84")
//{
//	Geocentric earth = get_earth< Geocentric>(ellps);
//	earth.Forward(lat, lon, alt, x, y, z);
//}
//
//pt3 lla2ecef(const pt3& lla, const char* ellps = "WGS84")
//{
//	pt3 ecef;
//	lla2ecef(lla.y, lla.x, lla.z, ecef.x, ecef.y, ecef.z);
//	return ecef;
//
//}
//
//template <typename Tin, typename Tout = Tin>
//std::vector<Tout> lla2ecef(const std::vector<Tin>& lla, const char* ellps = "WGS84")
//{
//	std::vector<Tout> ecef(lla.size());
//
//	Geocentric earth = get_earth< Geocentric>(ellps);
//	for (size_t i = 0; i < lla.size(); i++)
//		earth.Forward(lla[i].y, lla[i].x, lla[i].z, ecef[i].x, ecef[i].y, ecef[i].z);
//
//	return ecef;
//
//}
//
//matrix<pt3> lla2ecef(const matrix<pt3>& lla, const char* ellps = "WGS84")
//{
//	matrix<pt3> ecef(lla.xy_size());
//
//	Geocentric earth = get_earth< Geocentric>(ellps);
//	for (size_t ij = 0; ij < lla.xy_size(); ij++)
//		earth.Forward(lla[ij].y, lla[ij].x, lla[ij].z, ecef[ij].x, ecef[ij].y, ecef[ij].z);
//
//	return ecef;
//
//}



//Compute the geoid undulation for the EGM96 or GEOID12A geoid by bilinear
//interpolation from gridded data.
//
//Parameters
//----------
//lon_ip : ndarray of double
//Array(1 - dimensional) with geographic longitude[degree]
//lat_ip : ndarray of double
//Array(1 - dimensional) with geographic longitude[degree]
//geoid : str
//Geoid model(EGM96 or GEOID12A)
//
//Returns
//------
//Update the DEM
matrix<double> undulation(const matrix<pt3>& dem)
{
	assert(!dem.empty());
	//	"""Compute geoid undulation.

	// Spatial coverage of data
	std::array<std::pair<std::string, Domain>, 1> spat_cov =
	{ {
		{"EGM96",    {-180.0, 180.0, -90.0, 90.0 }},
	} };


	// Ensure that latitude values are monotonically increasing
	bool flip_lat = dem(0, 1).y < dem(0, 0).y;
	//if (dem(0, 1).y < dem(0, 0).y)
	//{
	//	lat_dec = true;
	//	//lat_ip = lat_ip[::-1]; RSA reverse of negative????
	//	for (size_t ij = 0; ij < dem.xy_size(); ij++)
	//		dem.vector()[ij].y *= -1;
	//}

	// Compute geoid undulation

	matrix<double> data(size_t(360 / 0.25) + 1, size_t(180 / 0.25) + 1);

	std::string file_path = get_path() + "Geoid/EGM96/WW15MGH.GRD";
	std::ifstream file(file_path);

	std::string line;
	std::getline(file, line);//skip header

	//std::istringstream input;
	//input.str("1\n2\n3\n4\n5\n6\n7\n");
	//int sum = 0;

	//size_t i = 0;
	//size_t j = 0;
	//while (std::getline(file, line))
	//{
	//	if (!line.empty())
	//	{
	//		std::istringstream ss(line);
	//		for (size_t ii = 0; ii < 8; ii++, i++)
	//		{
	//			ELEVATION_TYPE value = 0;
	//			ss >> value;
	//			data(i, j) = value;
	//		}
	//
	//		if (i == data.x_size())
	//		{
	//			j++;//next line in matrix
	//			i = 0;
	//		}
	//	}
	//}

	size_t i = 0;
	size_t j = 0;
	double value = 0;
	while (file >> value)
	{
		data(i, j) = value;
		i++;
		if (i == data.x_size())
		{
			j++;//next line in matrix
			i = 0;
		}
	}


	file.close();




	assert(false);
	//need to compute the new extents of the map
	//ans reprojeect in the proejction of the map
	//is it really important????
	matrix<double> data_ip = data.bilinear_interpolate(dem.x_size(), dem.y_size());

	if (flip_lat)
		data_ip.flip_up();

	return data_ip;
}


//class Transformer_LLA_ENU
//{
//public:
//
//	Transformer_LLA_ENU(double lon = 0, double lat = 0, double alt = 0, const char* ellps = "WGS84") :
//		lla0(lon, lat, alt),
//		geocentric_earth(get_earth<Geocentric>(ellps)),
//		geodesic_earth(get_earth<Geodesic>(ellps)),
//		localCartesian(lat, lon, alt, geocentric_earth)
//	{
//		init_ecef(lon, lat);
//	}
//
//	Transformer_LLA_ENU(pt3 lla, const char* ellps = "WGS84") :
//		lla0(lla),
//		geocentric_earth(get_earth<Geocentric>(ellps)),
//		geodesic_earth(get_earth<Geodesic>(ellps)),
//		localCartesian(lla.y, lla.x, lla.z, geocentric_earth)
//	{
//		init_ecef(lla.x, lla.y);
//	}
//	//double lon_or;
//	//double lat_or;
//
//	pt3 lla0;
//	Geocentric geocentric_earth;
//	Geodesic geodesic_earth;
//	LocalCartesian localCartesian;
//
//	//pt3 ref_ecef;
//	double sin_lon;
//	double cos_lon;
//	double sin_lat;
//	double cos_lat;
//	std::array< std::array<double, 3>, 3> R;
//
//
//
//	void init(double lon, double lat, double alt = 0, const char* ellps = "WGS84")
//	{
//		lla0 = pt3(lon, lat, alt);
//		geocentric_earth = get_earth<Geocentric>(ellps);
//		geodesic_earth = get_earth<Geodesic>(ellps);
//		localCartesian = LocalCartesian(lat, lon, alt, geocentric_earth);
//		init_ecef(lon, lat);
//	}
//
//	void init_ecef(double lon, double lat)
//	{
//		//ref_ecef = lla2ecef(pt3(lon, lat, alt), ellps);
//		//
//		////Set 3x3 transformation matrix
//		//// Trigonometric 
//		sin_lon = sin(deg2rad(lon));
//		cos_lon = cos(deg2rad(lon));
//		sin_lat = sin(deg2rad(lat));
//		cos_lat = cos(deg2rad(lat));
//
//		R =
//		{ {
//			{ -sin_lon, cos_lon, 0},
//			{-sin_lat * cos_lon, -sin_lat * sin_lon, cos_lat},
//			{cos_lat * cos_lon, cos_lat * sin_lon, sin_lat}
//		} };
//
//
//		//Rp =
//		//{ {
//		//	{-sin_lon, -sin_lat * cos_lon, cos_lat * cos_lon},
//		//	{cos_lon, -sin_lat * sin_lon, cos_lat * sin_lon},
//		//	{0, cos_lat, sin_lat}
//		//} };
//	}
//
//
//
//	pt3 lla2enu(const pt3& lla)
//	{
//		pt3 enu;
//
//		localCartesian.Forward(lla.y, lla.x, lla.z, enu.x, enu.y, enu.z);
//
//		return enu;
//	}
//
//
//	// Function to transform from pt3 to ENU
//	template <typename Tin, typename Tout = Tin>
//	std::vector<Tout> lla2enu(const std::vector<Tin>& lla)
//	{
//		std::vector<Tout> enu(lla.size());
//
//		for (size_t i = 0; i < lla.size(); i++)
//			enu[i] = lla2enu(lla[i]);
//
//		return enu;
//	}
//
//
//	matrix<pt3> lla2enu(const matrix<pt3>& lla)
//	{
//		matrix<pt3> enu(lla.x_size(), lla.y_size());
//
//		for (size_t ij = 0; ij < lla.xy_size(); ij++)
//		{
//			enu[ij] = lla2enu(lla[ij]);
//		}
//
//		return enu;
//	}
//
//
//	pt3 ecef2enu(const pt3& ecef)
//	{
//		pt3 enu;
//		enu[0] = (-sin_lon * ecef[0] + cos_lon * ecef[1]);
//		enu[1] = (-sin_lat * cos_lon * ecef[0] - sin_lat * sin_lon * ecef[1] + cos_lat * ecef[2]);
//		enu[2] = (+cos_lat * cos_lon * ecef[0] + cos_lat * sin_lon * ecef[1] + sin_lat * ecef[2]);
//		return enu;
//	}
//
//	std::vector<pt3> ecef2enu(const std::vector<pt3>& ecef)
//	{
//
//		std::vector<pt3> enu(ecef.size());
//
//		for (size_t i = 0; i < ecef.size(); i++)
//		{
//			enu[i] = ecef2enu(ecef[i]);
//		}
//
//		return enu;
//	}
//
//
//	//std::array< std::array<double, 3>, 3> Rp;
//
//
//	/*matrix<pt3> surf_norm(const matrix<pt3>& lla)
//	{
//		matrix<pt3> norm_ecef(lla.xy_size());
//
//		for (size_t ij = 0; ij < lla.xy_size(); ij++)
//		{
//			norm_ecef[ij] = surf_norm(lla[ij]);
//		}
//
//		return norm_ecef;
//	}*/
//
//	//aspect (in deg) clockwise from north
//	pt3 Normal(double slope, double aspect)
//	{
//		double x = cos(deg2rad(slope)) * cos(deg2rad(90 - aspect));
//		double y = cos(deg2rad(slope)) * sin(deg2rad(90 - aspect));
//		double z = sin(deg2rad(slope));
//		double d = sqrt(x * x + y * y + z * z);
//
//		return pt3(x, y, z) / d;
//
//
//		// Distance from the reference point(in meters)
//		//double d = 100;
//		//
//		//// Convert to radians
//		//double slope_rad = deg2rad(slope);
//		//double aspect_rad = deg2rad(aspect);
//		//
//		//// Calculate new geodetic coordinates
//		//double lat_new = lla.y + (d * std::sin(aspect_rad) * std::cos(slope_rad));
//		//double lon_new = lla.x + (d * std::cos(aspect_rad) * std::cos(slope_rad));
//		//double alt_new = lla.z + (d * std::sin(slope_rad));
//		//
//		//// Convert geodetic to ECEF using Geodesic
//		////geod = Geodesic(Geodesic.WGS84)  # Or other reference ellipsoid
//		//double x, y, z = 0;
//		//geocentric_earth.Forward(lat_new, lon_new, alt_new, x, y, z);
//		//
//		//
//		//d = sqrt(x * x + y * y + z * z);
//		//
//		//return pt3(x, y, z) / d;
//
//		//The normal at the ecef coordinate is the 
//		//double x, y, z = 0;
//		//localCartesian.Reverse(ecef.x, ecef.y, ecef.z, x, y, z);
//		//double d = sqrt(x * x + y * y + z * z);
//
//
//		//earth.Reverse(ecef.x, ecef.y, ecef.z, x, y, z);
//
//		//Vector3 normal = 
//		//geodesic_earth.Normal(lla.y, lla.x, lla.z);
//
//
//		//return { x / d,y / d,z / d }
//	}
//
//	//pt3 north(pt3 lla)
//	//{
//	//	//pt3 enu;
//	//	double azi1=0;
//	//	double azi2=0;
//	//	geodesic_earth.Inverse(pt0.y, pt0.x, lla.y, lla.y, azi1, azi2);
//	//	
//	//	localCartesian.Forward(
//	//	//# Create a LocalCartesian object
//	//	//	local_cartesian = wgs84.Geocentric(lat0, lon0, h0)
//	//
//	//	//	# Convert to ENU coordinates
//	//	//	e, n, u = local_cartesian.Forward(bearing, distance)
//	//
//	//	//	# The e, n, and u values are the ENU coordinates
//	//	//	print(f"ENU coordinates: East = {e}, North = {n}, Up = {u}")
//	//
//	//	return { azi1, azi2, lla.z - pt0.z };
//	//		
//	//	//print "The initial direction is {:.3f} degrees.".format(g['azi1'])
//	//	//The initial direction is - 57.792 degrees.
//	//}
//	pt3 norm(const pt3& lla)
//	{
//	}
//
//	pt3 north(const pt3& lla)
//	{
//		pt3 pt0;
//		geocentric_earth.Forward(lla0.y, lla0.x, lla0.z + 1e20, pt0.x, pt0.y, pt0.z);
//
//		pt3 pt;
//		geocentric_earth.Forward(lla.y, lla.x, lla.z, pt.x, pt.y, pt.z);
//
//		pt3 delta = pt - pt0;
//
//		return delta / delta.d();
//	}
//};

//
//class Transformer_ECEF_ENU
//{
//public:
//
//	Transformer_ECEF_ENU(double lon = 0, double lat = 0, double alt=0, const char* ellps = "WGS84")
//	{
//		init(lon, lat, alt, ellps);
//	}
//	//double lon_or;
//	//double lat_or;
//
//	void init(double lon, double lat, double alt = 0, const char* ellps = "WGS84")
//	{
//		//"""Class that stores attributes to transform from pt3 to ENU coordinates.
//
//		/*Transformer class that stores attributes to convert between pt3 and ENU
//		coordinates.The origin of the ENU coordinate system coincides with the
//		surface of the sphere / ellipsoid.
//
//		Parameters
//		------ -
//		lon_or : double
//		Longitude coordinate for origin of ENU coordinate system[degree]
//		lat_or : double
//		Latitude coordinate for origin of ENU coordinate system[degree]
//		ellps : str
//		Earth's surface approximation (sphere, GRS80 or WGS84)"""*/
//
//		//lon_or = lon;
//		//lat_or = lat;
//
//		//Geocentric earth = get_earth< Geocentric>(ellps);
//
//		/*double n = a / np.sqrt(1.0 - e_2 * np.sin(np.deg2rad(self.lat_or)) * *2)
//			self.x_ecef_or = n * np.cos(np.deg2rad(self.lat_or)) * np.cos(np.deg2rad(self.lon_or))
//			self.y_ecef_or = n * np.cos(np.deg2rad(self.lat_or)) * np.sin(np.deg2rad(self.lon_or))
//			self.z_ecef_or = (b * *2 / a * *2 * n) 	* np.sin(np.deg2rad(self.lat_or))*/
//
//
//		ref_ecef = lonlat2ecef(lon, lat, alt, ellps);
//
//		//Set 3x3 transformation matrix
//		// Trigonometric 
//		sin_lon = sin(deg2rad(lon));
//		cos_lon = cos(deg2rad(lon));
//		sin_lat = sin(deg2rad(lat));
//		cos_lat = cos(deg2rad(lat));
//
//		///double r = 6370997.0;  // earth radius[m]
//		///ref_ecef.x = r * cos_lat * cos_lon;
//		///ref_ecef.y = r * cos_lat * sin_lon;
//		///ref_ecef.z = r * sin_lat;
//
//
//
//		R =
//		{ {
//			{ -sin_lon, cos_lon, 0},
//			{-sin_lat * cos_lon, -sin_lat * sin_lon, cos_lat},
//			{cos_lat * cos_lon, cos_lat * sin_lon, sin_lat}
//		} };
//
//
//		Rp =
//		{ {
//			{-sin_lon, -sin_lat * cos_lon, cos_lat * cos_lon},
//			{cos_lon, -sin_lat * sin_lon, cos_lat * sin_lon},
//			{0, cos_lat, sin_lat}
//		} };
//	}
//
//	pt3 ecef2enu(const pt3& ecef)
//	{
//		pt3 enu;
//		pt3 tmp = ecef.operator-(ref_ecef);
//
//		for (size_t j = 0; j < 3; j++)
//		{
//			for (size_t k = 0; k < 3; k++)
//				enu[j] += R[j][k] * tmp[k];
//		}
//
//		// Trigonometric functions
//		//double sin_lon = sin(deg2rad(lon_or));
//		//double cos_lon = cos(deg2rad(lon_or));
//		//double sin_lat = sin(deg2rad(lat_or));
//		//double cos_lat = cos(deg2rad(lat_or));
//
//		//pt3 ecef_or = lonlat2ecef(lon_or, lat_or, 0);
//		//// Coordinate transformation
//
//		//pt3 enu2;
//		//enu2[0] = (-sin_lon * (ecef[0] - ecef_or[0]) + cos_lon * (ecef[1] - ecef_or[0]));
//		//enu2[1] = (-sin_lat * cos_lon * (ecef[0] - ecef_or[0]) - sin_lat * sin_lon * (ecef[1] - ecef_or[1]) + cos_lat * (ecef[2] - ecef_or[2]));
//		//enu2[2] = (+cos_lat * cos_lon * (ecef[0] - ecef_or[0]) + cos_lat * sin_lon * (ecef[1] - ecef_or[1]) + sin_lat * (ecef[2] - ecef_or[2]));
//
//
//
//		return enu;
//	}
//
//
//	// Function to transform from pt3 to ENU
//	std::vector<pt3> ecef2enu(const std::vector<pt3>& ecef)
//	{
//		std::vector<pt3> enu(ecef.size());
//
//		for (size_t i = 0; i < ecef.size(); i++)
//			ecef2enu(ecef[i]);
//
//		return enu;
//	}
//
//	matrix<pt3> ecef2enu(const matrix<pt3>& ecef)
//	{
//		//convert matrix -> vector
//		std::vector<pt3> ecef2(ecef.begin(), ecef.end());
//		assert(ecef2.size() == ecef.xy_size());
//
//		std::vector<pt3> enu2 = ecef2enu(ecef2);//call with vector
//		assert(enu2.size() == ecef2.size());
//
//		matrix<pt3> enu(ecef.x_size(), ecef.y_size(), enu2.data());//convert vector to matrix
//		assert(enu.xy_size() == ecef.xy_size());
//
//		//std::matrix<pt3> enu(ecef.x_size(), ecef.y_size());
//
//		//for (size_t j = 0; j < ecef.size(); j++)
//		//{
//		//	for (size_t i = 0; i < ecef.size(); i++)
//		//	{
//		//		pt3 tmp = ecef(i,j).operator-(ref_ecef);
//
//		//		for (size_t k = 0; k < 3; k++)
//		//		{
//		//			for (size_t kk = 0; kk < 3; kk++)
//		//				enu(i,j)[k] += R[k][kk] * tmp[kk];
//		//		}
//		//	}
//		//}
//
//		return enu;
//	}
//
//	// Function to transform from ENU to pt3
//	std::vector<pt3>  enu2ecef(const std::vector<pt3>& enu)
//	{
//		std::vector<pt3> ecef(enu.size());
//		/*
//
//		R * enu + ref_ecef;*/
//
//		for (size_t i = 0; i < enu.size(); i++)
//		{
//			pt3 tmp = { 0 };
//			for (size_t j = 0; j < 3; j++)
//			{
//				for (size_t k = 0; k < 3; k++)
//					tmp[j] += R[j][k] * enu[i][k];
//			}
//
//			ecef[i] = tmp.operator+(ref_ecef);
//		}
//
//		return ecef;
//	}
//
//
//	std::vector<pt3> ecef2enu2(const std::vector<pt3>& ecef)
//	{
//
//		std::vector<pt3> enu(ecef.size());
//
//		for (size_t i = 0; i < ecef.size(); i++)
//		{
//			//pt3 tmp = ecef[i].operator-(ref_ecef);
//			enu[i][0] = (-sin_lon * ecef[i][0] + cos_lon * ecef[i][1]);
//			enu[i][1] = (-sin_lat * cos_lon * ecef[i][0] - sin_lat * sin_lon * ecef[i][1] + cos_lat * ecef[i][2]);
//			enu[i][2] = (+cos_lat * cos_lon * ecef[i][0] + cos_lat * sin_lon * ecef[i][1] + sin_lat * ecef[i][2]);
//		}
//
//		return enu;
//	}
//
//	pt3 ref_ecef;
//	double sin_lon;
//	double cos_lon;
//	double sin_lat;
//	double cos_lat;
//	std::array< std::array<double, 3>, 3> R;
//	std::array< std::array<double, 3>, 3> Rp;
//
//
//};

//std::array<double, 3> crossProduct(const pt3& vecA, const pt3& vecB)
//{
//	double c1 = vecA[1] * vecB[2] - vecA[2] * vecB[1];
//	double c2 = vecA[2] * vecB[0] - vecA[0] * vecB[2];
//	double c3 = vecA[0] * vecB[1] - vecA[1] * vecB[0];
//
//	return { c1, c2, c3 };
//
//	//inline void cross_prod(float a_x, float a_y, float a_z, float b_x, float b_y,
//	//	float b_z, float& c_x, float& c_y, float& c_z) {
//	//	c_x = a_y * b_z - a_z * b_y;
//	//	c_y = a_z * b_x - a_x * b_z;
//	//	c_z = a_x * b_y - a_y * b_x;
//	//}
//
//}
//
//typedef std::array < std::array<double, 3>, 3> rotation_matrix;
////typedef std::tuple<rotation_matrix, std::vector<ENU>, std::vector<pt3>> glob2loc;
////typedef std::tuple<rotation_matrix, std::vector<ENU>, std::vector<pt3>> glob2loc;
////typedef matrix<rotation_matrix> glob2loc;
//
//rotation_matrix rotation_matrix_glob2loc(/*std::vector<pt3> vec_north_enu, std::vector<pt3> vec_norm_enu*/)
//{
//	//"""Matrices to rotate vectors from global to local ENU coordinates.
//
//	//Array with matrices to rotate vector from global to local ENU coordinates.
//	//Extend spatial dimensions by one grid cell on each side(filled with NaN)
//	//to make the dimensions consistent with the DEM domain used to compute slope
//	//angle and aspect.
//
//	//Parameters
//	//------ -
//	//vec_north_enu : ndarray of float
//	//Array(three - dimensional; vector components must be stored in last
//	//	dimension) with north vector components in ENU coordinates[metre]
//	//vec_norm_enu : ndarray of float
//	//Array(three - dimensional; vector components must be stored in last
//	//	dimension) with surface normal components in ENU coordinates
//	//[metre]
//
//	//Returns
//	//----------
//	//lon : ndarray
//	//Array(four - dimensional; rotation matrices stored in last two
//	//	dimensions) with rotation matrices[metre]'"""
//
//	//assert(vec_north_enu.size() == vec_norm_enu.size());
//
//	rotation_matrix R;
//
//	//for (size_t i = 0; i < vec_north_enu.size(); i++)
//	//{
//		//for (size_t j = 0; j < vec_norm_enu.size(); j++)
//		//{
//
//	pt3 norm_enu(0, 0, 1);
//	pt3 north_enu(0, 1, 0);
//
//	std::array<double, 3> x = crossProduct(north_enu, norm_enu);
//
//	for (size_t k = 0; k < 3; k++)
//	{
//		R[k][0] = x[k];
//		R[k][1] = north_enu[k];
//		R[k][2] = norm_enu[k];
//	}
//	//
//	//	glob2loc R(vec_north_enu.size(), vec_norm_enu.size());
//	//
//	//	//for (size_t i = 0; i < vec_north_enu.size(); i++)
//	//	//{
//	//		//for (size_t j = 0; j < vec_norm_enu.size(); j++)
//	//		//{
//	//
//	//	
//	//	R(i, j)[0] = crossProduct(vec_north_enu[i], vec_norm_enu[i]);
//	//	R(i, j)[1] = vec_north_enu[i].array();
//	//	R(i, j)[2] = vec_norm_enu[j].array();
//	//}
//	//}
//
//
//		//double vec_x = rot_mat[i, j, 0, 0] * coord[k, 0] + rot_mat[i, j, 0, 1] * coord[k, 1] + rot_mat[i, j, 0, 2] * coord[k, 2];
//		//double vec_y = rot_mat[i, j, 1, 0] * coord[k, 0] + rot_mat[i, j, 1, 1] * coord[k, 1] + rot_mat[i, j, 1, 2] * coord[k, 2];
//		//double vec_z = rot_mat[i, j, 2, 0] * coord[k, 0] + rot_mat[i, j, 2, 1] * coord[k, 1] + rot_mat[i, j, 2, 2] * coord[k, 2];
//
//	//}
//	//}
//
//	//	// vector pointing towards east
//	//rot_mat_glob2loc[1:-1, 1 : -1, 1, : ] = vec_north_enu;
//	//rot_mat_glob2loc[1:-1, 1 : -1, 2, : ] = vec_norm_enu;
//
//	return R;
//}
//
//void get_slope_aspect(matrix<pt3> window, double& slope, double& aspect)
//{
//	assert(window.x_size() == 3);
//	assert(window.y_size() == 3);
//
//	double x_res = (window(2,1).x - window(0,1).x)/ 2.0;
//	double y_res = (window(1,2).y - window(1,0).y)/ 2.0;
//	double scale = 1;
//
//
//	//slope = 0;
//	//aspect = 0;
//	//if( !IsValidSlopeWindow(window) )
//		//return;
//
//	double dy = ((window(0, 0).z + 2 * window(1, 0).z + window(2, 0).z) - 
//		(window(0, 2).z + 2 * window(1, 2).z + window(2, 2).z) ) / y_res;
//
//	double dx = ((window(2, 0).z + 2 * window(2, 1).z + window(2, 2).z) -
//		(window(0, 0).z + 2 * window(0, 1).z + window(0, 2).z) ) / x_res;
//
//	double d = sqrt(dx * dx + dy * dy);
//
//	slope = rad2deg(atan(d / (8 * scale)));//[deg]
//	aspect = rad2deg(atan2(dy, -dx));//[deg]
//	aspect = std::fmod(90 - aspect + 360, 360);//[deg]
//
//	if (dx == 0 && dy == 0)
//	{
//		// lat area 
//		slope = 0;
//		aspect = 0;
//	}
//
//	//else //transform from azimut angle to geographic angle
//	//{
//	//	if (aspect > 90.0f)
//	//		aspect = 450.0f - aspect;
//	//	else
//	//		aspect = 90.0f - aspect;
//	//}
//
//	//if (aspect == 360.0)
//	//	aspect = 0.0f;
//}

//pt3 get_slope_aspect(const matrix<pt3> & dem_3x3_enu)
//{
//	assert(dem_3x3_enu.x_size()==3);
//	assert(dem_3x3_enu.y_size() == 3);
//
//	double slope = 0;
//	double aspect = 0;
//
//	get_slope_aspect(dem_3x3_enu, slope, aspect);
//	N
//
//	return N;
//}

//matrix<pt3> slope_plane_meth(const matrix<pt3>& xyz, rotation_matrix& rot_mat, bool output_rot = false)
////double _slope_plane_meth_cy(double x, double y, double z, rotation_matrix* rot_mat = nullptr, bool output_rot = false)
//{
//	//"""Plane-based slope computation.
//
//	//Sources
//	//------ -
//	//-ArcGIS : https ://pro.arcgis.com/en/pro-app/tool-reference/spatial-analyst/
//	//how - slope - works.htm
//
//	//To do
//	//---- -
//	//Parallelise function with OpenMP.Consider that various arrays
//	//(vec, mat, ...) must be thread - private."""
//
//	//cdef int len_0 = x.shape[0]
//	//cdef int len_1 = x.shape[1]
//	//cdef int i, j, k, l
//	//cdef float vec_x, vec_y, vec_z, vec_mag
//	//cdef int num, nrhs, lda, ldb, info
//	//cdef float x_l_sum, y_l_sum, z_l_sum
//	//cdef float x_l_x_l_sum, x_l_y_l_sum, x_l_z_l_sum, y_l_y_l_sum, y_l_z_l_sum
//	//cdef int count
//	//cdef float[:, : , : ] vec_tilt = np.empty((len_0, len_1, 3),
//	//	dtype = np.float32)
//	//cdef float[:] vec = np.empty(3, dtype = np.float32)
//	//cdef float[:] mat = np.zeros(9, dtype = np.float32)
//	//cdef int[:] ipiv = np.empty(3, dtype = np.int32)
//	//cdef float[:, : ] coord = np.empty((9, 3), dtype = np.float32)
//
//	//# Settings for solving system of linear equations
//	//num = 3 # number of linear equations[-]
//	//nrhs = 1 # number of columns of matrix B[-]
//	//lda = 3 # leading dimension of array A[-]
//	//ldb = 3 # leading dimension of array B[-]
//
//	// Initialise array
//	matrix<pt3> vec_tilt(xyz.x_size(), xyz.y_size());
//
//	// Loop through grid cells
//	for (int i = 0; i < xyz.x_size(); i++)
//	{
//		for (int j = 0; j < xyz.y_size(); j++)
//		{
//			// Translate and rotate input coordinates
//
//			rotation_matrix
//
//			//std::array<pt3, 9>  coord = { NAN };
//			////coord.fill(NAN);
//
//			//int count = 0;
//			//for (int k = i - 1; k < i + 2; k++)//a revoir  hummmm
//			//{
//			//	for (int l = j - 1; l < j + 2; l++)
//			//	{
//			//		coord[count][0] = xyz(k, l)[0] - xyz(i, j)[0];
//			//		coord[count][1] = xyz(k, l)[1] - xyz(i, j)[1];
//			//		coord[count][2] = xyz(k, l)[2] - xyz(i, j)[2];
//			//		count = count + 1;
//			//	}
//			//}
//
//			//for (int k = 0; k < 9; k++)
//			//{
//			//	double vec_x = rot_mat(i, j)[0][0] * coord[k][0] + rot_mat(i, j)[0][1] * coord[k][1] + rot_mat(i, j)[0][2] * coord[k][2];
//			//	double vec_y = rot_mat(i, j)[1][0] * coord[k][0] + rot_mat(i, j)[1][1] * coord[k][1] + rot_mat(i, j)[1][2] * coord[k][2];
//			//	double vec_z = rot_mat(i, j)[2][0] * coord[k][0] + rot_mat(i, j)[2][1] * coord[k][1] + rot_mat(i, j)[2][2] * coord[k][2];
//			//	coord[k][0] = vec_x;
//			//	coord[k][1] = vec_y;
//			//	coord[k][2] = vec_z;
//			//}
//			//// Compute normal vector of plane
//			//double x_l_sum = 0.0;
//			//double y_l_sum = 0.0;
//			//double z_l_sum = 0.0;
//			//double x_l_x_l_sum = 0.0;
//			//double x_l_y_l_sum = 0.0;
//			//double x_l_z_l_sum = 0.0;
//			//double y_l_y_l_sum = 0.0;
//			//double y_l_z_l_sum = 0.0;
//			//for (int k = 0; k < 9; k++)
//			//{
//			//	x_l_sum = x_l_sum + coord[k][0];
//			//	y_l_sum = y_l_sum + coord[k][1];
//			//	z_l_sum = z_l_sum + coord[k][2];
//			//	x_l_x_l_sum = x_l_x_l_sum + (coord[k][0] * coord[k][0]);
//			//	x_l_y_l_sum = x_l_y_l_sum + (coord[k][0] * coord[k][1]);
//			//	x_l_z_l_sum = x_l_z_l_sum + (coord[k][0] * coord[k][2]);
//			//	y_l_y_l_sum = y_l_y_l_sum + (coord[k][1] * coord[k][1]);
//			//	y_l_z_l_sum = y_l_z_l_sum + (coord[k][1] * coord[k][2]);
//			//}
//			//// Fortran - contiguous
//			//std::array<double, 9> mat;
//			//mat[0] = x_l_x_l_sum;
//			//mat[3] = x_l_y_l_sum;
//			//mat[6] = x_l_sum;
//			//mat[1] = x_l_y_l_sum;
//			//mat[4] = y_l_y_l_sum;
//			//mat[7] = y_l_sum;
//			//mat[2] = x_l_sum;
//			//mat[5] = y_l_sum;
//			//mat[8] = 9.0;
//
//			//std::array<double, 3> vec;
//			//vec[0] = x_l_z_l_sum;
//			//vec[1] = y_l_z_l_sum;
//			//vec[2] = z_l_sum;
//			////sgesv(&num, &nrhs, &mat[0], &lda, &ipiv[0], &vec[0], &ldb, &info)
//			//vec[2] = -1.0;
//
//
//			//double vec_x = vec[0];
//			//double vec_y = vec[1];
//			//double vec_z = vec[2];
//
//			//// Normalise vector
//			//double vec_mag = sqrt(vec_x * vec_x + vec_y * vec_y + vec_z * vec_z);
//			//vec_x = vec_x / vec_mag;
//			//vec_y = vec_y / vec_mag;
//			//vec_z = vec_z / vec_mag;
//
//			//// Reverse orientation of plane's normal vector (if necessary)
//			//if (vec_z < 0.0)
//			//{
//			//	vec_x *= -1.0;
//			//	vec_y *= -1.0;
//			//	vec_z *= -1.0;
//			//}
//
//			//vec_tilt(i, j)[0] = vec_x;
//			//vec_tilt(i, j)[1] = vec_y;
//			//vec_tilt(i, j)[2] = vec_z;
//
//			//// Rotate output vectors
//			//if (output_rot)
//			//{
//			//	printf("Tilted surface normals are rotated according to 'rot_mat'\n");
//			//}
//			//else
//			//{
//
//			//	// Rotate vector back to input reference frame(->use transposes of  rotation matrices)
//			//	for (int i = 0; i < vec_tilt.x_size(); i++)
//			//	{
//			//		for (int j = 0; j < vec_tilt.y_size(); j++)
//			//		{
//
//			//			vec_x = rot_mat(i, j)[0][0] * vec_tilt(i, j)[0] + rot_mat(i, j)[1][0] * vec_tilt(i, j)[1] + rot_mat(i, j)[2][0] * vec_tilt(i, j)[2];
//			//			vec_y = rot_mat(i, j)[0][1] * vec_tilt(i, j)[0] + rot_mat(i, j)[1][1] * vec_tilt(i, j)[1] + rot_mat(i, j)[2][1] * vec_tilt(i, j)[2];
//			//			vec_z = rot_mat(i, j)[0][2] * vec_tilt(i, j)[0] + rot_mat(i, j)[1][2] * vec_tilt(i, j)[1] + rot_mat(i, j)[2][2] * vec_tilt(i, j)[2];
//			//			vec_tilt[i, j, 0] = vec_x;
//			//			vec_tilt[i, j, 1] = vec_y;
//			//			vec_tilt[i, j, 2] = vec_z;
//			//		}
//			//	}
//			//}
//		}//j
//	}//i
//
//	return vec_tilt;
//}


//std::vector<pt3> slope_plane_meth(const std::vector<pt3>& xyz, rotation_matrix* rot_mat = nullptr, bool output_rot = false)
//{/*
//	"""Plane-based slope computation.
//
//	Plane - based method that computes the surface normal by fitting a plane
//	to the central and 8 neighbouring grid cells.The optimal fit is computed
//	by minimising the sum of the squared errors in the z - direction.The same
//	method is used in ArcGIS.
//	To guarantee a solution to the system of linear equations for every grid
//	grid cell, the following two coordinate transformations are applied :
//-translation : coordinates are shifted so that the centre grid cell of the
//9x9 cells coincides with the coordinate system origin
//- rotation : the 9x9 cells are rotated so that local up aligns with the
//z - axis of the coordinate system.This is already the case for
//planar coordinates but e.g. not for global ENU coordinates.
//The option 'output_rot' determines, if computed tilted surface normals
//are outputted in the input reference frame('output_rot = False'; e.g.
//	global ENU coordinates) or in the internally applied rotated reference
//	frame('output_rot = True'; e.g.local ENU coordinates).This option has
//	no effect for planar coordinates.
//
//	Parameters
//	----------
//	x : ndarray of float
//	Array(two - dimensional) with x - coordinates[metre]
//	y : ndarray of float
//	Array(two - dimensional) with y - coordinates[metre]
//	z : ndarray of float
//	Array(two - dimensional) with z - coordinates[metre]
//	rot_mat : ndarray of float, optional
//	Array(four - dimensional; rotation matrices stored in last
//		two dimensions) with rotation matrices to transform
//	coordinates to a local reference frame in which the z - axis aligns
//	with local up
//	output_rot : bool
//	Rotate output according to rotation matrices
//
//	Returns
//	------ -
//	vec_tilt : ndarray of float
//	Array(three - dimensional; vector components are stored in
//		last dimension) with titled surface normals[metre]"""*/
//
//		// Check arguments
//	//if (x.shape != y.shape) or (y.shape != z.shape) :
//		//raise ValueError("Inconsistent shapes / number of dimensions of input arrays");
//		//if ((x.dtype != "float32") or (y.dtype != "float32") or (z.dtype != "float32")) :
//			//raise ValueError("Input array(s) has/have incorrect data type(s)")
//			//if rot_mat is not None :
//			//	if ((x.shape[0] != rot_mat.shape[0])
//			//		or (x.shape[1] != rot_mat.shape[1])) :
//			//		raise ValueError("Inconsistent shapes / number of dimensions of "
//			//			+ "input arrays")
//			//		if rot_mat.dtype != "float32" :
//			//			raise ValueError("'rot mat' has incorrect data type")
//
//						// Wrapper for Cython function
//	if (rot_mat == nullptr)
//	{
//		//rot_mat = np.empty(x.shape + (3, 3), dtype = np.float32);
//		//rot_mat[..., :, : ] = np.eye(3, dtype = np.float32);
//		//	print("No rotation matrices provided, use identity matrices");
//		//print(np.eye(3, dtype = np.float32));
//		//print("that cause no rotation");
//	}
//
//	double vec_tilt = _slope_plane_meth_cy(xyz, rot_mat, output_rot);
//	return vec_tilt;
//}


//double sky_view_factor(std::vector<double> azim, std::vector<horizon_result> hori, pt3 vec_tilt)
//{
//	//"""Sky view factor (SVF) computation.
//
//	//Compute sky view factor(SVF) in local horizontal coordinate system.The
//	//SVF is defined as the fraction of sky radiation received at a certain
//	//location in case of isotropic sky radiation.
//
//	//Parameters
//	//----------
//	//azim: ndarray of float
//	//Array(one - dimensional) with azimuth[radian]
//	//hori : ndarray of float
//	//Array(three - dimensional) with horizon(y, x, azim)[radian]
//	//vec_tilt : ndarray of float
//	//Array(three - dimensional) with titled surface normal components
//	//(y, x, components)[metre]
//
//	//Returns
//	//------ -
//	//svf : ndarray of float
//	//Array(two - dimensional) with sky view factor[-]"""
//
//	//# Check arguments
//	//if (len(azim) != hori.shape[2]) or (hori.shape[:2] != vec_tilt.shape[:2])\
//			//	or (vec_tilt.shape[2] != 3) :
//			//	raise ValueError("Inconsistent/incorrect shapes of input arrays")
//			//	if ((azim.dtype != "float32") or (hori.dtype != "float32")
//			//		or (vec_tilt.dtype != "float32")) :
//			//		raise ValueError("Input array(s) has/have incorrect data type(s)")
//
//			//# Wrapper for Cython function
//			//	svf = _sky_view_factor_cy(azim, hori, vec_tilt)
//			//	return svf
//		//}
//
//				//"""Sky view factor (SVF) computation."""
//
//				//cdef int len_0 = hori.shape[0]
//				//cdef int len_1 = hori.shape[1]
//				//cdef int len_2 = hori.shape[2]
//				//cdef int i, j, k
//				//cdef float azim_spac
//				//cdef float agg, hori_plane, hori_elev
//				//cdef float[:, : ] svf = np.empty((len_0, len_1), dtype = np.float32)
//				//cdef float[:] azim_sin = np.empty(len_2, dtype = np.float32)
//				//cdef float[:] azim_cos = np.empty(len_2, dtype = np.float32)
//
//	double svf;
//
//
//	std::vector<double> azim_sin(azim.size());
//	std::vector<double> azim_cos(azim.size());
//
//
//	// Precompute values of trigonometric functions
//	// -> these arrays can be shared between threads (read-only)
//	for (size_t i = 0; i < azim.size(); i++)
//	{
//		azim_sin[i] = sin(azim[i]);
//		azim_cos[i] = cos(azim[i]);
//	}
//
//
//	//pas clair
//
//	double agg = 0.0;
//	//for (size_t i = 0; i < vec_tilt.x_size(); i++)
//	//{
//		// Compute sky view factor
//
//		//for i in prange(len_0, nogil = True, schedule = "static") :
//			//for j in range(len_1) :
//		//for (size_t j = 0; j < vec_tilt.y_size(); j++)
//		//{
//			// 
//			// Iterate over azimuth directions
//
//	for (size_t k = 0; k < hori.size(); k++)
//	{//	for k in range(len_2) :
//
//			// Compute plane - sphere intersection
//		double hori_plane = atan(-azim_sin[k] * vec_tilt[0] / vec_tilt[2] - azim_cos[k] * vec_tilt[1] / vec_tilt[2]);
//
//		double hori_elev = 0;
//		if (hori[k].hori_angle >= hori_plane)
//			hori_elev = hori[k].hori_angle;
//		else
//			hori_elev = hori_plane;
//
//		// Compute inner integral
//		agg = agg + ((vec_tilt[0] * azim_sin[k] + vec_tilt[1] * azim_cos[k]) * ((M_PI / 2.0) - hori_elev - (sin(2.0 * hori_elev) / 2.0)) + vec_tilt[2] * square(cos(hori_elev)));
//	}
//
//	//}
//
//
//
////}
//
//	double azim_spac = (azim[1] - azim[0]);
//	svf = (azim_spac / (2.0 * M_PI)) * agg;
//
//	return svf;
//}
//
//// -----------------------------------------------------------------------------
//
//std::vector<double> visible_sky_fraction(std::vector<double> azim, horizon_results hori, std::vector<pt3> vec_tilt)
//{
//	//"""Visible sky fraction (VSF) computation.
//
//	//	Compute visible sky fraction(VSF) in local horizontal coordinate system.
//	//	The visible sky fraction is defined as the solid angle of the visible sky.
//
//	//	Parameters
//	//	----------
//	//	azim : ndarray of float
//	//	Array(one - dimensional) with azimuth[radian]
//	//	hori : ndarray of float
//	//	Array(three - dimensional) with horizon(y, x, azim)[radian]
//	//	vec_tilt : ndarray of float
//	//	Array(three - dimensional) with titled surface normal components
//	//	(y, x, components)[metre]
//
//	//	Returns
//	//	------ -
//	//	vsf : ndarray of float
//	//	Array(two - dimensional) with Visible Sky Fraction[-]"""
//
//	//	// Check arguments
//	//	if (len(azim) != hori.shape[2]) or (hori.shape[:2] != vec_tilt.shape[:2])\
//			//		or (vec_tilt.shape[2] != 3) :
//			//		raise ValueError("Inconsistent/incorrect shapes of input arrays")
//			//		if ((azim.dtype != "float32") or (hori.dtype != "float32")
//			//			or (vec_tilt.dtype != "float32")) :
//			//			raise ValueError("Input array(s) has/have incorrect data type(s)")
//
//			//			// Wrapper for Cython function
//			//			vsf = _visible_sky_fraction_cy(azim, hori, vec_tilt)
//			//			return vsf
//
//
//			//			def _visible_sky_fraction_cy(float[:] azim, float[:, : , : ] hori,
//			//				float[:, : , : ] vec_tilt) :
//			//			"""Visible sky fraction (VSF) computation."""
//
//			//			cdef int len_0 = hori.shape[0]
//			//			cdef int len_1 = hori.shape[1]
//			//			cdef int len_2 = hori.shape[2]
//			//			cdef int i, j, k
//			//			cdef float azim_spac
//			//			cdef float agg, hori_plane, hori_elev
//			//			cdef float[:, : ] vsf = np.empty((len_0, len_1), dtype = np.float32)
//			//			cdef float[:] azim_sin = np.empty(len_2, dtype = np.float32)
//			//			cdef float[:] azim_cos = np.empty(len_2, dtype = np.float32)
//
//
//	std::vector<double> vsf;
//
//	/*
//				// Precompute values of trigonometric functions
//				for i in range(len_2) :
//					azim_sin[i] = sin(azim[i])
//					azim_cos[i] = cos(azim[i])
//					// -> these arrays can be shared between threads (read-only)
//
//									// Compute visible sky fraction
//					azim_spac = (azim[1] - azim[0])
//					for i in prange(len_0, nogil = True, schedule = "static") :
//						for j in range(len_1) :
//
//							// Iterate over azimuth directions
//							agg = 0.0
//							for k in range(len_2) :
//
//								// Compute plane - sphere intersection
//								hori_plane = atan(-azim_sin[k] * vec_tilt[i, j, 0]
//									/ vec_tilt[i, j, 2]
//									- azim_cos[k] * vec_tilt[i, j, 1]
//									/ vec_tilt[i, j, 2])
//								if hori[i, j, k] >= hori_plane:
//	hori_elev = hori[i, j, k]
//								else:
//	hori_elev = hori_plane
//
//		// Compute inner integral
//		agg = agg + (1.0 - cos((M_PI / 2.0) - hori_elev))
//
//		vsf[i, j] = (azim_spac / (2.0 * M_PI)) * agg
//*/
//	return vsf;
//}

//
//class RotationMatrix : public std::array<std::array<double, 3>, 3>
//{
//public:
//
//	RotationMatrix(double  lon0_deg = 0, double lat0_deg = 0)
//	{
//		double lat0_rad = lat0_deg * M_PI / 180.0;
//		double lon0_rad = lon0_deg * M_PI / 180.0;
//
//		double cosLat = std::cos(lat0_rad);
//		double sinLat = std::sin(lat0_rad);
//		double cosLon = std::cos(lon0_rad);
//		double sinLon = std::sin(lon0_rad);
//
//		(*this)[0] = { -sinLon, cosLon, 0 };
//		(*this)[1] = { -cosLon * sinLat, -sinLon * sinLat, cosLat };
//		(*this)[2] = { cosLon * cosLat, sinLon * cosLat, sinLat };
//
//		/*{ {
//			{ -sin_lon, cos_lon, 0},
//			{-sin_lat * cos_lon, -sin_lat * sin_lon, cos_lat},
//			{cos_lat * cos_lon, cos_lat * sin_lon, sin_lat}
//		} };*/
//	};
//
//
//	// Matrix vector multiplication
//	pt3 operator*(const pt3& vec)const
//	{
//		const RotationMatrix& mat = *this;
//		pt3 vec_res;
//
//		vec_res[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2];
//		vec_res[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2];
//		vec_res[2] = mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2];
//
//		return vec_res;
//	}
//
//
//};
//
//
//// Function to convert geodetic (latitude, longitude, altitude) to pt3 coordinates
//pt3 lla_to_ecef(double lon_deg, double lat_deg, double alt_m)
//{
//	const double a = 6378137.0; // Semi-major axis
//	const double f = 1.0 / 298.257223563; // Flattening
//	const double b = a * (1.0 - f); // Semi-minor axis
//	const double e2 = 1 - (b * b) / (a * a); // Eccentricity squared
//
//	double lat_rad = lat_deg * M_PI / 180.0;
//	double lon_rad = lon_deg * M_PI / 180.0;
//
//	//double a = 6378137.0; // Semi-major axis of WGS84 ellipsoid
//	//double e2 = 0.00669437999013; // Eccentricity squared
//
//	//double N = a / std::sqrt(1 - e2 * std::sin(lat_rad) * std::sin(lat_rad));
//
//	//double X = (N + alt_m) * std::cos(lat_rad) * std::cos(lon_rad);
//	//double Y = (N + alt_m) * std::cos(lat_rad) * std::sin(lon_rad);
//	//double Z = (N * (1 - e2) + alt_m) * std::sin(lat_rad);
//
//	double N = a / std::sqrt(1 - e2 * std::sin(lat_rad) * std::sin(lat_rad));
//	double x = (N + alt_m) * std::cos(lat_rad) * std::cos(lon_rad);
//	double y = (N + alt_m) * std::cos(lat_rad) * std::sin(lon_rad);
//	double z = (N * (1 - e2) + alt_m) * std::sin(lat_rad);
//
//	return pt3(x, y, z);
//}
//
//pt3 lla_to_ecef(const pt3& pt) { return lla_to_ecef(pt.x, pt.y, pt.z); }
//
//
//
//// Function to convert pt3 to ENU coordinates
//pt3 ecef_to_enu2(double x, double y, double z, double lon0_deg, double lat0_deg, double alt0_m)
//{
//	// pt3 coordinates of ENU origin
//	pt3  ecef0 = lla_to_ecef(lon0_deg, lat0_deg, alt0_m);
//
//	// Rotation matrix
//	RotationMatrix R(lon0_deg, lat0_deg);
//
//	// Apply rotation
//	pt3  enu = R * pt3(x - ecef0.x, y - ecef0.y, z - ecef0.z);
//
//
//	return enu;
//}
//
//pt3 ecef_to_enu2(const pt3& pt, const pt3& pt0) { return ecef_to_enu2(pt0.x, pt0.y, pt0.z, pt0.x, pt0.y, pt0.z); }
//





//
//
//public List<Double> convertGpsToECEF(double lat, double longi, float alt) {
//
//	double a = 6378.1;
//	double b = 6356.8;
//	double N;
//	double e = 1 - (Math.pow(b, 2) / Math.pow(a, 2));
//	N = a / (Math.sqrt(1.0 - (e * Math.pow(Math.sin(Math.toRadians(lat)), 2))));
//	double cosLatRad = Math.cos(Math.toRadians(lat));
//	double cosLongiRad = Math.cos(Math.toRadians(longi));
//	double sinLatRad = Math.sin(Math.toRadians(lat));
//	double sinLongiRad = Math.sin(Math.toRadians(longi));
//	double x = (N + 0.001 * alt) * cosLatRad * cosLongiRad;
//	double y = (N + 0.001 * alt) * cosLatRad * sinLongiRad;
//	double z = ((Math.pow(b, 2) / Math.pow(a, 2)) * N + 0.001 * alt) * sinLatRad;
//
//	List<Double> ecef = new ArrayList<>();
//	ecef.add(x);
//	ecef.add(y);
//	ecef.add(z);
//
//	return ecef;
//
//
//}
//
//
//public List<Double> convertECEFtoENU(List<Double> ecefUser, List<Double> ecefPOI, double lat, double longi) {
//
//	double cosLatRad = Math.cos(Math.toRadians(lat));
//	double cosLongiRad = Math.cos(Math.toRadians(longi));
//	double sinLatRad = Math.sin(Math.toRadians(lat));
//	double sinLongiRad = Math.sin(Math.toRadians(longi));
//
//
//	List<Double> vector = new ArrayList<>();
//
//	vector.add(ecefUser.get(0) - ecefPOI.get(0));
//	vector.add(ecefUser.get(1) - ecefPOI.get(1));
//	vector.add(ecefUser.get(2) - ecefPOI.get(2));
//
//	double e = vector.get(0) * (-sinLongiRad) + vector.get(0) * (cosLongiRad);
//	double n = vector.get(0) * (-sinLatRad) * (cosLongiRad)+vector.get(1) * (-sinLatRad) * (sinLongiRad)+vector.get(2) * cosLatRad;
//	double u = vector.get(0) * (cosLatRad) * (cosLongiRad)+vector.get(1) * (cosLatRad) * (sinLongiRad)+vector.get(2) * sinLatRad;
//
//
//	List<Double> enu = new ArrayList<>();
//	enu.add(e);
//	enu.add(n);
//	enu.add(u);
//
//	return enu;
//}
//
//
//
#if 0

int main()
{

	GDALAllRegister();

	//Locations and computation settings
	std::vector<Location> loc_in =
	{
		//{ 5.01,49.95, 0, "test1" },
		//{ 9.92,45.05, 0, "test2" },
		//{8.00038, 46.58210, 2.0, "Eiger_Nordwand"			},
		//{7.92347, 46.60691, 2.0, "Wengen"					},
		//{7.89222, 46.55944, 2.0, "Muerren"					},
		//{8.58639, 46.66777, 2.0, "Goeschenen"				},
		//{7.62583, 46.13556, 2.0, "Zinal"					},
		//{7.82083, 46.42221, 2.0, "Blatten"					},
		//{7.63333, 46.38333, 2.0, "Leukerbad"				},
		//{8.54226, 47.37174, 5.0, "Zuerich"					},
		//{7.55674, 47.25203, 2.0, "Balm_bei_Guensberg"		},
		//{7.92887, 46.35742, 1.0, "Gredetschtal_(east-facing)"},
		//{7.93863, 46.35823, 1.0, "Gredetschtal_(west-facing)"},
		{ 7.988,46.604,0,"Fig8a" },
		{ 7.937,46.587,0,"Fig8b" },
		{ 7.95,46.59,0,"Fig8c" }
	};

	//std::vector<Location> loc_in =
	//{
	//	{8.00038, 46.58210, 2965, "Eiger_Nordwand"			},
	//	//{ 5.01,49.95, 372, "test" },
	//	//{7.9616217495, 46.5850880975, 2071, "Kleine Scheidegg"},
	//	{7.908486916463709, 46.59593755357986, 789, "Lauterbunnen Valley" },
	//	{7.92416,46.59583,1315,"Lauterbunnen Valley(East)"},
	//	{7.89583,46.59499,1385,"Lauterbunnen Valley(West)"},
	//	{7.95,46.59,0,"Fig8c"}
	//	//	{0.1, 0.1, 40, "test1"}
	//};


	//for (size_t i = 0; i<loc.size(); i++)
		//loc[i].z= 0;

	//


	const char* ray_algorithm = "binary_search";
	const char* geom_type = "grid";
	const char* ellps = "WGS84";//Earth's surface approximation (sphere, GRS80 or WGS84)
	int azim_num = 360; // number of azimuth sampling directions[-]
	float hori_acc = 0.1f;  // [degree]
	double dist_search = 50.0;// search distance for horizon[kilometre]
	std::string DEM_name = "srtm_38_03.tif";
	//std::string DEM_name = "test.tif";


	std::vector<double> azim(azim_num);
	for (size_t i = 0; i < azim.size(); i++)
		azim[i] = i * ((2 * M_PI) / azim.size());



	Domain domain = {
			std::min_element(loc_in.begin(), loc_in.end(), [](const auto& lhs, const auto& rhs) { return lhs.x < rhs.x; })->x,
			std::max_element(loc_in.begin(), loc_in.end(), [](const auto& lhs, const auto& rhs) { return lhs.x < rhs.x; })->x,
			std::min_element(loc_in.begin(), loc_in.end(), [](const auto& lhs, const auto& rhs) { return lhs.y < rhs.y; })->y,
			std::max_element(loc_in.begin(), loc_in.end(), [](const auto& lhs, const auto& rhs) { return lhs.y < rhs.y; })->y
	};

	// domain boundaries[degree]
	Domain domain_outer = get_domain(domain, dist_search, ellps);

	std::string dem_file_path = get_path() + "MapInput\\" + DEM_name;
	matrix<pt3> dem0 = load_dem(dem_file_path, domain_outer);
	// Compute ellipsoidal heights
	//matrix<double>undul = undulation(dem);  // [m]
	//for (size_t ij = 0; ij < dem.xy_size(); ij++)
	//	dem[ij].z += undul[ij];


	std::vector<std::array<double, 3>> result2(loc_in.size());

	horizon_results result1;
	result1.reserve(loc_in.size());


	// Compute pt3 coordinates
	//for (size_t i = 0; i < loc_in.size(); i++)
	//	size_t i = 0;
	{
		// Compute ENU coordinates
		Transformer_LLA_ENU T((domain_outer.x_min + domain_outer.x_max) / 2, (domain_outer.y_min + domain_outer.y_max) / 2, 0, ellps);
		//matrix<pt3> dem_enu = T.lla2enu(dem);

		//std::vector<pt3> loc(loc_in.begin() + i, loc_in.begin() + i + 1);
		std::vector<pt3> loc(loc_in.begin(), loc_in.end());

		/*Domain domain = {
			std::min_element(loc_in.begin(), loc_in.end(), [](const auto& lhs, const auto& rhs) { return lhs.x < rhs.x; })->x,
			std::max_element(loc_in.begin(), loc_in.end(), [](const auto& lhs, const auto& rhs) { return lhs.x > rhs.x; })->x,
			std::min_element(loc_in.begin(), loc_in.end(), [](const auto& lhs, const auto& rhs) { return lhs.y < rhs.y; })->y,
			std::max_element(loc_in.begin(), loc_in.end(), [](const auto& lhs, const auto& rhs) { return lhs.y > rhs.y; })->y
		};*/

		// domain boundaries[degree]
		//Domain domain= get_domain(loc[0], dist_search, ellps);

		matrix<pt3> dem = dem0;
		//matrix<pt3> dem = dem0.subset(domain);

		//Transformer_LLA_ENU T(loc[0], ellps);
		//Transformer_LLA_ENU T((domain_outer.x_min + domain_outer.x_max) / 2, (domain_outer.y_min + domain_outer.y_max) / 2, 0, ellps);
		//pt3 test = loc[0] + pt3(12, 12, 0);
		//Transformer_LLA_ENU T(test, ellps);

		matrix<pt3> dem_enu = T.lla2enu(dem);


		// -----------------------------------------------------------------------------
		// Prepare data for selected locations

		// Compute pt3 coordinates

		std::vector <pt3> loc_enu = T.lla2enu(loc);


		// Compute unit vectors(in ENU coordinates)
		//std::vector <pt3> loc_ecef = lla2ecef(loc);
		//std::vector<pt3> norm_ecef = surf_norm(loc);
		//std::vector<pt3> north_ecef = north_dir(loc_ecef, norm_ecef, ellps);

		//std::vector<pt3> norm_enu = T.ecef2enu(norm_ecef);
		//std::vector<pt3> north_enu = T.ecef2enu(north_ecef);

		//std::vector<pt3> norm_enu(loc.size(), { 0,0,1 });//T.surf_norm(loc);
		//std::vector<pt3> north_enu(loc.size(), { 0,1,0 });//T.north_dir(loc, vec_norm_ecef);

		// -----------------------------------------------------------------------------
		// Compute topographic parameters
		// -----------------------------------------------------------------------------


		//Rearrange digital elevation model data and pad geometry buffer
		std::vector<float> vec_dem_enu = dem_enu.vectorize();
		std::vector<float> vec_loc_enu = vectorize(loc_enu);
		//std::vector<float> vec_norm_enu = vectorize(norm_enu);
		//std::vector<float> vec_north_enu = vectorize(north_enu);
		//std::vector<float> ray_org_elev(loc.size(), 2.0);


		assert(vec_dem_enu.size() >= (dem_enu.xy_size() * 3));
		//assert(vec_loc_enu.size() == vec_norm_enu.size());
		//assert(vec_norm_enu.size() == vec_north_enu.size());
		assert(hori_acc <= 10.0);

		//assert(ray_org_elev.size() == 1 || ray_org_elev.size() == loc.size());
		//assert(*std::min_element(ray_org_elev.begin(), ray_org_elev.end()) >= 0.005);//minimal allowed value for 'ray_org_elev' is 0.005 m
		assert(ray_algorithm == "discrete_sampling" || ray_algorithm == "binary_search" || ray_algorithm == "guess_constant");
		assert(geom_type == "triangle" || geom_type == "quad" || geom_type == "grid");

		// Check size of input geometries
		assert(dem_enu.x_size() <= 32767 && dem_enu.x_size() <= 32767);
		//ray_algorithm = "guess_constant";
		//geom_type = "triangle";

		// Compute horizon
		horizon_results r = horizon_locations_comp(
			vec_dem_enu.data(), (int)dem_enu.y_size(), (int)dem_enu.x_size(),//DDEM part
			vec_loc_enu.data()/*, ray_org_elev.data(), vec_norm_enu.data(), vec_north_enu.data()*/, (int)loc.size(),//loc part
			azim_num, (float)dist_search, (float)hori_acc, ray_algorithm, geom_type); //options

		result1.insert(result1.end(), r.begin(), r.end());
		//}



		// Compute slope and sky view factor for locations

		assert(r.size() == loc.size());



		for (size_t i = 0; i < result1.size(); i++)
		{
			//std::vector<pt3> loc(loc_in.begin() + i, loc_in.begin() + i);
		//Transformer_LLA_ENU T(loc[i], ellps);


		//matrix<pt3> dem_5x5 = dem.subset(locations[i], 5);
		//matrix<pt3> dem_5x5_enu = T.lla2enu(dem_5x5);


		// Compute pt3 coordinates
		//std::vector<double> dem_x_5x5_ecef = lonlat2ecef(dem_x_5x5, dem_y_5x5, ellps);
		//matrix<pt3> DEM5x5_ecef = lonlat2ecef(rep(dem_x_5x5, dem_y_5x5.size()), rep(dem_y_5x5, dem_x_5x5.size()), dem_z_5x5, ellps);

		//x_ecef, y_ecef, z_ecef = T.lonlat2ecef(*np.meshgrid(lon[slice_5x5[1]], lat[slice_5x5[0]]), elevation[slice_5x5], ellps = ellps);


		// Compute ENU coordinates
		//x_enu, y_enu, z_enu
		//matrix<ENU> DEM5x5_enu = T.ecef2enu(DEM5x5_ecef);

		// Compute unit vectors(in ENU coordinates)
		//slice_3x3 = (slice(slice_5x5[0].start + 1, slice_5x5[0].stop - 1), slice(slice_5x5[1].start + 1, slice_5x5[1].stop - 1));
		//std::vector<double> dem_x_3x3(dem_x_5x5.begin() + 1, dem_x_5x5.begin() + 4);
		//std::vector<double> dem_y_3x3(dem_y_5x5.begin() + 1, dem_y_5x5.begin() + 4);

		//matrix<pt3> dem_3x3_ecef = lla2ecef(dem_3x3, ellps);
			matrix<pt3> dem_3x3 = dem.subset(loc[i], 3);
			matrix<pt3> dem_3x3_enu = T.lla2enu(dem_3x3);

			//matrix<pt3> dem_3x3_enu = dem_enu.subset(loc_enu[i], 3);


			//std::vector<pt3> dem_3x3_ecef = lonlat2ecef(rep(dem_x_3x3, dem_y_3x3.size()), rep(dem_y_3x3, dem_x_3x3.size()), dem_z_3x3.vector(), ellps);

			//vec_norm_ecef = surf_norm(*np.meshgrid(lon[slice_3x3[1]], lat[slice_3x3[0]]));
			//vec_north_ecef = hray.direction.north_dir(x_ecef[1:-1, 1 : -1],y_ecef[1:-1, 1 : -1],	z_ecef[1:-1, 1 : -1],	vec_norm_ecef, ellps = ellps)

			//matrix<pt3> dem_3x3_norm_ecef = surf_norm(dem_3x3);
			//matrix<pt3> dem_3x3_north_ecef = north_dir(dem_3x3_ecef, dem_3x3_norm_ecef, ellps);

			//matrix<pt3> dem_3x3_norm_enu = T.surf_norm(dem_3x3);
			//matrix<pt3> dem_3x3_north_enu = T.north_dir(dem_3x3);//norm compute twice!!! hummm

			//del x_ecef, y_ecef, z_ecef
			//std::vector<pt3> dem_3x3_norm_enu = T.ecef2enu(dem_3x3_norm_ecef);
			//std::vector<ENU> dem_3x3_north_enu = T.ecef2enu(dem_3x3_north_ecef);
			//del vec_norm_ecef, vec_north_ecef

			// Compute rotation matrix(global ENU->local ENU)
			//glob2loc R_glob2loc = rotation_matrix_glob2loc(dem_3x3_north_enu, dem_3x3_norm_enu);
			//del vec_north_enu, vec_norm_enu

			// Compute slope
			//vec_tilt = slope_plane_meth(x_enu, y_enu, z_enu, rot_mat = rot_mat_glob2loc, output_rot = True)[1:-1, 1 : -1, : ];
			//matrix<pt3> vec_tilt = 
			double slope = 0;
			double aspect = 0;
			get_slope_aspect(dem_3x3_enu, slope, aspect);

			pt3 N = T.Normal(slope, aspect);


			//double ewres = m_extents.XRes();
			//double nsres = m_extents.YRes();

			// Bilinear interpolation of slope at location
			//vec_tilt_ip = np.empty((1, 1, 3), dtype = np.float32);
			//pt3 vec_tilt_ip;
			//for (size_t j = 0; j < 3; j++)
			//{
			//	//f = interpolate.interp2d(*np.meshgrid(lon[slice_3x3[1]], lat[slice_3x3[0]]), vec_tilt[:, : , j], bounds_error = True);
			//	//vec_tilt_ip[0, 0, j] = f(loc_sel[i][1], loc_sel[i][0]);

			//	//loc_sel x and y was revert
			//	//matrix<pt3> m(3, 3);
			//	//dem_x_3x3
			//	//dem_y_3x3

			//	//don't take into account the lat and lon distortion here: todo
			//	//std::array < std::array<double, 3> 3> m;

			//	//bilinear_interpolation()
			//	//.bilinear_interpolate(lon_ip.size(), lat_ip.size());

			//	//take nearest
			//	vec_tilt[j] = vec_tilt(2, 2)[j];//take center cell
			//}

			//double d = sqrt(vec_tilt_ip.x * vec_tilt_ip.x + vec_tilt_ip.y * vec_tilt_ip.y + vec_tilt_ip.z * vec_tilt_ip.z);  // unit vector
			//for (size_t j = 0; j < 3; j++)
			//	vec_tilt_ip[j] /= d;

			//// Compute slope angle and aspect
			//double slope = acos(vec_tilt_ip[2]);
			////double aspect = M_PI / 2.0 - arctan2(vec_tilt_ip[1], vec_tilt_ip[0]);
			//double aspect = M_PI / 2.0 - atan2(vec_tilt_ip[1], vec_tilt_ip[0]);
			//if (aspect < 0.0)
			//	aspect += M_PI * 2.0; // [0.0, 2.0 * np.pi]



			//hori_buffer = np.empty((vec_norm.shape[0], azim_num),
			result2[i][0] = slope;
			result2[i][1] = aspect;// std::fmod(90 - aspect + 360, 360);//aspect from North 
			result2[i][2] = sky_view_factor(azim, r[i], N);
		}
	}


	std::string output_path1 = get_path() + "Output\\Horizon1.csv";
	std::ofstream out1(output_path1);
	out1 << "site,azimut,angle,distance" << std::endl;

	for (size_t i = 0; i < result1.size(); i++)
	{
		assert(azim.size() == result1[i].size());
		for (size_t j = 0; j < result1[i].size(); j++)
		{
			out1 << loc_in[i].m_name << "," << rad2deg(azim[j]) << "," << result1[i][j].hori_angle << "," << result1[i][j].hori_dist << std::endl;
		}
	}

	out1.close();

	std::string output_path2 = get_path() + "Output\\Horizon2.csv";
	std::ofstream out2(output_path2);
	out2 << "site,slope,aspect,svf" << std::endl;


	for (size_t i = 0; i < result2.size(); i++)
	{
		out2 << loc_in[i].m_name << "," << result2[i][0] << "," << result2[i][1] << "," << result2[i][2] << std::endl;
	}
	out2.close();
}

#endif
