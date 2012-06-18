/*
 foam-fullsim.h -- static simulation module header file
 Copyright (C) 2008--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
	FOAM_FullSim(int argc, char *argv[]): FOAM(argc, argv) { io.msg(IO_DEB2, "FOAM_FullSim::FOAM_FullSim()"); } 
	virtual ~FOAM_FullSim() { io.msg(IO_DEB2, "FOAM_FullSim::~FOAM_FullSim()"); } 
	
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

 Several windows will pop up with information on the AO system. There is one 
 'Control' tab for general control, one 'Log' tab with diagnostic information. 
 Each hardware device has its own tab as well.

 From the GUI you can run in 'Listen', 'Open loop' or 'Closed loop' mode.
 
 - Listen: does nothing and waits for commands
 - Open loop: simulates the image and calculates image shifts
 - Closed loop: open loop +simulate wavefront correction as well

 The simulation system has the following devices:
 
 - Simulcam
 
 This is a camera simulator. See SimulCam for details. The GUI presents 
 several commands for the camera:
 
 - 'Capture' starts capturing frames'
 - 'Display' requests frames from the control software to display
 - 'Store' stores N frames (fill in the field right to the button)
 - 'Exp', 'Offset', 'Intv', 'Gain' set these properties
 - 'Res' and 'Mode' are read-only information boxes showing the resolution and operating mode.
 Several display settings can be used to fine-tune GUI behaviour
 
 - Simwfs
 
 This is a shack-hartmann wavefront sensor using simulcam. It has the following
 GUI commands:
 
 - The 'Subimages' frame can be used to add/modify subapertures. Select one from drop-down list and modify as wanted.
 - 'Regen pattern' will re-generate the subap pattern according to the configuration file
 - 'Find pattern' will heuristically find a subap pattern 'Min I fac' determines how dim a spot is still considered usable. (0.6 is reasonable)
 - 'Show shifts' will display measured image shifts in the camera window (if available)
 
 - Simwfc
 
 This is a deformable mirror simulator. It simulates the wavefront generated
 by a DM by adding gaussian profiles at the actuator positions.
 
 Various fields in the GUI can be used to set actuators to specific voltages (should be between -1 and 1)
 
 \subsection ffs_usage_cal Calibration
 
 - Configure subapertures
 
 Before doing anything, align the system and make sure subimages are visible 
 on the camera. Change gain/exposure/etc. if necessary. Once you see 
 subimages, click 'Find pattern' to find a set of subapertures. Adjust 
 'Min I fac' to satisfy your needs.
 
 - Flat wavefront
 
 Before correcting anything, FOAM needs to know what a 'flat' wavefront is. 
 Ensure optically that a flat wavefront is entering the WFS (e.g. set sim 
 seeing and simwfcerr to zero) and then select calibration mode 'zero' on the 
 'Control' tab. This will measure the spot shifts and define these as flat. 
 The shifts will be stored as reference shift. For testing, it is also 
 possible to simply set the DM to 0 and 'calibrate zero' using that.
 
 Measuring the influence matrix takes 1 frame
 
 - Influence matrix
 
 FOAM needs to know what each actuator does to the measurements, so we need 
 the influence matrix of the DM on the WFS. To measure this, FOAM moves each 
 actuator back and forth while measuring the image shifts generated by this 
 deformation. This results in a matrix of n_act by n_subap*2 (*2 for x and y).
 This matrix gives the behaviour of the WFS given a certain DM actuation. 
 Since we need to know the inverse of this, we pseudo-invert this matrix. 
 The pseudo inverse is then used to calculate the DM deformation necessary
 to correct a certain WFS measurement.
 
 To get an influence matrix, run an 'Influence' calibration from the 'Control'
 tab. This measurement takes a while, don't disturb the system while calibrating.
 
 Measuring the influence matrix takes n_act * 2 frames
 
 - Re-calculate SVD
 
 Although the 'influence' calibration already applies an SVD to the influence
 data, you can safely re-calculate this with the 'svd' calibration mode on
 the 'Control' tab. This calibration mode takes a single parameter that
 can be filled in in the entry box beside it. The behaviour of an SVD
 calibration depends on this value:
 
 - if parameter < 0: drop this many modes from the SVD. i.e. if you have 100 actuators, and the param is 10, use only 90 modes for correcting.
 - if param > 1: use this many modes from the SVD (but never more than # actuators)
 - if 0 < param 1 <: use this much singular value in the SVD. 0.7 should be quite stable but not very accurate, 0.995 should be fairly accurate but might be less stable.
 
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
 try to re-calculate the SVD (see above) with a higher singular value
 
 \subsection ffs_usage_stop Shutting Down
 
 Stop the system by going in to 'Listen' mode, then click 'Shutdown'.
 
 \section ffs_moreinfo More information

 For more information on the devices that full simulation uses, see:
 - \ref dev_cam_simulcam
 - \ref dev_wfc_simulwfc
 - \ref dev_wfs_shwfs

 
 */
