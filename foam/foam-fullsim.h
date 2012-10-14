/*
 foam-fullsim.h -- static simulation module header file
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

#ifndef HAVE_FOAM_FULLSIM_H
#define HAVE_FOAM_FULLSIM_H

#define FOAM_CONFIG_PRE "foam-fullsim"


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
 @brief FOAM full simulation implementation
 
 This FOAM implementation provides a complete end-to-end simulation of the 
 atmosphere, the telescope, the wavefront correctors and -sensors.
 
 Extra command line arguments supported are:
 - none
 
 Extra networking commands supported are:
 - help (ok cmd help): show more help
 - get calib (ok calib <ncalib> <calib1> <calib2> ...): get calibration mdoes
 - calib <calib> (ok cmd calib): calibrate setup
 */
class FOAM_FullSim : public FOAM {
public:
	FOAM_FullSim(int argc, char *argv[]);
	virtual ~FOAM_FullSim() { io.msg(IO_DEB2, "FOAM_FullSim::~FOAM_FullSim()"); } 
	
	virtual int load_modules();
	virtual void on_message(Connection * const connection, string line);
	
	virtual int closed_init();
	virtual int closed_loop();
	virtual int closed_finish();
	
	virtual int open_init();
	virtual int open_loop();
	virtual int open_finish();
	
	virtual int calib(const string &calib_mode, const string &calib_opts);
};

#endif // HAVE_FOAM_FULLSIM_H

/*!	\page ud_foamfs FOAM full-simulation
 
 \section ffs_aboutdoc About this document
 
 This is the (user) documentation for FOAM full-simulation, a simulation 
 module included with FOAM. This describes how to use and test it, not how
 to extend it yourself.
 
 \section ffs_aboutfoamfs About FOAM full-simulation
 
 This module is capable of simulating a simple end-to-end adaptive optics 
 setup. It includes simulation of the wavefront, wind, telescope, deformable 
 mirror, microlens array optics and camera noise. The system is fully 
 functional and can run in open- or closed-loop. It is meant as a testcase for
 FOAM and used mostly for debugging, but can also be used as a showcase. The
 simulation is probably too simple to use in a wider context.
 
 \section ffs_simu Simulation procedure
 
 Most simulation is done by SimulCam::, although this class delegates work to
 SimSeeing::, SimulWfc:: and Shwfs:: classes as well. See the relevant classes
 for more information.

 \section ffs_usage Usage
 
 To run FOAM full-simulation, call:
 
 - ./foam/foam-fullsim [-c /path/to/configuration.cfg]

 and connect to it (preferably with the foam-gui):
 
 - ./ui/foam-gui
 - connect to 'localhost' port '1025' (default)

 \subsection ffs_usage_guioverview GUI Overview
 
 See \ref ud_fgui "FOAM GUI control" for details on how to operate the GUI.
 
 \subsection ffs_usage_cal Calibration
 
 See \ref shwfs_calib_oper "Shack-Hartmann calibration" for details on how to
 calibrate a system with a SHWFS.
 
 \subsection ffs_usage_oper Operating
 
 FOAM can operate in open-loop, where it will display measurement information 
 and print out attempted correction actuation vectors, but not do anything.
 
 While operating in closed-loop, FOAM measures the wavefront deformation and 
 then applies a correction to the DM to negate this. Since we measure the 
 *residual* error (because we are already correcting previously measured 
 shifts, so the DM is not in zero position anymore), we need to add the 
 measured deformation to the already applied correction. FOAM uses a gain 
 setting to scale the error added to the already applied correction. 
 By default this is 1.0, you can change this on the WFC device tab and
 select 'set gain' from the raw control commands dropdown box at the top. 
 Enter the three-valued PID gain and send the command. For example:
 select 'set gain', enter '1 0 0' will set a PID gain of {1,0,0}.
 
 If the system seems unstable: try reducing the P or I gain. Alternatively,
 try to re-calculate the SVD (see above).
 
 \subsection ffs_usage_stop Shutting Down
 
 Stop the system by going in to 'Listen' mode, then click 'Shutdown'.
 
 \section ffs_moreinfo More information

 For more information on the devices that full simulation uses, see:
 - \ref dev_cam_simulcam
 - \ref dev_wfc_simulwfc
 - \ref dev_wfs_shwfs

 
 */
