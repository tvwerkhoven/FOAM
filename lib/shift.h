/*
 shift.h -- Shift measurements header file
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
		int id;
	} pool_t;
	
	pool_t workpool;										//!< Work pool

	pthread::mutex work_mutex;					//!< Mutex used to limit access to frame data
	pthread::cond work_cond;						//!< Cond used to signal threads about new frames

	int nworker;												//!< Number of workers
	int workid;													//!< Worker counter
	pthread::thread *workers;						//!< Worker threads

	void _worker_func();								//!< Worker function
	int _worker_getid() { return workid++; }
	
public:
	Shift(Io &io, int nthr=4);
	~Shift(void);
	
	bool calc_shifts(void *img, dtype_t dt, coord_t res, crop_t *crops, gsl_vector_float *shifts, mode_t mode=COG);
};

#endif // HAVE_SHIFT_H
