/*
 wfcctrl.h -- wavefront corrector network control class
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

#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <vector>

#include "format.h"
#include "protocol.h"
#include "devicectrl.h"
#include "wfcctrl.h"
#include "log.h"

using namespace std;

WfcCtrl::WfcCtrl(Log &log, const string h, const string p, const string n):
DeviceCtrl(log, h, p, n)
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	cmd_ign_list.insert("ctrl");
}

WfcCtrl::~WfcCtrl() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void WfcCtrl::on_connected(bool conn) {
	DeviceCtrl::on_connected(conn);
	log.term(format("%s (%d)", __PRETTY_FUNCTION__, conn));
	
	if (conn) {
		send_cmd("get nact");
		send_cmd("get gain");
		send_cmd("get ctrl");
	}
}

void WfcCtrl::on_message(string line) {
	// Save original line in case this function does not know what to do
	string orig = line;
	bool parsed = true;

	// Discard first 'ok' or 'err' (DeviceCtrl::on_message_common() already parsed this)
	string stat = popword(line);
	
	// Get command
	string what = popword(line);

	if (what == "nact") {
		nact = popint(line);
	} else if (what == "gain") {
		gain.p = popdouble(line);
		gain.i = popdouble(line);
		gain.d = popdouble(line);
	} else if (what == "ctrl") {
		int nctrl = popint(line);
		if (nctrl <= 0) {
			ok = false;
			errormsg = format("Got %d<=0 ctrl values", nctrl);
			signal_message();
			return;
		}
		
		ctrlvec.clear();
		for (int n=0; n<nctrl; n++)
			ctrlvec.push_back(popdouble(line));
		
		signal_wfcctrl();
	} else
		parsed = false;
	
	
	if (!parsed)
		DeviceCtrl::on_message(orig);
	else
		signal_message();
}
