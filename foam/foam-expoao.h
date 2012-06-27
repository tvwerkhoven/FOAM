/*
 foam-expoao.h -- FOAM expoao build header
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

/*!	\page ud_foamex FOAM expoao
 
 \section fex_aboutdoc About this document
 
 This is the (user) documentation for FOAM expoao
 
 \section fex_aboutfoamex About FOAM expoao
 
 This FOAM program will operate on the ExPo instrument
 
 \section fex_usage Usage
 
 To run FOAM expoao, call:
 
 - ./foam/foam-expoao -c conf/foam-expoao.cfg

 and connect to it (preferably with the foam-gui):
 
 - ./ui/foam-gui
 - connect to 'localhost' port '1025' (default)
 
 From the GUI you can run in 'Listen', 'Open loop' or 'Closed loop' mode.
 
 - Listen: does nothing and waits for commands
 - Open loop: captures frames and calculates image shifts
 - Closed loop: open loop and correct the image
 
 For more information on the devices that this build uses, see:
 - \ref dev_cam_andor
 - \ref dev_wfs_shwfs
 
 */
