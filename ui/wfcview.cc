/*
 wfcview.cc -- wavefront corrector viewer class
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
#include "wfcctrl.h"
#include "wfcview.h"
#include "camview.h"

using namespace std;
using namespace Gtk;

WfcView::WfcView(WfcCtrl *wfcctrl, Log &log, FoamControl &foamctrl, string n): 
DevicePage((DeviceCtrl *) wfcctrl, log, foamctrl, n), 
wfcctrl(wfcctrl),
wfc_nact("#Act."),
calib_frame("Calibration"),
wfcact_frame("WFC actuators"), wfcact_gr(480,100)
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	wfc_nact.set_width_chars(8);
	wfc_nact.set_editable(false);

	clear_gui();
	disable_gui();
	
	// Extra device info
	devhbox.pack_start(vsep0, PACK_SHRINK);
	devhbox.pack_start(wfc_nact, PACK_SHRINK);
		
	// Wavefront corrector actuator 'spectrum' (separate window)
	wfcact_hbox.pack_start(wfcact_gr);
	wfcact_frame.add(wfcact_hbox);
	
	// Extra window
	extra_win.set_title("FOAM WFC " + devname);
	
	extra_vbox.pack_start(wfcact_frame, PACK_SHRINK);
	extra_win.add(extra_vbox);
	
	extra_win.show_all_children();
	extra_win.present();
	
	// wfcact_gr needs to know about the function to get updated values
	wfcact_gr.slot_update = sigc::mem_fun(*this, &WfcView::do_wfcact_update);
	
	// Event handlers
	wfcctrl->signal_wfcctrl.connect(sigc::mem_fun(*this, &WfcView::on_wfcact_update));
	wfcctrl->signal_message.connect(sigc::mem_fun(*this, &WfcView::on_message_update));
	
	// finalize
	show_all_children();
}

WfcView::~WfcView() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void WfcView::enable_gui() {
	DevicePage::enable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));

}

void WfcView::disable_gui() {
	DevicePage::disable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
}

void WfcView::clear_gui() {
	DevicePage::clear_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void WfcView::on_wfcact_update() {
	// Return if nothing to be drawn
	if (!wfcact_frame.is_visible())
		return;

	wfcact_gr.on_update(wfcctrl->get_ctrlvec());
}

void WfcView::on_message_update() {
	DevicePage::on_message_update();
	
	wfc_nact.set_text(format("%d", wfcctrl->get_nact()));
}
