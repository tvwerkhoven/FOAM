/*
 simulwfc.cc -- wavefront corrector (membrane) simulator
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

#include <math.h>
#include <string>
#include <gsl/gsl_blas.h>

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include "utils.h"
#include "io.h"
#include "config.h"
#include "csv.h"
#include "pthread++.h"

#include "wfc.h"
#include "devices.h"

#include "simulwfc.h"

using namespace std;

SimulWfc::SimulWfc(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online):
Wfc(io, ptc, name, simulwfc_type, port, conffile, online),
min_actvec_amp(0.01),
wfc_sim(NULL)
{
	io.msg(IO_DEB2, "SimulWfc::SimulWfc()");
	
	add_cmd("simact");
	
	// Configure initial settings
	//! @todo implement try ... catch clauses for all configuration loading
	try {
		actsize = cfg.getdouble("actsize", 0.1);
		actres.x = cfg.getdouble("actresx", 512);
		actres.y = cfg.getdouble("actresy", 512);
		
		Path actpos_file = ptc->datadir + Path(cfg.getstring("actpos_file"));
		io.msg(IO_DEB1, "SimulWfc::SimulWfc(): actsize: %f, res: %dx%d, file: %s", 
					actsize, actres.x, actres.y, actpos_file.c_str());
		
		Csv reader(actpos_file.str());
		for (size_t i=0; i<reader.csvdata.size(); i++) {
			actpos.push_back( fcoord_t(reader.csvdata[i][0], reader.csvdata[i][1]) );
			if (reader.csvdata[i][0] > 1 || reader.csvdata[i][1] > 1)
				throw(std::runtime_error(format("WFC positions should be normalized from [0 to 1) in %s", actpos_file.c_str())));

			io.msg(IO_DEB2, "SimulWfc::SimulWfc(): new actuator at (%g, %g)", 
						 reader.csvdata[i][0], reader.csvdata[i][1]);
		}
	} catch (std::runtime_error &e) {
		fprintf(stderr, "!!! SimulWfc: problem with configuration file: %s\n", e.what());
		throw( std::runtime_error(format("SimulWfc: problem with configuration file: %s", e.what() )) );
	} catch (...) { 
		fprintf(stderr, "!!! SimulWfc: unknown error at initialisation.\n");
		throw( std::runtime_error("SimulWfc: unknown error at initialisation.") );
	}
	
	// Set number of actuators
	real_nact = actpos.size();
	
	// Calibrate to allocate memory
	calibrate();
}

SimulWfc::~SimulWfc() {
	io.msg(IO_DEB2, "SimulWfc::~SimulWfc()");

	//!< @todo Save all device settings back to cfg file
	cfg.set("actsize", actsize);
	cfg.set("actresx", actres.x);
	cfg.set("actresy", actres.y);

	gsl_matrix_free(wfc_sim);
}

int SimulWfc::calibrate() {
	// 'Calibrate' simulator (allocate memory)

	// (Re-)allocate memory for Wfc pattern
	gsl_matrix_free(wfc_sim);
	wfc_sim = gsl_matrix_calloc(actres.y, actres.x);
	
	// Call calibrate() in base class (for wfc_amp)
	return Wfc::calibrate();
}

int SimulWfc::actuate(const bool /*block*/) {
	gsl_matrix_set_zero(wfc_sim);
	
	if (actpos.size() != ctrlparams.ctrl_vec->size)
		return io.msg(IO_ERR, "SimulWfc::actuate() # of actuator position != # of actuator amplitudes!");

	float amp_abssum = gsl_blas_sasum(ctrlparams.ctrl_vec);
	if (amp_abssum < min_actvec_amp) {						// if vector amplitude is small, set WFC 'flat'
		io.msg(IO_INFO, "SimulWfc::actuate() sum(actvec) (%g) < %g, setting to 0", amp_abssum, min_actvec_amp);
		return 0;
	}
	
	for (size_t i=0; i<actpos.size(); i++) {
		float amp = gsl_vector_float_get(ctrlparams.ctrl_vec, i);
		add_gauss(wfc_sim, actpos[i], actsize, clamp(amp, float(-1.0), float(1.0)));
	}
	
	return 0;
}

void SimulWfc::add_gauss(gsl_matrix *const wfc, const fcoord_t pos, const double stddev, const double amp) {
	double sum=0, count=0;
	const double cutoff = 0.05;
	
	// Gaussian function: A * exp( (x-x_0)^2 / (2*stddev^2) )
	// Position 'pos' is normalized from 0 to 1
	for (size_t i=0; i<wfc->size1; i++) {
		double yi = i*1.0/actres.y;
		double valy = exp(-1.0*(yi-pos.y)*(yi-pos.y)/(2.0*stddev*stddev));
		
		// Value < 0.05 and position > gaussian center: we're done
		if (valy < cutoff && yi > pos.y)
			break;
		
		// Only calculate x when y > 0.05*amp
		if (valy > cutoff) {
			for (size_t j=0; j<wfc->size2; j++) {
				double xi = j*1.0/actres.x;
				double valx = exp(-1.0*(xi-pos.x)*(xi-pos.x)/(2.0*stddev*stddev));
				if (valx < cutoff && xi > pos.x)
					break;
				double prev = gsl_matrix_get(wfc, i, j);
				gsl_matrix_set (wfc, i, j, prev + amp * (valy*valx));
				sum += amp * (valy*valx);
				count++;
			}
		}
	}
}

void SimulWfc::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = false;
		
	// If not parsed here, call parent
	if (parsed == false)
		Wfc::on_message(conn, orig);
}
