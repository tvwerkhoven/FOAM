/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.

 $Id$
 */
/*! 
 @file foam_modules-sh.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-07-15
 
 @brief This header file prototypes functions used in SH wavefront sensing.
  */

#ifndef FOAM_MODULES_SH
#define FOAM_MODULES_SH

// LIBRABRIES //
/**************/

#include <gsl/gsl_math.h>			// for small integer powers
#include <gsl/gsl_linalg.h>			// for vectors and stuff
#include "foam.h"
#include "types.h"

// DATATYPES //
/*************/

/*!
 @brief This stores information on SH tracking
 
 The things prefixed by '(user)' must be supplied by the user immediately (i.e.
 hardcode it in or read some configuration file). The (foam) things will be calculcated
 by this or other modules. (runtime) is something the user can change during runtime.
 */
typedef struct {
	int nsubap;						//!< (foam) amount of subapertures used (coordinates stored in subc)
	int skipframes;					//!< (user) amount of frames to skip before measuring WFC influence
	int measurecount;				//!< (user) amount of measurements to average for WFC influence
	
	coord_t cells;					//!< (user) number of cells in this SH WFS (i.e. lenslet resolution)
	coord_t shsize;					//!< (user) pixel resolution per cell
	coord_t track;					//!< (user) tracker window resolution in pixels (i.e. 1/2 of shsize per definition)
	int samxr;					//!< (user) use this for edge erosion or to force maximum distance from reference subapt
	float samini;					//!< (user) use this value as a minimum intensity for valid subapertures
	
	gsl_vector_float *singular;		//!< (foam) stores singular values from SVD (nact big)
	gsl_matrix_float *dmmodes;		//!< (foam) stores dmmodes from SVD (nact*nact big)
	gsl_matrix_float *wfsmodes;		//!< (foam) stores wfsmodes from SVD (nact*nsubap*2 big)
	
	coord_t *subc;					//!< (foam) this will hold the coordinates of each sub aperture
	coord_t *gridc;					//!< (foam) this will hold the grid origin for a certain subaperture
	gsl_vector_float *refc;			//!< (foam) reference displacements (i.e. definition of the origin)
	gsl_vector_float *disp;			//!< (foam) measured displacements (compare with refence for actual shift)
	
	fcoord_t stepc;					//!< (runtime) add this to the reference displacement during correction
	
	char *pinhole;					//!< (user) base filename to store the pinhole calibration (stored in *refc)
	char *influence;				//!< (user) base filename to store the influence matrix (stored in singular/dmmodes/wfsmodes)
} mod_sh_track_t;

/*! 
 @brief This enum distinguished between different alignments of data.
 
 An image can be stored simply in row-major format in a matrix ('default' C
 alignment), or per subaperture (meaning that the first n pixels belong to 
 the first subaperture, followed by the next subaperture), which is used for
 example for the gain and dark images used in fast dark-flat field calibration.
 */
typedef enum {
	ALIGN_RECT,
	ALIGN_SUBAP
} mod_sh_align_t;

// PROTOTYPES //
/**************/

/*!
 @brief Initialize the SH module for a certain configuration.

 This routine allocates some data for you so you don't have to :) 
 Call this before using any routines in this module.
 
 @param [in] *wfsinfo Pointer to the WFS to init for
 @param [in] *shtrack A pre-filled mod_sh_track_t struct with the SH sensor configuration
 */
int shInit(wfs_t *wfsinfo, mod_sh_track_t *shtrack);

/*!
 @brief Selects suitable subapts to work with for a certain WFS
 
 This routine takes a WFS image and selects suitable subapertures to work 
 with. It checks things like minimum intensity, and also provides a means
 to exclude subapertures through 'edge erosion'.
 
 @param [in] *image The image that we need to look for subapts on
 @param [in] data The datatype of 'image'
 @param [in] align The alignment of 'image'
 @param [in] *shtrack The SH sensor configuration for this WFS
 @param [in] *shwfs The wavefront sensor configuration
 */
int shSelSubapts(void *image, foam_datat_t data, mod_sh_align_t align, mod_sh_track_t *shtrack, wfs_t *shwfs);

/*!
 @brief Tracks the targets in center of gravity tracking
 
 This calculates the center of gravity of each subaperture. The coordinates 
 will be relative to the center of the tracking window. Note that this will 
 only work for star-like images, extended images will probably not work.
 
 @param [in] *image The WFS image to do the tracking on
 @param [in] data The datatype of 'image'
 @param [in] align The alignment of 'image'
 @param [in] *shtrack An initialized mod_sh_track_t struct
 @param [out] *aver The average pixel intensity in all tracked subapertures
 @param [out] *max The maximum pixel intensity in all tracked subapertures
 */
int shCogTrack(void *image, foam_datat_t data, mod_sh_align_t align, mod_sh_track_t *shtrack, float *aver, float *max);

/*!
 @brief Calculates the controls to be sent to the various WFC, given displacements.
 
 This function calculates the control signals (voltages) that need to be sent to the
 correcting actuator (such as a DM), given the offsets measured by a SH WFS and a 
 previously completed influence function calibration. It needs the matrices and 
 vectors shtrack->dmmodes, shtrack->singular and shtrack->wfsmodes to be calculated.

 @param [in] *ptc The global configuration struct
 @param [in] *shtrack An initialized mod_sh_track_t struct
 @param [in] wfs The WFS to use as input for voltage calculation
 @param [in] nmodes The number of WFC modes to take into account, should be less than the total nr of actuators in the system
 */
int shCalcCtrl(control_t *ptc, mod_sh_track_t *shtrack, const int wfs, int nmodes);

/*!
 @brief Search for a maxium within (xc, yc) -> (xc+width, yc+height) in a certain WFS output.
 */
void shCogFind(wfs_t *wfsinfo, int xc, int yc, int width, int height, float samini, float *sum, float *cog);


#endif /* FOAM_MODULES_SH */
