// Copyright (c) 2022 ETH Zurich, Christian R. Steger
// MIT License
#pragma once


#include <array>

#pragma warning( disable : 4244)
#include "embree3/rtcore.h"


// Compute horizon for gridded domain
//
//
//Computes horizon from a Digital Elevation Model(DEM) with Intel Embree
//high performance ray tracing kernels.
//
//Parameters
//----------
//vert_grid : ndarray of float
//Array(one - dimensional) with vertices of DEM[metre]
//dem_dim_0 : int
//Dimension length of DEM in y - direction
//dem_dim_1 : int
//Dimension length of DEM in x - direction
//vec_norm : ndarray of float
//Array(three - dimensional) with surface normal components
//(y, x, components)[metre]
//vec_north : ndarray of float
//Array(three - dimensional) with north vector components
//(y, x, components)[metre]
//offset_0 : int
//Offset of inner domain in y - direction
//offset_1 : int
//Offset of inner domain in x - direction
//dist_search : float
//Search distance for horizon[kilometre]
//azim_num : int
//Number of azimuth sectors
//hori_acc : float
//Accuracy of horizon computation[degree]
//ray_algorithm : str
//Algorithm for horizon detection(discrete_sampling, binary_search,
//    guess_constant)
//    geom_type : str
//    Embree geometry type(triangle, quad, grid)
//    vert_simp : ndarray of float
//    Array(one - dimensional) with vertices of simplified outer DEM[metre]
//    num_vert_simp : int
//    Number of vertices of outer simplified DEM
//    tri_ind_simp : ndarray of int
//    Array(one - dimensional) with vertex indices of triangles
//    num_tri_simp : int
//    Number of triangles
//    elev_ang_low_lim : float
//    Lower limit for elevation angle search[degree]
//    mask : ndarray of uint8
//    Array(two - dimensional) with locations for which horizon is computed
//    hori_fill : float
//    Horizon fill values for masked locations
//    ray_org_elev : float
//    Vertical elevation of ray origin above surface[metre]
//
//    Returns
//    ------ -
//    hori_buffer : ndarray of float
//    Array(three - dimensional) with horizon(y, x, azim_num)[radian]
//    azim : ndarray of float
//    Array(one - dimensional) with azimuth(azim_num)[radian]"""
//np.ndarray[np.float32_t, ndim = 1] vert_grid,
//int dem_dim_0, int dem_dim_1,
//np.ndarray[np.float32_t, ndim = 2] coords,
//np.ndarray[np.float32_t, ndim = 2] vec_norm,
//np.ndarray[np.float32_t, ndim = 2] vec_north,
//float dist_search,
//int azim_num = 360,
//float hori_acc = 0.25,
//str ray_algorithm = "binary_search",
//str geom_type = "grid",
//float elev_ang_low_lim = -89.98,
//np.ndarray[np.float32_t, ndim = 1] ray_org_elev \
//= np.array([0.01], dtype = np.float32),
//bint hori_dist_out = False):

//void horizon_gridded_comp(float* vert_grid, 
//	int dem_dim_0, int dem_dim_1,
//	//float* vec_norm, float* vec_north,
//	int offset_0, int offset_1,
//	float* hori_buffer,
//	int dim_in_0, int dim_in_1,
//	int azim_num=360, float dist_search=50,
//	float hori_acc=0.25, const char* ray_algorithm="binary_search", const char* geom_type= "grid",
//	float* vert_simp = std::array<float, 4>({ 0, 0, 0, 0 }).data(), int num_vert_simp = 1,
//	int* tri_ind_simp = std::array<int, 4>({ 0, 0, 0, 0 }).data(), int num_tri_simp = 1,
//    float elev_ang_low_lim= -89.98,
//    uint8_t* mask=NULL, float hori_fill=0.0,
//    float ray_org_elev=0.01);

// Compute horizon for arbitrary locations
//Computes horizon from a Digital Elevation Model(DEM) with Intel Embree
//high performance ray tracing kernels.
//
//Parameters
//----------
//vert_grid : ndarray of float
//Array(one - dimensional) with vertices of DEM[metre]
//dem_dim_0 : int
//Dimension length of DEM in y - direction
//dem_dim_1 : int
//Dimension length of DEM in x - direction
//coords : ndarray of float
//Array(two - dimensional) with coordinates of locations
//(number of locations, x - / y - / z - coordinates)
//vec_norm : ndarray of float
//Array(two - dimensional) with surface normal components
//(number of locations, components)[metre]
//vec_north : ndarray of float
//Array(two - dimensional) with north vector components
//(number of locations, components)[metre]
//dist_search : float
//Search distance for horizon[kilometre]
//azim_num : int
//Number of azimuth sectors
//hori_acc : float
//Accuracy of horizon computation[degree]
//ray_algorithm : str
//Algorithm for horizon detection(discrete_sampling, binary_search,
//    guess_constant)
//    geom_type : str
//    Embree geometry type(triangle, quad, grid)
//    elev_ang_low_lim : float
//    Lower limit for elevation angle search[degree]
//    ray_org_elev : ndarray of float
//    Vertical elevation of ray origin above surface[metre]
//    hori_dist_out : bool
//    Option to output distance to horizon
//
//    Returns
//    ------ -
//    hori_buffer : ndarray of float
//    Array(two - dimensional) with horizon(number of locations,
//        azim_num)[radian]
//    azim : ndarray of float
//    Array(one - dimensional) with azimuth(azim_num)[radian]"""


//class horizon_result
//{
//public:
//	double hori_angle;
//	double hori_dist;
//};
//
//typedef std::vector < std::vector<horizon_result>> horizon_results;
//
//
//
//horizon_results horizon_locations_comp(
//	float* vert_grid, int dem_dim_0, int dem_dim_1,//DEM part
//	float* coords/*, float* ray_org_elev, float* vec_norm, float* vec_north*/,	int num_loc,//loc part
//	int azim_num=72, float dist_search=50,float hori_acc=0.1f, //option
//	const char* ray_algorithm= "binary_search", const char* geom_type="grid",//option
//    float elev_ang_low_lim= -89.98f,bool hori_dist_out=true//option
//);



//-----------------------------------------------------------------------------
// Declare function pointer and assign function
//-----------------------------------------------------------------------------

typedef void (*def_function_pointer_hori_dist)(double ray_org_x, double ray_org_y, double
	ray_org_z, size_t azim_num, double hori_acc, double dist_search,
	double elev_ang_low_lim, double elev_ang_up_lim, int elev_num,
	RTCScene scene, size_t& num_rays, double* hori_buffer, double* dist_buffer,
	double* azim_sin, double* azim_cos, double* elev_ang,
	double* elev_cos, double* elev_sin, const double(&rot_inv)[3][3]);



typedef void (*def_function_pointer)(double ray_org_x, double ray_org_y, double ray_org_z,
	size_t azim_num, double hori_acc, double dist_search,
	double elev_ang_low_lim, double elev_ang_up_lim, int elev_num,
	RTCScene scene, size_t& num_rays, double* hori_buffer,
	double* azim_sin, double* azim_cos, double* elev_ang,
	double* elev_cos, double* elev_sin, const double(&rot_inv)[3][3]);





class embree_horizon
{
public:

	embree_horizon(size_t  azim_num = 320, double dist_search = 50, double hori_acc = 0.1, double ray_org_elev = 2, double hori_fill = 0, double elev_ang_low_lim = -89.98f, double elev_ang_up_lim = 89.98f);
	~embree_horizon() { clean(); }

	void init(float* vert_grid, size_t  dem_dim_0, size_t  dem_dim_1, const char* ray_algorithm, const char* geom_type);
	size_t compute_horizon(size_t i, size_t j, double* hori_buffer);
	size_t compute_horizon(double x, double y, double z, double* angle_buffer, double* distance_buffer=nullptr);
	

	void clean();
	


protected:

	RTCDevice m_device;
	RTCScene m_scene;

	size_t m_azim_num;
	double m_dist_search;
	double m_hori_acc;
	double m_elev_ang_low_lim;
	double m_elev_ang_up_lim;
	double m_ray_org_elev;
	double m_hori_fill;
	float* m_vert_grid;
	size_t m_dem_dim_0;
	size_t m_dem_dim_1;
	size_t m_elev_num;


	//comnputation variable
	// Azimuth angles (allocate on stack)
	std::vector<double> m_azim_sin;
	std::vector<double> m_azim_cos;
	std::vector<double> m_elev_ang;
	std::vector<double> m_elev_sin;
	std::vector<double> m_elev_cos;
	
	
	
	


	def_function_pointer m_function_pointer_angle_only;
	def_function_pointer_hori_dist  m_function_pointer_angle_and_distance;
	//const char* ray_algorithm;
	//const char* geom_type;
};

