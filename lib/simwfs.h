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
#include "simseeing.h"

const string simwfs_type = "simwfs";
const int SIMWFS_MAXLENSES = 128;

/*!
 @brief This class simulates a wavefront sensor
 
 Given a wavefront, this class images it onto a wavefront sensor (initially 
 only of the Shack-Hartmann type).
 */
class SimWfs : public Device {
private:
	SimSeeing *seeing;					//!< We will get a wavefront from SimSeeing
	
	struct sh_lenslet {
		coord_t pos;
		coord_t size;
		sh_lenslet(): pos(0,0), size(0,0) { ; }
	} sh_lenslet_t;
	
	struct shwfs {
		int nmla;									//!< Number of microlenses
		sh_lenslet_t mla[SIMWFS_MAXLENSES]; //!< Microlens array positions
		float f;									//!< Microlens focal length
	} shwfs_t;
	
public:
	SimWfs(Io &io, foamctrl *ptc, string name, string type, string port, Path &conffile);
	~SimWfs();
	
	gsl_matrix *sim_shwfs(gsl_matrix *wavefront);
	bool setup(...); //! @todo implement setup routine
	shwfs_t gen_mla_grid(...) //! @todo implement this routine (should be in shwfs.cc probably?)
};

#endif // HAVE_SIMWFS_H
