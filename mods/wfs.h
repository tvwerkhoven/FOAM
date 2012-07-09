/*
 wfs.h -- a wavefront sensor abstraction class
 Copyright (C) 2009--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_WFS_H
#define HAVE_WFS_H

#include <string>
#include <gsl/gsl_vector.h>

#include "io.h"
#include "types.h"
#include "config.h"

#include "camera.h"
//#include "zernike.h"

using namespace std;

const string wfs_type = "wfs";

/*!
 @brief Base wavefront-sensor class. 
 
 This class provides a template for the implementation of wavefront sensors 
 such as a Shack-Hartmann WFS. This class should be implemented with WFS
 specific routines. The Wfs class is independent of the camera used and
 only provides the data processing/interpretation of the camera. The camera
 itself can be accessed through the reference &cam.
 
 \section wfs_netio Network IO
 
 - measuretest: force fake measurements being produced (to test GUI etc.)
 - calibrate: calibrate device (mostly allocate memory etc.)
 - measure: force manual measurement (should normally be done by FOAM itself)

 - get basis: return the basis functions used for measurement (i.e. shifts, zernike, KL)
 - get modes: return N modes as <N> <M#1> <M#2> ... <M#N>. 'Modes' can be 
 - get calib: get calibration state
 - get camera: return camera name associated with this WFS
 
 \section wfs_cfg Configuration
 
 - none

 */
class Wfs: public foam::Device {
private:
	void init();												//!< Common initialisation for constructors
	
public:	
	enum wfbasis {
		ZERNIKE=0,												//!< Zernike
		KL,																//!< Karhunen-Loéve 
		SENSOR,														//!< Sensor modes (e.g. shift vectors)
		MIRROR,														//!< Mirror modes
		UNDEFINED
	};
	
	/*!
	 @brief This holds information on the wavefront
	 */
	typedef struct wavefront {
		wavefront() : wfamp(NULL), wf_full(NULL), nmodes(0), basis(SENSOR) { ; }
		gsl_vector_float *wfamp;					//!< Residual mode amplitudes (i.e. to be corrected, see Shwfs::measure)
		gsl_vector_float *wf_full;				//!< Full mode amplitudes (i.e. what is currently corrected, see Shwfs::comp_tt, might be NULL)
		int nmodes;												//!< Number of modes
		enum wfbasis basis;								//!< Basis functions used for this representation (see Wfs::wfbasis)
	} wf_info_t;                        //!< Capture all wavefront information in flexible format
	
	//Zernike zernbasis;									//!< Zernike polynomials basis
	//gsl_matrix_float *zerninfl;					//!< Influence matrix to convert WFS data to Zernike modes

	wf_info_t wf;												//!< Wavefront representation
	
	Camera &cam;												//!< Reference to the camera class used for this WFS
	
	// To be implemented in derived classes:
	/*! Measure wavefront aberraions using a camera frame
	 
	 @param [in] *frame Camera frame to process
	 @return Wavefront information given *frame
	 */
	virtual wf_info_t* measure(Camera::frame_t *frame=NULL);
	
	/*! @brief Calibrate sensor, set up reference and mode basis
	 
	 */
	virtual int calibrate();
	
	// From Device::
	virtual void on_message(Connection *const conn, string line);
	
	virtual ~Wfs();
	Wfs(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const & conffile, Camera &wfscam, const bool online=true); //!< Constructor for derived WFSs (i.e. SHWFS)
	Wfs(Io &io, foamctrl *const ptc, const string name, const string port, Path const & conffile, Camera &wfscam, const bool online=true); //!< Constructor for bare WFS
};

#endif // HAVE_WFS_H

/*!
 \page dev_wfs Wavefront sensor devices
 
 The Wfs class provides control for wavefront sensors.
 
 \section dev_wfs_der Derived classes
 - \subpage dev_wfs_shwfs "Shack-Hartmann wavefront sensor device"

 \section dev_wfs_more See also
 - \ref dev_cam "Camera devices"
 
*/ 
