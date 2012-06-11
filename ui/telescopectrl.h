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

#include "devicectrl.h"

using namespace std;

/*!
 @brief Generic telescope control class
 
 */
class TelescopeCtrl: public DeviceCtrl {
private:
	
	string tel_track_s;					//!< Telescope tracking position as string
	string tel_units_s;					//!< Telescope tracking units as string
	string tt_raw_s;						//!< Raw tip-tilt coordinates as string
	string tt_conv_s;						//!< Converted tip-tilt coordinates as string
	string tt_ctrl_s;						//!< Control tip-tilt coordinates as string

public:
	string get_tel_track_s() const { return tel_track_s; }
	string get_tt_raw_s() const { return tt_raw_s; }
	string get_tt_conv_s() const { return tt_conv_s; }
	string get_tt_ctrl_s() const { return tt_ctrl_s; }

	// From DeviceCtrl::
	virtual void on_message(string line);
	virtual void on_connected(bool connected);
	

public:
	TelescopeCtrl(Log &log, const string name, const string host, const string port);
	~TelescopeCtrl();
	
};

#endif // HAVE_TELESCOPECTRL_H
