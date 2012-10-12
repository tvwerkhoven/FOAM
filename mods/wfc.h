/*
 wfc.h -- a wavefront corrector base class
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

#ifndef HAVE_WFC_H
#define HAVE_WFC_H

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <gsl/gsl_vector.h>

#include "types.h"
#include "config.h"

#include "devices.h"

using namespace std;

const string wfc_type = "wfc";

/*!
 @brief Base wavefront corrector class. This will be overloaded with the specific WFC type
 
 \section wfc_ctrl WFC control overview
 
 WFC control goes through several steps, depending on what configuration data is
 available.
 
 - Clamp values (maxact)
 - Control offset (ctrl_offset)
 - Actuation mapping matrix (actmap)
 
 The input control signal is first clamped between [-maxact, maxact]. Then an
 offset vector is added which can be used to correct non-flattness of the 
 mirror. Finally, the control vector is multiplied by an optional actuation 
 mapping which maps from control space (i.e. Zernike modes, KL modes, mirror 
 modes) to the WFC actuator space. In the case that this matrix is identity, 
 operations all happen in WFC actuator space.
 
 If an actuation mapping is used, the number of control parameters of the WFC
 is usually reduced, and actmap is not square.
 
 \section wfc_cmds WFC control commands
 
 The WFC can be driven in various ways. The following commands obey the 
 actuator mapping:
 
 - update_control() add vector of amplitude to all modes, normal operation
 - set_control() set all modes to a certain value
 - set_act() set one mode to a certain value
 - set_randompattern() set all modes to random values:
 
 The following commands always work directly on WFC actuators:

 - set_wafflepattern() set all actuators to a waffle pattern
 - reset() set all actuators to 0

 \section wfc_actmap Actuator mapping
 
 In some cases, one does not want to drive the WFC actuators as they are, but
 drive a set of modes instead. This can be achieved by using a matrix that
 maps the desired actuation basis to WFC actuator signals, which should be of
 size (real_nact, n_modes) with real_nact < n_modes such that a vector with
 n_modes elements results in a control vector of real_nact

 \section wfc_netio Network IO
 
 Valid commends include:
 - set gain \<p\> \<i\> \<d\>: change PID gain for WFC
 - get gain: return current gain
 
 \section wfc_cfg Configuration parameters
 
 - waffle_odd / waffle_even: space or comma-seperated list of actuators for a 
	 waffle pattern. Stored in waffle_even and waffle_odd.

 */
class Wfc: public foam::Device {
protected:
	int real_nact;											//!< Number of hardware actuators in this device. This will be used internally to drive the WFC
	int virt_nact;											//!< Number of modes to use. This will be visible to the outside world (i.e. GUI)

	Path actmap_f;											//!< Actuator mapping matrix file path
	gsl_matrix_float *load_actmap_matrix(Path filepath);
	
	gsl_matrix_float *actmap_mat;				//!< Actuator mapping matrix, from actuation modes (i.e Zernike) to WFC actuators
	int ctrl_apply_actmap();						//!< Apply actuation mapping matrix, if necessary.
	
	string str_waffle_even;							//!< String representation of even actuators. Should be *real* actuators
	string str_waffle_odd;							//!< String representation of odd actuators. Should be *real* actuators
	vector<int> waffle_even;						//!< 'Even' actuators for waffle pattern. Should be *real* actuators, not virtual
	vector<int> waffle_odd;							//!< 'Odd' actuators for waffle pattern. Should be *real* actuators, not virtual
	void parse_waffle(string &odd, string &even); //!< Interpret data in waffle_even and waffle_odd. Should be *real* actuators, not virtual
	bool have_waffle;										//!< Do we know about the waffle pattern?
	
	string ctrl_as_str(const char *fmt="%.4f") const; //!< Return control vector ctrlparams.target as string (not thread-safe)
	string offset_str;									//!< String representation of offset vector

	float maxact;												//!< Maximum actuation signal to allow, clamp all WFC control to [-maxact, maxact]
	gsl_vector_float *workvec;					//!< Workspace for actuator control (size virt_nact)

public:
	// Common Wfc settings
	typedef struct wfc_ctrl {
		wfc_ctrl(): ctrl_vec(NULL), target(NULL), err(NULL), prev(NULL), gain(1,0,0), pid_int(NULL) { }
		gsl_vector_float *ctrl_vec;				//!< Control vector sent to the WFC (size real_nact).

		gsl_vector_float *offset;					//!< Offset added to all control modes (size virt_nact)
		
		gsl_vector_float *target;					//!< (Requested) actuator amplitudes, should be between -1 and 1. (size virt_nact)
		gsl_vector_float *err;						//!< Error between current and target actuation (size virt_nact)
		gsl_vector_float *prev;						//!< Previous actuator amplitudes (size virt_nact)
		gain_t gain;											//!< Operating gain for this device
		gsl_vector_float *pid_int;				//!< Integral part of the PID gain
		float i_ran[2];										//!< Range for individual pid_int elements
	} wfc_ctrl_t;
	
	wfc_ctrl_t ctrlparams;
	
	int get_nact() const { return virt_nact; } //!< Return the number of actuators in use by the WFC
	void set_nact(const int val) { virt_nact = val; }

	void set_gain(const double p, const double i, const double d) { ctrlparams.gain.p = p; ctrlparams.gain.i = i; ctrlparams.gain.d = d; } //!< Set PID gain for WFC control
	
	/*! @brief Update WFC control
	 
	 @param [in] err Error between target and current signal
	 @param [in] g Gain for update
	 @param [in] retain Factor of old control vector to keep (default: 1.0)
	 */
	int update_control(const gsl_vector_float *const err, const gain_t g, const float retain=1.0);
	/*! @brief Update WFC control using default gain & retain
	 
	 @param [in] err Error between target and current signal
	 */
	int update_control(const gsl_vector_float *const err) { return update_control(err, ctrlparams.gain); }
	
	/*! @brief Set WFC control, ignoring current signal
	 
	 @param [in] newctrl New control target for WFC
	 */
	int set_control(const gsl_vector_float *const newctrl);
	/*! @brief Set WFC control for all actuators, ignoring current signal
	 
	 @param [in] val New control target for all WFC actuators
	 */
	int set_control(const float val=0.0);

	/*! @brief Set WFC control for a specific actuators, ignoring current signal
	 
	 @param [in] val New control target for WFC
	 @param [in] act_id WFC actuator to set
	 */
	int set_control_act(const float val, const size_t act_id);
	
	/*! @brief Get WFC control for a specific actuators
	 
	 @param [in] act_id WFC actuator to set
	 @return Actuator signal for act_id
	 */
	float get_control_act(const size_t act_id);
	
	/*! @brief Set wafflepattern on DM with value 'val'
	 
	 Uses waffle pattern as loaded from cfg file at init and stored in 
	 waffle_even and waffle_odd.
	 
	 @param [in] val Value to set on actuators
	 */
	int set_wafflepattern(const float val);
	/*! @brief Set random pattern on DM with maximum value 'maxval'
	 
	 @param [in] maxval Range of random values to set on actuators
	 */
	int set_randompattern(const float maxval);

	// To be implemented by derived classes:
	virtual int actuate(const bool block=false) = 0; //!< Send actuation signal to hardware

	virtual int calibrate();						//!< Calibrate actuator
	virtual int reset();								//!< Reset mirror to best known 'flat' position
	virtual void loosen(const double amp, const int niter=10, const double delay=0.1);	//!< Loosen the mirror by jolting it a few times
	
	// From Device::
	virtual void on_message(Connection *const conn, string line);
	
	virtual ~Wfc();
	//!< Constructor for derived WFCs (i.e. SimWfc)
	Wfc(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const & conffile, const bool online=true);
};

#endif // HAVE_WFC_H

/*!
 \page dev_wfc Wavefront corrector (WFC) devices
 
 The Wfc class provides control for wavefront correctors.
 
 \section dev_wfc_der Derived classes
 - \subpage dev_wfc_simulwfc "Simulate wavefront corrector"
 - \subpage dev_wfc_alpaodm "Alpao deformable mirror"
 
 \section dev_wfs_more See also
 - \ref dev_wfs "Wavefront sensor devices"


*/
