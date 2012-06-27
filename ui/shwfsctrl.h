/*
 shwfsctrl.h -- Shack-Hartmann wavefront sensor network control class
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_SHWFSCTRL_H
#define HAVE_SHWFSCTRL_H

#include <glibmm/dispatcher.h>
#include <string>
#include <vector>

#include "pthread++.h"
#include "protocol.h"
#include "types.h"

#include "devicectrl.h"
#include "wfsctrl.h"

using namespace std;

/*!
 @brief Shack-Hartmann wavefront sensor control class
 
 This class controls Shack-Hartmann wavefront sensors. 
 */
class ShwfsCtrl: public WfsCtrl {
protected:
	// From WfsCtrl::
	virtual void on_message(string line);
	virtual void on_connected(bool connected);
	
	std::vector<fvector_t> mlacfg;			//!< Simple subimage configuration
	std::vector<fvector_t> shifts_v;		//!< SHWFS shift vectors
	std::vector<fvector_t> refshift_v;	//!< SHWFS reference shift vectors
	
public:
	ShwfsCtrl(Log &log, const string name, const string host, const string port);
	~ShwfsCtrl();
	
	// From WfsCtrl::
	virtual void connect();
	
	// ==== Get properties ====
	size_t get_mla_nsi() const { return mlacfg.size(); }
	fvector_t get_mla_si(const size_t idx) const { return mlacfg[idx]; }
	
	size_t get_nshifts() const { return shifts_v.size(); }
	fvector_t get_shift(const size_t idx) const { return shifts_v[idx]; }

	size_t get_nrefshifts() const { return refshift_v.size(); }
	fvector_t get_refshift(const size_t idx) const { return refshift_v[idx]; }
	
	// ==== Network control ====
	
	// Control MLA configuration of remote system
	void cmd_get_mla() { send_cmd("mla get"); }
	void mla_add_si(const int lx, const int ly, const int tx, const int ty) { send_cmd(format("mla add %d %d %d %d", lx, ly, tx, ty)); }
	void mla_del_si(const int idx) { send_cmd(format("mla del %d", idx)); }
	void mla_update_si(const int idx, const int lx, const int ly, const int tx, const int ty) { send_cmd(format("mla update %d %d %d %d %d", idx, lx, ly, tx, ty)); }
	void mla_regen_pattern() { send_cmd("mla generate"); }
	void mla_find_pattern(const double minif=0.6) { send_cmd(format("mla find %g", minif)); }
	
	// Get SHWFS shifts
	void cmd_get_shifts() { send_cmd("get shifts"); }
	
	Glib::Dispatcher signal_sh_shifts;	//!< New SHWFS shifts available
};

#endif // HAVE_SHWFSCTRL_H
