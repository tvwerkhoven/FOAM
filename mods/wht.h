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
#include "socket.h"

#include "devices.h"

using namespace std;

const string wht_type = "wht";

/*!
 @brief William Herschel Telescope (WHT) routines
 
 WHT (dev.telescope.wht) can control the William Herschel Telescope. 
 
 @section wht_guiding Guiding coordinates
 
 The guiding control of the William Herschel Telescope is done by the TCS, the 
 Telescope Control System. The tracking is done by a camera in the Cassegrain 
 focus of the WHT, which measures the offset of some reference point in 
 pixels. This is described in the "ING Autoguider: TCS Interface Control 
 Document (ICD)", INT-PF-7 (Issue 1.2; 24th August 1995). If one wants to 
 influence the guiding, that is, to introduce a correctional offset, this has
 to be done in pixel coordinates in the guiding camera frame. For a Nasmyth 
 focus this entails converting from one reference frame to a rotated frame 
 for the Cassegrain focus.
 
 For ExPo, the conversion is:
 
 alt: +- 0.01 [ x * sin(0.001745 * (45 - ele)) +- y * cos(0.001745 * (45 - ele)) ]
 az: -+ 0.01 [ y * sin(0.001745 * (45 - ele)) -+ x * cos(0.001745 * (45 - ele)) ]
 
 with x, y the measured shift, 45 the rotation of the ExPo camera, ele the 
 current elevation of the telescope, and 0.001745 180/pi. The scaling, gain 
 etc is all encapsulated in the constant 0.01. This might have to be adjusted
 live.

 @section wht_guiding Guiding RS232 control

 Once these coordinates are known, they have to be sent to a RS232 port over 
 ethernet. A Digi PortServer II Rack is used for this, see drivers below. 
 Installing the driver will make a /dev/tty<xxxx> port on the Linux machine
 which can be used to send guiding offset commands. The syntax for these 
 commands is:
 
 packet ::= xGuidePosition SPACE yGuidePosition SPACE code CR
 
 xGuidePosition ::= {s0000p00 ..... s9999p99}
 yGuidePosition ::= {s0000p00 ..... s9999p99}
 
 code ::= time | terminating | suspended
  time ::= {00000p01 ..... 99999p99}
  terminating ::= 00000p00 | -0000p00
	suspended ::= {-0000p01 ..... -9999p99}
 
 s ::= 0 | SPACE | -
 p ::= ASCII code (‘.’)
 SPACE ::= ASCII code (‘ ‘), 0x20
 CR ::= ASCII code 0x0d
 
 for example:
 00050.00 00050.00 00000.100x20
 
 to send neutral guiding information (=do nothing) every 100msec (10 Hz).
 
 @section wht_guiding Live telescope pointing information
 
 To correctly convert from pixel shift to telescope guiding coordinates, we 
 need the elevation as input. This is available at 
 <http://whtics.roque.ing.iac.es:8081/TCSStatus/TCSStatusExPo>. The syntax of 
 this is document is:
 
 TODO!!! ELE AZ\r\\n
 
 @section wht_todo Todo
 
 - conversion form unit pointing to image shift (using ele)
 - get coordinates from webpage
 
 @section wht_more More info
 
 http://www.ing.iac.es/~eng/ops/general/digi_portserver.html
 http://www.ing.iac.es/~cfg/group_notes/portserv.htm
 http://www.digi.com/support/productdetail?pid=2240&type=drivers
 http://whtics.roque.ing.iac.es:8081/TCSStatus/TCSStatusExPo
 ftp://ftp1.digi.com/support/beta/linux/dgrp/
 
 \section wht_cfg Configuration params
 
 - track_host: live WHT pointing host (whtics.roque.ing.iac.es)
 - track_port: live WHT pointing port (8001)
 - track_file: live WHT pointing file (/TCSStatus/TCSStatusExPo)
 - alt_fac: WHT::alt_fac 
 
 \section wht_netio Network commands
 
 - get trackurl: get WHT tracker information URL
 - get telpos: get WHT::alt WHT::az
 
 
*/
class WHT: public Telescope {
private:
	serial::port *wht_ctrl;	//!< Hardware interface (RS323) to WHT. Needs device, speed, parity, delim
	string sport;						//!< Serial port to use (/dev/tty...)
	
	pthread::thread wht_cfg_thr;	//!< WHT configuration querying thread
	void wht_updater();						//!< Handler thread that continuously updates WHT configuration
	
	Socket::Socket sock_track; //!< Socket to read live WHT position
	string track_prot;			//!< track protocol (http)
	string track_host;			//!< Hostname to read for live WHT position
	string track_file;			//!< file to read wht_live_url on;
	string track_port;			//!< port to read wht_live_url on;
	
	int update_wht_coords(double *const alt, double *const az, double *const delay); //!< Update WHT pointings coordinates from online thing

public:
	WHT(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~WHT();
	
	double alt;							//!< Telescope altititude
	int	alt_fac;						//!< Factor to multiple alt with before rotation (i.e. rotate counter-clockwise if -1)
	double az;							//!< Telescope azimuthal
	double delay;						//!< WHT info update period
	std::map<string, string> wht_info; // Full WHT info from website
		
	int get_wht_coords(float *const alt, float *const az); //!< Get last known WHT pointings coordinates
	
	// From Telescope::
	virtual int update_telescope_track(const float sht0, const float sht1);
	
	// From Devices::
	virtual void on_message(Connection *const conn, string);
};

#endif // HAVE_WHT_H

/*!
 \page dev_telescope_wht William Herschel Telescope control class
 
 Telescope implementation for WHT.
 */

