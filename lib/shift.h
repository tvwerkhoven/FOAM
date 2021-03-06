/*
 shift.h -- Shift measurements header file
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef HAVE_SHIFT_H
#define HAVE_SHIFT_H

#include <gsl/gsl_vector.h>
#include <vector>

#include "io.h"
#include "pthread++.h"
#include "types.h"

/*!
 @brief Image shift calculation class
 
 This is a generic class for calculating 2-dimensional shifts over 
 (subsections) of images. This is primarily meant for Shack-Hartmann wavefront
 sensors (Shwfs::), but can be used in other applications as well.
 
 \section shift_usage Shift usage
 
 calc_shifts() is the public method to be used for calculating image shifts.
 This method fills Shift::workpool with the relevant information and then
 signals the worker threads using 
 
 \section shift_threads Shift thread model
 
 When a Shift instance is created, several worker threads are started 
 (stored in Shift::workers). These threads each call _worker_getid() in turn,
 which is guarded by Shift::work_mutex. Once each thread has its unique ID,
 they wait on Shift::work_cond.
 
 When calc_shifts() is called, Shift::workpool is filled with the job info. 
 Then Shift::work_mutex is locked and Shift::work_cond is broadcasted, but 
 before the broadcast is sent Shift::work_done_mutex is locked by 
 calc_shifts().
 
 As the workers are woken up, they fetch work from Shift::workpool (guarded by 
 Shift::workpool.mutex). When a worker is done and there is no more work in 
 the pool, it increments Shift::workpool.done. If the current thread is the 
 last thread to finish, it broadcasts Shift::work_done_cond to signal the
 main thread that the work is done.
 
 Since calc_shifts() locked Shift::work_done_mutex before broadcasting the 
 work, this ensures that the workers don't signal the main thread before it 
 is ready for it (which might happen when the work is processed very quickly).
 
 */
class Shift {
public:
	typedef enum {
		COG=0,														//!< Center of Gravity method
	} method_t;													//!< Different image shift calculation methods
	
private:
	Io &io;															//!< Message IO
	bool running;												//!< Are we running?
	
	typedef struct jobinfo {
		jobinfo() : bpp(-1), img(NULL), refimg(NULL), shifts(NULL), jobid(-1), done(-1) { }
		method_t method;
		int bpp;													//!< Image bitdepth (8 for uint8_t, 16 for uint16_t)
		void *img;												//!< Image data to process
		coord_t res;											//!< Image size (width x height)
		void *refimg;											//!< Reference image (for method=CORR)
		uint32_t mini;										//!< Minimum intensity to consider (for method=COG)
		std::vector<vector_t> crops;			//!< Crop fields within the bigger image
		fcoord_t maxshift;								//!< Clamp the calculated shifts with this range
		gsl_vector_float *shifts;					//!< Pre-allocated output vector
		pthread::mutex mutex;							//!< Lock for jobid and done
		int jobid;												//!< Next crop field to process
		int done;													//!< Number of worker threads done
	} job_t;
	
	job_t workpool;											//!< Work pool, used by different threads to get work from

	pthread::mutex work_mutex;					//!< Mutex used to limit access to frame data
	pthread::cond work_cond;						//!< Cond used to signal threads about new frames
	
	pthread::mutex work_done_mutex;			//!< Signal for work completed
	pthread::cond work_done_cond;				//!< Cond used for work completed

	int nworker;												//!< Number of workers requested
	int workid;													//!< Worker counter
	std::vector<pthread::thread> workers; //!< Worker threads

	void _worker_func();								//!< Worker function
	int _worker_getid() { return workid++; }
	
	/*! @brief Calculate CoG in a crop field of img
	 
	 @param [in] img Pointer to image data.
	 @param [in] res Resolution of image data (i.e. data stride)
	 @param [in] crop Crop field to process
	 @param [out] *vec Shift found within crop field in img
	 @param [in] mini Minimum intensity to consider
	 */
	void _calc_cog(const uint8_t *img, const coord_t &res, const vector_t &crop, const fcoord_t maxshift, float *vec, const uint8_t mini=0);
	void _calc_cog(const uint16_t *img, const coord_t &res, const vector_t &crop, const fcoord_t maxshift, float *vec, const uint16_t mini=0);
	
public:
	Shift(Io &io, const int nthr=1);
	~Shift();
	
	/*! @brief Calculate shifts in a series of crop fields within an image
	 
	 @param [in] img Pointer to image data.
	 @param [in] res Resolution of image data (i.e. data stride)
	 @param [in] *crops Array of crop field to process
	 @param [in] maxshift Maximum shift to allow, if higher: clamp at +- this value
	 @param [out] *shifts Buffer to hold results (pre-allocated)
	 @param [in] method Tracking method (see method_t)
	 @param [in] wait Block until complete, or return asap
	 @param [in] mini Minimum intensity to consider (for COG)
	 */
	bool calc_shifts(const uint8_t *img, const coord_t res, const std::vector<vector_t> &crops, const fcoord_t maxshift, gsl_vector_float *shifts, const method_t method=COG, const bool wait=true, const uint8_t mini=0);
	bool calc_shifts(const uint16_t *img, const coord_t res, const std::vector<vector_t> &crops, const fcoord_t maxshift, gsl_vector_float *shifts, const method_t method=COG, const bool wait=true, const uint16_t mini=0);
};

#endif // HAVE_SHIFT_H
