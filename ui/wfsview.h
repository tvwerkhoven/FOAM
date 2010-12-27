/*
 wfsview.h -- wavefront sensor control class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

/*!
 @brief Generic wavefront sensor GUI class
 
 This is the GUI element for WfsCtrl, it shows controls for wavefront sensors.
 It mainly shows a graphical representation of the power in each wavefront 
 mode.
 */
class WfsView: public DevicePage {
protected:
	WfsCtrl *wfsctrl;
	
	Frame wfpow_frame;
	HBox wfpow_hbox;

	LabeledEntry wfpow_mode;								//!< Wavefront representation modes used (KL, Zernike, mirror, etc.)
	Alignment wfpow_align;
	EventBox wfpow_events;
	Image wfpow_img;
	Glib::RefPtr<Gdk::Pixbuf> wfpow_pixbuf;
	
	void do_wfspow_update();								//!< Update WF display

	// From DevicePage::
	virtual void enable_gui();
	virtual void disable_gui();
	virtual void clear_gui();
	
public:
	WfsView(WfsCtrl *wfsctrl, Log &log, FoamControl &foamctrl, string n);
	~WfsView();
};


#endif // HAVE_WFSVIEW_H


/*!
 \page dev_wfs Wavefront sensor devices : WfsView & WfsCtrl

 */
