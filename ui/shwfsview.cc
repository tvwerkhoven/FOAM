/*
 shwfsview.cc -- shack-hartmann wavefront sensor viewer class
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#include "types.h"

#include "wfsview.h"
#include "shwfsctrl.h"
#include "shwfsview.h"

using namespace std;
using namespace Gtk;

ShwfsView::ShwfsView(ShwfsCtrl *ctrl, Log &log, FoamControl &foamctrl, string n): 
WfsView((WfsCtrl *) ctrl, log, foamctrl, n), shwfsctrl(ctrl),
shwfs_addnew("Add new"),
subi_frame("Subimages"),
subi_lx("X_0"), subi_ly("Y_0"), subi_tx("X_1"), subi_ty("Y_1"), 
subi_update("Update"), subi_del("Del"), subi_add("Add"), subi_clear("Clear"), subi_regen("Regen pattern"), subi_find("Find pattern"), subi_find_minif("Min I", "fac"),
subi_bounds("Show subaps"), subi_vecs("Show shifts"), subi_vecdelayi("every", "s")
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	// Set last update to 'never'
	subi_last.tv_sec = 0;
	subi_last.tv_usec = 0;

	// Widget properties
	subi_lx.set_digits(0);
	subi_lx.set_increments(1, 10);
	subi_ly.set_digits(0);
	subi_ly.set_increments(1, 10);
	subi_tx.set_digits(0);
	subi_tx.set_increments(1, 10);
	subi_ty.set_digits(0);
	subi_ty.set_increments(1, 10);
	
	subi_vecdelayi.set_digits(2);
	subi_vecdelayi.set_range(0.0, 5.0);
	subi_vecdelayi.set_increments(0.1, 1);
	
	subi_find_minif.set_digits(2);
	subi_find_minif.set_range(0.0, 1.0);
	subi_find_minif.set_increments(0.05, 0.01);
	
	// Signals & callbacks
	subi_select.signal_changed().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_select_changed));
	subi_add.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_add_clicked));
	subi_clear.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_clear_clicked));
	subi_del.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_del_clicked));
	subi_update.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_update_clicked));
	
	subi_regen.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_regen_clicked));
	subi_find.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_find_clicked));
	
	subi_vecs.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_vecs_clicked));
	subi_bounds.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_bounds_clicked));
	//subi_vecdelayi.entry.signal_activate().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_vecs_clicked));
	
	shwfsctrl->signal_sh_shifts.connect(sigc::mem_fun(*this, &ShwfsView::do_sh_shifts_update));
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &ShwfsView::on_timeout), 1000.0/30.0);
	
	// Add widgets
	// Subimage/ MLA pattern controls
	subi_hbox1.set_spacing(4);
	
	subi_hbox111.pack_start(subi_add, PACK_SHRINK);
	subi_hbox111.pack_start(subi_clear, PACK_SHRINK);
	subi_hbox111.pack_start(subi_del, PACK_SHRINK);
	subi_hbox111.pack_start(subi_update, PACK_SHRINK);
	subi_vbox11.pack_start(subi_select, PACK_SHRINK);
	subi_vbox11.pack_start(subi_hbox111, PACK_SHRINK);
	
	subi_hbox121.pack_start(subi_lx, PACK_SHRINK);
	subi_hbox121.pack_start(subi_ly, PACK_SHRINK);
	subi_hbox122.pack_start(subi_tx, PACK_SHRINK);
	subi_hbox122.pack_start(subi_ty, PACK_SHRINK);
	subi_vbox12.pack_start(subi_hbox121 ,PACK_SHRINK);
	subi_vbox12.pack_start(subi_hbox122 ,PACK_SHRINK);
	
	subi_vbox13.pack_start(subi_regen, PACK_SHRINK);
	subi_find_box.pack_start(subi_find, PACK_SHRINK);
	subi_find_box.pack_start(subi_find_minif, PACK_SHRINK);
	subi_vbox13.pack_start(subi_find_box, PACK_SHRINK);
	
	subi_hbox141.pack_start(subi_bounds, PACK_SHRINK);
	subi_hbox141.pack_start(subi_vecs, PACK_SHRINK);
	subi_hbox141.pack_start(subi_vecdelayi, PACK_SHRINK);
	subi_vbox14.pack_start(subi_hbox141, PACK_SHRINK);
	
	subi_hbox1.pack_start(subi_vbox11, PACK_SHRINK);
	subi_hbox1.pack_start(vsep1, PACK_SHRINK);
	subi_hbox1.pack_start(subi_vbox12, PACK_SHRINK);
	subi_hbox1.pack_start(vsep2, PACK_SHRINK);
	subi_hbox1.pack_start(subi_vbox13, PACK_SHRINK);
	subi_hbox1.pack_start(vsep3, PACK_SHRINK);
	subi_hbox1.pack_start(subi_vbox14, PACK_SHRINK);

	subi_frame.add(subi_hbox1);
	
	pack_start(subi_frame, PACK_SHRINK);
	
	clear_gui();
	disable_gui();
	
	// Show bounds by default
	on_subi_bounds_clicked();
	
	// finalize
	show_all_children();
}

ShwfsView::~ShwfsView() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void ShwfsView::enable_gui() {
	WfsView::enable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	subi_select.set_sensitive(true);
	subi_update.set_sensitive(true);
	subi_del.set_sensitive(true);
	subi_add.set_sensitive(true);
	subi_clear.set_sensitive(true);
	subi_regen.set_sensitive(true);
	subi_find.set_sensitive(true);
	subi_bounds.set_sensitive(true);
	subi_vecs.set_sensitive(true);
}

void ShwfsView::disable_gui() {
	WfsView::disable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	subi_select.set_sensitive(false);
	subi_update.set_sensitive(false);
	subi_del.set_sensitive(false);
	subi_add.set_sensitive(false);
	subi_clear.set_sensitive(false);
	subi_regen.set_sensitive(false);
	subi_find.set_sensitive(false);
	subi_bounds.set_sensitive(false);
	subi_vecs.set_sensitive(false);	
}

void ShwfsView::clear_gui() {
	WfsView::clear_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	subi_select.clear_items();
	subi_select.append_text("-");
	subi_lx.set_value(0);
	subi_ly.set_value(0);
	subi_tx.set_value(0);
	subi_ty.set_value(0);
	subi_vecdelayi.set_value(1.0);
	subi_find_minif.set_value(0.6);
		
	if (wfscam_ui) {
		wfscam_ui->glarea.clearboxes();
		wfscam_ui->glarea.clearlines();
	}
	
	subi_bounds.set_state(SwitchButton::CLEAR);
	subi_vecs.set_state(SwitchButton::CLEAR);
}

void ShwfsView::on_subi_select_changed() {
	// Get current selected item, convert to int
	string tmp = subi_select.get_active_text();
	int curr_si = strtol(tmp.c_str(), NULL, 10);
	
	// Check if selected sub image is within bounds. This also filters out 'Add New'
	if (curr_si < 0 || curr_si >= (int) shwfsctrl->get_mla_nsi())
		return;
	
	// Get coordinates for that subimage
	fvector_t tmp_si = shwfsctrl->get_mla_si((size_t) curr_si);
	
	subi_lx.set_value(tmp_si.lx);
	subi_ly.set_value(tmp_si.ly);
	subi_tx.set_value(tmp_si.tx);
	subi_ty.set_value(tmp_si.ty);
}

void ShwfsView::on_subi_clear_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));

	shwfsctrl->mla_clear();
}

void ShwfsView::on_subi_add_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	string tmp = subi_select.get_active_text();
	if (tmp != shwfs_addnew) {
        log.term(format("%s Select '%s' first to add new subimgs", __PRETTY_FUNCTION__, shwfs_addnew.c_str()));

		return;
	}

	// Get text from GUI, convert to floats
	// Read as floats to process potential decimal signals, then cast to int. 
	// Reading as ints directly might break on decimals.
	int new_lx = subi_lx.get_value_as_int();
	int new_ly = subi_ly.get_value_as_int();
	int new_tx = subi_tx.get_value_as_int();
	int new_ty = subi_ty.get_value_as_int();
	
	// Call shwfsctrl to add this subimage to the MLA grid
	shwfsctrl->mla_add_si(new_lx, new_ly, new_tx, new_ty);
}

void ShwfsView::on_subi_del_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	// Get current selected item, convert to int
	string tmp = subi_select.get_active_text();
	int curr_si = strtol(tmp.c_str(), NULL, 10);
	
	// Check if selected sub image is within bounds. This also filters out 'Add New'
	if (curr_si < 0 || curr_si >= (int) shwfsctrl->get_mla_nsi())
		return;
	
	// Call shwfsctrl to remove this subimage
	shwfsctrl->mla_del_si(curr_si);
}

void ShwfsView::on_subi_update_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	string tmp;
	
	// Get text from GUI, convert to floats
	// Read as floats to process potential decimal signals, then cast to int. 
	// Reading as ints directly might break on decimals.
	int new_lx = subi_lx.get_value_as_int();
	int new_ly = subi_ly.get_value_as_int();
	int new_tx = subi_tx.get_value_as_int();
	int new_ty = subi_ty.get_value_as_int();
	
	// Get current selected item, convert to int
	tmp = subi_select.get_active_text();
	int curr_si = strtol(tmp.c_str(), NULL, 10);
	
	// Check if selected sub image is within bounds. This also filters out 'Add New'
	if (curr_si < 0 || curr_si >= (int) shwfsctrl->get_mla_nsi())
		return;
	
	// Add new subimage, delete old one
	shwfsctrl->mla_update_si(curr_si, new_lx, new_ly, new_tx, new_ty);
}

void ShwfsView::on_subi_regen_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	// Re-generate subimage pattern
	shwfsctrl->mla_regen_pattern();
}

void ShwfsView::on_subi_find_clicked() {
	//! @todo Implement mla find with parameters: mla find [simini_f] [sisize] [nmax] [iter]
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	// Find subimage pattern heuristically
	shwfsctrl->mla_find_pattern(subi_find_minif.get_value());
}

void ShwfsView::on_subi_bounds_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	// If the button is clear: show bounds, otherwise: set button to clear (and hide bounds)
	if (subi_bounds.get_state() == SwitchButton::CLEAR) {
		wfscam_ui->glarea.showboxes(true);
		subi_vecs.set_state(SwitchButton::OK);
	} else {
		wfscam_ui->glarea.showboxes(false);
		subi_vecs.set_state(SwitchButton::CLEAR);
	}
}

void ShwfsView::on_subi_vecs_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	// If the button is clear: start acquisition, otherwise: set button to clear (and stop acquisition)
	if (subi_vecs.get_state() == SwitchButton::CLEAR) {
		shwfsctrl->cmd_get_shifts();
		subi_vecs.set_state(SwitchButton::WAITING);
	}
	else {
		// Clear current lines from the GUI
		if (wfscam_ui)
			wfscam_ui->glarea.clearlines();
		subi_vecs.set_state(SwitchButton::CLEAR);
	}
}

bool ShwfsView::on_timeout() {
	// This function fires 30 times/s, if we do not want that many updates we can throttle it with subi_vecdelay
	struct timeval now;
	gettimeofday(&now, NULL);
	// If button is 'clear' we don't want shifts, if 'waiting' we're expecting new shifts soon, otherwise if button is 'OK': get new shifts
	if (subi_vecs.get_state() == SwitchButton::OK && 
			((now.tv_sec - subi_last.tv_sec) + (now.tv_usec - subi_last.tv_usec)/1000000.) > subi_vecdelayi.get_value()) {
		subi_vecs.set_state(SwitchButton::WAITING);
		shwfsctrl->cmd_get_shifts();
		gettimeofday(&subi_last, NULL);
	}
	return true;
}

void ShwfsView::do_sh_shifts_update() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	subi_vecs.set_state(SwitchButton::OK);
		
	// Add image shift vectors to glarea
	if (wfscam_ui) {
		wfscam_ui->glarea.clearlines();
		
		for (size_t i=0; i<shwfsctrl->get_nrefshifts(); i++) {
			fvector_t refline = shwfsctrl->get_refshift((size_t) i);
			// Add (0.5, 0.5) to the *end* of the vector because we want to end up in the middle of the pixel, not at the origin
			refline.tx+=0.5;
			refline.ty+=0.5;
			wfscam_ui->glarea.addline(refline);
		}

		for (size_t i=0; i<shwfsctrl->get_nshifts(); i++) {
			fvector_t shline = shwfsctrl->get_shift((size_t) i);
			// Add (0.5, 0.5) to both ends of the vector because we want to end up in the middle of the pixel, not at the origin
			shline.lx+=0.5;
			shline.ly+=0.5;
			shline.tx+=0.5;
			shline.ty+=0.5;
			wfscam_ui->glarea.addline(shline);
		}
		
		wfscam_ui->glarea.do_update();
	}
}

void ShwfsView::on_message_update() {
	WfsView::on_message_update();
	log.term(format("%s", __PRETTY_FUNCTION__));

	// Add list of subimages to dropdown box
	subi_select.clear_items();
	for (size_t i=0; i<shwfsctrl->get_mla_nsi(); i++)
		subi_select.append_text(format("%d", (int) i));
	
	// Add text to add a new subimage
	subi_select.append_text(shwfs_addnew);
	
	// Add number of subaps
	
	// Add subimage boxes & wavefront vectors to glarea
	if (wfscam_ui) {
		wfscam_ui->glarea.clearboxes();
		
		for (size_t i=0; i<shwfsctrl->get_mla_nsi(); i++)
			wfscam_ui->glarea.addbox(shwfsctrl->get_mla_si((size_t) i));
	
		wfscam_ui->glarea.do_update();
	}
	// If we have wfscam_ctrl, set subimage coordinate ranges to camera resolution
	if (wfscam_ctrl) {
		subi_lx.set_range(0, wfscam_ctrl->get_width());
		subi_ly.set_range(0, wfscam_ctrl->get_height());
		subi_tx.set_range(0, wfscam_ctrl->get_width());
		subi_ty.set_range(0, wfscam_ctrl->get_height());
	}
}
