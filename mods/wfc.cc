/*
 wfc.cc -- a wavefront corrector base class
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <gsl/gsl_blas.h>

#include "types.h"
#include "config.h"
#include "path++.h"

#include "foamctrl.h"
#include "devices.h"
#include "wfc.h"

Wfc::Wfc(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const & conffile, const bool online):
Device(io, ptc, name, wfc_type + "." + type, port, conffile, online),
nact(0) {	
	io.msg(IO_DEB2, "Wfc::Wfc()");
	
	add_cmd("set gain");
	add_cmd("get gain");
}

Wfc::~Wfc() {
	io.msg(IO_DEB2, "Wfc::~Wfc()");
	gsl_vector_float_free(ctrlparams.target);
	gsl_vector_float_free(ctrlparams.err);
	gsl_vector_float_free(ctrlparams.prev);
	gsl_vector_float_free(ctrlparams.pid_int);
}

int Wfc::update_control(const gsl_vector_float *const error, const gain_t g, const float retain) {
	// gsl_blas_saxpy(alpha, x, y): compute the sum y = \alpha x + y for the vectors x and y.
	// gsl_blas_sscal(alpha, x): rescale the vector x by the multiplicative factor alpha. 
	if (!get_calib())
		calibrate();
	
	// Copy error to our memory (ctrlparams.err), unless it is the same memory
	if (error != ctrlparams.err)
		gsl_blas_scopy(error, ctrlparams.err);
	
	// If retain is unequal to 1, apply
	if (retain != 1.0) {
		if (retain == 0)
			gsl_vector_float_set_zero(ctrlparams.target);
		else
			gsl_blas_sscal(retain, ctrlparams.target);
	}
	
	// If proportional gain is unequal zero, use it
	if (g.p != 0) {
		// ctrlparams.target += pid->p * ctrlparams.err
		gsl_blas_saxpy(g.p, ctrlparams.err, ctrlparams.target);
	}
	
	//! @todo Extend update_control() with (P)ID control
#if 0
	// If integral gain is unequal zero, use it
	if (g.i != 0) {
		// ctrlparams.pid_int += error
		gsl_blas_saxpy(1.0, error, ctrlparams.pid_int);
		// check if ctrlparams.pid_int is still within range, clip if necessary
		
		// ctrlparams.target += pid->i * ctrlparams.pid_int
		gsl_blas_saxpy(g.i, ctrlparams.pid_int, ctrlparams.target);
	}
	
	// If differential gain is unequal zero, use it
	// This is the same as proportional control?
	if (g.d != 0) {
		// ctrlparams.target += pid->d + ctrlparams.err
		gsl_blas_saxpy(g.d, ctrlparams.err, ctrlparams.target);
	}
#endif
	
	// Copy current target to ctrlparams.prev
	gsl_blas_scopy(ctrlparams.target, ctrlparams.prev);
		
	return 0;
}

int Wfc::calibrate() {
	// Allocate memory for control command
	gsl_vector_float_free(ctrlparams.target);
	ctrlparams.target = gsl_vector_float_calloc(nact);
	gsl_vector_float_free(ctrlparams.err);
	ctrlparams.err = gsl_vector_float_calloc(nact);
	gsl_vector_float_free(ctrlparams.prev);
	ctrlparams.prev = gsl_vector_float_calloc(nact);
	gsl_vector_float_free(ctrlparams.pid_int);
	ctrlparams.pid_int = gsl_vector_float_calloc(nact);
		
	set_calib(true);
	return 0;
}

void Wfc::on_message(Connection *const conn, string line) { 
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "get") {							// get ...
		string what = popword(line);
		
		if (what == "gain") {							// get gain
			conn->addtag("gain");
			conn->write(format("ok gain %g %g %g", ctrlparams.gain.p, ctrlparams.gain.i, ctrlparams.gain.d));
		} else
			parsed = false;

	} else if (command == "set") {			// set ...
		string what = popword(line);
		
		if (what == "gain") {							// set gain <p> <i> <d>
			conn->addtag("gain");
			ctrlparams.gain.p = popdouble(line);
			ctrlparams.gain.i = popdouble(line);
			ctrlparams.gain.d = popdouble(line);
			netio.broadcast(format("ok gain %g %g %g", ctrlparams.gain.p, ctrlparams.gain.i, ctrlparams.gain.d));
		} else
			parsed = false;
	}
		
	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}
