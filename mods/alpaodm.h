/*
 alpaodm.h -- Alpao deformable mirror module header
 Copyright (C) 2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_ALPAODM_H
#define HAVE_ALPAODM_H

#include "io.h"
#include "config.h"
#include "pthread++.h"

#include "wfc.h"
#include "devices.h"

using namespace std;
const string alpaodm_type = "alpaodm";

/*!
 @brief Alpao deformable mirror driver class
 
 AlpaoDM is derived from Wfc. It can control an Alpao deformable mirror.
 
 Although the underlying library supports control for multiple DMs at the same 
 time, support is currently limited for 1 device in total.
 
 \section alpaodm_cfg Configuration parameters
 
 AlpaoDM supports the following configuration parameters:
 
 - serial: serial number of the DM to drive (required)
 
 \section alpaodm_cfg Network IO
 
 - get serial: return DM serial
 - get offset: return factory defined offset
 
 */
class AlpaoDM: public Wfc {
private:
	string serial;										//!< Alpao DM serial number
	int dm_id;												//!< IDs of the DM we are driving
	
	Path conf_acfg;										//!< Alpao DM .acfg file
	Path conf_data;										//!< Alpao DM associated binary data file

	vector<double> offset;						//!< DM offset (calibrated safe 'zero' position)
	string offset_str;								//!< Space-separated representation of offset

	vector<double> act_vec;						//!< Local temporary actuate vector (copy of ctrlparms.ctrl_vec)
	
	pthread::mutex alpao_mutex;				//!< acedev5Send() can only be called sequentially! Lock mutex before actuating, or the Alpao driver double free()'s some memory.

public:
	AlpaoDM(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~AlpaoDM();
	
	// From Wfc::
	virtual int actuate(const bool block=false);
	virtual int calibrate();
	virtual int reset();
	
	// From Devices::
	void on_message(Connection *const conn, string);
};

#endif // HAVE_ALPAODM_H

/*!
 \page dev_wfc_alpaodm Alpao deformable mirror devices
 
 The AlpaoDM class can drive an Alpao deformable mirror.
 
 */