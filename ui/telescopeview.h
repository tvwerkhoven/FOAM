/*
 telescopeview.h -- Telescope GUI
 Copyright (C) 2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_TELESCOPEVIEW_H
#define HAVE_TELESCOPEVIEW_H

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkglmm.h>

#include "widgets.h"

#include "telescopectrl.h"
#include "deviceview.h"

/*!
 @brief Generic telescope GUI class
 
 Displays current telescope coordinates (and which units this are), as well 
 as tip-tilt tracking information from the wavefront sensor. The tip-tilt is 
 shown as raw shifts from the WFS, intermediate converted shifts and final 
 telescope control commands
 
 */
class TelescopeView: public DevicePage {
private:
	TelescopeCtrl *telescopectrl;
	
	Frame track_frame;
	HBox track_hbox;

	LabeledEntry tel_pos;						//!< Telescope track position

	VSeparator vsep0;
	
	LabeledEntry tt_raw;						//!< Raw tip-tilt coordinate
	LabeledEntry tt_conv;						//!< Converted tip-tilt coordinate
	LabeledEntry tt_ctrl;						//!< Telescope control tip-tilt coordinate
	
	VSeparator vsep1;
	
	Button b_refresh;								//!< Update once button
	SwitchButton b_autoupd;					//!< Auto update button
	LabeledSpinEntry e_autointval;	//!< Auto update interval
	
	Frame ctrl_frame;
	HBox ctrl_hbox;

	LabeledEntry ccd_ang_e;					//!< CCD rotation
	LabeledEntry scalefac0_e;				//!< Scale factor for axis 0
	LabeledEntry scalefac1_e;				//!< Scale factor for axis 1
	LabeledEntry ttgain_e;					//!< Telescope tip-tilt track gain
	
	bool on_timeout();							//!< Run this 30x/sec to enable auto updating, throttle with e_autointval
	void do_teltrack_update() const;//!< Update telescope tracking info
	
	void on_autoupd_clicked();			//!< Callback for b_autoupd
	sigc::connection refresh_timer;	//!< For Glib::signal_timeout()
	struct timeval lastupd;					//!< Time of last update

	void on_info_change();					//!< Callback for ccd_ang / scalefac / ttgain

	// From DevicePage::
	virtual void on_message_update();
	
	virtual void enable_gui();
	virtual void disable_gui();
	virtual void clear_gui();
	
public:
	TelescopeView(TelescopeCtrl *telescopectrl, Log &log, FoamControl &foamctrl, string n);
	~TelescopeView();
};


#endif // HAVE_TELESCOPEVIEW_H

/*!
 \page dev_telescope_ui Telescope UI : TelescopeView & TelescopeCtrl

 \section telescopeview_telescopeview TelescopeView
 
 Shows a basic GUI for a generic telescope. See TelescopeView
 
 \section telescopeview_telescopectrl TelescopeCtrl
 
 Controls a generic telescope. See TelescopeCtrl.
 
 \section telescopeview_derived Derived classes
 
 The following classes are derived from this class:
 - none
 
 \section telescopeview_related Related classes
 
 The following classes are closely related to this class:
 - none
 
 
 */
