/*
 camview.h -- camera control class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 Copyright (C) 2010 Guus Sliepen
 
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

#ifndef HAVE_CAMVIEW_H
#define HAVE_CAMVIEW_H

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkglmm.h>

#include "widgets.h"

#include "deviceview.h"
#include "camctrl.h"
#include "glviewer.h"

/*!
 @brief Camera viewing class  
 @todo Document this
 */
class CamView: public DevicePage {
protected:
	Gtk::Frame infoframe;
	Gtk::Frame dispframe;
	Gtk::Frame ctrlframe;
	Gtk::Frame camframe;
	Gtk::Frame histoframe;
	
	// Info stuff
	HBox infohbox;
	LabeledEntry e_exposure;		//!< For exposure time, RW
	LabeledEntry e_offset;			//!< For offset, RW
	LabeledEntry e_interval;		//!< For interval, RW
	LabeledEntry e_gain;				//!< For gain, RW
	LabeledEntry e_res;					//!< For resolution, RO
	LabeledEntry e_mode;				//!< For mode, RO
	LabeledEntry e_stat;				//!< For status, RO
	
	// display stuff
	// Need: flipv, fliph, zoom in out 100, crosshair
	//! @todo contrast, underover, colorsel, histogram
	HBox disphbox;
	CheckButton flipv;
	CheckButton fliph;
	CheckButton crosshair;
	CheckButton grid;
	VSeparator vsep1;
	Button zoomin;
	Button zoomout;
	Button zoom100;
	ToggleButton zoomfit;

	// control stuff
	// Need: darkflat, fsel, tiptilt, capture, thumb, ...?
	HBox ctrlhbox;
	//Button refresh;
	SwitchButton capture;
	SwitchButton display;
	SwitchButton store;
	Entry store_n;
		
	// Camera image
	HBox camhbox;
	OpenGLImageViewer glarea;
	
	// Histogram stuff
	HBox histohbox;
//	Gtk::VBox histogramvbox;
//	Gtk::Alignment histogramalign;
//	Gtk::EventBox histogramevents;
//	Gtk::Image histogramimage;
//	Glib::RefPtr<Gdk::Pixbuf> histogrampixbuf;
//	LabeledSpinEntry scale;
//	LabeledSpinEntry minval;
//	LabeledSpinEntry maxval;
	LabeledEntry mean;
	LabeledEntry stddev;

	bool waitforupdate;
	time_t lastupdate;
	float dx;
	float dy;
	int s;
	//	float sx;
	//	float sy;
	//	uint32_t *histo;
	//	int depth;
	//	float sxstart;
	//	float systart;
	//	gdouble xstart;
	//	gdouble ystart;
	
	// User interaction
	void on_zoom100_activate();
	void on_zoomin_activate();
	void on_zoomout_activate();
	void on_capture_clicked();			//!< Called when capture button is toggled
	void on_display_clicked();			//!< Called when display button is toggled
	void on_store_clicked();				//!< Called when store button is toggled
	void on_info_change();

	void on_image_realize();
	void on_image_expose_event(GdkEventExpose *event);
	void on_image_configure_event(GdkEventConfigure *event);
	bool on_image_scroll_event(GdkEventScroll *event);
	bool on_image_motion_event(GdkEventMotion *event);
	bool on_image_button_event(GdkEventButton *event);
	
	// GUI updates
	void on_glarea_view_update();		//!< Callback from glarea class
//	void on_colorsel_activate();
//	void on_fullscreen_toggled();
	void force_update();
//	void do_histo_update();
	void do_update();
//	void on_close_activate();

	// Overload from DeviceView:
	virtual void on_message_update();
	virtual void on_connect_update();
	
	virtual void disable_gui();
	virtual void enable_gui();
	virtual void clear_gui();
	
	// New event capture
	virtual void on_monitor_update();
	bool on_timeout();

public:
	CamCtrl *camctrl;
	CamView(Log &log, FoamControl &foamctrl, string n, bool is_parent=false);
	~CamView();
	
	virtual void init();
};


#endif // HAVE_CAMVIEW_H


/*!
 \page dev_cam Camera devices : CamView & CamCtrl
 
 \section camview_camview CamView
 
 \section camview_camctrl CamCtrl
 
 \section camview_derived Derived classes
 
 The following classes are dervied from the Camera device:
 - \subpage dev_cam_wfs "Wavefront sensor devices"
 
 */
