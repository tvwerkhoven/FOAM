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

const string dev_type = "simwfs";

/*!
 @brief This class simulates a wavefront sensor
 
 Given a wavefront, this class images it onto a wavefront sensor (initially 
 only of the Shack-Hartmann type).
 */
class SimWfs : public Device {
private:
	SimSeeing *seeing;					//!< We will get a wavefront from SimSeeing
	
	struct shwfs_t {
		coord_t lensarray;				//!< Number of microlenses
		float f;									//!< Microlens focal length
	};
	
public:
	SimWfs(Io &io, foamctrl *ptc, string name, string type, string port, string conffile);
	~SimWfs();
	
	gsl_matrix *sim_shwfs(gsl_matrix *wavefront);
}

#endif // HAVE_SIMWFS_H
