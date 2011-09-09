/*
 wfsview.cc -- wavefront sensor viewer class
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

#include "utils.h"

#include "deviceview.h"
#include "devicectrl.h"
#include "wfsctrl.h"
#include "wfsview.h"
#include "camview.h"

using namespace std;
using namespace Gtk;

WfsView::WfsView(WfsCtrl *wfsctrl, Log &log, FoamControl &foamctrl, string n): 
DevicePage((DeviceCtrl *) wfsctrl, log, foamctrl, n), wfsctrl(wfsctrl),
wf_cam("Cam"), 
wfpow_frame("Wavefront info"), 
wfpow_gr(480,100), wfpow_mode("Basis"), 
wfscam_ui(NULL)
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	wfpow_mode.set_width_chars(12);
	wfpow_mode.set_editable(false);
	
	wf_cam.set_width_chars(16);
	wf_cam.set_editable(false);
	
	clear_gui();
	disable_gui();
	
	// Extra device info
	devhbox.pack_start(vsep0, PACK_SHRINK);
	devhbox.pack_start(wf_cam, PACK_SHRINK);
		
	// Wavefront power 'spectrum' (separate window)
	wfpow_hbox.pack_start(wfpow_mode, PACK_SHRINK);
	wfpow_hbox.pack_start(wfpow_gr);
	wfpow_frame.add(wfpow_hbox);
	
	// Extra window
	extra_win.set_title("FOAM WFS " + devname);

	extra_win.set_default_size(640, 140);

	extra_vbox.pack_start(wfpow_frame, PACK_SHRINK);
	extra_win.add(extra_vbox);
	
	extra_win.show_all_children();
	extra_win.present();
	
	// wfpow_gr needs to know about the function to get updated values
	wfpow_gr.slot_update = sigc::mem_fun(*this, &WfsView::do_wfpow_update);
	
	// Event handlers
	
	wfsctrl->signal_message.connect(sigc::mem_fun(*this, &WfsView::on_message_update));
	wfsctrl->signal_wavefront.connect(sigc::mem_fun(*this, &WfsView::on_wfpow_update));
	wfsctrl->signal_wfscam.connect(sigc::mem_fun(*this, &WfsView::on_cam_update));
	
	// finalize
	show_all_children();
}

WfsView::~WfsView() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void WfsView::enable_gui() {
	DevicePage::enable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));

}

void WfsView::disable_gui() {
	DevicePage::disable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
}

void WfsView::clear_gui() {
	DevicePage::clear_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	wf_cam.set_text("N/A");
	
	wfpow_mode.set_text("N/A");
}

void WfsView::on_wfpow_update() {
	// Return if nothing to be drawn
	if (!wfpow_frame.is_visible())
		return;
	
	wfpow_gr.on_update(wfsctrl->get_modes_vec());
}

void WfsView::on_message_update() {
	DevicePage::on_message_update();
	
	// Set wavefront basis text
	wfpow_mode.set_text(wfsctrl->get_basis());
	
	if (wfscam_ui)
		wf_cam.set_text(wfsctrl->wfscam);
}

void WfsView::on_cam_update() {
	// New camera for this WFS, get from foamctrl and store in this class.
	FoamControl::device_t *dev_wfscam = foamctrl.get_device(wfsctrl->wfscam);
	wfscam_ui = (CamView *) dev_wfscam->page;
	
	// update gui with new information
	on_message_update();
	on_wfpow_update();
}


