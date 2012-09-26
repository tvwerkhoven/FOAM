/*
 wfcctrl.h -- camera control class
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
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


#ifndef HAVE_WFCCTRL_H
#define HAVE_WFCCTRL_H

#include <glibmm/dispatcher.h>
#include <string>
#include <vector>

#include "pthread++.h"
#include "protocol.h"
#include "types.h"

#include "devicectrl.h"

using namespace std;

/*!
 @brief Generic wavefront corrector control class
 
 This class controls generic wavefront correctors. 
 */
class WfcCtrl: public DeviceCtrl {
private:
	// These represent the Wfc device settings
	int nact;
	gain_t gain;
	vector< double > ctrlvec;
	
	// From DeviceCtrl::
	virtual void on_message(string line);
	virtual void on_connected(bool connected);
	

public:
	WfcCtrl(Log &log, const string name, const string host, const string port);
	~WfcCtrl();
	
	vector< double > get_ctrlvec() const { return ctrlvec; }
	int get_nact() const { return nact; }
	gain_t get_gain() const { return gain; }
	
	Glib::Dispatcher signal_wfcctrl;	//!< New actuator voltages available
};

#endif // HAVE_WFCCTRL_H
