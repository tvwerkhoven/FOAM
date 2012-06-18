/*
 wfsview.h -- wavefront sensor control class
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_WFSVIEW_H
#define HAVE_WFSVIEW_H

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkglmm.h>

#include "widgets.h"

#include "wfsctrl.h"
#include "deviceview.h"
#include "camview.h"

/*!
 @brief Generic wavefront sensor GUI class
 
 This is the GUI element for WfsCtrl, it shows controls for wavefront sensors.
 It mainly shows a graphical representation of the power in each wavefront 
 mode.
 */
class WfsView: public DevicePage {
protected:
	WfsCtrl *wfsctrl;
	
	VSeparator vsep0;
	LabeledEntry wf_cam;								//!< Camera used for this wavefront sensor	
	
	Frame wfpow_frame;
	HBox wfpow_hbox;
	BarGraph wfpow_gr;
	LabeledEntry wfpow_mode;						//!< Wavefront representation modes used (KL, Zernike, mirror, etc.)
	
	CamView *wfscam_ui;									//!< Camera GUI class used with this SHWFS
	CamCtrl *wfscam_ctrl;								//!< Camera ctrl class used with this SHWFS
	
	void on_wfpow_update();							//!< Update WF display
	virtual void on_cam_update();				//!< Update on WFS camera

	void do_wfpow_update() const { wfsctrl->send_cmd("get modes"); } //!< Request update of modes vector

	// From DevicePage::
	virtual void on_message_update();

	virtual void enable_gui();
	virtual void disable_gui();
	virtual void clear_gui();
	
public:
	WfsView(WfsCtrl *wfsctrl, Log &log, FoamControl &foamctrl, string n);
	~WfsView();
};


#endif // HAVE_WFSVIEW_H


/*!
 \page dev_wfs_ui Wavefront sensor devices UI : WfsView & WfsCtrl

 \section wfsview_wfsview WfsView
 
 Shows a basic GUI for a generic wavefront sensor. See WfsView
 
 \section wfsview_wfsctrl WfsCtrl
 
 Controls a generic wavefront sensor. See WfsCtrl.
 
 \section wfsview_derived Derived classes
 
 The following classes are derived from this class:
 - \subpage dev_wfs_shwfs_ui "Shack-Hartmann Wavefront sensor device UI"
 
 \section wfsview_derived Related classes
 
 The following classes are closely related to this class:
 - \ref dev_cam_ui "Camera device UI"
 
 
 */
