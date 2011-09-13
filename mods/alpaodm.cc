/*
 alpaodm.cc -- Alpao deformable mirror module
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

#include <acedev5.h>		// Alpao DM headers
#include <acecs.h>

#include "config.h"
#include "path++.h"
#include "io.h"

#include "foam.h"
#include "foamctrl.h"
#include "devices.h"
#include "wfc.h"
#include "alpaodm.h"

using namespace std;

AlpaoDM::AlpaoDM(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online):
Wfc(io, ptc, name, alpaodm_type, port, conffile, online)
{
	io.msg(IO_DEB2, "AlpaoDM::AlpaoDM()");
	
	// Configure initial settings
	try {
		// Get unique Alpao DM serial
		serial = cfg.getstring("serial");
		conf_acfg = ptc->datadir + cfg.getstring("acfg");
		conf_data = ptc->datadir + cfg.getstring("datafile");
	} catch (std::runtime_error &e) {
		io.msg(IO_ERR | IO_FATAL, "AlpaoDM: problem with configuration file: %s", e.what());
	} catch (...) { 
		io.msg(IO_ERR | IO_FATAL, "AlpaoDM: unknown error at initialisation.");
		throw;
	}
	
	// Check if conf_acfg and conf_data exist
	if (!conf_data.isfile() || !conf_acfg.isfile())
		throw std::runtime_error(format("AlpaoDM: conf_acfg (%s) or conf_data (%s) doesn't exist.", 
								 conf_acfg.c_str(), conf_data.c_str()));
	
	// Check if conf_acfg and conf_data exist in local directory
	if (!conf_data.basename().exists() || !conf_acfg.basename().exists())
		throw std::runtime_error(format("AlpaoDM: conf_acfg (%s) or conf_data (%s) don't exist in working dir.", 
								 conf_acfg.basename().c_str(), conf_data.basename().c_str()));
	
	// Init DM
	char serial_char[128];
	snprintf(serial_char, 128, "%s", serial.c_str());
	if (acedev5Init(1, &dm_id, serial_char) == acecsFAILURE) {
		acecsErrDisplay();
		acedev5Release(1, &dm_id);
		throw std::runtime_error("AlpaoDM: error at acedev5Init()");
	}
	io.msg(IO_DEB2, "AlpaoDM::AlpaoDM() init ok sleep 2 sec (dm ID: %d, serial: %s)", dm_id, serial.c_str());
	sleep(2);
	
	// Retreive number of actuators
	acedev5GetNbActuator(1, &dm_id, &nact);
	io.msg(IO_DEB2, "AlpaoDM::AlpaoDM()::%d got %d actuators", dm_id, nact);

	// Retrieve offset
	offset.resize(nact);
	double *offset_tmp = &offset[0];
	io.msg(IO_DEB2, "AlpaoDM::AlpaoDM()::%d acquiring offset...", dm_id, nact);
	acedev5GetOffset(1, &dm_id, offset_tmp);
	
	for (size_t i=0; i < (size_t) nact; i++)
		offset_str += format("%.4f ", offset.at(i));
	
	io.msg(IO_DEB2, "AlpaoDM::AlpaoDM()::%d offset: %s", dm_id, offset_str.c_str());
	
	// Enable DEV5 trigger signal (?)
	acedev5EnableTrig(1, &dm_id);
	
	add_cmd("get serial");
	add_cmd("get offset");

	// Calibrate to allocate memory
	calibrate();
}

AlpaoDM::~AlpaoDM() {
	io.msg(IO_DEB2, "AlpaoDM::~AlpaoDM()");
	
	// Do we need this? What is triggering anyway?
	//acedev5DisableTrig(1, &dm_id);
	
	// Send a software DAC reset to restore 0A on all actuators.
	io.msg(IO_INFO, "AlpaoDM::~AlpaoDM()::%d resetting actuators...", dm_id);
	if (acedev5SoftwareDACReset(1, &dm_id) == acecsFAILURE)
		acecsErrDisplay();
	
	io.msg(IO_INFO, "AlpaoDM::~AlpaoDM()::%d releasing...", dm_id);
	if (acedev5Release(1, &dm_id) == acecsFAILURE)
		acecsErrDisplay();
}

int AlpaoDM::calibrate() {
	// 'Calibrate' simulator (allocate memory)
	act_vec.resize(nact);
	
	// Call calibrate() in base class (for wfc_amp)
	return Wfc::calibrate();
}

int AlpaoDM::reset() {
	// Do not use acedev5SoftwareDACReset here as it sets 0 volts to all 
	// actuators. Setting control vector 0 to all actuators applies a 
	// pre-calibrated offset vector as well (acedev5GetOffset) such that it is
	// closer to flat.
	set_control(0.0);
	
	return 0;
}

int AlpaoDM::actuate(const bool /*block*/) {
	// Copy from ctrlparams to local double array:
	string act_vec_str = "";
	for (size_t i=0; i<nact; i++) {
		act_vec.at(i) = gsl_vector_float_get(ctrlparams.target, i);
		act_vec_str += format("%g, ", act_vec.at(i));
	}
	
	// acedev5Send expected pointer to double-array, take address of first
	// vector element to satisfy this need. std::vector guarantees data contiguity
	// so this is legal
	double *act_vec_arr = &act_vec[0];
	if (acedev5Send(1, &dm_id, act_vec_arr) == acecsFAILURE)	{
		acecsErrDisplay();
		acedev5Release(1, &dm_id);
		throw std::runtime_error("AlpaoDM: error at acedev5Send()");
	}

	return 0;
}

void AlpaoDM::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "get") {							// get ...
		string what = popword(line);
		
		if (what == "serial")							// get serial
			conn->write(format("ok serial %s", serial.c_str()));
		else if (what == "offset")				// get offset
			conn->write(format("ok offset %zu %s", offset.size(), offset_str.c_str()));			
		else
			parsed = false;
	} else 
		parsed = false;
	
	// If not parsed here, call parent
	if (parsed == false)
		Wfc::on_message(conn, orig);
}

