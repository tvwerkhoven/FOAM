/*
 camview.h -- camera control class
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkglmm.h>

#include "widgets.h"

#include "deviceview.h"
#include "camctrl.h"
#include "glviewer.h"

using namespace Gtk;
using namespace std;

/*!
 @brief Generic camera viewing class
 
 This is the GUI element for CamCtrl, it shows basic controls for generic 
 cameras and can display frames.
 */
class CamView: public DevicePage {
	friend class ShwfsView;
protected:
	CamCtrl *camctrl;
	
	// These frames go in the main VBox
	Frame ctrlframe;
	Frame dispframe;
	
	// The Camera image goes in the extra window
	Frame camframe;
	Frame histoframe;	
	
	// Control stuff
	// Need: darkflat, fsel, tiptilt, capture, thumb, ...?
	HBox ctrlhbox;
	SwitchButton capture;								//!< Start/stop capturing frames, CamView::on_capture_clicked()
	SwitchButton display;								//!< Start/stop displaying frames, CamView::on_display_clicked(). OK: frame just updated. WAITING: frame requested. CLEAR: don't display. ERROR: something went wrong, stop display.
	SwitchButton store;									//!< Start/stop storing frames on the camera, CamView::on_store_clicked().  CLEAR: not storing. WAITING: store in progress. ERROR: something went wrong, abort. OK: unused
	Entry store_n;											//!< How many frames to store when clicking CamView:store
	VSeparator ctrl_vsep;
	LabeledEntry e_exposure;						//!< For exposure time, RW
	LabeledEntry e_offset;							//!< For offset, RW
	LabeledEntry e_interval;						//!< For interval, RW
	LabeledEntry e_gain;								//!< For gain, RW
	LabeledEntry e_res;									//!< For resolution, RO
	LabeledEntry e_mode;								//!< For mode, RO
	
	// Display stuff
	HBox disphbox;
	CheckButton flipv;									//!< Flip image vertically (only GUI)
	CheckButton fliph;									//!< Flip image horizontally (only GUI)
	CheckButton crosshair;							//!< Show crosshair
	CheckButton grid;										//!< Show grid
	CheckButton histo;									//!< Show histogram
	CheckButton underover;							//!< Show under-/overexposed areas as red and green
	VSeparator vsep1;
	Button zoomin;											//!< Zoom in, CamView::on_zoomin_activate()
	Button zoomout;											//!< Zoom out, CamView::on_zoomout_activate()
	Button zoom100;											//!< Zoom to original size, CamView::on_zoom100_activate()
	ToggleButton zoomfit;								//!< Zoom to fit to window
		
	// Camera image
	HBox camhbox;
	OpenGLImageViewer glarea;
	
	// Histogram GUI stuff
	HBox histohbox;
	Alignment histoalign;
	EventBox histoevents;
	Image histoimage;
	Glib::RefPtr<Gdk::Pixbuf> histopixbuf;

	VBox histovbox;
	LabeledSpinEntry minval;						//!<
	LabeledSpinEntry maxval;						//!<
	HBox histohbox2;
	LabeledEntry e_avg;									//!< Shows avg value
	LabeledEntry e_rms;									//!< Shows rms/sigma
	HBox histohbox3;
	LabeledEntry e_datamin;							//!< Shows data minimum
	LabeledEntry e_datamax;							//!< Shows data maximum

	uint32_t *histo_img;								//!< Local histogram copy for GUI
	
	bool waitforupdate;
	time_t lastupdate;
	float dx;
	float dy;
	int s;
	
	// User interaction
	void on_zoom100_activate();					//!< Zoom to original size
	void on_zoomin_activate();					//!< Zoom in
	void on_zoomout_activate();					//!< Zoom out
	void on_capture_clicked();					//!< (De-)activate camera when user presses CamView::capture button.
	void on_display_clicked();					//!< (De-)activate camera frame grabbing when user presses CamView::display button.
	void on_store_clicked();						//!< Called when user clicks CamView::store or activates CamView::store_n
	void on_info_change();							//!< Propagate user changed settings in GUI to camera
	
	void on_histo_toggled();						//!< Toggle histogram frame display
	void on_underover_toggled();				//!< Toggle under-/overexposure indication
	bool on_histo_clicked(GdkEventButton *);
	void on_minmax_change();						//!< Set new min/max values for display clipping
	
	//!< @todo Sort these functions out
	void on_image_realize();
	void on_image_expose_event(GdkEventExpose *event);
	void on_image_configure_event(GdkEventConfigure *event);
	bool on_image_scroll_event(GdkEventScroll *event);
	bool on_image_motion_event(GdkEventMotion *event);
	bool on_image_button_event(GdkEventButton *event);
	
	// GUI updates
	void do_full_update();							//!< Update full glimage image & histo etc (corresponds to OpenGLImageViewer::do_full_update())
	void do_update();										//!< Update glarea with new view settings (crosshair, flip, etc.) (corresponds to OpenGLImageViewer::do_update())
	void do_histo_update();							//!< Update histogram (

	void on_glarea_view_update();				//!< Callback for OpenGLImageViewer update on viewstate (zoom (scale), shift, etc.)

	// From DeviceView:
	virtual void disable_gui();
	virtual void enable_gui();
	virtual void clear_gui();

	virtual void on_message_update();

	// New events
	virtual void on_monitor_update();		//!< New camera image available, display it
	bool on_timeout();									//!< Timeout function, do regular updates (camera image etc.)
	
	int histo_scale_func(int i);				//!< Histogram scaling function (set through histo_scale_f). Must scale input from 0 to 100 onto 0 to 100

public:
	enum _histo_scale_f {
		LINEAR=0,
		SQRT,
		LOG10,
		LOG20,
		LOG100,
		LOG2,
	};
	enum _histo_scale_f histo_scale_f;
	
	CamView(CamCtrl *camctrl, Log &log, FoamControl &foamctrl, string n);
	~CamView();
	
//	virtual void init();
};


#endif // HAVE_CAMVIEW_H


/*!
 \page dev_cam_ui Camera devices UI: CamView & CamCtrl
 
 \section camview_camview CamView
 
 Shows a basic GUI for a generic camera. See CamView
 
 \section camview_camctrl CamCtrl
 
 Controls a generic camera. See CamCtrl.
 
 \section camview_derived Derived classes
 
 The following classes are derived from the Camera device:
 - SimCamView and SimCamCtrl
 
 */
