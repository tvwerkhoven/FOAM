/*
 wht.h -- William Herschel Telescope control
 Copyright (C) 2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_WHT_H
#define HAVE_WHT_H

#include <gsl/gsl_vector.h>

#include "io.h"
#include "config.h"
#include "pthread++.h"
#include "serial.h"

#include "devices.h"

using namespace std;

const string wht_type = "wht";

/*!
 @brief William Herschel Telescope (WHT) routines
 
 WHT (dev.telescope.wht) can control the William Herschel Telescope.
 
 Todo:
 
 - hardware interface
 - conversion form unit pointing to image shift
 - get coordinates from webpage
 
 More info
 http://www.ing.iac.es/~eng/ops/general/digi_portserver.html
 http://www.ing.iac.es/~cfg/group_notes/portserv.htm
 http://www.digi.com/products/serialservers/portserverts
 
 \section wht_cfg Configuration params
 
 - scalefac: magnification from primary focus to relevant unit for setup
 - signs: need sign flipping
 
 \section wht_netio Network commands
 
 - set pointing <c0> <c1>: set pointing to c0, c1
 - get pointing: get last known pointing
 - add pointing <dc0> <dc1>: add (dc0, dc1) to current pointing
 
*/
class WHT: public Telescope {
private:
	
public:
	WHT(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online=true);
	~WHT();
	
	serial::port wht_ctrl; //!< Hardware interface (RS323) to WHT. Needs device, speed, parity, delim
	
	int get_wht_coords(float *c0, float *c1); //!< Get WHT pointings coordinates from online thing
	
	// From Telescope::
	virtual int delta_pointing(const float dc0, const float dc1);
	virtual int conv_pix_point(const float dpix0, const float dpix1, float *dc0, float *dc1);

	// From Devices::
	void on_message(Connection *const conn, string);
};

#endif // HAVE_WHT_H

/*!
 \page dev_telescope_wht William Herschel Telescope control class
 
 Telescope implementation for WHT.
 */

