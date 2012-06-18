/*
 telescopectrl.h -- Telescope control GUI
 Copyright (C) 2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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


#ifndef HAVE_TELESCOPECTRL_H
#define HAVE_TELESCOPECTRL_H

#include <string>
#include <vector>

#include "pthread++.h"
#include "protocol.h"
#include "types.h"
#include "format.h"

#include "devicectrl.h"

using namespace std;

/*!
 @brief Generic telescope control class
 
 */
class TelescopeCtrl: public DeviceCtrl {
private:
	
	float tel_track[2];					//!< Telescope tracking position
	string tel_units_s[2];			//!< Telescope tracking units as string
	float tt_raw[2];						//!< Raw tip-tilt coordinates
	float tt_conv[2];						//!< Converted tip-tilt coordinates
	float tt_ctrl[2];						//!< Control tip-tilt coordinates
	
	float ccd_ang;							//!< CCD rotation angle
	float scalefac[2];					//!< Scalefactor for x,y shift
	gain_t tt_gain;							//!< Tip-tilt gain control
	float altfac;								//!< Altitude conversion factor

public:
	string get_tel_track_s() const { return format("%.3g, %.3g", tel_track[0], tel_track[1]); }
	string get_tel_units_s() const { return format("%s/%s", tel_units_s[0].c_str(), tel_units_s[1].c_str()); }
	string get_tt_raw_s() const { return format("%.3g, %.3g", tt_raw[0], tt_raw[1]); }
	string get_tt_conv_s() const { return format("%.3g, %.3g", tt_conv[0], tt_conv[1]); }
	string get_tt_ctrl_s() const { return format("%g, %g", tt_ctrl[0], tt_ctrl[1]); }

	void set_ccd_ang(double ang) { send_cmd(format("set ccd_ang %lf", ang)); };
	void set_scalefac(double s0, double s1) { send_cmd(format("set scalefac %lf %lf", s0, s1)); };
	void set_ttgain(double gain) { send_cmd(format("set ttgain %lf 0 0", gain)); };
//	void set_altfac(double fac) { send_cmd(format("set altfac %lf", fac)); };

	double get_ccd_ang() const { return ccd_ang; };
	double get_scalefac0() const { return scalefac[0]; };
	double get_scalefac1() const { return scalefac[1]; };
	double get_ttgain() const { return tt_gain.p; };
//	double get_altfac() const { return altfac };

	// From DeviceCtrl::
	virtual void on_message(string line);
	virtual void on_connected(bool connected);
	

public:
	TelescopeCtrl(Log &log, const string name, const string host, const string port);
	~TelescopeCtrl();
	
};

#endif // HAVE_TELESCOPECTRL_H
