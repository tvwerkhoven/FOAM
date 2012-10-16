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
 
 For ExPo, the conversion was (N.B. this is unverified and might be wrong!):

\verbatim
 alt: =  -0.01 [ x * sin(0.001745 * (45 - ele)) + y * cos(0.001745 * (45 - ele)) ]
 az:  =  +0.01 [ y * sin(0.001745 * (45 - ele)) - x * cos(0.001745 * (45 - ele)) ]
\endverbatim
 
 with x, y the measured shift, 45 the rotation of the ExPo camera, ele the 
 current elevation of the telescope, and 0.001745 180/pi. The scaling, gain 
 etc is all encapsulated in the constant 0.01. This might have to be adjusted
 live.

 @section wht_guiding Guiding RS232 control

 Once these coordinates are known, they have to be sent to a RS232 port over 
 ethernet. A Digi PortServer II Rack is used for this, see drivers below.
 Installing the driver will make a <tt>/dev/tty<xxxx></tt> port on the Linux machine
 which can be used to send guiding offset commands. The syntax for these 
 commands is:

\verbatim
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
\endverbatim
 
 for example:
\verbatim
 s0000p00 s0000p00 00000p01\r
 00050.00 00050.00 00000.10
\endverbatim
 
 to send neutral guiding information (=do nothing) and timeout after 0.1 
 second. Currently, this is implemented as:

\code{.cc}
 string cmdstr = format("%07.2f %07.2f %07.2f\r", ctrl0, ctrl1, delay*10.0+drand48()*0.1);
\endcode
 
 @section wht_guiding Live telescope pointing information
 
 To correctly convert from pixel shift to telescope guiding coordinates, we 
 need the elevation as input. This is available at 
 <http://whtics.roque.ing.iac.es:8081/TCSStatus/TCSStatusExPo>. The syntax of 
 this is document is:
 
 \verbatim
AZ=298.6429727035839
ALT=90.28124237374263
LAST=21 9 15.184
DELAY=1.0
SEQUENCE=7055
RA=0
DEC=0
EQUINOX=2000
COORDTYPE=J
TELSTAT=ENGMODE 
AIRMASS=1.000012040138244629
MJD=55181.694
SOURCENAME=AGB
SOURCERA=4.55967267083519
SOURCEDEC=0.698131700797732
SOURCEEQUINOX=2000
SOURCECOORDTYPE=J
 \endverbatim
 
 We extract the AZ and ALT parameters, and update if these are different from 
 what we have. We then rotate the coordinate system using the ALT parameter 
 as folows:

 In general:
\verbatim
 x' = [ x cos(th) - y sin(th) ]
 y' = [ x sin(th) + y cos(th) ]
\endverbatim

 in WHT::update_telescope_track():
\code{.cc}
 ctrl0 = 50 + (ttgain.p * (sht0 * cos(altfac * (telpos[0]*M_PI/180.0)) - sht1 * sin(altfac * (telpos[0]*M_PI/180.0))));
 ctrl1 = 50 + (ttgain.p * (sht0 * sin(altfac * (telpos[0]*M_PI/180.0)) + sht1 * cos(altfac * (telpos[0]*M_PI/180.0))));
\endcode
 
 @section wht_oper Guiding operations
 
 To use guiding, the user can toggle telescope rotation by setting altfac to
 0 or 1 to disable or enable respectively. The rest of the controls are done
 through the parent Telescope class.
 
 @section wht_todo Todo
 
 - test coordinate conversion
 - make standalone WHT controller
  
 @section wht_more More info
 
 - http://www.ing.iac.es/~eng/ops/general/digi_portserver.html
 - http://www.ing.iac.es/~cfg/group_notes/portserv.htm
 - http://www.ing.iac.es/Astronomy/tonotes/wht/expo_working.html
 - http://whtics.roque.ing.iac.es:8081/TCSStatus/TCSStatusExPo
 - http://www.digi.com/support/productdetail?pid=2240&type=drivers
 - ftp://ftp1.digi.com/support/beta/linux/dgrp/
 
 \section wht_cfg Configuration params
 
 - track_host: live WHT pointing host (whtics.roque.ing.iac.es)
 - track_port: live WHT pointing port (8001)
 - track_file: live WHT pointing file (/TCSStatus/TCSStatusExPo)
 - port: Serial port to use (/dev/ttyao00)
 - altfac: WHT::altfac 
 
 \section wht_netio Network commands
 
 - get trackurl: get WHT tracker information URL
 - get/set altfac: get/set WHT::altfac
 
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
	void tcs_control(float tcs0, float tcs1, const float thisdelay); //!< Send control command to William Herschel Telescope control system (TCS)
	
public:
	WHT(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~WHT();
	
	double altfac;					//!< Factor to multiple alt with before rotation (i.e. rotate counter-clockwise if -1)
	double delay;						//!< WHT info update period
	std::map<string, string> wht_info; // Full WHT info from website
		
	int get_wht_coords(float *const alt, float *const az); //!< Get last known WHT pointings coordinates
	
	// From Telescope::
	/*! @brief Given generic shift coordinates, track the WHT
	
	 1. Rotate the generic shift by the telescope position (depending on altfac)
	 2. Apply gain to correction
	 3. Convert to TCS coordinates, i.e. 50 - \<output of step 2.\>
	 
	 Spefically, the code does the following:
	 \code{.cc}
	 ctrl0 = 50 + (ttgain.p * (sht0 * cos(altfac * (telpos[0]*M_PI/180.0)) - sht1 * sin(altfac * (telpos[0]*M_PI/180.0))));
	 ctrl1 = 50 + (ttgain.p * (sht0 * sin(altfac * (telpos[0]*M_PI/180.0)) + sht1 * cos(altfac * (telpos[0]*M_PI/180.0))));
	 \endcode
	 
	 and send ctrl0, ctrl1 to the WHT TCS.
	 
	 @param [in] sht0 Generic shift dimension 0
	 @param [in] sht1 Generic shift dimension 1
	 */
	virtual int update_telescope_track(const float sht0, const float sht1);
	
	// From Devices::
	virtual void on_message(Connection *const conn, string);
};

#endif // HAVE_WHT_H

/*!
 \page dev_telescope_wht William Herschel Telescope control class
 
 Telescope implementation for WHT.
 */

