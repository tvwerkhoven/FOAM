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
#include <gsl/gsl_vector.h>

#include "types.h"
#include "config.h"
#include "path++.h"

#include "foamctrl.h"
#include "devices.h"
#include "wfc.h"

Wfc::Wfc(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const & conffile, const bool online):
Device(io, ptc, name, wfc_type + "." + type, port, conffile, online),
nact(0), have_waffle(false) {	
	io.msg(IO_DEB2, "Wfc::Wfc()");

	try {
		// Get waffle pattern actuators
		str_waffle_odd = cfg.getstring("waffle_odd", "");
		str_waffle_even = cfg.getstring("waffle_even", "");
		
	} catch (std::runtime_error &e) {
		io.msg(IO_ERR | IO_FATAL, "Wfc: problem with configuration file: %s", e.what());
	} catch (...) { 
		io.msg(IO_ERR | IO_FATAL, "Wfc: unknown error at initialisation.");
		throw;
	}
	
	add_cmd("set gain");
	add_cmd("get gain");
	add_cmd("get nact");
	add_cmd("get ctrl");

	add_cmd("act waffle");
	add_cmd("act random");
	add_cmd("act all");
	add_cmd("act one");
}

Wfc::~Wfc() {
	io.msg(IO_DEB2, "Wfc::~Wfc()");
	gsl_vector_float_free(ctrlparams.target);
	gsl_vector_float_free(ctrlparams.err);
	gsl_vector_float_free(ctrlparams.prev);
	gsl_vector_float_free(ctrlparams.pid_int);
}

string Wfc::ctrl_as_str(const char *fmt) const {
	if (!ctrlparams.target)
		return "0"
	
	// Init string with number of values
	string ctrl_str;
	ctrl_str = format("%zu", ctrlparams.target->size);
	
	// Add all values seperated by commas
	for (size_t i=0; i < ctrlparams.target->size; i++)
		ctrl_str += ", " + format(fmt, gsl_vector_float_get(ctrlparams.target, i));
	
	return ctrl_str;
}

int Wfc::update_control(const gsl_vector_float *const error, const gain_t g, const float retain) {
	// gsl_blas_saxpy(alpha, x, y): compute the sum y = \alpha x + y for the vectors x and y.
	// gsl_blas_sscal(alpha, x): rescale the vector x by the multiplicative factor alpha. 
	if (!get_calib())
		calibrate();
	
	// Copy error to our memory (ctrlparams.err), unless it is the same memory
	if (error != ctrlparams.err)
		gsl_blas_scopy(error, ctrlparams.err);
	
	// Copy current target to ctrlparams.prev
	gsl_blas_scopy(ctrlparams.target, ctrlparams.prev);
	
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
	
	return 0;
}

int Wfc::set_control(const gsl_vector_float *const newctrl) {
	if (!get_calib())
		calibrate();
	
	// Copy new target to ctrlparams.target
	gsl_blas_scopy(newctrl, ctrlparams.target);
	return 0;
}

int Wfc::set_control(const float val) {
	if (!get_calib())
		calibrate();
	
	// Set all actuators to 'val'
	gsl_vector_float_set_all(ctrlparams.target, val);
	return 0;
}

int Wfc::set_control_act(const float val, const size_t act_id) {
	if (!get_calib())
		calibrate();
	
	// Set actuator 'act_id' to 'val'
	gsl_vector_float_set(ctrlparams.target, act_id, val);
	return 0;
}

int Wfc::set_wafflepattern(const float val) {
	if (!have_waffle) {
		io.msg(IO_WARN, "Wfc::set_wafflepattern() no waffle!");
		return 1;
	}
	
	if (!get_calib())
		calibrate();
	
	// Set all to zero first
	gsl_vector_float_set_zero(ctrlparams.target);
	
	// Set 'even' actuators to +val, set 'odd' actuators to -val:
	for (size_t idx=0; idx < waffle_even.size(); idx++)
		gsl_vector_float_set(ctrlparams.target, waffle_even.at(idx), val);

	for (size_t idx=0; idx < waffle_odd.size(); idx++)
		gsl_vector_float_set(ctrlparams.target, waffle_odd.at(idx), -val);
	
	return 0;
}

int Wfc::set_randompattern(const float maxval) {
	if (!get_calib())
		calibrate();

	// Set all to zero first
	gsl_vector_float_set_zero(ctrlparams.target);
	
	for (size_t idx=0; idx < ctrlparams.target->size; idx++)
		gsl_vector_float_set(ctrlparams.target, idx, (drand48()*2.0-1.0)*maxval);
	
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
	
	// Parse waffle pattern strings (only here because otherwise nact is 0)
	parse_waffle(str_waffle_odd, str_waffle_even);
		
	set_calib(true);
	return 0;
}

int Wfc::reset() {
	set_control(0.0);
	actuate();
	return 0;
}

void Wfc::parse_waffle(string &odd, string &even) {
	io.msg(IO_DEB2, "Wfc::parse_waffle(odd=%s, even=%s)", odd.c_str(), even.c_str());
	if (odd.size() <= 0 || even.size() <= 0)
		return;

	string thisact;
	int thisact_i;
	
	while (odd.size() > 0) {
		thisact = popword(odd, " \t\n,");
		thisact_i = strtol(thisact.c_str(), (char **) NULL, 10);
		if (thisact_i >= 0 && thisact_i <= nact) {
			waffle_odd.push_back(thisact_i);
			io.msg(IO_DEB2, "Wfc::parse_waffle(odd) add %d", thisact_i);
		}
		else
			break;
	}
	
	while (even.size() > 0) {
		thisact = popword(even, " \t\n,");
		thisact_i = strtol(thisact.c_str(), (char **) NULL, 10);
		if (thisact_i >= 0 && thisact_i <= nact) {
			waffle_even.push_back(thisact_i);
			io.msg(IO_DEB2, "Wfc::parse_waffle(even) add %d", thisact_i);
		}
		else
			break;
	}
	
	have_waffle = true;
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
		} else if (what == "nact") {			// get nact
			conn->write(format("ok nact %d", nact));
		} else if (what == "ctrl") {			// get ctrl
			conn->write(format("ok ctrl %s", ctrl_as_str().c_str()));
		} else
			parsed = false;

	} else if (command == "set") {			// set ...
		string what = popword(line);
		
		if (what == "gain") {							// set gain <p> <i> <d>
			conn->addtag("gain");
			ctrlparams.gain.p = popdouble(line);
			ctrlparams.gain.i = popdouble(line);
			ctrlparams.gain.d = popdouble(line);
			net_broadcast(format("ok gain %g %g %g", ctrlparams.gain.p, ctrlparams.gain.i, ctrlparams.gain.d));
		} else
			parsed = false;
	} else if (command == "act") { 
		string actwhat = popword(line);
		
		if (actwhat == "waffle") {				// act waffle
			double w_amp = popdouble(line);
			if (w_amp < 0 || w_amp > 1)
				w_amp = 0.5;
			
			set_wafflepattern(w_amp);
			actuate();
			conn->write(format("ok act waffle %g", w_amp));
		} else if (actwhat == "random") { // act random
			double w_amp = popdouble(line);
			if (w_amp < 0 || w_amp > 1)
				w_amp = 0.5;
			
			set_randompattern(w_amp);
			actuate();
			conn->write(format("ok act random %g", w_amp));
		} else if (actwhat == "one") { 		// act one <id> <val>
			int actid = popint(line);
			double actval = popdouble(line);
			set_control_act(actval, actid);
			actuate();
			conn->write(format("ok act one"));
		} else if (actwhat == "all") { 		// act all <val>
			double actval = popdouble(line);
			set_control(actval);
			actuate();
			conn->write(format("ok act all"));
		} else
			parsed = false;
	} else {
		parsed = false;
	}
		
	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}
