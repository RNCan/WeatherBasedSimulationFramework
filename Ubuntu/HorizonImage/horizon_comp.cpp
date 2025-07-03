// Copyright (c) 2022 ETH Zurich, Christian R. Steger
// MIT License
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <cstdio>
#include <stdio.h>
#include <limits>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <cassert>


//#include "embree4/rtcore.h"
//#include <tbb/parallel_for.h>
//#include <tbb/parallel_reduce.h>
#include "horizon_comp.h"

using namespace std;

//#############################################################################
// Auxiliary functions
//#############################################################################

// Compute linear index from subscripts (2D-array)
inline size_t lin_ind_2d(size_t dim_1, size_t ind_0, size_t ind_1) {
	return (ind_0 * dim_1 + ind_1);
}

// Compute linear index from subscripts (3D-array)
inline size_t lin_ind_3d(size_t dim_1, size_t dim_2, size_t ind_0, size_t ind_1,
	size_t ind_2) {
	return (ind_0 * (dim_1 * dim_2) + ind_1 * dim_2 + ind_2);
}

// Convert degree to radian
inline double deg2rad(double ang) {
	return ((ang / 180.0) * M_PI);
}

// Convert radian to degree
inline double rad2deg(double ang) {
	return ((ang / M_PI) * 180.0);
}

// Cross product
inline void cross_prod(double a_x, double a_y, double a_z, double b_x, double b_y,
	double b_z, double& c_x, double& c_y, double& c_z) {
	c_x = a_y * b_z - a_z * b_y;
	c_y = a_z * b_x - a_x * b_z;
	c_z = a_x * b_y - a_y * b_x;
}

// Matrix vector multiplication
inline void mat_vec_mult(const double(&mat)[3][3], const double(&vec)[3],
	double(&vec_res)[3]) {

	vec_res[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2];
	vec_res[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2];
	vec_res[2] = mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2];

}

//#############################################################################
// Miscellaneous
//#############################################################################

// Namespace
#if defined(RTC_NAMESPACE_USE)
RTC_NAMESPACE_USE
#endif

// Error function
static void errorFunction(void* userPtr, enum RTCError error, const char* str) {
	printf("error %d: %s\n", error, str);
}

// Initialisation of device and registration of error handler
static RTCDevice initializeDevice() {
	RTCDevice device = rtcNewDevice(NULL);
	if (!device) {
		printf("error %d: cannot create device\n", rtcGetDeviceError(NULL));
	}
	rtcSetDeviceErrorFunction(device, errorFunction, NULL);
	return device;
}

//#############################################################################
// Create scene from geometries
//#############################################################################

// Structures for triangle and quad
struct Triangle { int v0, v1, v2; };
struct Quad { int v0, v1, v2, v3; };
// -> above structures must contain 32-bit integers (-> Embree documentation).
//    Theoretically, these integers should be unsigned but the binary
//    representation until 2'147'483'647 is identical between signed/unsigned
//    integer.

// Initialise scene
static RTCScene initializeScene(RTCDevice device, float* vert_grid,
	int dem_dim_0, int dem_dim_1, const char* geom_type,
	float* vert_simp, int num_vert_simp, int* tri_ind_simp, int num_tri_simp) {

	RTCScene scene = rtcNewScene(device);
	rtcSetSceneFlags(scene, RTC_SCENE_FLAG_ROBUST);
	//rtcSetSceneFlags(scene, RTC_SCENE_FLAG_ROBUST | RTC_SCENE_FLAG_COMPACT);
	//rtcSetSceneFlags(scene, RTC_SCENE_FLAG_ROBUST | RTC_SCENE_FLAG_DYNAMIC);
	//rtcSetSceneBuildQuality(scene, RTC_BUILD_QUALITY_LOW);
	//rtcSetSceneBuildQuality(scene, RTC_BUILD_QUALITY_HIGH);

	int num_vert = (dem_dim_0 * dem_dim_1);
	//printf("DEM dimensions: (%d, %d) \n", dem_dim_0, dem_dim_1);
	//printf("Number of vertices: %d \n", num_vert);

	RTCGeometryType rtc_geom_type;
	if (strcmp(geom_type, "triangle") == 0) {
		rtc_geom_type = RTC_GEOMETRY_TYPE_TRIANGLE;
	}
	else if (strcmp(geom_type, "quad") == 0) {
		rtc_geom_type = RTC_GEOMETRY_TYPE_QUAD;
	}
	else {
		rtc_geom_type = RTC_GEOMETRY_TYPE_GRID;
	}

	RTCGeometry geom = rtcNewGeometry(device, rtc_geom_type);
	rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, vert_grid, 0, 3 * sizeof(float), num_vert);

	//-------------------------------------------------------------------------
	// Triangle
	//-------------------------------------------------------------------------
	if (strcmp(geom_type, "triangle") == 0) {
		//cout << "Selected geometry type: triangle" << endl;
		int num_tri = ((dem_dim_0 - 1) * (dem_dim_1 - 1)) * 2;
		//printf("Number of triangles: %d \n", num_tri);
		Triangle* triangles = (Triangle*)rtcSetNewGeometryBuffer(geom,
			RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle),
			num_tri);
		int n = 0;
		for (int i = 0; i < (dem_dim_0 - 1); i++) {
			for (int j = 0; j < (dem_dim_1 - 1); j++) {
				triangles[n].v0 = (i * dem_dim_1) + j;
				triangles[n].v1 = (i * dem_dim_1) + j + 1;
				triangles[n].v2 = ((i + 1) * dem_dim_1) + j;
				n++;
				triangles[n].v0 = (i * dem_dim_1) + j + 1;
				triangles[n].v1 = ((i + 1) * dem_dim_1) + j + 1;
				triangles[n].v2 = ((i + 1) * dem_dim_1) + j;
				n++;
			}
		}
		//-------------------------------------------------------------------------
		// Quad
		//-------------------------------------------------------------------------
	}
	else if (strcmp(geom_type, "quad") == 0) {
		//cout << "Selected geometry type: quad" << endl;
		int num_quad = ((dem_dim_0 - 1) * (dem_dim_1 - 1));
		//printf("Number of quads: %d \n", num_quad);
		Quad* quads = (Quad*)rtcSetNewGeometryBuffer(geom,
			RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT4, sizeof(Quad),
			num_quad);
		int n = 0;
		for (int i = 0; i < (dem_dim_0 - 1); i++) {
			for (int j = 0; j < (dem_dim_1 - 1); j++) {
				//  identical to grid scene (-> otherwise reverse v0, v1, ...)
				quads[n].v0 = (i * dem_dim_1) + j;
				quads[n].v1 = (i * dem_dim_1) + j + 1;
				quads[n].v2 = ((i + 1) * dem_dim_1) + j + 1;
				quads[n].v3 = ((i + 1) * dem_dim_1) + j;
				n++;
			}
		}
		//-------------------------------------------------------------------------
		// Grid
		//-------------------------------------------------------------------------  	
	}
	else {
		//cout << "Selected geometry type: grid" << endl;
		RTCGrid* grid = (RTCGrid*)rtcSetNewGeometryBuffer(geom,
			RTC_BUFFER_TYPE_GRID, 0, RTC_FORMAT_GRID, sizeof(RTCGrid), 1);
		grid[0].startVertexID = 0;
		grid[0].stride = dem_dim_1;
		grid[0].width = dem_dim_1;
		grid[0].height = dem_dim_0;
	}
	//-------------------------------------------------------------------------

	auto start = std::chrono::high_resolution_clock::now();

	// Commit geometry
	rtcCommitGeometry(geom);

	rtcAttachGeometry(scene, geom);
	rtcReleaseGeometry(geom);

	//-------------------------------------------------------------------------
	// Add triangles for outer simplified domain
	//-------------------------------------------------------------------------

	if (num_vert_simp >= 3) {

		//cout << "Add triangles for outer simplified domain" << endl;
		//printf("- number of verties: %d \n", num_vert_simp);
		//printf("- number of triangles: %d \n", num_tri_simp);

		RTCGeometry geom_add = rtcNewGeometry(device,
			RTC_GEOMETRY_TYPE_TRIANGLE);
		rtcSetSharedGeometryBuffer(geom_add, RTC_BUFFER_TYPE_VERTEX, 0,
			RTC_FORMAT_FLOAT3, vert_simp, 0, 3 * sizeof(float), num_vert_simp);
		rtcSetSharedGeometryBuffer(geom_add, RTC_BUFFER_TYPE_INDEX, 0,
			RTC_FORMAT_UINT3, tri_ind_simp, 0, 3 * sizeof(int), num_tri_simp);

		// Commit geometry
		rtcCommitGeometry(geom_add);

		rtcAttachGeometry(scene, geom_add);
		rtcReleaseGeometry(geom_add);

	}

	//-------------------------------------------------------------------------

	// Commit scene
	rtcCommitScene(scene);

	//auto end = std::chrono::high_resolution_clock::now();
	//std::chrono::duration<double> time = end - start;
	//cout << "BVH build time: " << time.count() << " s" << endl;

	return scene;

}

//#############################################################################
// Ray casting
//#############################################################################

//-----------------------------------------------------------------------------
// Cast single ray (occluded; higher performance)
//-----------------------------------------------------------------------------

bool castRay_occluded1(RTCScene scene, double ox, double oy, double oz, double dx,
	double dy, double dz, double dist_search) {

	// Intersect context
	struct RTCIntersectContext context;
	rtcInitIntersectContext(&context);
	//struct RTCOccludedArguments args = { 0 };
	//rtcInitRayQueryContext(args.context);


	// Ray structure
	struct RTCRay ray;
	ray.org_x = ox;
	ray.org_y = oy;
	ray.org_z = oz;
	ray.dir_x = dx;
	ray.dir_y = dy;
	ray.dir_z = dz;
	ray.tnear = 0.0;
	//ray.tfar = std::numeric_limits<double>::infinity();
	ray.tfar = dist_search;
	//ray.mask = -1;
	//ray.flags = 0;

	// Intersect ray with scene
	rtcOccluded1(scene, &context, &ray);
	//rtcOccluded1(scene, &context&ray);

	return (ray.tfar < 0.0);

}

//-----------------------------------------------------------------------------
// Cast single ray (intersect1; lower performance)
//-----------------------------------------------------------------------------

bool castRay_intersect1(RTCScene scene, double ox, double oy, double oz, double dx,
	double dy, double dz, double dist_search, double& dist) {

	// Intersect context
	struct RTCIntersectContext context;
	rtcInitIntersectContext(&context);
	//    struct RTCIntersectArguments args;
	  //  rtcInitRayQueryContext(args.context);

		// Ray hit structure
	struct RTCRayHit rayhit;
	rayhit.ray.org_x = ox;
	rayhit.ray.org_y = oy;
	rayhit.ray.org_z = oz;
	rayhit.ray.dir_x = dx;
	rayhit.ray.dir_y = dy;
	rayhit.ray.dir_z = dz;
	rayhit.ray.tnear = 0.0;
	//rayhit.ray.tfar = std::numeric_limits<float>::infinity();
	rayhit.ray.tfar = dist_search;
	//rayhit.ray.mask = -1;
	//rayhit.ray.flags = 0;
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
	rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

	// Intersect ray with scene
	rtcIntersect1(scene, &context, &rayhit);
	//rtcIntersect1(scene, &rayhit);
	dist = rayhit.ray.tfar;

	return (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID);

}

//#############################################################################
// Horizon detection algorithms (horizon elevation angle)
//#############################################################################

//-----------------------------------------------------------------------------
// Discrete sampling
//-----------------------------------------------------------------------------

void ray_discrete_sampling(double ray_org_x, double ray_org_y, double ray_org_z,
	size_t azim_num, double hori_acc, double dist_search,
	double elev_ang_low_lim, double elev_ang_up_lim, int elev_num,
	RTCScene scene, size_t& num_rays, double* hori_buffer,
	const double* azim_sin, const double* azim_cos, const double* elev_ang,
	const double* elev_cos, const double* elev_sin, const double(&rot_inv)[3][3]) {

	for (size_t k = 0; k < azim_num; k++) {

		int ind_elev = 0;
		int ind_elev_prev = 0;
		bool hit = true;
		while (hit) {

			ind_elev_prev = ind_elev;
			ind_elev = min(ind_elev + 10, elev_num - 1);
			double ray[3] = { elev_cos[ind_elev] * azim_sin[k],
							elev_cos[ind_elev] * azim_cos[k],
							elev_sin[ind_elev] };
			double ray_rot[3];
			mat_vec_mult(rot_inv, ray, ray_rot);
			hit = castRay_occluded1(scene, ray_org_x, ray_org_y,
				ray_org_z, ray_rot[0], ray_rot[1], ray_rot[2],
				dist_search);
			num_rays += 1;

		}
		hori_buffer[k] = (elev_ang[ind_elev_prev] + elev_ang[ind_elev]) / 2.0;

	}

}

//-----------------------------------------------------------------------------
// Binary search
//-----------------------------------------------------------------------------

void ray_binary_search(double ray_org_x, double ray_org_y, double ray_org_z,
	size_t azim_num, double hori_acc, double dist_search,
	double elev_ang_low_lim, double elev_ang_up_lim, int elev_num,
	RTCScene scene, size_t& num_rays, double* hori_buffer,
	const double* azim_sin, const double* azim_cos, const double* elev_ang,
	const double* elev_cos, const double* elev_sin, const double(&rot_inv)[3][3]) {

	for (size_t k = 0; k < azim_num; k++) {

		double lim_up = elev_ang_up_lim;
		double lim_low = elev_ang_low_lim;
		double elev_samp = (lim_up + lim_low) / 2.0;
		int ind_elev = ((int)roundf((elev_samp - elev_ang_low_lim)
			/ (hori_acc / 5.0)));

		while (max(lim_up - elev_ang[ind_elev],
			elev_ang[ind_elev] - lim_low) > hori_acc) {

			double ray[3] = { elev_cos[ind_elev] * azim_sin[k],
							elev_cos[ind_elev] * azim_cos[k],
							elev_sin[ind_elev] };
			double ray_rot[3];
			mat_vec_mult(rot_inv, ray, ray_rot);
			bool hit = castRay_occluded1(scene, ray_org_x, ray_org_y,
				ray_org_z, ray_rot[0], ray_rot[1], ray_rot[2],
				dist_search);
			num_rays += 1;

			if (hit) {
				lim_low = elev_ang[ind_elev];
			}
			else {
				lim_up = elev_ang[ind_elev];
			}
			elev_samp = (lim_up + lim_low) / 2.0;
			ind_elev = ((int)roundf((elev_samp - elev_ang_low_lim)
				/ (hori_acc / 5.0)));

		}
		hori_buffer[k] = elev_samp;

	}

}

//-----------------------------------------------------------------------------
// Guess horizon from previous azimuth direction
//-----------------------------------------------------------------------------

void ray_guess_const(double ray_org_x, double ray_org_y, double ray_org_z,
	size_t azim_num, double hori_acc, double dist_search,
	double elev_ang_low_lim, double elev_ang_up_lim, int elev_num,
	RTCScene scene, size_t& num_rays, double* hori_buffer,
	const double* azim_sin, const double* azim_cos, const double* elev_ang,
	const double* elev_cos, const double* elev_sin, const double(&rot_inv)[3][3]) {

	// ------------------------------------------------------------------------
	// First azimuth direction (binary search)
	// ------------------------------------------------------------------------

	double lim_up = elev_ang_up_lim;
	double lim_low = elev_ang_low_lim;
	double elev_samp = (lim_up + lim_low) / 2.0;
	int ind_elev = ((int)roundf((elev_samp - elev_ang_low_lim)
		/ (hori_acc / 5.0)));

	while (max(lim_up - elev_ang[ind_elev],
		elev_ang[ind_elev] - lim_low) > hori_acc) {

		double ray[3] = { elev_cos[ind_elev] * azim_sin[0],
						elev_cos[ind_elev] * azim_cos[0],
						elev_sin[ind_elev] };
		double ray_rot[3];
		mat_vec_mult(rot_inv, ray, ray_rot);
		bool hit = castRay_occluded1(scene, ray_org_x, ray_org_y,
			ray_org_z, ray_rot[0], ray_rot[1], ray_rot[2],
			dist_search);
		num_rays += 1;

		if (hit) {
			lim_low = elev_ang[ind_elev];
		}
		else {
			lim_up = elev_ang[ind_elev];
		}
		elev_samp = (lim_up + lim_low) / 2.0;
		ind_elev = ((int)roundf((elev_samp - elev_ang_low_lim)
			/ (hori_acc / 5.0)));

	}

	hori_buffer[0] = elev_samp;
	int ind_elev_prev_azim = ind_elev;

	// ------------------------------------------------------------------------
	// Remaining azimuth directions (guess horizon from previous
	// azimuth direction)
	// ------------------------------------------------------------------------

	for (size_t k = 1; k < azim_num; k++) {

		// Move upwards
		ind_elev = max(ind_elev_prev_azim - 5, 0);
		int ind_elev_prev = 0;
		bool hit = true;
		int count = 0;
		while (hit) {

			ind_elev_prev = ind_elev;
			ind_elev = min(ind_elev + 10, elev_num - 1);
			double ray[3] = { elev_cos[ind_elev] * azim_sin[k],
							elev_cos[ind_elev] * azim_cos[k],
							elev_sin[ind_elev] };
			double ray_rot[3];
			mat_vec_mult(rot_inv, ray, ray_rot);
			hit = castRay_occluded1(scene, ray_org_x, ray_org_y,
				ray_org_z, ray_rot[0], ray_rot[1], ray_rot[2],
				dist_search);
			num_rays += 1;
			count += 1;

		}

		if (count > 1) {

			elev_samp = (elev_ang[ind_elev_prev] + elev_ang[ind_elev]) / 2.0;
			ind_elev = ((int)roundf((elev_samp - elev_ang_low_lim)
				/ (hori_acc / 5.0)));
			hori_buffer[k] = elev_ang[ind_elev];
			ind_elev_prev_azim = ind_elev;
			continue;

		}

		// Move downwards
		ind_elev = min(ind_elev_prev_azim + 5, elev_num - 1);
		hit = false;
		while (!hit) {

			ind_elev_prev = ind_elev;
			ind_elev = max(ind_elev - 10, 0);
			double ray[3] = { elev_cos[ind_elev] * azim_sin[k],
							elev_cos[ind_elev] * azim_cos[k],
							elev_sin[ind_elev] };
			double ray_rot[3];
			mat_vec_mult(rot_inv, ray, ray_rot);
			hit = castRay_occluded1(scene, ray_org_x, ray_org_y,
				ray_org_z, ray_rot[0], ray_rot[1], ray_rot[2],
				dist_search);
			num_rays += 1;

		}

		elev_samp = (elev_ang[ind_elev_prev] + elev_ang[ind_elev]) / 2.0;
		ind_elev = ((int)roundf((elev_samp - elev_ang_low_lim)
			/ (hori_acc / 5.0)));
		hori_buffer[k] = elev_ang[ind_elev];
		ind_elev_prev_azim = ind_elev;

	}

}

//-----------------------------------------------------------------------------
// Declare function pointer and assign function
//-----------------------------------------------------------------------------

//void (*function_pointer)(double ray_org_x, double ray_org_y, double ray_org_z,
//	size_t azim_num, double hori_acc, double dist_search,
//	double elev_ang_low_lim, double elev_ang_up_lim, int elev_num,
//	RTCScene scene, size_t& num_rays, double* hori_buffer,
//	double* azim_sin, double* azim_cos, double* elev_ang,
//	double* elev_cos, double* elev_sin, const double(&rot_inv)[3][3]);

//#############################################################################
// Horizon detection algorithms (horizon elevation angle and distance)
//#############################################################################

//-----------------------------------------------------------------------------
// Discrete sampling
//-----------------------------------------------------------------------------

void ray_discrete_sampling_hori_dist(double ray_org_x, double ray_org_y,
	double ray_org_z, size_t azim_num, double hori_acc, double dist_search,
	double elev_ang_low_lim, double elev_ang_up_lim, int elev_num,
	RTCScene scene, size_t& num_rays, double* hori_buffer, double* dist_buffer,
	const double* azim_sin, const double* azim_cos, const double* elev_ang,
	const double* elev_cos, const double* elev_sin, const double(&rot_inv)[3][3]) {

	double dist = 0.0;
	double dist_hit = 0.0;

	for (size_t k = 0; k < azim_num; k++) {

		int ind_elev = 0;
		int ind_elev_prev = 0;
		bool hit = true;
		while (hit) {

			ind_elev_prev = ind_elev;
			ind_elev = min(ind_elev + 10, elev_num - 1);
			double ray[3] = { elev_cos[ind_elev] * azim_sin[k],
							elev_cos[ind_elev] * azim_cos[k],
							elev_sin[ind_elev] };
			double ray_rot[3];
			mat_vec_mult(rot_inv, ray, ray_rot);
			hit = castRay_intersect1(scene, ray_org_x, ray_org_y,
				ray_org_z, ray_rot[0], ray_rot[1], ray_rot[2],
				dist_search, dist);
			if (hit) {
				dist_hit = dist;
			}
			num_rays += 1;

		}
		hori_buffer[k] = (elev_ang[ind_elev_prev] + elev_ang[ind_elev]) / 2.0;
		dist_buffer[k] = dist_hit;

	}

}

//-----------------------------------------------------------------------------
// Binary search
//-----------------------------------------------------------------------------

void ray_binary_search_hori_dist(double ray_org_x, double ray_org_y,
	double ray_org_z, size_t azim_num, double hori_acc, double dist_search,
	double elev_ang_low_lim, double elev_ang_up_lim, int elev_num,
	RTCScene scene, size_t& num_rays, double* hori_buffer, double* dist_buffer,
	const double* azim_sin, const double* azim_cos, const double* elev_ang,
	const double* elev_cos, const double* elev_sin, const double(&rot_inv)[3][3]) {

	double dist = 0.0;
	double dist_hit = 0.0;

	for (size_t k = 0; k < azim_num; k++) {

		double lim_up = elev_ang_up_lim;
		double lim_low = elev_ang_low_lim;
		double elev_samp = (lim_up + lim_low) / 2.0;
		int ind_elev = ((int)roundf((elev_samp - elev_ang_low_lim)
			/ (hori_acc / 5.0)));

		while (max(lim_up - elev_ang[ind_elev],
			elev_ang[ind_elev] - lim_low) > hori_acc) {

			double ray[3] = { elev_cos[ind_elev] * azim_sin[k],
							elev_cos[ind_elev] * azim_cos[k],
							elev_sin[ind_elev] };
			double ray_rot[3];
			mat_vec_mult(rot_inv, ray, ray_rot);
			bool hit = castRay_intersect1(scene, ray_org_x, ray_org_y,
				ray_org_z, ray_rot[0], ray_rot[1], ray_rot[2],
				dist_search, dist);
			if (hit) {
				dist_hit = dist;
			}
			num_rays += 1;

			if (hit) {
				lim_low = elev_ang[ind_elev];
			}
			else {
				lim_up = elev_ang[ind_elev];
			}
			elev_samp = (lim_up + lim_low) / 2.0;
			ind_elev = ((int)roundf((elev_samp - elev_ang_low_lim)
				/ (hori_acc / 5.0)));

		}
		hori_buffer[k] = elev_samp;
		dist_buffer[k] = dist_hit;

	}

}


//#############################################################################
// Compute horizon for gridded domain
//#############################################################################

//void horizon_gridded_comp(float* vert_grid,
//	int dem_dim_0, int dem_dim_1,
//	//float* vec_norm, float* vec_north,
//	int offset_0, int offset_1,
//	float* hori_buffer,
//	int dim_in_0, int dim_in_1,
//	int azim_num, float dist_search,
//	float hori_acc, const char* ray_algorithm, const char* geom_type,
//	float* vert_simp, int num_vert_simp,
//	int* tri_ind_simp, int num_tri_simp,
//	float elev_ang_low_lim,
//	uint8_t* mask, float hori_fill,
//	float ray_org_elev) {
//
//	vector<uint8_t> mask_data;
//	if (mask == NULL)
//	{
//		mask_data.resize(dim_in_0 * dim_in_1, 1);
//		mask = mask_data.data();
//	}
//
//	//cout << "--------------------------------------------------------" << endl;
//	//cout << "Horizon computation with Intel Embree" << endl;
//	//cout << "--------------------------------------------------------" << endl;
//
//	// Hard-coded settings
//	float elev_ang_up_lim = 89.98f;  // upper limit for elevation angle [degree]
//
//	// Initialization
//	auto start_ini = std::chrono::high_resolution_clock::now();
//
//	RTCDevice device = initializeDevice();
//	RTCScene scene = initializeScene(device, vert_grid, dem_dim_0, dem_dim_1,
//		geom_type, vert_simp, num_vert_simp, tri_ind_simp, num_tri_simp);
//
//	// Query properties of device
//	// bool cullingEnabled = rtcGetDeviceProperty(device,
//	//	RTC_DEVICE_PROPERTY_BACKFACE_CULLING_ENABLED);
//	// cout << "Backface culling enabled: " << cullingEnabled << endl;
//
//	auto end_ini = std::chrono::high_resolution_clock::now();
//	std::chrono::duration<double> time = end_ini - start_ini;
//	//cout << "Total initialisation time: " << time.count() << " s" << endl;
//
//	// Unit conversions
//	hori_acc = deg2rad(hori_acc);
//	elev_ang_low_lim = deg2rad(elev_ang_low_lim);
//	elev_ang_up_lim = deg2rad(elev_ang_up_lim);
//	dist_search *= 1000.0;  // [kilometre] -> [metre]
//
//	// Select algorithm for horizon detection
//	//cout << "Horizon detection algorithm: ";
//	if (strcmp(ray_algorithm, "discrete_sampling") == 0) {
//		//cout << "discrete_sampling" << endl;
//		function_pointer = ray_discrete_sampling;
//	}
//	else if (strcmp(ray_algorithm, "binary_search") == 0) {
//		//cout << "binary search" << endl;
//		function_pointer = ray_binary_search;
//	}
//	else if (strcmp(ray_algorithm, "guess_constant") == 0) {
//		//cout << "guess horizon from previous azimuth direction" << endl;
//		function_pointer = ray_guess_const;
//	}
//
//	int num_gc_tot = (dim_in_0 * dim_in_1);
//	int num_gc = 0;
//	for (size_t i = 0; i < (size_t)(dim_in_0 * dim_in_1); i++) {
//		if (mask[i] == 1) {
//			num_gc += 1;
//		}
//	}
//	//printf("Number of grid cells for which horizon is computed: %d \n",
//		//num_gc);
//	//cout << "Fraction of total number of grid cells: " << ((float)num_gc
//	//	/ (float)num_gc_tot * 100.0) << " %" << endl;
//
//	float hori_buffer_size = (((float)dim_in_0 * (float)dim_in_1
//		* (float)azim_num * 4.0) / pow(10.0, 9.0));
//	//cout << "Total memory required for horizon output: "
////		<< hori_buffer_size << " GB" << endl;
//
//	size_t num_rays = 0;
//	std::chrono::duration<double> time_ray = std::chrono::seconds(0);
//	std::chrono::duration<double> time_out = std::chrono::seconds(0);
//
//	// ------------------------------------------------------------------------
//	// Allocate and initialise arrays with evaluated trigonometric functions
//	// ------------------------------------------------------------------------ 
//
//	// Azimuth angles (allocate on stack)
//	std::vector<float> azim_sin(azim_num);
//	std::vector<float> azim_cos(azim_num);
//	float ang;
//	for (int i = 0; i < azim_num; i++) {
//		ang = ((2 * M_PI) / azim_num * i);
//		azim_sin[i] = sin(ang);
//		azim_cos[i] = cos(ang);
//	}
//
//	// Elevation angles (allocate on stack)
//	int elev_num = ((int)ceil((elev_ang_up_lim - elev_ang_low_lim)
//		/ (hori_acc / 5.0)) + 1);
//	std::vector<float> elev_ang(elev_num);
//	std::vector<float> elev_sin(elev_num);
//	std::vector<float> elev_cos(elev_num);
//	for (int i = 0; i < elev_num; i++) {
//		ang = elev_ang_up_lim - (hori_acc / 5.0) * i;
//		elev_ang[elev_num - i - 1] = ang;
//		elev_sin[elev_num - i - 1] = sin(ang);
//		elev_cos[elev_num - i - 1] = cos(ang);
//	}
//
//	// ------------------------------------------------------------------------
//	// Perform ray tracing
//	// ------------------------------------------------------------------------
//
//	auto start_ray = std::chrono::high_resolution_clock::now();
//
//	num_rays += tbb::parallel_reduce(
//		tbb::blocked_range<size_t>(0, dim_in_0), 0.0,
//		[&](tbb::blocked_range<size_t> r, size_t num_rays)
//		{  // parallel
//
//		//for (size_t i = 0; i < (size_t)dim_in_0; i++) {  // serial
//			for (size_t i = r.begin(); i < r.end(); ++i) {  // parallel
//				for (size_t j = 0; j < (size_t)dim_in_1; j++) {
//
//					size_t ind_arr = lin_ind_2d(dim_in_1, i, j);
//					size_t ind_hori = lin_ind_3d(dim_in_1, azim_num, i, j, 0);
//					if (mask[ind_arr] == 1) {
//
//						// Get vector components
//						size_t ind_vec = lin_ind_2d(dim_in_1, i, j) * 3;
//						//float norm_x = vec_norm[ind_vec];
//						//float north_x = vec_north[ind_vec];
//						//ind_vec += 1;
//						//float norm_y = vec_norm[ind_vec];
//						//float north_y = vec_north[ind_vec];
//						//ind_vec += 1;
//						//float norm_z = vec_norm[ind_vec];
//						//float north_z = vec_north[ind_vec];
//						float norm_x = 0;//vec_norm[i * 3 + 0];
//						float norm_y = 0;//vec_norm[i * 3 + 1];
//						float norm_z = 1;//vec_norm[i * 3 + 2];
//
//						float north_x = 0;//vec_north[i * 3 + 0];
//						float north_y = 1;//vec_north[i * 3 + 1];
//						float north_z = 0;// vec_north[i * 3 + 2];
//
//						// Ray origin
//						size_t ind_2d = lin_ind_2d(dem_dim_1, i + offset_0,
//							j + offset_1);
//						float ray_org_x = vert_grid[ind_2d * 3 + 0]
//							+ norm_x * ray_org_elev;
//						float ray_org_y = vert_grid[ind_2d * 3 + 1]
//							+ norm_y * ray_org_elev;
//						float ray_org_z = vert_grid[ind_2d * 3 + 2]
//							+ norm_z * ray_org_elev;
//
//						// Compute inverse of rotation matrix
//						float east_x, east_y, east_z;
//						cross_prod(north_x, north_y, north_z,
//							norm_x, norm_y, norm_z,
//							east_x, east_y, east_z);
//						float rot_inv[3][3] = { {east_x, north_x, norm_x},
//											   {east_y, north_y, norm_y},
//											   {east_z, north_z, norm_z} };
//
//						// Perform ray tracing
//						function_pointer(ray_org_x, ray_org_y, ray_org_z,
//							azim_num, hori_acc, dist_search,
//							elev_ang_low_lim, elev_ang_up_lim, elev_num,
//							scene, num_rays, &hori_buffer[ind_hori],
//							azim_sin.data(), azim_cos.data(), elev_ang.data(),
//							elev_cos.data(), elev_sin.data(), rot_inv);
//
//					}
//					else {
//						for (int k = 0; k < azim_num; k++) {
//							hori_buffer[ind_hori] = hori_fill;  // [radian]
//							ind_hori += 1;
//						}
//					}
//
//				}
//			}
//
//			return num_rays;  // parallel
//		}, std::plus<size_t>());  // parallel
//
//	auto end_ray = std::chrono::high_resolution_clock::now();
//	time_ray += (end_ray - start_ray);
//
//	//cout << "Ray tracing time: " << time_ray.count() << " s" << endl;
//
//	// Print number of rays needed for location and azimuth direction
//	//cout << "Number of rays shot: " << num_rays << endl;
//	float ratio = (float)num_rays / (float)(num_gc * azim_num);
//	//printf("Average number of rays per location and azimuth: %.2f \n", ratio);
//
//	// Release resources allocated through Embree
//	rtcReleaseScene(scene);
//	rtcReleaseDevice(device);
//
//	auto end_tot = std::chrono::high_resolution_clock::now();
//	time = end_tot - start_ini;
//	//cout << "Total run time: " << time.count() << " s" << endl;
//
//	//cout << "--------------------------------------------------------" << endl;
//
//}
//
////#############################################################################
//// Compute horizon for arbitrary locations
////#############################################################################
//
//horizon_results  horizon_locations_comp(
//	float* vert_grid, int dem_dim_0, int dem_dim_1,//DEM part
//	float* coords/*, float* ray_org_elev, float* vec_norm, float* vec_north*/, int num_loc,//Loc part
//	int azim_num, float dist_search, float hori_acc, //option
//	const char* ray_algorithm, const char* geom_type,//option
//	float elev_ang_low_lim, bool hori_dist_out//option
//)
//{
//
//	//float* out_hori_buffer=nullptr;
//	//float* out_hori_dist_buffer = nullptr;
//
//	float ray_org_elev = 0;
//	std::vector<float> out_hori_buffer(num_loc * azim_num);
//	std::vector<float> out_hori_dist_buffer(num_loc * azim_num);
//
//
//	//cout << "--------------------------------------------------------" << endl;
//	//cout << "Horizon computation with Intel Embree" << endl;
//	//cout << "--------------------------------------------------------" << endl;
//
//	// Hard-coded settings
//	float elev_ang_up_lim = 89.98f;  // upper limit for elevation angle [degree]
//
//	// Dummy variables for simplified triangle mesh
//	float vert_simp[4] = { 0.0, 0.0, 0.0, 0.0 };
//	int num_vert_simp = 1;
//	int tri_ind_simp[4] = { 0, 0, 0, 0 };
//	int num_tri_simp = 1;
//
//	// Initialization
//	auto start_ini = std::chrono::high_resolution_clock::now();
//
//	RTCDevice device = initializeDevice();
//	RTCScene scene = initializeScene(device, vert_grid, dem_dim_0, dem_dim_1,
//		geom_type, vert_simp, num_vert_simp, tri_ind_simp, num_tri_simp);
//
//	auto end_ini = std::chrono::high_resolution_clock::now();
//	std::chrono::duration<double> time = end_ini - start_ini;
//	//cout << "Total initialisation time: " << time.count() << " s" << endl;
//
//	// Unit conversions
//	hori_acc = deg2rad(hori_acc);
//	elev_ang_low_lim = deg2rad(elev_ang_low_lim);
//	elev_ang_up_lim = deg2rad(elev_ang_up_lim);
//	dist_search *= 1000.0;  // [kilometre] -> [metre]
//
//	//printf("Number of locations for which horizon is computed: %d \n",
//		//num_loc);
//
//	size_t num_rays = 0;
//	std::chrono::duration<double> time_ray = std::chrono::seconds(0);
//	std::chrono::duration<double> time_out = std::chrono::seconds(0);
//
//	// ------------------------------------------------------------------------
//	// Allocate and initialise arrays with evaluated trigonometric functions
//	// ------------------------------------------------------------------------ 
//
//	// Azimuth angles (allocate on stack)
//	std::vector<float> azim_sin(azim_num);
//	std::vector<float> azim_cos(azim_num);
//	float ang;
//	for (int i = 0; i < azim_num; i++) {
//		ang = ((2 * M_PI) / azim_num * i);
//		azim_sin[i] = sin(ang);
//		azim_cos[i] = cos(ang);
//	}
//
//	// Elevation angles (allocate on stack)
//	int elev_num = ((int)ceil((elev_ang_up_lim - elev_ang_low_lim) / (hori_acc / 5.0)) + 1);
//	std::vector<float> elev_ang(elev_num);
//	std::vector<float> elev_sin(elev_num);
//	std::vector<float> elev_cos(elev_num);
//	for (int i = 0; i < elev_num; i++) {
//		ang = elev_ang_up_lim - (hori_acc / 5.0) * i;
//		elev_ang[elev_num - i - 1] = ang;
//		elev_sin[elev_num - i - 1] = sin(ang);
//		elev_cos[elev_num - i - 1] = cos(ang);
//	}
//
//	// ------------------------------------------------------------------------
//	// Perform ray tracing
//	// ------------------------------------------------------------------------
//
//	if (!hori_dist_out) {  // (horizon elevation angle only)
//
//		// Select algorithm for horizon detection
//		//cout << "Horizon detection algorithm: ";
//		if (strcmp(ray_algorithm, "discrete_sampling") == 0) {
//			//cout << "discrete_sampling" << endl;
//			function_pointer = ray_discrete_sampling;
//		}
//		else if (strcmp(ray_algorithm, "binary_search") == 0) {
//			//cout << "binary search" << endl;
//			function_pointer = ray_binary_search;
//		}
//		else if (strcmp(ray_algorithm, "guess_constant") == 0) {
//			//cout << "guess horizon from previous azimuth direction" << endl;
//			function_pointer = ray_guess_const;
//		}
//
//		auto start_ray = std::chrono::high_resolution_clock::now();
//
//		//num_rays += tbb::parallel_reduce(
//			//tbb::blocked_range<size_t>(0, num_loc), 0.0,
//			//[&](tbb::blocked_range<size_t> r, size_t num_rays) {  // parallel
//
//		float norm_x = 0;
//		float norm_y = 0;
//		float norm_z = 1;
//
//		float north_x = 0;
//		float north_y = 1;
//		float north_z = 0;
//
//
//		for (size_t i = 0; i < num_loc; i++) {  // serial
//			//for (size_t i=r.begin(); i<r.end(); ++i) {  // parallel
//
//				// Get vector components
//				//size_t ind_vec = i * 3;
//			//float norm_x = vec_norm[i * 3 + 0];
//			//float norm_y = vec_norm[i * 3 + 1];
//			//float norm_z = vec_norm[i * 3 + 2];
//			//
//			//float north_x = vec_north[i * 3 + 0];
//			//float north_y = vec_north[i * 3 + 1];
//			//float north_z = vec_north[i * 3 + 2];
//
//			// Compute horizon in case location normal intersects with
//			// triangle mesh
//			float ini_x = coords[i * 3 + 0];
//			float ini_y = coords[i * 3 + 1];
//			float ini_z = coords[i * 3 + 2];
//			float dist = 0.0;
//			bool hit = false;
//			hit = castRay_intersect1(scene, ini_x, ini_y, ini_z,
//				norm_x, norm_y, norm_z, 100000.0, dist);
//			if (!hit) {  // check downwards if no intersection is found
//				hit = castRay_intersect1(scene, ini_x, ini_y, ini_z,
//					-norm_x, -norm_y, -norm_z, 100000.0, dist);
//				dist *= -1.0;
//			}
//			if (hit) {
//
//				// Ray origin
//				float ray_org_x = ini_x + norm_x * (dist + ray_org_elev);
//				float ray_org_y = ini_y + norm_y * (dist + ray_org_elev);
//				float ray_org_z = ini_z + norm_z * (dist + ray_org_elev);
//
//				// Compute inverse of rotation matrix
//				float east_x, east_y, east_z;
//				cross_prod(north_x, north_y, north_z,
//					norm_x, norm_y, norm_z,
//					east_x, east_y, east_z);
//				float rot_inv[3][3] = { {east_x, north_x, norm_x},
//									   {east_y, north_y, norm_y},
//									   {east_z, north_z, norm_z} };
//
//				// Perform ray tracing
//				size_t ind_out = i * azim_num;
//				function_pointer(ray_org_x, ray_org_y, ray_org_z,
//					azim_num, hori_acc, dist_search,
//					elev_ang_low_lim, elev_ang_up_lim, elev_num,
//					scene, num_rays, &out_hori_buffer[ind_out],
//					azim_sin.data(), azim_cos.data(), elev_ang.data(),
//					elev_cos.data(), elev_sin.data(), rot_inv);
//
//			}
//
//		}
//
//		//return num_rays;  // parallel
//	//}, std::plus<size_t>());  // parallel
//
//		auto end_ray = std::chrono::high_resolution_clock::now();
//		time_ray += (end_ray - start_ray);
//
//	}
//	else { // (horizon elevation angle and distance)
//
//		// Select algorithm for horizon detection
//		//cout << "Horizon detection algorithm: ";
//		if (strcmp(ray_algorithm, "discrete_sampling") == 0) {
//			//cout << "discrete_sampling" << endl;
//			function_pointer_hori_dist = ray_discrete_sampling_hori_dist;
//		}
//		else if (strcmp(ray_algorithm, "binary_search") == 0) {
//			//cout << "binary search" << endl;
//			function_pointer_hori_dist = ray_binary_search_hori_dist;
//		}
//
//		auto start_ray = std::chrono::high_resolution_clock::now();
//
//		//num_rays += tbb::parallel_reduce(
//			//tbb::blocked_range<size_t>(0, num_loc), 0.0,
//			//[&](tbb::blocked_range<size_t> r, size_t num_rays) {  // parallel
//
//		for (size_t i = 0; i < num_loc; i++) {  // serial
//			//for (size_t i=r.begin(); i<r.end(); ++i) {  // parallel
//
//				// Get vector components
//			//size_t ind_vec = i * 3;
//			float norm_x = 0;//vec_norm[i * 3 + 0];
//			float norm_y = 0;//vec_norm[i * 3 + 1];
//			float norm_z = 1;//vec_norm[i * 3 + 2];
//
//			float north_x = 0;//vec_north[i * 3 + 0];
//			float north_y = 1;//vec_north[i * 3 + 1];
//			float north_z = 0;// vec_north[i * 3 + 2];
//
//			// Compute horizon in case location normal intersects with
//			// triangle mesh
//			float ini_x = coords[i * 3 + 0];
//			float ini_y = coords[i * 3 + 1];
//			float ini_z = coords[i * 3 + 2];
//			float dist = 0.0;
//			bool hit = false;
//			hit = castRay_intersect1(scene, ini_x, ini_y, ini_z,
//				norm_x, norm_y, norm_z, 100000.0, dist);
//			if (!hit) {  // check downwards if no intersection is found
//				hit = castRay_intersect1(scene, ini_x, ini_y, ini_z,
//					-norm_x, -norm_y, -norm_z, 100000.0, dist);
//				dist *= -1.0;
//			}
//			if (hit) {
//
//				// Ray origin
//				float ray_org_x = ini_x + norm_x * (dist + ray_org_elev);
//				float ray_org_y = ini_y + norm_y * (dist + ray_org_elev);
//				float ray_org_z = ini_z + norm_z * (dist + ray_org_elev);
//
//				// Compute inverse of rotation matrix
//				float east_x, east_y, east_z;
//				cross_prod(north_x, north_y, north_z,
//					norm_x, norm_y, norm_z,
//					east_x, east_y, east_z);
//				float rot_inv[3][3] = { {east_x, north_x, norm_x},
//									   {east_y, north_y, norm_y},
//									   {east_z, north_z, norm_z} };
//
//				// Perform ray tracing
//				size_t ind_out = i * azim_num;
//				function_pointer_hori_dist(ray_org_x, ray_org_y, ray_org_z,
//					azim_num, hori_acc, dist_search,
//					elev_ang_low_lim, elev_ang_up_lim, elev_num,
//					scene, num_rays, &out_hori_buffer[ind_out],
//					&out_hori_dist_buffer[ind_out],
//					azim_sin.data(), azim_cos.data(), elev_ang.data(),
//					elev_cos.data(), elev_sin.data(), rot_inv);
//
//			}
//
//		}
//
//		//return num_rays;  // parallel
//	//}, std::plus<size_t>());  // parallel
//
//		auto end_ray = std::chrono::high_resolution_clock::now();
//		time_ray += (end_ray - start_ray);
//
//	}
//
//	//cout << "Ray tracing time: " << time_ray.count() << " s" << endl;
//
//	// Print number of rays needed for location and azimuth direction
//	//cout << "Number of rays shot: " << num_rays << endl;
//	float ratio = (float)num_rays / (float)(num_loc * azim_num);
//	//printf("Average number of rays per location and azimuth: %.2f \n", ratio);
//
//	// Release resources allocated through Embree
//	rtcReleaseScene(scene);
//	rtcReleaseDevice(device);
//
//	auto end_tot = std::chrono::high_resolution_clock::now();
//	time = end_tot - start_ini;
//	//cout << "Total run time: " << time.count() << " s" << endl;
//
//	//cout << "--------------------------------------------------------" << endl;
//
//
//
//	// Recompute azimuth
//   /* cdef np.ndarray[np.float32_t, ndim = 1, mode = "c"] \
//		azim = np.empty(azim_num, dtype = np.float32)
//		cdef int i
//		for i in range(azim_num) :
//			azim[i] = ((2 * np.pi) / azim_num * i)
//
//			if hori_dist_out :
//				return hori_buffer, hori_dist_buffer, azim
//			else :
//				return hori_buffer, azim*/
//	horizon_results result(num_loc);
//	for (size_t l = 0; l < num_loc; l++)
//	{
//		result[l].resize(azim_num);
//		for (size_t a = 0; a < azim_num; a++)
//		{
//			size_t la = l * azim_num + a;
//			//result[l][a].loc = l;
//			result[l][a].hori_angle = rad2deg(out_hori_buffer[la]);//angle in deg
//			result[l][a].hori_dist = out_hori_dist_buffer[la] / 1000.0; // [m] -> [km]
//		}
//	}
//
//	return result;
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//








embree_horizon::embree_horizon(size_t  azim_num, double dist_search, const char* ray_algorithm, double hori_acc, double ray_org_elev, double hori_fill, double elev_ang_low_lim, double elev_ang_up_lim) :
	m_device(nullptr),
	m_scene(nullptr)
{
	// Unit conversions
	m_azim_num = azim_num;
	m_dist_search = dist_search *= 1000.0;  // [kilometre] -> [metre];
	m_hori_acc = deg2rad(hori_acc);
	m_elev_ang_low_lim = deg2rad(elev_ang_low_lim);
	m_elev_ang_up_lim = deg2rad(elev_ang_up_lim);
	m_ray_org_elev = ray_org_elev;
	m_hori_fill = hori_fill;

	// Select algorithm for horizon detection
	//cout << "Horizon detection algorithm: ";
	if (strcmp(ray_algorithm, "discrete_sampling") == 0) {
		//cout << "discrete_sampling" << endl;
		m_function_pointer_angle_only = ray_discrete_sampling;
		m_function_pointer_angle_and_distance = ray_discrete_sampling_hori_dist;
	}
	else if (strcmp(ray_algorithm, "binary_search") == 0) {
		//cout << "binary search" << endl;
		m_function_pointer_angle_only = ray_binary_search;
		m_function_pointer_angle_and_distance = ray_binary_search_hori_dist;
	}
	else if (strcmp(ray_algorithm, "guess_constant") == 0)
	{
		assert(false);// this option is bugged
		//cout << "guess horizon from previous azimuth direction" << endl;
		m_function_pointer_angle_only = ray_guess_const;
		m_function_pointer_angle_and_distance = nullptr;

	}
};







void embree_horizon::init(float* vert_grid, size_t  dem_dim_0, size_t  dem_dim_1, const char* geom_type)
{
	// Initialization
	m_vert_grid = vert_grid;
	m_dem_dim_0 = dem_dim_0;
	m_dem_dim_1 = dem_dim_1;

	m_device = initializeDevice();
	m_scene = initializeScene(m_device, vert_grid, (int)dem_dim_0, (int)dem_dim_1, geom_type, nullptr, 0, nullptr, 0);
	

	// ------------------------------------------------------------------------
	// Allocate and initialise arrays with evaluated trigonometric functions
	// ------------------------------------------------------------------------ 

	// Azimuth angles (allocate on stack)
	m_azim_sin.resize(m_azim_num);
	m_azim_cos.resize(m_azim_num);

	for (size_t i = 0; i < m_azim_num; i++)
	{
		double ang = ((2 * M_PI) / m_azim_num * i);
		m_azim_sin[i] = sin(ang);
		m_azim_cos[i] = cos(ang);
	}

	// Elevation angles (allocate on stack)
	m_elev_num = ((size_t)ceil((m_elev_ang_up_lim - m_elev_ang_low_lim) / (m_hori_acc / 5.0)) + 1);

	m_elev_ang.resize(m_elev_num);
	m_elev_sin.resize(m_elev_num);
	m_elev_cos.resize(m_elev_num);

	for (size_t i = 0; i < m_elev_num; i++)
	{
		double ang = m_elev_ang_up_lim - (m_hori_acc / 5.0) * i;
		m_elev_ang[m_elev_num - i - 1] = ang;
		m_elev_sin[m_elev_num - i - 1] = sin(ang);
		m_elev_cos[m_elev_num - i - 1] = cos(ang);
	}




	// ------------------------------------------------------------------------
	// Perform ray tracing
	// ------------------------------------------------------------------------

	//auto start_ray = std::chrono::high_resolution_clock::now();

}

void embree_horizon::clean()
{
	// Release resources allocated through Embree
	if (m_scene)
		rtcReleaseScene(m_scene);
	m_scene = nullptr;

	if (m_device)
		rtcReleaseDevice(m_device);
	m_device = nullptr;
}



//i=y, j=x
size_t embree_horizon::compute_horizon(size_t i, size_t j, double* hori_buffer)const
{
	size_t num_rays = 0;

	// Ray origin
	size_t ind_2d = lin_ind_2d(m_dem_dim_1, i, j);
	double ray_org_x = m_vert_grid[ind_2d * 3 + 0];
	double ray_org_y = m_vert_grid[ind_2d * 3 + 1];
	double ray_org_z = m_vert_grid[ind_2d * 3 + 2] + m_ray_org_elev;

	static const double rot_inv[3][3] =
	{
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1}
	};


	// Perform ray tracing
	m_function_pointer_angle_only(ray_org_x, ray_org_y, ray_org_z,
		m_azim_num, m_hori_acc, m_dist_search,
		m_elev_ang_low_lim, m_elev_ang_up_lim, (int)m_elev_num,
		m_scene, num_rays, hori_buffer,
		m_azim_sin.data(), m_azim_cos.data(), m_elev_ang.data(),
		m_elev_cos.data(), m_elev_sin.data(), rot_inv);


	return num_rays;  // parallel

}

//#############################################################################
// Compute horizon for arbitrary locations
//#############################################################################
size_t embree_horizon::compute_horizon(double x, double y, double z, double* angle_buffer, double* distance_buffer)const
{

	static const double rot_inv[3][3] =
	{
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1}
	};


	size_t num_rays = 0;
	// ------------------------------------------------------------------------
	// Perform ray tracing
	// ------------------------------------------------------------------------

	if (distance_buffer == nullptr)
	{  
		assert(m_function_pointer_angle_only);
		// (horizon elevation angle only)
		// Compute horizon in case location normal intersects with triangle mesh

		double dist = 0.0;
		bool hit = false;
		hit = castRay_intersect1(m_scene, x, y, z,	0, 0, 1, 100000.0, dist);
		if (!hit)
		{  // check downwards if no intersection is found
			hit = castRay_intersect1(m_scene, x, y, z,	0, 0, -1, 100000.0, dist);
			dist *= -1.0;
		}

		if (hit)
		{

			// Ray origin
			//float ray_org_x = ini_x + norm_x * (dist + ray_org_elev);
			//float ray_org_y = ini_y + norm_y * (dist + ray_org_elev);
			//float ray_org_z = ini_z + norm_z * (dist + ray_org_elev);

			//// Compute inverse of rotation matrix
			//float east_x, east_y, east_z;
			//cross_prod(north_x, north_y, north_z,
			//	norm_x, norm_y, norm_z,
			//	east_x, east_y, east_z);
			//float rot_inv[3][3] = { {east_x, north_x, norm_x},
			//					   {east_y, north_y, norm_y},
			//					   {east_z, north_z, norm_z} };
			
			double ray_org_x = x;
			double ray_org_y = y;
			double ray_org_z = z + m_ray_org_elev;

			
			//// Perform ray tracing
			m_function_pointer_angle_only(ray_org_x, ray_org_y, ray_org_z,
				m_azim_num, m_hori_acc, m_dist_search,
				m_elev_ang_low_lim, m_elev_ang_up_lim, (int)m_elev_num,
				m_scene, num_rays, angle_buffer,
				m_azim_sin.data(), m_azim_cos.data(), m_elev_ang.data(),
				m_elev_cos.data(), m_elev_sin.data(), rot_inv);

		}

	}
	else  // (horizon elevation angle and distance)
	{
		assert(m_function_pointer_angle_and_distance);

		// Compute horizon in case location normal intersects with triangle mesh

		double dist = 0;
		bool hit = false;
		hit = castRay_intersect1(m_scene, x, y, z, 0, 0, 1, 100000.0, dist);
		
		if (!hit) 
		{  // check downwards if no intersection is found
			hit = castRay_intersect1(m_scene, x, y, z, 0, 0, -1, 100000.0, dist);
			dist *= -1.0;
		}

		if (hit) {

			// Ray origin
			float ray_org_x = x;
			float ray_org_y = y;
			float ray_org_z = z + m_ray_org_elev;

			

			// Perform ray tracing
			
			m_function_pointer_angle_and_distance(ray_org_x, ray_org_y, ray_org_z,
				m_azim_num, m_hori_acc, m_dist_search,
				m_elev_ang_low_lim, m_elev_ang_up_lim, (int)m_elev_num,
				m_scene, num_rays, angle_buffer, distance_buffer,
				m_azim_sin.data(), m_azim_cos.data(), m_elev_ang.data(),
				m_elev_cos.data(), m_elev_sin.data(), rot_inv);
		}
	}

	return num_rays;
}


















