/*
 simulwfc.h -- wavefront corrector (membrane) simulator header
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_SIMULWFC_H
#define HAVE_SIMULWFC_H

#include <gsl/gsl_matrix.h>

#include "io.h"
#include "config.h"
#include "pthread++.h"

#include "wfc.h"
#include "devices.h"

using namespace std;

const string simulwfc_type = "simulwfc";

/*!
 @brief Simulation class for wavefront corrector (membrane mirror)
 
 SimulWfc (dev.wfc.simulwfc) simulates a 'membrane mirror' by adding gaussian
 peaks on top of eachother at the actuator locations specified in a file.
 
 \section simulwfc_cfg Configuration params
 
 - actpos_file: SimulWfc::actpos_f
 - actsize: SimulWfc::actsize
 - actres.x,y: SimulWfc::actres
 
 \section simulwfc_netio Network commands
 
 - none
*/
class SimulWfc: public Wfc {
private:
	std::vector<fcoord_t> actpos;				//!< List of actuator positions (in normalized coordinates from 0 to 1)
	string actpos_f;										//!< File containing actuator positions in CSV format (for SimulWfc::actpos)
	double actsize;											//!< 'Size' of actuators (stddev of gaussians). Should be around the same as the actuator pitch.
	coord_t actres;											//!< Resolution of actuator pattern (i.e., number of pixels) @todo get this from simulcam

	const float min_actvec_amp;					//!< Minimum actuation vector amplitude in order to proceed with simulation.
	
	void add_gauss(gsl_matrix *const wfc, const fcoord_t pos, const double stddev, const double amp); //!< Add a Gaussian to an existing matrix *wfc
	
public:
	SimulWfc(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~SimulWfc();
	
	gsl_matrix *wfc_sim;								//!< Simulated wavefront correction

	// From Wfc::
	int actuate(const bool block=false);
	int calibrate();
	
	// From Devices::
	void on_message(Connection *const conn, string);
};

#endif // HAVE_SIMULWFC_H

/*!
 \page dev_wfc_simulwfc Simulate wavefront corrector
 
 The SimulWfc class simulates a membrane mirror.
 */

