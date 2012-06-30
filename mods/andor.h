/*
 andor.h -- Andor iXON camera modules
 Copyright (C) 2011-2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef HAVE_ANDOR_H
#define HAVE_ANDOR_H

#include <stdio.h>
#include <stdint.h>
#include <map>
#include <vector>

#include <atmcdLXd.h> // For Andor camera

#include "config.h"
#include "io.h"
#include "path++.h"

#include "foam.h"
#include "foamctrl.h"
#include "camera.h"

using namespace std;
const string andor_type = "andorcam";

/*!
 @brief Andor iXON camera class
 
 AndorCam is derived from Camera. It can control an Andor iXON camera.
 
 \section andorcam_cfg Configuration parameters
 
 AlpaoDM supports the following configuration parameters:
 
 - cooltemp: default requested cooling temperature, see cool_info
 - andor_cfgdir: see andordir (default "/usr/local/etc/andor")
 
 \section andorcam_cfg Network IO
 
 - get/set cooling: control cooling temperature, see cool_info
 
 */
class AndorCam: public Camera {
private:
	std::map< int, std::string > error_desc; //!< Error descriptions (from Andor SDK)
	
	std::vector< unsigned short* > img_buffer; //!< Local image buffer
	
	AndorCapabilities caps;							//!< Andor camera capabilities
	std::vector< string > caps_vec;			//!< Andor camera capabilities, human readable

	// Cooling settings
	struct cooling {
		cooling(): status(DRV_TEMP_OFF), operating(false) { range[0] = range[1] = 0; target=25; current=25; }
		int range[2];											//!< Camera cooling temperature range
		int target;												//!< Requested cooling temperature
		int current;											//!< Current camera temperature
		int status;												//!< Cooler status: DRV_TEMP_OFF, DRV_TEMP_STABILIZED, DRV_TEMP_NOT_REACHED, DRV_TEMP_DRIFT, DRV_TEMP_NOT_STABILIZED
		bool operating;										//!< Cooler on/off status
	};
	
	struct cooling cool_info;						//!< Camera cooling info
	
	int frametransfer;									//!< Current frametransfer mode
	int emgain_range[2];								//!< EM CCD gain range for the camera
	
	string andordir;										//!< Andor configuration file directory (i.e. /usr/local/etc/andor)
	
	// Interal functions go here (should not be exposed as API ever)
	void read_capabilities(AndorCapabilities *caps, std::vector< string > &cvec); //!< Read capabilities from *caps into a human readable vector of strings in cvec
	int get_cooltempstat(int what=0);		//!< Get camera cooler temperature (what == 0) or status (what != 0)
	void init_errors();									//!< Initialize error_desc array of human readable error descriptions
	int initialize();										//!< Initialize standard Andor camera features
	
	// Additional funcionality for this Camera (probably only used in this class):
	void cam_get_capabilities();				//!< Get camera capabilities
	
	void cam_get_coolrange(int *mintemp, int *maxtemp); //!< Get cooling temperature range
	void cam_get_coolrange();						//!< Get cooling temperature range, store in cool_info
	bool cam_get_cooleron();						//!< Get cooler status
	void cam_set_cooltarget(const int value); //!< Set new cooler temperature target
	int cam_get_cooltemp();									//!< Get current camera temperature
	int cam_get_coolstatus();								//!< Get current cooler status (DRV_TEMP_OFF, DRV_TEMP_STABILIZED, DRV_TEMP_NOT_REACHED, DRV_TEMP_DRIFT, DRV_TEMP_NOT_STABILIZED)
	void cam_set_cooler(bool status=true); //!< Turn cooler on or off
	
	
	/*!
	 @brief Set frametransfer mode on camera
	 	 
	 @param [in] ft Enable frametransfer mode (1) or disable it (0)
	 @return Frametransfer status set.
	 */
	int cam_set_frametranfer(const int ft);

	int cam_get_frametranfer() const; //!< Get frametransfer mode from camera

	/*!
	 @brief Set EM gain mode (0: DAC 0--255, 1: DAC 0--4095, 2: Linear, 3: Real EM gain)
	 
	 Mode 0: The EM Gain is controlled by DAC settings in the range 0-255. Default mode.
	 Mode 1: The EM Gain is controlled by DAC settings in the range 0-4095. 
	 Mode 2: Linear mode.
	 Mode 3: Real EM gain
	 
	 @param [in] mode The EM gain mode to set
	 */
	void cam_set_gain_mode(const int mode); 
	void cam_get_gain_range(int *gain_min, int *gain_max); //!< Get EM CCD gain range given the current EM CCD gain mode (cam_set_gain_mode) and the current cooling temperature (cam_set_cooltarget)
	
	void cam_set_shift_speed(const int hs, const int vs, const int vamp); //!< Set horizontal and vertical shift speed and vertical shift amplitude.
	
	/*! @brief Get acquisition timings from Andor camera: exposure, accumulate and kinetic cycle time
	 */
	void cam_get_timings();
	
public:
	AndorCam(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~AndorCam();
	
	void set_temperature(const int temp) { cam_set_cooltarget(temp); }
	int get_temperature() { return cam_get_cooltemp(); }
	
	std::vector< string > get_andor_caps() { return caps_vec; }
	void print_andor_caps(FILE *fd=stdout);
	
	// From Camera::
	void cam_handler();
	void cam_set_exposure(const double value);
	double cam_get_exposure();
	void cam_set_interval(const double value);
	double cam_get_interval();
	void cam_set_gain(const double value);
	double cam_get_gain();
	void cam_set_offset(const double value);
	double cam_get_offset();
	void cam_set_shutter(const int status);
	
	void cam_set_mode(const mode_t newmode);
	void do_restart();
	
	// From Devices::
	int verify() { return 0; }
	void on_message(Connection *const conn, string line);
};

#endif // HAVE_ANDOR_H

/*!
 \page dev_cam_andor Andor camera devices
 
 The AndorCam class can drive an Andor iXON camera.
 
 */