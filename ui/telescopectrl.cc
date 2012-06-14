/*
 telescopectrl.cc -- Telescope control GUI
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

#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <vector>

#include "format.h"
#include "protocol.h"
#include "devicectrl.h"
#include "telescopectrl.h"
#include "log.h"

using namespace std;

TelescopeCtrl::TelescopeCtrl(Log &log, const string h, const string p, const string n):
DeviceCtrl(log, h, p, n)
{
	tel_track[0] = tel_track[1] = 0;
	tel_units_s[0] = tel_units_s[1] = "";
	tt_raw[0] = tt_raw[1] = 0;
	tt_conv[0] = tt_conv[1] = 0;
	tt_ctrl[0] = tt_ctrl[1] = 0;
	
	scalefac[0] = scalefac[1] = 0;
	altfac = -1;
	
	log.term(format("%s", __PRETTY_FUNCTION__));
	
}

TelescopeCtrl::~TelescopeCtrl() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void TelescopeCtrl::on_connected(bool conn) {
	DeviceCtrl::on_connected(conn);
	log.term(format("%s (%d)", __PRETTY_FUNCTION__, conn));
	
	if (conn) {
		send_cmd("get tel_track");
		send_cmd("get tel_units");
		send_cmd("get pixshift");
		send_cmd("get scalefac");
		send_cmd("get gain");
		send_cmd("get ccd_ang");
		send_cmd("get altfac");

	}
}

void TelescopeCtrl::on_message(string line) {
	// Save original line in case this function does not know what to do
	string orig = line;
	bool parsed = true;

	// Discard first 'ok' or 'err' (DeviceCtrl::on_message_common() already parsed this)
	string stat = popword(line);
	
	// Get command
	string what = popword(line);

	if (what == "tel_track") {
		tel_track[0] = popdouble(line);
		tel_track[1] = popdouble(line);
	} else if (what == "tel_units") {
		tel_units_s[0] = popword(line);
		tel_units_s[1] = popword(line);
	} else if (what == "pixshift") {
		tt_raw[0] = popdouble(line);
		tt_raw[1] = popdouble(line);
		tt_conv[0] = popdouble(line);
		tt_conv[1] = popdouble(line);
	} else if (what == "scalefac") {
		scalefac[0] = popdouble(line);
		scalefac[1] = popdouble(line);
	} else if (what == "gain") {
		tt_gain.p = popdouble(line);
		tt_gain.i = popdouble(line);
		tt_gain.d = popdouble(line);
	} else if (what == "altfac") {
		altfac = popint(line);
	} else
		parsed = false;
	
	
	if (!parsed)
		DeviceCtrl::on_message(orig);
	else
		signal_message();
}
