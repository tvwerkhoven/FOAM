/*
 simwfs.cc -- simulate a wavefront sensor header
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.	If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef HAVE_SIMWFS_H
#define HAVE_SIMWFS_H

#include <stdint.h>

#include "types.h"
#include "config.h"
#include "io.h"

#include "devices.h"
#include "shwfs.h"
#include "simseeing.h"

const string simwfs_type = "simwfs";

/*!
 @brief This class simulates a wavefront sensor
 
 Given a wavefront, this class images it onto a wavefront sensor (initially 
 only of the Shack-Hartmann type).
 */
class SimWfs : public Device {
private:
	typedef struct shwfs {
		int nmla;									//!< Number of microlenses
		Shwfs::sh_simg_t mla[shwfs_maxlenses]; //!< Microlens array positions
		float f;									//!< Microlens focal length
	} shwfs_t;
	
	shwfs_t mla;								//!< Microlens array of simulated WFS
	
	uint8_t *frame_out;					//!< Data used to store output frame (realloc'ed if necessary)
	size_t out_size;						//!< Size of output frame
	
	
public:
	SimWfs(Io &io, foamctrl *ptc, string name, string port, Path &conffile);
	~SimWfs();
	
	uint8_t *sim_shwfs(gsl_matrix *wavefront);
	bool setup(SimSeeing *ref);	//!< Setup SimWfs instance (i.e. from SimulCam)
	
	/*! @brief Generate subaperture/subimage (sa/si) positions for a given configuration.
	
	@param [in] res Resolution of the sa pattern (before scaling) [pixels]
	@param [in] size size of the sa's [pixels]
	@param [in] pitch pitch of the sa's [pixels]
	@param [in] xoff the horizontal position offset of odd rows [fraction of pitch]
	@param [in] disp global displacement of the sa positions [pixels]
	@param [out] *pattern the calculated subaperture pattern
	@return number of subapertures found
	 */
	Shwfs::sh_simg_t *gen_mla_grid(coord_t res, coord_t size, coord_t pitch, int xoff, coord_t disp, int &nsubap);
};

#endif // HAVE_SIMWFS_H
