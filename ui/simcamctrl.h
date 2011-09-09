/*
 simcamctrl.h -- simulation camera control class
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

#ifndef HAVE_SIMCAMCTRL_H
#define HAVE_SIMCAMCTRL_H

#include <string>

#include "devicectrl.h"
#include "camctrl.h"

using namespace std;

/*!
 @brief Simulation camera control class
 
 Extends CamCtrl with control for:
 - noisefac/noiseamp
 - simulation modes
 
 */
class SimCamCtrl: public CamCtrl {
private:
	bool do_simwf;
	bool do_simwfc;
	bool do_simwfcerr;
	bool do_simtel;
	bool do_simmla;
	
	double noisefac;
	double noiseamp;
	
	double seeingfac;
	
protected:
	// From DeviceCtrl::
	virtual void on_message(string line);
	virtual void on_connected(bool connected);

public:
	SimCamCtrl(Log &log, const string name, const string host, const string port);
	~SimCamCtrl();
	
	bool get_simseeing() const { return do_simwf; }
	bool get_simwfc() const { return do_simwfc; }
	bool get_simwfcerr() const { return do_simwfcerr; }
	bool get_simtel() const { return do_simtel; }
	bool get_simwfs() const { return do_simmla; }
	void set_simseeing(const bool v) { send_cmd(format("set simwf %d", v)); }
	void set_simwfc(const bool v) { send_cmd(format("set simwfc %d", v)); }
	void set_simwfcerr(const bool v) { send_cmd(format("set simwfcerr %d", v)); }
	void set_simtel(const bool v) { send_cmd(format("set simtel %d", v)); }
	void set_simwfs(const bool v) { send_cmd(format("set simmla %d", v)); }
	
	double get_noisefac() const { return noisefac; }
	double get_noiseamp() const { return noiseamp; }
	void set_noisefac(const double f) { send_cmd(format("set noisefac %g", f)); }
	void set_noiseamp(const double f) { send_cmd(format("set noiseamp %g", f)); }
	
	double get_seeingfac() const { return seeingfac; }
	void set_seeingfac(const double f) { send_cmd(format("set seeingfac %g", f));  }
	
};

#endif // HAVE_SIMCAMCTRL_H
