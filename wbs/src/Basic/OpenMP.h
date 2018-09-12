//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
#pragma once


#ifdef _OPENMP

#include <omp.h>

#else

typedef void * omp_lock_t;
typedef void * omp_nest_lock_t;
inline void omp_set_num_threads(int _Num_threads){}
inline int omp_get_num_threads(void){ return 1; }
inline int omp_get_max_threads(void){ return 1; }
inline int omp_get_thread_num(void){ return 0; }
inline int omp_get_num_procs(void){ return 1; }
inline void omp_set_dynamic(int _Dynamic_threads){}
inline int omp_get_dynamic(void){ return 0; }
inline int omp_in_parallel(void){ return 0; }
inline void omp_set_nested(int _Nested){}
inline int omp_get_nested(void){ return 0; }
inline void omp_init_lock(omp_lock_t * _Lock){}
inline void omp_destroy_lock(omp_lock_t * _Lock){}
inline void omp_set_lock(omp_lock_t * _Lock){}
inline void omp_unset_lock(omp_lock_t * _Lock){}
inline int omp_test_lock(omp_lock_t * _Lock){ return 0; }
inline void omp_init_nest_lock(omp_nest_lock_t * _Lock){}
inline void omp_destroy_nest_lock(omp_nest_lock_t * _Lock){}
inline void omp_set_nest_lock(omp_nest_lock_t * _Lock){}
inline void omp_unset_nest_lock(omp_nest_lock_t * _Lock){}
inline int omp_test_nest_lock(omp_nest_lock_t * _Lock){ return 0; }
inline double omp_get_wtime(void){ return 0; }
inline double omp_get_wtick(void){ return 0; }

#endif

