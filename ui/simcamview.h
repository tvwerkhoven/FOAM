/*
 simcamview.h -- simulation camera control class
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_SIMCAMVIEW_H
#define HAVE_SIMCAMVIEW_H

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif


#include "widgets.h"

#include "deviceview.h"
#include "camview.h"
#include "simcamctrl.h"
#include "glviewer.h"

using namespace Gtk;
using namespace std;

/*!
 @brief Simulation camera viewing class
 
 Based on CamView, adds some controls for simulation cameras
 */
class SimCamView: public CamView {
private:
	SimCamCtrl *simcamctrl;
	
	// Simulation stuff, make buttons for each simulation step
	Frame simframe;
	HBox simhbox;
	SwitchButton sim_seeing;						//!< Simulate seeing
	SwitchButton sim_wfcerr;						//!< Simulate WFC as error source
	SwitchButton sim_wfc;								//!< Simulate WFC as correction source
	SwitchButton sim_tel;								//!< Simulate telecope cropping
	SwitchButton sim_wfs;								//!< Simulate wavefront sensor (MLA)
	VSeparator sim_vsep1;
	LabeledSpinEntry e_noisefac;				//!< Factor of the CCD with noise
	LabeledSpinEntry e_noiseamp;				//!< Noise amplitude
	LabeledSpinEntry e_seeingfac;				//!< Seeing factor
	
	void on_sim_seeing_clicked();
	void on_sim_wfcerr_clicked();
	void on_sim_wfc_clicked();
	void on_sim_tel_clicked();
	void on_sim_wfs_clicked();
	void on_noise_activate();						//!< Callback for e_noisefac/e_noiseamp
	void on_seeing_activate();					//!< Callback for e_seeingfac

	// From CamView:
	virtual void disable_gui();
	virtual void enable_gui();
	virtual void clear_gui();

	virtual void on_message_update();

	//virtual void on_message_update();
public:
	SimCamView(SimCamCtrl *ctrl, Log &log, FoamControl &foamctrl, string n);
	~SimCamView();
	
};


#endif // HAVE_SIMCAMVIEW_H


/*!
 \page dev_simcam_ui Simulation Camera devices UI: SimCamView & SimCamCtrl
 
 \section simcamview_simcamview SimCamView
 
 Shows a basic GUI for a simulation camera. See SimCamView
 
 \section simcamview_camctrl SimCamCtrl
 
 Controls a simulation camera. See SimCamCtrl.
 
 \section simcamview_derived Derived classes
 
 The following classes are derived from the SimCamera device:
 - none
 
 */
