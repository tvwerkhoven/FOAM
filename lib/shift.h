/*
 shift.h -- Shift measurements header file
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include "io.h"
#include "pthread++.h"
#include "types.h"

/*!
 @brief Image shift calculation class (for SHWFS)
 
 This is a generic class for calculating 2-dimensional shifts over 
 (subsections) of images. This is primarily meant for Shack-Hartmann wavefront
 sensors, but can hopefully be used in other applications as well.
 */
class Shift {
public:
	typedef enum {
		UINT8=0,													//!< uint8_t datatype
		UINT16,														//!< uint16_t datatype
	} dtype_t;
	
	typedef enum {
		COG=0,														//!< Center of Gravity method
		CORR,															//!< Cross-correlation method
	} mode_t;														//!< Different image shift calculation modes
		
	// N.B.: This is a copy of Shwfs::sh_simg_t but I want to keep this class standalone
	typedef struct _crop_t {
		coord_t llpos;										//!< Lower-left position of this cropwindow
		coord_t size;											//!< Cropwindow size (pixels)
		size_t id;
		_crop_t(): llpos(0,0), size(0,0), id(0) { ; }
	} crop_t;														//!< Image cropping configuration
	
private:
	Io &io;															//!< Message IO
	
	typedef struct pool {
		mode_t mode;
		int bpp;													//!< Image bitdepth (8 for uint8_t, 16 for uint16_t)
		uint8_t *img;											//!< Image to process
		coord_t res;											//!< Image size (width x height)
		uint8_t *refimg;									//!< Reference image (for method=CORR)
		uint8_t mini;											//!< Minimum intensity to consider (for method=COG)
		crop_t *crops;										//!< Crop fields within the bigger image
		int ncrop;												//!< Number of crop fields
		gsl_vector_float *shifts;					//!< Pre-allocated output vector
		pthread::mutex mutex;							//!< Lock for jobid
		int jobid;												//!< Next crop field to process
		int done;													//!< Number of workers done
	} pool_t;
	
	pool_t workpool;										//!< Work pool

	pthread::mutex work_mutex;					//!< Mutex used to limit access to frame data
	pthread::cond work_cond;						//!< Cond used to signal threads about new frames
	
	pthread::cond work_done_cond;				//!< Signal for work completed
	pthread::mutex work_done_mutex;			//!< Signal for work completed

	int nworker;												//!< Number of workers requested
	int workid;													//!< Worker counter
	pthread::thread *workers;						//!< Worker threads

	void _worker_func();								//!< Worker function
	int _worker_getid() { return workid++; }
	
	void _calc_cog(uint8_t *img, coord_t &res, crop_t &crop, float *vec); //!< Calculate CoG in a crop field of img
	
public:
	Shift(Io &io, int nthr=4);
	~Shift();
	
	bool calc_shifts(uint8_t *img, coord_t res, crop_t *crops, int ncrop, gsl_vector_float *shifts, mode_t mode=COG, bool wait=true);
};

#endif // HAVE_SHIFT_H
