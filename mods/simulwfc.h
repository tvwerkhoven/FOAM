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
 peaks on top of eachother at the actuator locations.
 
 Configuration params:
 - actpos
 - actsize
 - actres
 
 @todo document SimulWfc class
 */
class SimulWfc: public Wfc {
private:
	std::vector<fcoord_t> actpos;				//!< List of actuator positions (in normalized coordinates from 0 to 1)
	string actpos_f;										//!< File containing actuator positions (for actpos)
	double actsize;											//!< 'Size' of actuators (stddev of gaussians). Should be around the same as the actuator pitch.
	coord_t actres;											//!< Resolution of actuator pattern (i.e., number of pixels)
	
	gsl_matrix *wfc_sim;								//!< Simulated wavefront correction
	
	void add_gauss(gsl_matrix *wfc, const fcoord_t pos, const double stddev, const double amp); //!< Add a Gaussian to an existing matrix *wfc
	
public:
	SimulWfc(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~SimulWfc();
	
	// From Wfc::
	int actuate(gsl_vector_float *wfcamp, gain_t gain); //!< Set Wfc in specific state using custom gain
	int calibrate();										//!< Calibrate actuator
	
	// From Devices::
	void on_message(Connection *const conn, string);
};

#endif // HAVE_SIMULWFC_H

/*!
 \page dev_wfc_simulwfc Simulate wavefront corrector
 
 The SimulWfc class simulates a membrane mirror.
 */

