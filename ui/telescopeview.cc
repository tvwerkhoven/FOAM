/*
 telescopeview.cc -- Telescope GUI
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

#include "utils.h"

#include "deviceview.h"
#include "devicectrl.h"
#include "telescopectrl.h"
#include "telescopeview.h"

using namespace std;
using namespace Gtk;

TelescopeView::TelescopeView(TelescopeCtrl *telescopectrl, Log &log, FoamControl &foamctrl, string n): 
DevicePage((DeviceCtrl *) telescopectrl, log, foamctrl, n), 
telescopectrl(telescopectrl),
track_frame("Telescope tracking"), 
tel_pos("Tel pos."), tel_units(""),
tt_raw("Raw"), tt_conv("Conv."), tt_ctrl("Ctrl."),
b_refresh(Gtk::Stock::REFRESH), b_autoupd("Auto"), e_autointval("", "s"),
ctrl_frame("Track control"),
ccd_ang_e("CCD rot.","˚"), scalefac0_e("Scalefac"), scalefac1_e(""), ttgain_e("TT Gain", "(P)")
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	// Init lastupd to 0 ('never')
	lastupd.tv_sec = 0;
	lastupd.tv_usec = 0;

	tel_pos.set_width_chars(8);
	tel_pos.set_editable(false);
	tt_raw.set_width_chars(12);
	tt_raw.set_editable(false);
	tt_conv.set_width_chars(12);
	tt_conv.set_editable(false);
	tt_ctrl.set_width_chars(12);
	tt_ctrl.set_editable(false);
	
	e_autointval.set_digits(2);
	e_autointval.set_value(1.0);
	e_autointval.set_increments(0.1, 1);
	e_autointval.set_range(0, 10.0);
	
	ccd_ang_e.set_width_chars(4);
	scalefac0_e.set_width_chars(6);
	scalefac1_e.set_width_chars(6);
	ttgain_e.set_width_chars(4);
	
	// Pack boxes for track_frame
	track_hbox.pack_start(tel_pos, PACK_SHRINK);
	track_hbox.pack_start(tel_units, PACK_SHRINK);

	track_hbox.pack_start(vsep0, PACK_SHRINK);

	track_hbox.pack_start(tt_raw, PACK_SHRINK);
	track_hbox.pack_start(tt_conv, PACK_SHRINK);
	track_hbox.pack_start(tt_ctrl, PACK_SHRINK);

	track_hbox.pack_start(vsep1, PACK_SHRINK);
	
	track_hbox.pack_start(b_refresh, PACK_SHRINK);
	track_hbox.pack_start(b_autoupd, PACK_SHRINK);
	track_hbox.pack_start(e_autointval, PACK_SHRINK);

	track_frame.add(track_hbox);

	// Pack boxes for ctrl_frame
	ctrl_hbox.pack_start(ccd_ang_e, PACK_SHRINK);
	ctrl_hbox.pack_start(scalefac0_e, PACK_SHRINK);
	ctrl_hbox.pack_start(scalefac1_e, PACK_SHRINK);
	ctrl_hbox.pack_start(ttgain_e, PACK_SHRINK);

	ctrl_frame.add(ctrl_hbox);

	// Add to main GUI page
	pack_start(track_frame, PACK_SHRINK);
	pack_start(ctrl_frame, PACK_SHRINK);

	// Connect events
	b_refresh.signal_clicked().connect(sigc::mem_fun(*this, &TelescopeView::do_teltrack_update));
	b_autoupd.signal_clicked().connect(sigc::mem_fun(*this, &TelescopeView::on_autoupd_clicked));

	ccd_ang_e.entry.signal_activate().connect(sigc::mem_fun(*this, &TelescopeView::on_info_change));
	scalefac0_e.entry.signal_activate().connect(sigc::mem_fun(*this, &TelescopeView::on_info_change));
	scalefac1_e.entry.signal_activate().connect(sigc::mem_fun(*this, &TelescopeView::on_info_change));
	ttgain_e.entry.signal_activate().connect(sigc::mem_fun(*this, &TelescopeView::on_info_change));

	telescopectrl->signal_message.connect(sigc::mem_fun(*this, &TelescopeView::on_message_update));
	refresh_timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &TelescopeView::on_timeout), 1000.0/30.0);

	clear_gui();
	disable_gui();
	
	// finalize
	show_all_children();
}

TelescopeView::~TelescopeView() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	refresh_timer.disconnect();
}

void TelescopeView::enable_gui() {
	DevicePage::enable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
}

void TelescopeView::disable_gui() {
	DevicePage::disable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void TelescopeView::clear_gui() {
	DevicePage::clear_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	tel_pos.set_text("N/A");
	tt_raw.set_text("N/A");
	tt_conv.set_text("N/A");
	tt_ctrl.set_text("N/A");
}

void TelescopeView::do_teltrack_update() const { 
	telescopectrl->send_cmd("get tel_track");
	telescopectrl->send_cmd("get tel_units");
	telescopectrl->send_cmd("get shifts");
}

bool TelescopeView::on_timeout() {
	// This function fires 30 times/s, if we do not want that many updates we can throttle it with e_autointval
	struct timeval now;
	gettimeofday(&now, NULL);
	// If b_autoupd is 'clear' we don't want new data, if 'waiting' we're 
	// expecting new data soon, otherwise if button is 'OK': get new data
	double timediff = ((now.tv_sec - lastupd.tv_sec) + (now.tv_usec - lastupd.tv_usec)/1000000.);
	if (b_autoupd.get_state() == SwitchButton::OK && 
			timediff > e_autointval.get_value() ) {
		b_autoupd.set_state(SwitchButton::WAITING);
		log.term(format("%s: do_teltrack_update", __PRETTY_FUNCTION__));
		do_teltrack_update();
		gettimeofday(&lastupd, NULL);
	}
	return true;
}

void TelescopeView::on_autoupd_clicked() {
	// Click the 'b_autoupd' button: 
	// -> if CLEAR: set to OK and run on_timeout()
	// -> if WAITING, OK or ERROR: set to CLEAR and stop everything
	if (b_autoupd.get_state() == SwitchButton::CLEAR) {
		b_autoupd.set_state(SwitchButton::OK);
		log.term(format("%s: on_timeout start", __PRETTY_FUNCTION__));
		on_timeout();
		log.term(format("%s: on_timeout done", __PRETTY_FUNCTION__));
	}
	else {
		b_autoupd.set_state(SwitchButton::CLEAR);
		log.term(format("%s: clear", __PRETTY_FUNCTION__));
	}
}

void TelescopeView::on_info_change() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	telescopectrl->set_ccd_ang( ccd_ang_e.get_value() );
	telescopectrl->set_scalefac( scalefac0_e.get_value(), scalefac1_e.get_value() );
	telescopectrl->set_ttgain( ttgain_e.get_value() );
}


void TelescopeView::on_message_update() {
	DevicePage::on_message_update();
	log.term(format("%s", __PRETTY_FUNCTION__));

	tel_pos.set_text(telescopectrl->get_tel_track_s());
	tel_units.set_text(telescopectrl->get_tel_units_s());
	tt_raw.set_text(telescopectrl->get_tt_raw_s());
	tt_conv.set_text(telescopectrl->get_tt_conv_s());
	tt_ctrl.set_text(telescopectrl->get_tt_ctrl_s());

	ccd_ang_e.set_text( format("%g", telescopectrl->get_ccd_ang()) );
	scalefac0_e.set_text( format("%g", telescopectrl->get_scalefac0()) );
	scalefac1_e.set_text( format("%g", telescopectrl->get_scalefac1()) );
	ttgain_e.set_text( format("%g", telescopectrl->get_ttgain()) );

	// If we were waiting for this update, set button to 'OK' state again. If 
	// this is only a one-time 'b_refresh' event, this probably won't happen.
	if (b_autoupd.get_state() == SwitchButton::WAITING)
		b_autoupd.set_state(SwitchButton::OK);

}
