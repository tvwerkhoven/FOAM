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

#include <iostream>
#include <arpa/inet.h>
#include <string>

#include "format.h"
#include "protocol.h"
#include "devicectrl.h"
#include "camctrl.h"
#include "simcamctrl.h"
#include "log.h"

using namespace std;

SimCamCtrl::SimCamCtrl(Log &log, const string h, const string p, const string n):
	CamCtrl(log, h, p, n)
{
	log.term(format("%s", __PRETTY_FUNCTION__));

}

SimCamCtrl::~SimCamCtrl() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void SimCamCtrl::on_connected(bool conn) {
	CamCtrl::on_connected(conn);
	log.term(format("%s (%d)", __PRETTY_FUNCTION__, conn));
	
	if (conn) {
		send_cmd("get simwf");
		send_cmd("get simwfc");
		send_cmd("get simwfcerr");
		send_cmd("get simtel");
		send_cmd("get simmla");
		send_cmd("get noisefac");
		send_cmd("get noiseamp");
		send_cmd("get seeingfac");
	}
}

void SimCamCtrl::on_message(string line) {
	// Save original line in case this function does not know what to do
	string orig = line;
	bool parsed = true;

	// Discard first 'ok' or 'err' (DeviceCtrl::on_message_common() already parsed this)
	string stat = popword(line);
	
	// Get command
	string what = popword(line);
	
	if (what == "simwf")
		do_simwf = popbool(line);
	else if(what == "simwfc")
		do_simwfc = popbool(line);
	else if(what == "simwfcerr")
		do_simwfcerr = popbool(line);
	else if(what == "simtel")
		do_simtel = popbool(line);
	else if(what == "simmla")
		do_simmla = popbool(line);
	else if(what == "noisefac")
		noisefac = popdouble(line);
	else if(what == "noiseamp")
		noiseamp = popdouble(line);
	else if(what == "seeingfac")
		seeingfac = popdouble(line);
	else
		parsed = false;
	
	if (!parsed)
		DeviceCtrl::on_message(orig);
	else
		signal_message();
}
