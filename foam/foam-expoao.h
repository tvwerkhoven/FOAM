/*
 foam-expoao.h -- FOAM expoao build header
 Copyright (C) 2008-2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_FOAM_EXPOAO_H
#define HAVE_FOAM_EXPOAO_H

// LIBRARIES //
/*************/

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include "foam.h"
#include "types.h"
#include "io.h"

using namespace std;

/*!
 @brief FOAM expoao build target
 
 This FOAM implementation will operate for the ExPo instrument using an
 Andor iXon camera as (Shack-Hartmann) wavefront sensor and an Alpao 
 deformable mirror.
 
 Extra command line arguments supported are:
 - none
 
 Extra networking commands supported are:
 - help (ok cmd help): show more help
 - get calib (ok calib <ncalib> <calib1> <calib2> ...): get calibration mdoes
 - calib <calib> (ok cmd calib): calibrate setup
 */
class FOAM_ExpoAO : public FOAM {
public:
	FOAM_ExpoAO(int argc, char *argv[]): FOAM(argc, argv) { io.msg(IO_DEB2, "FOAM_ExpoAO::FOAM_ExpoAO()"); } 
	virtual ~FOAM_ExpoAO() { io.msg(IO_DEB2, "FOAM_ExpoAO::~FOAM_ExpoAO()"); } 
	
	virtual int load_modules();
	virtual void on_message(Connection * const connection, string line);
	
	virtual int closed_init();
	virtual int closed_loop();
	virtual int closed_finish();
	
	virtual int open_init();
	virtual int open_loop();
	virtual int open_finish();
	
	virtual int calib();
};

#endif // HAVE_FOAM_EXPOAO_H

/*!	\page ud_foamexpo FOAM expoao
 
 \section fex_aboutdoc About this document
 
 This is the (user) documentation for FOAM expoao
 
 \section fex_aboutfoamex About FOAM expoao
 
 This FOAM program will operate the adaptive optics for ExPo, the Extreme 
 Polarimeter\cite rodenhuis2008. The AO for ExPo was designed primarily 
 stabilize the image such
 that a coronagraph can be used to block out stellar light and increase 
 constrast. Besides this it should also aid in increasing the resolution in 
 the ExPo science camera.
 
 The adaptive optics system is described in detail in\cite homs2012, but a 
 brief summary is included below.
 
 \subsection fex_expoao ExPo adaptive optics
 
 The ExPo adaptive optics system is designed to stabilize the image onto the 
 coronagraph in ExPo while keeping polarisation effects to a minimum. To this 
 end, there are only 4 optical components that influence the ExPo science beam.
 
 \subsection fex_expohw ExPo hardware
 
 - ExPoAO computer
 - Andor camera + driver
 - AlpaoDM + driver
 - William Herschel Telescope

 \subsection fex_exposw ExPo software

 - Ubuntu 10.04 
 - foam-expoao
 - digiport software

 \section fex_install Installation
 
 To install foam-expoao, follow the regular installation instructions.

 \section fex_usage Usage
 
 To run FOAM expoao, call:
 
 - foam-expoao [-c /path/to/foam-expoao.cfg]

 and connect to it (preferably with the foam-gui):
 
 - foam-gui
 - connect to 'localhost' port '1025' (default)
 
 From the GUI you can run in 'Listen', 'Open loop' or 'Closed loop' mode.
 
 - Listen: does nothing and waits for commands
 - Open loop: captures frames and calculates image shifts
 - Closed loop: open loop and correct the image
 
 \section fex_related More information
 
 For more information on the devices that this build uses, see:
 - \ref dev_cam_andor
 - \ref dev_wfs_shwfs
 - \ref dev_wfc_alpaodm
 - \ref dev_telescope_wht
 
 */
