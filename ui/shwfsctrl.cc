/*
 shwfsctrl.h -- Shack-Hartmann wavefront sensor network control class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

/*!
 @file shwfsctrl.cc
 @brief Wavefront sensor control
 */

#include <iostream>
#include <arpa/inet.h>
#include <string>

#include "format.h"
#include "protocol.h"
#include "log.h"

#include "wfsctrl.h"
#include "shwfsctrl.h"

using namespace std;

ShwfsCtrl::ShwfsCtrl(Log &log, const string h, const string p, const string n):
	WfsCtrl(log, h, p, n)
{
	fprintf(stderr, "%x:ShwfsCtrl::ShwfsCtrl()\n", (int) pthread_self());
	
}

ShwfsCtrl::~ShwfsCtrl() {
	fprintf(stderr, "%x:ShwfsCtrl::~ShwfsCtrl()\n", (int) pthread_self());
}

void ShwfsCtrl::connect() {
	WfsCtrl::connect();
}

void ShwfsCtrl::on_connected(bool conn) {
	WfsCtrl::on_connected(conn);
	fprintf(stderr, "%x:ShwfsCtrl::on_connected(conn=%d)\n", (int) pthread_self(), conn);
	
	if (conn) {
		// send_cmd("measuretest");
		// send_cmd("get modes");
		// send_cmd("get basis");
	}
}

void ShwfsCtrl::on_message(string line) {
	WfsCtrl::on_message(line);
	fprintf(stderr, "%x:ShwfsCtrl::on_message(line=%s)\n", (int) pthread_self(), line.c_str());
	
	if (!ok) {
		return;
	}
	// Discard first 'ok' or 'err' (DeviceCtrl::on_message() already parsed this)
	string stat = popword(line);
	
	// Get command
	string what = popword(line);

	//signal_message();
}

