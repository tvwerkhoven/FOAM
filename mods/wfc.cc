/*
 wfc.cc -- a wavefront corrector base class
 Copyright (C) 2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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
real_nact(0), virt_nact(0), use_actmap(false), 
have_waffle(false) {	
	io.msg(IO_DEB2, "Wfc::Wfc()");

	try {
		// Get actuator map. Syntax should be '<N_virt> [virt_act1 real_act1,real_act2,real_act3 [virt_act2 real_act1,real_act2,real_act3]]
		str_actmap = cfg.getstring("actmap", "");
		
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
	//! @todo	add_cmd("get real_nact");
	add_cmd("get ctrl");

	add_cmd("act waffle");
	add_cmd("act random");
	add_cmd("act all");
	add_cmd("act one");
}

Wfc::~Wfc() {
	io.msg(IO_DEB2, "Wfc::~Wfc()");
	if (use_actmap)
		gsl_vector_float_free(ctrlparams.ctrl_vec);
	gsl_vector_float_free(ctrlparams.target);
	gsl_vector_float_free(ctrlparams.err);
	gsl_vector_float_free(ctrlparams.prev);
	gsl_vector_float_free(ctrlparams.pid_int);
}

string Wfc::ctrl_as_str(const char *fmt) const {
	if (!ctrlparams.target)
		return "0";
	
	// Init string with number of values
	string ctrl_str;
	ctrl_str = format("%zu", ctrlparams.target->size);
	
	// Add all values seperated by commas
	for (size_t i=0; i < ctrlparams.target->size; i++)
		ctrl_str += ", " + format(fmt, gsl_vector_float_get(ctrlparams.target, i));
	
	return ctrl_str;
}

int Wfc::ctrl_apply_actmap() {
	// If we don't use act_map, ctrl_vec already points to target and we're done
	if (!use_actmap) {
//		io.msg(IO_DEB2, "Wfc::ctrl_apply_actmap() no act_map");
		return 0;
	}
	string ctrl_str;
	
	// Loop over all virtual actuators in 'actmap'
	for (size_t v_act=0; v_act<actmap.size(); v_act++) {
		float ctrl_val = gsl_vector_float_get(ctrlparams.target, v_act);
//		ctrl_str += format("%d (%g) -> ", v_act, ctrl_val);
		// Map over all real actuators associated with virtual actuator 'v_act'
		for (size_t r_act=0; r_act<actmap.at(v_act).size(); r_act++) {
			// Set real actuator 'r_act' to value of 'v_act'
			gsl_vector_float_set(ctrlparams.ctrl_vec, actmap.at(v_act).at(r_act), ctrl_val);
//			ctrl_str += format("%d ", actmap.at(v_act).at(r_act));
		}
	}
//	io.msg(IO_DEB2, "Wfc::ctrl_apply_actmap() %s", ctrl_str.c_str());

	return 0;
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
	
	return ctrl_apply_actmap();
}

int Wfc::set_control(const gsl_vector_float *const newctrl) {
	if (!get_calib())
		calibrate();
	
	// Copy new target to ctrlparams.target
	gsl_blas_scopy(newctrl, ctrlparams.target);
	return ctrl_apply_actmap();
}

int Wfc::set_control(const float val) {
	if (!get_calib())
		calibrate();
	
	// Set all actuators to 'val'
	gsl_vector_float_set_all(ctrlparams.target, val);
	return ctrl_apply_actmap();
}

int Wfc::set_control_act(const float val, const size_t act_id) {
	if (!get_calib())
		calibrate();
	
	// Set actuator 'act_id' to 'val'
	gsl_vector_float_set(ctrlparams.target, act_id, val);
	return ctrl_apply_actmap();
}

int Wfc::set_wafflepattern(const float val) {
	if (!have_waffle) {
		io.msg(IO_WARN, "Wfc::set_wafflepattern() no waffle!");
		return 1;
	}
	
	if (!get_calib())
		calibrate();
	
	// Set all to zero first
	gsl_vector_float_set_zero(ctrlparams.ctrl_vec);
	
	// Set 'even' actuators to +val, set 'odd' actuators to -val:
    //! @todo Check bounds here, waffle_even.at(idx) can be higher than ctrl_vec length 
	for (size_t idx=0; idx < waffle_even.size(); idx++)
		gsl_vector_float_set(ctrlparams.ctrl_vec, waffle_even.at(idx), val);

	for (size_t idx=0; idx < waffle_odd.size(); idx++)
		gsl_vector_float_set(ctrlparams.ctrl_vec, waffle_odd.at(idx), -val);
	
    //! @bug Waffle pattern works for *all* actuators, not just actmap, fix
	return 0;
}

int Wfc::set_randompattern(const float maxval) {
	if (!get_calib())
		calibrate();

	// Set all to zero first
	gsl_vector_float_set_zero(ctrlparams.target);
	
	for (size_t idx=0; idx < ctrlparams.target->size; idx++)
		gsl_vector_float_set(ctrlparams.target, idx, (drand48()*2.0-1.0)*maxval);
	
	return ctrl_apply_actmap();
}


int Wfc::calibrate() {
	// Parse actuator map string
	virt_nact = parse_actmap(str_actmap);
	
	// Parse waffle pattern strings (only here because otherwise real_nact is 0)
	parse_waffle(str_waffle_odd, str_waffle_even);
	
	// Allocate memory for control command
	gsl_vector_float_free(ctrlparams.target);
	ctrlparams.target = gsl_vector_float_calloc(virt_nact);
	gsl_vector_float_free(ctrlparams.err);
	ctrlparams.err = gsl_vector_float_calloc(virt_nact);
	gsl_vector_float_free(ctrlparams.prev);
	ctrlparams.prev = gsl_vector_float_calloc(virt_nact);
	gsl_vector_float_free(ctrlparams.pid_int);
	ctrlparams.pid_int = gsl_vector_float_calloc(virt_nact);
	
	if (use_actmap) {
		gsl_vector_float_free(ctrlparams.ctrl_vec);
		ctrlparams.ctrl_vec = gsl_vector_float_calloc(real_nact);
	} else {
		ctrlparams.ctrl_vec = ctrlparams.target;
	}
	
	set_calib(true);
	return 0;
}

int Wfc::reset() {
	set_control(0.0);
	actuate();
	return 0;
}

void Wfc::loosen(const double amp, const int niter, const double delay) {
	for (int iter=0; iter<niter; iter++) {
		set_control(-1.0*amp);
		actuate();
		set_control(1.0*amp);
		actuate();
		usleep(delay * 1E6);
	}
}

void Wfc::parse_waffle(string &odd, string &even) {
	io.msg(IO_DEB2, "Wfc::parse_waffle(odd=%s, even=%s)", odd.c_str(), even.c_str());
	if (odd.size() <= 0 || even.size() <= 0)
		return;

	string thisact;
	size_t thisact_i=0;
	
	string odd_act_l = "";
	string even_act_l = "";
	
	while (odd.size() > 0) {
		thisact = popword(odd, " \t\n,");
		thisact_i = strtol(thisact.c_str(), (char **) NULL, 10);
		if (thisact_i >= 0 && thisact_i <= real_nact) {
			waffle_odd.push_back(thisact_i);
			odd_act_l += format(" %d", thisact_i);
		}
		else
			break;
	}
	
	while (even.size() > 0) {
		thisact = popword(even, " \t\n,");
		thisact_i = strtol(thisact.c_str(), (char **) NULL, 10);
		if (thisact_i >= 0 && thisact_i <= real_nact) {
			waffle_even.push_back(thisact_i);
			even_act_l += format(" %d", thisact_i);
		}
		else
			break;
	}
	
	io.msg(IO_DEB2, "Wfc::parse_waffle() odd = %s", odd_act_l.c_str());
	io.msg(IO_DEB2, "Wfc::parse_waffle() even = %s", even_act_l.c_str());

	
	have_waffle = true;
}

int Wfc::parse_actmap(string &map) {
	io.msg(IO_DEB2, "Wfc::parse_actmap(map=%s)", map.c_str());
	if (map.size() <= 0)
		return real_nact;
	
	string actmap_result;
	// First 'word' is the number of virtual actuators
	int n_virt = popint(map);
	
	actmap_result = format("n_vact: %d ", n_virt);
	
	int this_vact;
	string these_ract;
	int this_ract;
	std::vector<int> this_actmap;
	int vact_count=0;
	
	while (map.size() > 0) {
		vact_count++;
		this_actmap.clear();
		// Each virtual actuator map syntax is like: 'v_act r_act1,r_act2,r_act3'
		this_vact = popint(map);
		actmap_result += format("vact %d -> ", this_vact);
		
		// This will contain 'r_act1,r_act2,...'
		these_ract = popword(map);
		while (these_ract.size() > 0) {
			this_ract = str2int(popword(these_ract, ","));
			this_actmap.push_back(this_ract);
			actmap_result += format("%d ", this_ract);
		}
		actmap.push_back(this_actmap);
	}
	if (n_virt != vact_count)
		io.msg(IO_ERR, "Wfc::parse_actmap() n_virt %d != vact_count %d", n_virt, vact_count);
	if (n_virt != actmap.size())
		io.msg(IO_ERR, "Wfc::parse_actmap() n_virt %d != actmap.size %zu", n_virt, actmap.size());

	io.msg(IO_XNFO, "Wfc::parse_actmap() map: %s", actmap_result.c_str());

	use_actmap = true;
	return actmap.size();
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
			conn->write(format("ok nact %d", get_nact()));
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
            
        //! @todo Implement setting all actuators with a vector
//		} else if (actwhat == "vec") { 		// act vec <val> <val> <val> <val>
//            for (size_t acti=0; acti < ctrlparams.target->size; acti++) {
//                double actval = popdouble(line);
//                gsl_vector_float_set(ctrlparams.target, acti, actval);
//            }
//			actuate();
//			conn->write(format("ok act all"));
		} else
			parsed = false;
	} else {
		parsed = false;
	}
		
	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}
