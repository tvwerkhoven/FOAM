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
	FOAM_ExpoAO(int argc, char *argv[]);
	virtual ~FOAM_ExpoAO() { io.msg(IO_DEB2, "FOAM_ExpoAO::~FOAM_ExpoAO()"); } 
	
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
 - Andor camera + driver, see Camera and AndorCam
 - Alpao DM97 + driver: see Wfc and AlpaoDM
 - 8x8 microlens array Shack-Hartmann wavefront sensor: see Wfs and Shwfs
 - William Herschel Telescope: see Telescope and WHT

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
 
 \subsection ffs_usage_guioverview GUI Overview
 
 See \ref ud_fgui "FOAM GUI control" for details on how to operate the GUI.
 
 \subsection ffs_usage_cal Calibration
 
 See \ref shwfs_calib_oper "Shack-Hartmann calibration" for details on how to
 calibrate a system with a SHWFS.
 
 \subsection ffs_usage_live Live control
 
 After calibration, live control is usually necessary due to changing seeing 
 conditions. These are some parameters the user can tweak:
 
 - Number of modes (Control tab): tweak the number of modes corrected by choosing 'SVD' and then enter a number of modes (2<N) or enter a decimal number indicating how much power you want to use ([0,1]). This will update while running closed or open-loop
 - Wfc::maxact (WFC tab): limit the maximum amplitude each mode is set to by changing maxact. This will clamp all actuation values between [-maxact, maxact].
 - Shwfs::shift_mini (WFS tab): change the background bias ('dark-field'), pixels below this value will be neglected for CoG. Set this to the maximum dark signal you expect.
 - Shwfs::maxshift (WFS tab): change the maximum shift allowed, like maxact. Shifts above this value will be clipped.
 - If the signal is too low, you can increase the exposure time of the camera. This is of course a trade-off between noise and speed.
 - Telescope tracking can be tuned through the Telescope tab. See Telescope and WHT for specific controls.
 

 \section fex_troubleshoot Troubleshooting
 
 These are some known issues on the FOAM ExPoAO system
 
 \subsection fex_trbl_andor FOAM does not exit
 
 FOAM has a bug that it does not exit properly at the end. Simply pressing ^C 
 after you see the Andor and Alpao drivers have exited will quit the program 
 just fine. 

 Note that the Andor camera is first warmed up to room temperature before FOAM
 exits so be sure that this is not happening.

 \subsection fex_trbl_andor Could not initialize andor camera
 
 When you get an error like:
 <pre>
 [err ] FOAM_ExpoAO::load_modules: Could not initialize andor camera! error: 20992, DRV_NOT_AVAILABLE
 </pre>
 the Andor driver has come in a bad state and needs to be reset. This can be 
 done by re-installing the Andor camera driver located in 
 ~expoao/drivers/andor_sdk. Simply run 
 <pre>
 sudo ./install_andor
 </pre>
 and install the driver for the iXon series and restart FOAM.
 
 This error especially occurs when FOAM does not exit gracefully

 \subsection fex_trbl_alpao Could not initialize Alpao driver
 
 The Alpao driver can also crash, which also requires a reinstall.
 
 \section fex_related More information
 
 For more information on the devices that this build uses, see:
 - \ref dev_cam_andor
 - \ref dev_wfs_shwfs
 - \ref dev_wfc_alpaodm
 - \ref dev_telescope_wht
 
 */
