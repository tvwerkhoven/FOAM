/*
 wfcview.h -- wavefront corrector viewer class
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

#ifndef HAVE_WFCVIEW_H
#define HAVE_WFCVIEW_H

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkglmm.h>

#include "widgets.h"

#include "wfcctrl.h"
#include "deviceview.h"
#include "camview.h"

/*!
 @brief Generic wavefront corrector GUI class
 
 This is the GUI element for WfcCtrl.
 */
class WfcView: public DevicePage {
private:
	WfcCtrl *wfcctrl;
	
	VSeparator vsep0;
	LabeledEntry nact;									//!< Number of actuators
	
	Frame calib_frame;

	Frame wfcact_frame;
	HBox wfcact_hbox;
	LabeledEntry wfcact_ctrl;						//!< WFC ctrl vector
	Alignment wfcact_align;
	EventBox wfcact_events;
	Image wfcact_img;
	Glib::RefPtr<Gdk::Pixbuf> wfcact_pixbuf;
	
	virtual void do_wfcact_update();		//!< Update WF display
	virtual void do_info_update();			//!< Update general info in GUI

	// From DevicePage::
	virtual void on_message_update();
	
	virtual void enable_gui();
	virtual void disable_gui();
	virtual void clear_gui();
	
public:
	WfcView(WfcCtrl *wfcctrl, Log &log, FoamControl &foamctrl, string n);
	~WfcView();
};


#endif // HAVE_WFCVIEW_H


/*!
 \page dev_wfc_ui Wavefront corrector devices UI : WfcView & WfcCtrl

 \section wfcview_wfcview WfcView
 
 Shows a basic GUI for a generic wavefront corrector. See WfcView
 
 \section wfcview_wfcctrl WfcCtrl
 
 Controls a generic wavefront corrector. See WfcCtrl.
 
 \section wfcview_derived Derived classes
 
 The following classes are derived from this class:
 - none
 
 \section wfcview_derived Related classes
 
 The following classes are closely related to this class:
 - none
 
 
 */
