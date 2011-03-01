/*
 WfsCtrl.cc -- camera control class
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 Copyright (C) 2010 Guus Sliepen
 
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


#ifndef HAVE_WFSCTRL_H
#define HAVE_WFSCTRL_H

#include <glibmm/dispatcher.h>
#include <gsl/gsl_vector.h>
#include <string>

#include "pthread++.h"
#include "protocol.h"

#include "devicectrl.h"

using namespace std;

/*!
 @brief Generic wavefront sensor control class
 
 This class controls generic wavefront sensors. Mostly it queries the current
 wavefront, and stores it it wf.
 */
class WfsCtrl: public DeviceCtrl {
protected:
	// From DeviceCtrl::
	virtual void on_message(string line);
	virtual void on_connected(bool connected);

private:
	
	/*!
	 @brief This holds information on the wavefront. Based on Wfs::wavefront, but more versatile (strings instead of enums)
	 */
	struct wavefront {
		wavefront() : wfamp(NULL), nmodes(0), basis("UNDEF") { ; }
		gsl_vector_float *wfamp;					//!< Mode amplitudes
		int nmodes;												//!< Number of modes
		string basis;											//!< Basis functions used for this representation
	};
	
	struct wavefront wf;								//!< Wavefront information
	
	string wfscam;											//!< Cameraname associated with this wavefront sensor

public:
	WfsCtrl(Log &log, const string name, const string host, const string port);
	~WfsCtrl();
	
	// From DeviceCtrl::
	virtual void connect();
	
	string get_basis() { return wf.basis; }
	int get_nmodes() { return wf.nmodes; }
	gsl_vector_float *get_modes() { return wf.wfamp; }
	
	Glib::Dispatcher signal_wfscam;			//!< WFS camera available now
	Glib::Dispatcher signal_wavefront;	//!< New wavefront information available
};

#endif // HAVE_WFSCTRL_H
