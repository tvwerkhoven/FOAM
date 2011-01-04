/*
 shwfsctrl.h -- Shack-Hartmann wavefront sensor network control class
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

/*!
 @file shwfsctrl.h
 @brief Camera UI control 
 */

#ifndef HAVE_SHWFSCTRL_H
#define HAVE_SHWFSCTRL_H

#include <glibmm/dispatcher.h>
#include <string>


#include "pthread++.h"
#include "protocol.h"

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
	virtual void on_message(std::string line);
	virtual void on_connected(bool connected);

public:
	ShwfsCtrl(Log &log, const std::string name, const std::string host, const std::string port);
	~ShwfsCtrl();
	
	// From WfsCtrl::
	virtual void connect();
	
};

#endif // HAVE_SHWFSCTRL_H
