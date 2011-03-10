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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include "io.h"
#include "config.h"
#include "csv.h"
#include "pthread++.h"

#include "wfc.h"
#include "devices.h"

#include "simulwfc.h"

using namespace std;

SimulWfc::SimulWfc(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online):
Wfc(io, ptc, name, simulwfc_type, port, conffile, online)
{
	io.msg(IO_DEB2, "SimulWfc::SimulWfc()");
	// Register network commands with base device:
	add_cmd("get gain");
	add_cmd("set gain");
	
	add_cmd("act");
	
	// Configure initial settings
	actsize = cfg.getdouble("actsize", 0.2);
	actres.x = cfg.getdouble("actresx", 512);
	actres.y = cfg.getdouble("actresy", 512);
	
	string actpos_file = cfg.getstring("actpos_file");
	
	Csv reader(actpos_file);
	for (size_t i=0; i<reader.csvdata.size(); i++)
		actpos.push_back( fcoord_t(reader.csvdata[i][0], reader.csvdata[i][1]) );
	
}

SimulWfc::~SimulWfc() {
	io.msg(IO_DEB2, "SimulWfc::~SimulWfc()");

	//!< @todo Save all device settings back to cfg file
	cfg.set("actsize", actsize);
	cfg.set("actresx", actres.x);
	cfg.set("actresy", actres.y);
}

int SimulWfc::calibrate() {
	// 'Calibrate' simulator

	if (wfc_sim)
		gsl_matrix_free(wfc_sim);
	
	// (Re-)allocate memory for Wfc pattern
	wfc_sim = gsl_matrix_calloc(actres.y, actres.x);
	
	
	is_calib = true;
	
	return 0;
}

int SimulWfc::actuate(gsl_vector_float *wfcamp, gain_t /* gain */) {
	//!< @todo Implement gain here
	if (actpos.size() != wfcamp->size)
		return io.msg(IO_ERR, "SimulWfc::actuate() # of actuator position != # of actuator amplitudes!");
	
	if (!is_calib)
		calibrate();
	
	gsl_matrix_set_zero(wfc_sim);
	
	for (size_t i=0; i<actpos.size(); i++) {
		float amp = gsl_vector_float_get(wfcamp, i);
		io.msg(IO_WARN, "Wavefront corrector amplitude saturated, abs(%g) > 1!", amp);
		add_gauss(wfc_sim, actpos[i], actsize, clamp(amp, float(-1.0), float(1.0)));
	}
	
	return 0;
}

void SimulWfc::add_gauss(gsl_matrix *wfc, const fcoord_t pos, const double stddev, const double amp) {

	for (size_t i=0; i<wfc->size1; i++) {
		double yi = i*1.0/actres.y;
		double valy = exp((yi-pos.y)*(yi-pos.y)/(2*stddev*stddev));
		
		// Value = 0 and position > gaussian center: we're done
		if (valy == 0 && yi > pos.y)
			break;
		
		// Only calculate x when y != 0
		if (valy != 0) {
			for (size_t j=0; j<wfc->size2; j++) {
				double xi = j*1.0/actres.x;
				double valx = exp((xi-pos.x)*(xi-pos.x)/(2*stddev*stddev));
				double prev = gsl_matrix_get(wfc, i, j);
				gsl_matrix_set (wfc, i, j, prev + amp * (valy*valx));
				if (valx == 0 && xi > pos.x)
					break;
			}
		}
	}
}

void SimulWfc::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "set") {
		string what = popword(line);
		
		parsed = false;
	}
	else
		parsed = false;
	
	// If not parsed here, call parent
	if (parsed == false)
		Wfc::on_message(conn, orig);
}
