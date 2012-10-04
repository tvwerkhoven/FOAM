/*
 foam-simstatic.h -- static simulation module header file
 Copyright (C) 2008--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_FOAM_SIMSTATIC_H
#define HAVE_FOAM_SIMSTATIC_H

#define FOAM_CONFIG_PRE "foam-simstat"

#include "config.h"
#include "path++.h"

// LIBRARIES //
/*************/

#include "foam.h"
#include "types.h"
#include "io.h"

using namespace std;

/*!
 @brief FOAM static simulation implementation
 
 This FOAM implementation provides a simple static simulation. The WFS camera
 is fed by an image stored on disk and the shifts calculated that way are 
 thus static. This setup is intended as a benchmark.
 
 Extra command line arguments supported are:
 - none
 
 Extra networking commands supported are: 
 - help (ok cmd help): show more help
 - get calib (ok calib <ncalib> <calib1> <calib2> ...): get calibration mdoes
 - calib <calib> (ok cmd calib): calibrate setup
 
 */
class FOAM_simstatic : public FOAM {
public:
	FOAM_simstatic(int argc, char *argv[]): FOAM(argc, argv) { io.msg(IO_DEB2, "FOAM_simstatic::FOAM_simstatic()"); } 
	virtual ~FOAM_simstatic() { io.msg(IO_DEB2, "FOAM_simstatic::~FOAM_simstatic()"); } 
	
	virtual int load_modules();
	virtual void on_message(Connection *connection, string line);
	
	virtual int closed_init();
	virtual int closed_loop();
	virtual int closed_finish();
	
	virtual int open_init();
	virtual int open_loop();
	virtual int open_finish();
	
	virtual int calib(const string &calib_mode, const string &calib_opts);
};

#endif // HAVE_FOAM_SIMSTATIC_H

/*! \page ud_foamss FOAM static-simulation
 
 \section fss_aboutdoc About this document
 
 This is the (user) documentation for FOAM static-simulation, a simulation 
 module included with FOAM. This describes how to use and test it, not how
 to extend it yourself.
 
 \section fss_aboutfoamss About FOAM static-simulation
 
 This module is capable of operating like a 'real' AO system without hardware.
 It uses a static frame as input from the camera, and does not drive any
 electronics such as a deformable mirror or tip-tilt mirror. The computational
 load is the same, however, such that it can be used to test and improve the 
 performance that a real system with hardware would have.
 
 \section fss_simp Simulation procedure
 
 Static simulation is performed as follows:
 
 - A simulated camera frame is loaded from disk. This image is used as 'camera'
 - The image is processed as if it were real, resulting in a wavefront
 - The wavefront information is converted into a set of actuation commands, 
    which are not used 
 
 \section fss_simu Usage
 
 To use this module, first start it up like:
 
 - ./foam/foam-simstat -c conf/foam-simstat.cfg
 
 and connect to it (preferably with the foam-gui):
 
 - ./ui/foam-gui
 - connect to 'localhost' port '1025' (default)
 
 From the GUI you can run in 'Listen', 'Open loop' or 'Closed loop' mode.
 
 - Listen does nothing and waits for commands
 - Open loop calculates image shifts but nothing else
 - Closed loop calculates image shifts and then calculates what commands to 
 send to a hypothetical deformable mirror
 
 For more information on the devices that static simulation uses, see the 
 \ref dev_cam_imgcam and \ref dev_wfs_shwfs documentation.

 */
