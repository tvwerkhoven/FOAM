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
	LabeledEntry wfc_nact;							//!< Number of actuators
	
	Frame calib_frame;
	HBox calib_hbox;
	LabeledEntry calib_setall;
	VSeparator vsep1;
	LabeledSpinEntry calib_setactid;
	LabeledEntry calib_setactval;
	VSeparator vsep2;
	Button calib_random;
	Button calib_waffle;
	
	Frame wfcact_frame;
	HBox wfcact_hbox;
	BarGraph wfcact_gr;
	
	void do_wfcact_update() const { wfcctrl->send_cmd("get ctrl"); } //!< Request update of ctrl vector
	void on_wfcact_update();						//!< Update WF display
	
	void on_calib_random_clicked();			//!< Callback for calib_random button
	void on_calib_waffle_clicked();			//!< Callback for calib_waffle button
	void on_calib_setall_act();					//!< Callback for calib_setall entry
	void on_calib_setact_act();					//!< Callback for calib_setactid/calib_setactval entries
	
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
