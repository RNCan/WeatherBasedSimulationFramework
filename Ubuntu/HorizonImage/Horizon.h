// Horizon.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#pragma once


#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <valarray>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
#include <numeric>

#include "horizon_comp.h"
#include "msvc/packages/geographiclib.2.5.0/include/GeographicLib/Geodesic.hpp"
#include "msvc/packages/geographiclib.2.5.0/include/GeographicLib/Geocentric.hpp"
#include "msvc/packages/geographiclib.2.5.0/include/GeographicLib/GeodesicLine.hpp"
#include "msvc/packages/geographiclib.2.5.0/include/GeographicLib/LocalCartesian.hpp"
#include "msvc/packages/geodetic_utils/include/geodetic_utils/geodetic_conv.hpp"

//#include <GeographicLib/Geocentric.hpp>


//#pragma warning( disable : 4251)
//#include "gdal.h"
//#include "gdal_priv.h"
//#include "ogr_spatialref.h"

//typedef double ELEVATION_TYPE;
//
//using namespace GeographicLib;
//using namespace geodetic_converter;


//std::string get_path()
//{
//	return "G:\\TravauxModels\\Horizon\\";
//}


inline double deg2rad(double deg) {
	return deg * M_PI / 180.0;
}

inline double rad2deg(double rad) {
	return rad * 180.0 / M_PI;
}

inline double square(double x) {
	return x * x;
}


inline double lerp(double c1, double c2, double v1, double v2, double x)
{
	if ((v1 == v2)) return c1;
	double inc = ((c2 - c1) / (v2 - v1)) * (x - v1);
	double val = c1 + inc;
	return val;
};


template <typename T>
inline T get_earth(const char* ellps = "WGS84")
{
	T earth(6370997.0, 0.0);

	if (ellps == nullptr || std::strcmp(ellps, "sphere") == 0)
		earth = T(6370997.0, 0.0);
	else if (std::strcmp(ellps, "GRS80") == 0)
		earth = T(GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f());
	else if (std::strcmp(ellps, "WGS84") == 0)
		earth = T(GeographicLib::Constants::GRS80_a(), 1.0 / 298.257222101);
	else
	{
		assert(false);
	}

	return earth;
}



template <typename T, size_t Rows, size_t Cols>
inline T bilinear_interpolation(const std::array<std::array<T, Cols>, Rows>& matrix, double x, double y)
{
	if (x < 0 || x >= Rows - 1 || y < 0 || y >= Cols - 1)
	{
		// Handle out-of-bounds cases
		return T{};
	}

	size_t row_low = static_cast<size_t>(x);
	size_t row_high = row_low + 1;
	size_t col_low = static_cast<size_t>(y);
	size_t col_high = col_low + 1;

	double row_frac = x - row_low;
	double col_frac = y - col_low;

	T v1 = matrix[row_low][col_low];
	T v2 = matrix[row_low][col_high];
	T v3 = matrix[row_high][col_low];
	T v4 = matrix[row_high][col_high];

	T interpolated = (1 - row_frac) * ((1 - col_frac) * v1 + col_frac * v2) +
		row_frac * ((1 - col_frac) * v3 + col_frac * v4);

	return interpolated;
}



class pt2
{
public:

	double x;
	double y;

};

class pt3
{
public:

	pt3(double _x = 0, double _y = 0, double _z = 0)
	{
		x = _x;
		y = _y;
		z = _z;
	}


	double& operator[](size_t i) { return i == 0 ? x : i == 1 ? y : z; }
	const double& operator[](size_t i)const { return i == 0 ? x : i == 1 ? y : z; }

	//pt3 operator-(const pt3& in) { return pt3({ x - in.x,y - in.y,z - in.z }); }
	pt3 operator-(const pt3& in)const { return pt3({ x - in.x,y - in.y,z - in.z }); }
	pt3 operator+(const pt3& in)const { return pt3({ x + in.x,y + in.y,z + in.z }); }
	pt3 operator/(double f)const { return pt3({ x / f,y / f,z / f }); }

	pt3& operator/=(double f) { x /= f; y /= f; z /= f; return *this; }
	//friend pt3 operator-(const pt3& in1, const pt3& in2) { return pt3({ in1.x - in2.x,in1.y - in2.y,in1.z - in2.z }); }

	std::array<double, 3> array()const { return { x,y,z }; }

	double d()const { return sqrt(x * x + y * y + z * z); }




	double x;
	double y;
	double z;

};

//typedef pt3 pt3;
//typedef pt3 ENU;



class Location : public pt3
{
public:

	Location(double x, double y, double z, const char* name) :
		m_name(name), pt3(x, y, z)
	{

	}

	const char* m_name;
	//double m_latitude;// (latitude [degree], 
	//double m_longitude;//longitude [degree], 
	//double m_elevation;//elevation above surface [m])
};

class Domain
{
public:

	//Domain curved_grid(double dist_search = 50.0, const char* ellps = "WGS84");
	pt3 operator[](size_t i)
	{
		pt3 pt;
		assert(i < 4);
		switch (i)
		{
		case 0: pt = { x_min, y_min, 0 }; break;
		case 1: pt = { x_max, y_min, 0 }; break;
		case 2: pt = { x_max, y_max, 0 }; break;
		case 3: pt = { x_min, y_max, 0 }; break;
		default: assert(false);
		};

		return pt;
	}

	Domain& operator+=(const Domain& in)
	{
		x_min = std::min(x_min, in.x_min);
		x_max = std::max(x_max, in.x_max);
		y_min = std::min(y_min, in.y_min);
		y_max = std::max(y_max, in.y_max);

		return *this;
	}


	double x_min;
	double x_max;
	double y_min;
	double y_max;
};

template <typename T = float>
inline std::vector<T> vectorize(std::vector<pt3> xyz)
{
	std::vector<T> out(xyz.size() * 3);
	for (auto it = xyz.begin(); it != xyz.end(); it++)
	{
		size_t i = std::distance(xyz.begin(), it);
		out[i * 3 + 0] = (float)it->x;
		out[i * 3 + 1] = (float)it->y;
		out[i * 3 + 2] = (float)it->z;
	}

	return out;
}


template <typename T>
class matrix
{
public:

	using container = std::vector<T>;
	using iterator = typename container::iterator;
	using const_iterator = typename container::const_iterator;

	iterator begin() { return m_data.begin(); }
	iterator end() { return m_data.end(); }
	const_iterator begin() const { return m_data.begin(); }
	const_iterator end() const { return m_data.end(); }



	matrix(size_t x_size = 0, size_t y_size = 0, T* data_ptr = nullptr) :
		m_x_size(x_size), m_y_size(y_size),
		m_data(x_size* y_size)
	{
		if (data_ptr)
		{
			m_data.assign(data_ptr, data_ptr + x_size * y_size);
		}
	}

	size_t x_size()const { return m_x_size; }
	size_t y_size()const { return m_y_size; }
	size_t xy_size()const { return m_data.size(); }


	void resize(size_t x_size = 0, size_t y_size = 0)
	{
		m_x_size = x_size;
		m_y_size = y_size;
		m_data.resize(x_size * y_size);
	}

	bool empty()const { return m_data.empty(); }
	void clear() { m_x_size = m_y_size = 0; m_data.clear(); }

	size_t ij(size_t i, size_t j)const { return j * m_x_size + i; }


	T& operator()(size_t i, size_t j) { return m_data[ij(i, j)]; }
	T operator()(size_t i, size_t j) const { return m_data[ij(i, j)]; }


	T& operator[](size_t ij) { return m_data[ij]; }
	T operator[](size_t ij) const { return m_data[ij]; }


	matrix<T>& operator+=(const matrix<T>& in)
	{
		assert(empty() || in.x_size() == in.x_size());
		assert(empty() || in.y_size() == in.y_size());

		if (empty())
		{
			operator=(in);
		}
		else
		{
			for (size_t ij = 0; ij < m_data.size(); ij++)
				m_data[ij] += in.m_data[ij];
		}

		return *this;
	}

	const T* data() const { return m_data.data(); }
	T* data() { return m_data.data(); }


	void flip_up()
	{
		std::vector<T> line(x_size());
		//swap lines
		for (size_t j = 0; j < y_size() / 2; j++)
		{
			iterator f1 = m_data.begin() + j * x_size();
			iterator l1 = m_data.begin() + (j + 1) * x_size() - 1;
			iterator f2 = m_data.begin() + (y_size() - j - 1) * x_size();
			iterator l2 = m_data.begin() + (y_size() - j) * x_size() - 1;
			//swap line
			std::copy(f1, l1, line.begin());//copy first line to tmp
			std::copy(f2, l2, f1);//copy last line to first line 
			std::copy(line.begin(), line.end(), f2);//copy tmp to last line
		}
	}

	matrix<T> bilinear_interpolate(size_t new_x_size, size_t new_y_size)
	{
		matrix<T> out(new_x_size, new_y_size);

		//size_t height = y_size();
		//size_t width = x_size();

		// x and y ratios
		double rx = (double)(x_size()) / (double)(out.x_size()); // old / new
		double ry = (double)(y_size()) / (double)(out.y_size()); // old / new



		// loop through destination image
		for (size_t y = 0; y < out.y_size(); ++y)
		{
			for (size_t x = 0; x < out.x_size(); ++x)
			{
				double sx = std::max(0.0, (x + 0.5) * rx - 0.5);
				double sy = std::max(0.0, (y + 0.5) * ry - 0.5);

				size_t xl = std::min(x_size() - 1, (size_t)std::floor(sx));
				size_t xr = std::min(x_size() - 1, (size_t)std::floor(sx + 1));
				size_t yt = std::min(y_size() - 1, (size_t)std::floor(sy));
				size_t yb = std::min(y_size() - 1, (size_t)std::floor(sy + 1));
				assert(xl < x_size());
				assert(xr < x_size());
				assert(yt < y_size());
				assert(yb < y_size());

				T tl = (*this)(xl, yt);
				T tr = (*this)(xr, yt);
				T bl = (*this)(xl, yb);
				T br = (*this)(xr, yb);

				double t = lerp(tl, tr, (double)xl, (double)xr, sx);
				double b = lerp(bl, br, (double)xl, (double)xr, sx);
				double m = lerp(t, b, (double)yt, (double)yb, sy);
				out(x, y) = (T)m;
			}
		}

		return out;
	}

	//template <typename U>
	size_t get_nearest(T xyz, bool use_z = true)const
	{
		size_t nearest = -1;
		//size_t size = k == 0 ? x_size() : y_size();
		double min_d = std::numeric_limits<double>::max();

		for (size_t ij = 0; ij < xy_size(); ij++)
		{
			pt3 d = xyz - m_data[ij];
			double n = use_z ? sqrt(d.x * d.x + d.y * d.y + d.z * d.z) : sqrt(d.x * d.x + d.y * d.y);
			if (n < min_d)
			{
				min_d = n;
				nearest = ij;
			}
		}

		return nearest;
	}


	matrix<T> subset(size_t pos_x, size_t size_x, size_t pos_y, size_t size_y)const
	{
		assert(pos_x < m_x_size && pos_x + size_x - 1 < m_x_size);
		assert(pos_y < m_y_size && pos_y + size_y - 1 < m_y_size);

		matrix<T> out(size_x, size_y);
		for (size_t j = 0; j < size_y; j++)
		{
			for (size_t i = 0; i < size_x; i++)
			{
				out[j * size_x + i] = m_data[ij(pos_x + i, pos_y + j)];
			}
		}

		return out;
	}

	matrix<pt3> subset(pt3 xyz, size_t n)const
	{
		matrix<pt3> sub(n, n);

		size_t ij = get_nearest(xyz, false);

		size_t i = ij2i(ij);
		size_t j = ij2j(ij);
		for (size_t jj = 0; jj < n; jj++)
		{
			for (size_t ii = 0; ii < n; ii++)
			{
				size_t iii = std::max(0, std::min(int(x_size() - 1), int(i + ii) - int(n / 2)));
				size_t jjj = std::max(0, std::min(int(y_size() - 1), int(j + jj) - int(n / 2)));
				sub(ii, jj) = (*this)(iii, jjj);
			}
		}

		return sub;
	}

	matrix<pt3> subset(Domain domain)const
	{
		std::array<size_t, 4> i;
		std::array<size_t, 4> j;
		for (size_t k = 0; k < 4; k++)
		{
			pt3 xyz = domain[k];
			size_t ij = get_nearest(xyz, false);
			i[k] = ij2i(ij);
			j[k] = ij2j(ij);
		}

		size_t min_i = *min_element(i.begin(), i.end());
		size_t min_j = *min_element(j.begin(), j.end());
		size_t max_i = *max_element(i.begin(), i.end());
		size_t max_j = *max_element(j.begin(), j.end());

		return subset(min_i, max_i - min_i + 1, min_j, max_j - min_j + 1);
	}


	template <typename U = float>
	std::vector<U> vectorize()const
	{
		return ::vectorize<U>(m_data);
	}

	const std::vector<T>& vector()const { return m_data; }
	//std::vector<T>& vector() { return m_data; }

	size_t ij2i(size_t ij)const { return ij - x_size() * ij2j(ij); }
	size_t ij2j(size_t ij)const { return size_t(ij / x_size()); }



	void get_slope_aspect(size_t i, size_t j, double& slope, double& aspect)const
	{
		const matrix<pt3>& dem_enu = *this;
		assert(i < dem_enu.x_size());
		assert(j < dem_enu.y_size());

		//don't compute slope and aspect at the border
		if (i == 0 || j == 0 || i == dem_enu.x_size() - 1 || j == dem_enu.y_size() - 1)
			return;

		//	assert(dem_3x3.x_size() == 3);
			//assert(dem_3x3.y_size() == 3);

		double x_res = (dem_enu(i + 1, j).x - dem_enu(i - 1, j).x) / 2.0;
		double y_res = (dem_enu(i, j + 1).y - dem_enu(i, j - 1).y) / 2.0;
		double scale = 1;


		//slope = 0;
		//aspect = 0;
		//if( !IsValidSlopeWindow(window) )
			//return;

		double dy = ((dem_enu(i - 1, j - 1).z + 2 * dem_enu(i, j - 1).z + dem_enu(i + 1, j - 1).z) -
			(dem_enu(i - 1, j + 1).z + 2 * dem_enu(i, j + 1).z + dem_enu(i + 1, j + 1).z)) / y_res;

		double dx = ((dem_enu(i + 1, j - 1).z + 2 * dem_enu(i + 1, j).z + dem_enu(i + 1, j + 1).z) -
			(dem_enu(i - 1, j - 1).z + 2 * dem_enu(i - 1, j).z + dem_enu(i - 1, j + 1).z)) / x_res;

		double d = sqrt(dx * dx + dy * dy);

		slope = rad2deg(atan(d / (8 * scale)));//[deg]
		aspect = rad2deg(atan2(dy, -dx));//[deg]
		aspect = std::fmod(90 - aspect + 360, 360);//[deg]

		if (dx == 0 && dy == 0)
		{
			// lat area 
			slope = 0;
			aspect = 0;
		}

	}

protected:

	size_t m_x_size;
	size_t m_y_size;
	std::vector<T> m_data;
};

template <typename T>
inline std::vector<T> rep(const std::vector<T>& in, size_t rep)
{
	std::vector<T> out;
	out.reserve(in.size() * rep);
	for (size_t i = 0; i < rep; i++)
		out.insert(out.end(), in.begin(), in.end());

	return out;
}

inline Domain get_domain(pt3 p0, double search_distance, const char* ellps)
{
	GeographicLib::Geodesic earth = get_earth<GeographicLib::Geodesic>(ellps);

	// Calculate the northernmost point
	double lat2_north = 0;
	double lon2_north = 0;// , azi2_north
	earth.Direct(p0.y, p0.x, 0, search_distance * 1000, lat2_north, lon2_north);

	// Calculate the easternmost point
	double lat2_east = 0;
	double lon2_east = 0;
	earth.Direct(p0.y, p0.x, 90, search_distance * 1000, lat2_east, lon2_east);

	// Calculate the southernmost point

	double lat2_south = 0;
	double lon2_south = 0;
	earth.Direct(p0.y, p0.x, 180, search_distance * 1000, lat2_south, lon2_south);

	// Calculate the westernmost point
	double lat2_west = 0;
	double lon2_west = 0;
	earth.Direct(p0.y, p0.x, 270, search_distance * 1000, lat2_west, lon2_west);

	// Find the minimum and maximum latitude and longitude
	Domain domain;
	domain.y_min = std::min(lat2_north, lat2_south);
	domain.y_max = std::max(lat2_north, lat2_south);
	domain.x_min = std::min(lon2_east, lon2_west);
	domain.x_max = std::max(lon2_east, lon2_west);

	return domain;
}

inline Domain get_domain(Domain domain, double dist_search, const char* ellps)
{
	Domain domain_out;
	for (size_t i = 0; i < 4; i++)
	{
		if (i == 0)
			domain_out = get_domain(domain[i], dist_search, ellps);
		else
			domain_out += get_domain(domain[i], dist_search, ellps);
	}

	return domain_out;
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
//matrix<pt3> load_dem(const std::string& DEM_file_path, const Domain& domain_outer)
//{
//	std::vector<double> dem_x;
//	std::vector<double> dem_y;
//	matrix<double> dem_z;
//
//	std::cout << "Read GeoTIFF with GDAL";
//
//	bool bReadOnly = true;
//	unsigned int flag = bReadOnly ? GDAL_OF_READONLY | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR : GDAL_OF_UPDATE | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR;
//	GDALDataset* poDataset = (GDALDataset*)GDALOpenEx(DEM_file_path.c_str(), flag, NULL, NULL, NULL);
//
//	assert(poDataset->GetRasterCount() == 1);
//
//	int raster_size_x = poDataset->GetRasterXSize();
//	int raster_size_y = poDataset->GetRasterYSize();
//
//	std::array<double, 6> gt;
//	poDataset->GetGeoTransform(gt.data());
//
//	//elevation = ds.GetRasterBand(1).ReadAsArray();  // 16 - bit integer
//
//	double x_ulc = gt[0];
//	double y_ulc = gt[3];
//	double d_x = gt[1];
//	double d_y = gt[5];
//
//	// Warning: unclear where sign of n - s pixel resolution is stored!
//	std::vector<double> x_edge(raster_size_x);
//	for (size_t i = 0; i < x_edge.size(); i++)
//	{
//		x_edge[i] = x_ulc + (i + 0.5) * (d_x * raster_size_x) / x_edge.size();
//	}
//
//	//std::vector<double> y_edge(raster_size_y + 1);
//	std::vector<double> y_edge(raster_size_y);
//	for (size_t j = 0; j < y_edge.size(); j++)
//	{
//		y_edge[j] = y_ulc + (j + 0.5) * (d_y * raster_size_y) / y_edge.size();
//	}
//
//	// Crop relevant domain
//	std::array<size_t, 2> i_min_max = { std::numeric_limits<size_t>::max(), 0 };
//
//	dem_x.reserve(size_t((domain_outer.x_max - domain_outer.x_min) / d_x));
//	for (size_t i = 0; i < x_edge.size(); i++)
//	{
//		if (x_edge[i] >= domain_outer.x_min && x_edge[i] <= domain_outer.x_max)
//		{
//			i_min_max[0] = std::min(i_min_max[0], i);
//			i_min_max[1] = std::max(i_min_max[1], i);
//
//			dem_x.push_back(x_edge[i]);
//		}
//
//		//slice_lon[i] = lon_ulc + i * (d_lon * raster_size_x) / lon_edge.size();
//	}
//
//	std::array<size_t, 2> j_min_max = { std::numeric_limits<size_t>::max(), 0 };
//	//std::vector<double> lat;
//	dem_y.reserve(size_t((domain_outer.y_max - domain_outer.y_min) / std::abs(d_y)));
//	for (size_t j = 0; j < y_edge.size(); j++)
//	{
//		if (y_edge[j] >= domain_outer.y_min && y_edge[j] <= domain_outer.y_max)
//		{
//			j_min_max[0] = std::min(j_min_max[0], j);
//			j_min_max[1] = std::max(j_min_max[1], j);
//
//			dem_y.push_back(y_edge[j]);
//		}
//
//		//slice_lon[i] = lon_ulc + i * (d_lon * raster_size_x) / lon_edge.size();
//	}
//
//	assert(i_min_max[0] + dem_x.size() - 1 == i_min_max[1]);
//	assert(j_min_max[0] + dem_y.size() - 1 == j_min_max[1]);
//
//	//elevation = elevation[slice_lat, slice_lon].astype(np.float32)
//	//lon, lat = lon[slice_lon], lat[slice_lat]
//
//
//
//
//	GDALRasterBand* poBand = poDataset->GetRasterBand(1);
//	dem_z.resize(dem_x.size(), dem_y.size());
//	poBand->RasterIO(GF_Read, (int)i_min_max[0], (int)j_min_max[0], (int)dem_x.size(), (int)dem_y.size(), dem_z.data(), (int)dem_x.size(), (int)dem_y.size(), GDT_Float64, 0, NULL);
//
//	matrix<pt3> DEM(dem_x.size(), dem_y.size());
//	for (size_t j = 0; j < dem_y.size(); j++)
//	{
//		for (size_t i = 0; i < dem_x.size(); i++)
//		{
//			size_t ij = j * dem_x.size() + i;
//			{
//				DEM[ij].x = dem_x[i];
//				DEM[ij].y = dem_y[j];
//				DEM[ij].z = dem_z[ij];
//			}
//		}
//	}
//
//	return DEM;
//}


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
//

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


inline void lla2ecef(double lon, double lat, double alt, double& x, double& y, double& z, const char* ellps = "WGS84")
{
	GeographicLib::Geocentric earth = get_earth< GeographicLib::Geocentric>(ellps);
	earth.Forward(lat, lon, alt, x, y, z);
}

inline pt3 lla2ecef(const pt3& lla, const char* ellps = "WGS84")
{
	pt3 ecef;
	lla2ecef(lla.y, lla.x, lla.z, ecef.x, ecef.y, ecef.z);
	return ecef;

}

template <typename Tin, typename Tout = Tin>
inline std::vector<Tout> lla2ecef(const std::vector<Tin>& lla, const char* ellps = "WGS84")
{
	std::vector<Tout> ecef(lla.size());

	GeographicLib::Geocentric earth = get_earth< GeographicLib::Geocentric>(ellps);
	for (size_t i = 0; i < lla.size(); i++)
		earth.Forward(lla[i].y, lla[i].x, lla[i].z, ecef[i].x, ecef[i].y, ecef[i].z);

	return ecef;

}

inline matrix<pt3> lla2ecef(const matrix<pt3>& lla, const char* ellps = "WGS84")
{
	matrix<pt3> ecef(lla.xy_size());

	GeographicLib::Geocentric earth = get_earth< GeographicLib::Geocentric>(ellps);
	for (size_t ij = 0; ij < lla.xy_size(); ij++)
		earth.Forward(lla[ij].y, lla[ij].x, lla[ij].z, ecef[ij].x, ecef[ij].y, ecef[ij].z);

	return ecef;

}



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
matrix<double> undulation(const matrix<pt3>& dem);
//{
//	assert(!dem.empty());
//	//	"""Compute geoid undulation.
//
//	// Spatial coverage of data
//	std::array<std::pair<std::string, Domain>, 1> spat_cov =
//	{ {
//		{"EGM96",    {-180.0, 180.0, -90.0, 90.0 }},
//	} };
//
//
//	// Ensure that latitude values are monotonically increasing
//	bool flip_lat = dem(0, 1).y < dem(0, 0).y;
//	//if (dem(0, 1).y < dem(0, 0).y)
//	//{
//	//	lat_dec = true;
//	//	//lat_ip = lat_ip[::-1]; RSA reverse of negative????
//	//	for (size_t ij = 0; ij < dem.xy_size(); ij++)
//	//		dem.vector()[ij].y *= -1;
//	//}
//
//	// Compute geoid undulation
//
//	matrix<double> data(size_t(360 / 0.25) + 1, size_t(180 / 0.25) + 1);
//
//	std::string file_path = get_path() + "Geoid/EGM96/WW15MGH.GRD";
//	std::ifstream file(file_path);
//
//	std::string line;
//	std::getline(file, line);//skip header
//
//	//std::istringstream input;
//	//input.str("1\n2\n3\n4\n5\n6\n7\n");
//	//int sum = 0;
//
//	//size_t i = 0;
//	//size_t j = 0;
//	//while (std::getline(file, line))
//	//{
//	//	if (!line.empty())
//	//	{
//	//		std::istringstream ss(line);
//	//		for (size_t ii = 0; ii < 8; ii++, i++)
//	//		{
//	//			ELEVATION_TYPE value = 0;
//	//			ss >> value;
//	//			data(i, j) = value;
//	//		}
//	//
//	//		if (i == data.x_size())
//	//		{
//	//			j++;//next line in matrix
//	//			i = 0;
//	//		}
//	//	}
//	//}
//
//	size_t i = 0;
//	size_t j = 0;
//	double value = 0;
//	while (file >> value)
//	{
//		data(i, j) = value;
//		i++;
//		if (i == data.x_size())
//		{
//			j++;//next line in matrix
//			i = 0;
//		}
//	}
//
//
//	file.close();
//
//
//
//
//	assert(false);
//	//need to compute the new extents of the map
//	//ans reprojeect in the proejction of the map
//	//is it really important????
//	matrix<double> data_ip = data.bilinear_interpolate(dem.x_size(), dem.y_size());
//
//	if (flip_lat)
//		data_ip.flip_up();
//
//	return data_ip;
//}


class Transformer_LLA_ENU
{
public:

	Transformer_LLA_ENU(double lon = 0, double lat = 0, double alt = 0, const char* ellps = "WGS84") :
		lla0(lon, lat, alt),
		geocentric_earth(get_earth<GeographicLib::Geocentric>(ellps)),
		geodesic_earth(get_earth<GeographicLib::Geodesic>(ellps)),
		localCartesian(lat, lon, alt, geocentric_earth)
	{
		init_ecef(lon, lat);
	}

	Transformer_LLA_ENU(pt3 lla, const char* ellps = "WGS84") :
		lla0(lla),
		geocentric_earth(get_earth<GeographicLib::Geocentric>(ellps)),
		geodesic_earth(get_earth<GeographicLib::Geodesic>(ellps)),
		localCartesian(lla.y, lla.x, lla.z, geocentric_earth)
	{
		init_ecef(lla.x, lla.y);
	}
	//double lon_or;
	//double lat_or;

	pt3 lla0;
	GeographicLib::Geocentric geocentric_earth;
	GeographicLib::Geodesic geodesic_earth;
	GeographicLib::LocalCartesian localCartesian;

	//pt3 ref_ecef;
	double sin_lon;
	double cos_lon;
	double sin_lat;
	double cos_lat;
	std::array< std::array<double, 3>, 3> R;



	void init(double lon, double lat, double alt = 0, const char* ellps = "WGS84")
	{
		lla0 = pt3(lon, lat, alt);
		geocentric_earth = get_earth<GeographicLib::Geocentric>(ellps);
		geodesic_earth = get_earth<GeographicLib::Geodesic>(ellps);
		localCartesian = GeographicLib::LocalCartesian(lat, lon, alt, geocentric_earth);
		init_ecef(lon, lat);
	}

	void init_ecef(double lon, double lat)
	{
		//ref_ecef = lla2ecef(pt3(lon, lat, alt), ellps);
		//
		////Set 3x3 transformation matrix
		//// Trigonometric 
		sin_lon = sin(deg2rad(lon));
		cos_lon = cos(deg2rad(lon));
		sin_lat = sin(deg2rad(lat));
		cos_lat = cos(deg2rad(lat));

		R =
		{ {
			{ -sin_lon, cos_lon, 0},
			{-sin_lat * cos_lon, -sin_lat * sin_lon, cos_lat},
			{cos_lat * cos_lon, cos_lat * sin_lon, sin_lat}
		} };


		//Rp =
		//{ {
		//	{-sin_lon, -sin_lat * cos_lon, cos_lat * cos_lon},
		//	{cos_lon, -sin_lat * sin_lon, cos_lat * sin_lon},
		//	{0, cos_lat, sin_lat}
		//} };
	}



	pt3 lla2enu(const pt3& lla)
	{
		pt3 enu;

		localCartesian.Forward(lla.y, lla.x, lla.z, enu.x, enu.y, enu.z);

		return enu;
	}


	// Function to transform from pt3 to ENU
	template <typename Tin, typename Tout = Tin>
	std::vector<Tout> lla2enu(const std::vector<Tin>& lla)
	{
		std::vector<Tout> enu(lla.size());

		for (size_t i = 0; i < lla.size(); i++)
			enu[i] = lla2enu(lla[i]);

		return enu;
	}


	matrix<pt3> lla2enu(const matrix<pt3>& lla)
	{
		matrix<pt3> enu(lla.x_size(), lla.y_size());

		for (size_t ij = 0; ij < lla.xy_size(); ij++)
		{
			enu[ij] = lla2enu(lla[ij]);
		}

		return enu;
	}


	pt3 ecef2enu(const pt3& ecef)
	{
		pt3 enu;
		enu[0] = (-sin_lon * ecef[0] + cos_lon * ecef[1]);
		enu[1] = (-sin_lat * cos_lon * ecef[0] - sin_lat * sin_lon * ecef[1] + cos_lat * ecef[2]);
		enu[2] = (+cos_lat * cos_lon * ecef[0] + cos_lat * sin_lon * ecef[1] + sin_lat * ecef[2]);
		return enu;
	}

	std::vector<pt3> ecef2enu(const std::vector<pt3>& ecef)
	{

		std::vector<pt3> enu(ecef.size());

		for (size_t i = 0; i < ecef.size(); i++)
		{
			enu[i] = ecef2enu(ecef[i]);
		}

		return enu;
	}


	//std::array< std::array<double, 3>, 3> Rp;


	/*matrix<pt3> surf_norm(const matrix<pt3>& lla)
	{
		matrix<pt3> norm_ecef(lla.xy_size());

		for (size_t ij = 0; ij < lla.xy_size(); ij++)
		{
			norm_ecef[ij] = surf_norm(lla[ij]);
		}

		return norm_ecef;
	}*/

	//aspect (in deg) clockwise from north
	pt3 Normal(double slope, double aspect)
	{
		
		double x = sin(deg2rad(slope)) * cos(deg2rad(90 - aspect));
		double y = sin(deg2rad(slope)) * sin(deg2rad(90 - aspect));
		double z = cos(deg2rad(slope));
		double d = sqrt(x * x + y * y + z * z);

		return pt3(x, y, z) / d;


		// Distance from the reference point(in meters)
		//double d = 100;
		//
		//// Convert to radians
		//double slope_rad = deg2rad(slope);
		//double aspect_rad = deg2rad(aspect);
		//
		//// Calculate new geodetic coordinates
		//double lat_new = lla.y + (d * std::sin(aspect_rad) * std::cos(slope_rad));
		//double lon_new = lla.x + (d * std::cos(aspect_rad) * std::cos(slope_rad));
		//double alt_new = lla.z + (d * std::sin(slope_rad));
		//
		//// Convert geodetic to ECEF using Geodesic
		////geod = Geodesic(Geodesic.WGS84)  # Or other reference ellipsoid
		//double x, y, z = 0;
		//geocentric_earth.Forward(lat_new, lon_new, alt_new, x, y, z);
		//
		//
		//d = sqrt(x * x + y * y + z * z);
		//
		//return pt3(x, y, z) / d;

		//The normal at the ecef coordinate is the 
		//double x, y, z = 0;
		//localCartesian.Reverse(ecef.x, ecef.y, ecef.z, x, y, z);
		//double d = sqrt(x * x + y * y + z * z);


		//earth.Reverse(ecef.x, ecef.y, ecef.z, x, y, z);

		//Vector3 normal = 
		//geodesic_earth.Normal(lla.y, lla.x, lla.z);


		//return { x / d,y / d,z / d }
	}

	//pt3 north(pt3 lla)
	//{
	//	//pt3 enu;
	//	double azi1=0;
	//	double azi2=0;
	//	geodesic_earth.Inverse(pt0.y, pt0.x, lla.y, lla.y, azi1, azi2);
	//	
	//	localCartesian.Forward(
	//	//# Create a LocalCartesian object
	//	//	local_cartesian = wgs84.Geocentric(lat0, lon0, h0)
	//
	//	//	# Convert to ENU coordinates
	//	//	e, n, u = local_cartesian.Forward(bearing, distance)
	//
	//	//	# The e, n, and u values are the ENU coordinates
	//	//	print(f"ENU coordinates: East = {e}, North = {n}, Up = {u}")
	//
	//	return { azi1, azi2, lla.z - pt0.z };
	//		
	//	//print "The initial direction is {:.3f} degrees.".format(g['azi1'])
	//	//The initial direction is - 57.792 degrees.
	//}
	pt3 norm(const pt3& lla)
	{
	}

	pt3 north(const pt3& lla)
	{
		pt3 pt0;
		geocentric_earth.Forward(lla0.y, lla0.x, lla0.z + 1e20, pt0.x, pt0.y, pt0.z);

		pt3 pt;
		geocentric_earth.Forward(lla.y, lla.x, lla.z, pt.x, pt.y, pt.z);

		pt3 delta = pt - pt0;

		return delta / delta.d();
	}
};

//std::array<double, 3> crossProduct(const pt3& vecA, const pt3& vecB)
//{
//	double c1 = vecA[1] * vecB[2] - vecA[2] * vecB[1];
//	double c2 = vecA[2] * vecB[0] - vecA[0] * vecB[2];
//	double c3 = vecA[0] * vecB[1] - vecA[1] * vecB[0];
//
//	return { c1, c2, c3 };
//}

//typedef std::array < std::array<double, 3>, 3> rotation_matrix;
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

inline void get_slope_aspect(const matrix<pt3>& dem_3x3, double& slope, double& aspect)
{
	assert(dem_3x3.x_size() == 3);
	assert(dem_3x3.y_size() == 3);

	double x_res = (dem_3x3(2,1).x - dem_3x3(0, 1).x) / 2.0;
	double y_res = (dem_3x3(1, 2).y - dem_3x3(1,0).y)/ 2.0;
	double scale = 1;


	//slope = 0;
	//aspect = 0;
	//if( !IsValidSlopeWindow(window) )
		//return;

	double dy = ((dem_3x3(0, 0).z + 2 * dem_3x3(1, 0).z + dem_3x3(2, 0).z) - 
		(dem_3x3(0, 2).z + 2 * dem_3x3(1, 2).z + dem_3x3(2, 2).z) ) / y_res;

	double dx = ((dem_3x3(2, 0).z + 2 * dem_3x3(2, 1).z + dem_3x3(2, 2).z) -
		(dem_3x3(0, 0).z + 2 * dem_3x3(0, 1).z + dem_3x3(0, 2).z) ) / x_res;

	double d = sqrt(dx * dx + dy * dy);

	slope = rad2deg(atan(d / (8 * scale)));//[deg]
	aspect = rad2deg(atan2(dy, -dx));//[deg]
	aspect = std::fmod(90 - aspect + 360, 360);//[deg]

	if (dx == 0 && dy == 0)
	{
		// lat area 
		slope = 0;
		aspect = 0;
	}

	//else //transform from azimut angle to geographic angle
	//{
	//	if (aspect > 90.0f)
	//		aspect = 450.0f - aspect;
	//	else
	//		aspect = 90.0f - aspect;
	//}

	//if (aspect == 360.0)
	//	aspect = 0.0f;
}

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

///Sky view factor (SVF) computation.

//Compute sky view factor(SVF) in local horizontal coordinate system.The
//SVF is defined as the fraction of sky radiation received at a certain
//location in case of isotropic sky radiation.

//Parameters
//----------
//azim: ndarray of float
//Array(one - dimensional) with azimuth[radian]
//hori : ndarray of float
//Array(three - dimensional) with horizon(y, x, azim)[radian]
//vec_tilt : ndarray of float
//Array(three - dimensional) with titled surface normal components
//(y, x, components)[metre]

//Returns
//------ -
//svf : ndarray of float
//Array(two - dimensional) with sky view factor[-]"""
class Azimuth
{
public:
	double angle;
	double sin;
	double cos;
};

template <typename U>
inline U sky_view_factor(std::vector<Azimuth> azimuth, const std::vector<U>& hori, const pt3& vec_tilt)
{
	
	

	double agg = 0.0;
	for (size_t k = 0; k < hori.size(); k++)
	{//	for k in range(len_2) :

			// Compute plane - sphere intersection
		double hori_plane = atan(-azimuth[k].sin * vec_tilt[0] / vec_tilt[2] - azimuth[k].cos * vec_tilt[1] / vec_tilt[2]);

		double hori_elev = 0;
		if (hori[k] >= hori_plane)
			hori_elev = hori[k];
		else
			hori_elev = hori_plane;

		// Compute inner integral
		agg = agg + ((vec_tilt[0] * azimuth[k].sin + vec_tilt[1] * azimuth[k].cos) * ((M_PI / 2.0) - hori_elev - (sin(2.0 * hori_elev) / 2.0)) + vec_tilt[2] * square(cos(hori_elev)));
	}

	double azim_spac = (azimuth[1].angle - azimuth[0].angle);
	U svf = U((azim_spac / (2.0 * M_PI)) * agg);

	return svf;
}

// -----------------------------------------------------------------------------

//inline std::vector<double> visible_sky_fraction(std::vector<double> azim, horizon_results hori, std::vector<pt3> vec_tilt)
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


// Function to convert geodetic (latitude, longitude, altitude) to pt3 coordinates
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
