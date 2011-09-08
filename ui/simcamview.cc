/*
 simcamview.cc -- simulation camera control class
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <stdexcept>
#include <cstring>
#include <stdlib.h>
#include <math.h>

#include "log.h"
#include "utils.h"
#include "simcamctrl.h"
#include "glviewer.h"
#include "simcamview.h"

using namespace std;
using namespace Gtk;

SimCamView::SimCamView(SimCamCtrl *thisctrl, Log &log, FoamControl &foamctrl, string n): 
CamView((CamCtrl *) thisctrl, log, foamctrl, n),
simcamctrl(thisctrl),
simframe("Simulation params"),
sim_seeing("Seeing"), sim_wfcerr("WFC error"), sim_wfc("WFC corr."), sim_tel("Telescope"), sim_wfs("WFS/MLA"),
e_noisefac("Noise fraction"), e_noiseamp("amplitude"), e_seeingfac("Seeing factor")
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	// Noise simulation control
	e_noisefac.set_digits(3);
	e_noisefac.set_increments(0.1, 0.5);
	e_noisefac.set_range(0, 1.0);

	e_noiseamp.set_digits(3);
	e_noiseamp.set_increments(0.1, 0.5);
	e_noiseamp.set_range(0, 5.0);
	
	e_seeingfac.set_digits(2);
	e_seeingfac.set_increments(0.1, 0.5);
	e_seeingfac.set_range(0, 10.0);
	
	// Simulation controls
	simhbox.pack_start(sim_seeing, PACK_SHRINK);
	simhbox.pack_start(sim_wfcerr, PACK_SHRINK);
	simhbox.pack_start(sim_wfc, PACK_SHRINK);
	simhbox.pack_start(sim_tel, PACK_SHRINK);
	simhbox.pack_start(sim_wfs, PACK_SHRINK);

	simhbox.pack_start(sim_vsep1, PACK_SHRINK);
	simhbox.pack_start(e_noisefac, PACK_SHRINK);
	simhbox.pack_start(e_noiseamp, PACK_SHRINK);
	simhbox.pack_start(e_seeingfac, PACK_SHRINK);

	simframe.add(simhbox);
	pack_start(simframe, PACK_SHRINK);
	
	// Callbacks
	sim_seeing.signal_clicked().connect(sigc::mem_fun(*this, &SimCamView::on_sim_seeing_clicked));
	sim_wfcerr.signal_clicked().connect(sigc::mem_fun(*this, &SimCamView::on_sim_wfcerr_clicked));
	sim_wfc.signal_clicked().connect(sigc::mem_fun(*this, &SimCamView::on_sim_wfc_clicked));
	sim_tel.signal_clicked().connect(sigc::mem_fun(*this, &SimCamView::on_sim_tel_clicked));
	sim_wfs.signal_clicked().connect(sigc::mem_fun(*this, &SimCamView::on_sim_wfs_clicked));

	e_noisefac.entry.signal_activate().connect(sigc::mem_fun(*this, &SimCamView::on_noise_activate));
	e_noiseamp.entry.signal_activate().connect(sigc::mem_fun(*this, &SimCamView::on_noise_activate));
	e_seeingfac.entry.signal_activate().connect(sigc::mem_fun(*this, &SimCamView::on_seeing_activate));
}

SimCamView::~SimCamView() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void SimCamView::enable_gui() {
	CamView::enable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	sim_seeing.set_sensitive(true);
	sim_wfcerr.set_sensitive(true);
	sim_wfc.set_sensitive(true);
	sim_tel.set_sensitive(true);
	sim_wfs.set_sensitive(true);

	e_noisefac.set_sensitive(true);
	e_noiseamp.set_sensitive(true);
}

void SimCamView::disable_gui() {
	CamView::disable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	sim_seeing.set_sensitive(false);
	sim_wfcerr.set_sensitive(false);
	sim_wfc.set_sensitive(false);
	sim_tel.set_sensitive(false);
	sim_wfs.set_sensitive(false);
	
	e_noisefac.set_sensitive(false);
	e_noiseamp.set_sensitive(false);
}

void SimCamView::clear_gui() {
	CamView::clear_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
		
	sim_seeing.set_state(SwitchButton::CLEAR);
	sim_wfcerr.set_state(SwitchButton::CLEAR);
	sim_wfc.set_state(SwitchButton::CLEAR);
	sim_tel.set_state(SwitchButton::CLEAR);
	sim_wfs.set_state(SwitchButton::CLEAR);
	
	e_noisefac.set_value(0);
	e_noiseamp.set_value(0);
}

void SimCamView::on_seeing_activate() {
	log.term(format("%s", __PRETTY_FUNCTION__));

	double seeingfac = e_seeingfac.get_value();
	simcamctrl->set_seeingfac(seeingfac);
}

void SimCamView::on_noise_activate() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	double noisefac = e_noisefac.get_value();
	double noiseamp = e_noiseamp.get_value();
	simcamctrl->set_noisefac(noisefac);
	simcamctrl->set_noiseamp(noiseamp);
}

void SimCamView::on_sim_seeing_clicked() {
	if (sim_seeing.get_state() == SwitchButton::CLEAR)
		simcamctrl->set_simseeing(true);
	else
		simcamctrl->set_simseeing(false);
}

void SimCamView::on_sim_wfcerr_clicked() {
	if (sim_wfcerr.get_state() == SwitchButton::CLEAR)
		simcamctrl->set_simwfcerr(true);
	else
		simcamctrl->set_simwfcerr(false);
}

void SimCamView::on_sim_wfc_clicked() {
	if (sim_wfc.get_state() == SwitchButton::CLEAR)
		simcamctrl->set_simwfc(true);
	else
		simcamctrl->set_simwfc(false);
}

void SimCamView::on_sim_tel_clicked() {
	if (sim_tel.get_state() == SwitchButton::CLEAR)
		simcamctrl->set_simtel(true);
	else
		simcamctrl->set_simtel(false);
}

void SimCamView::on_sim_wfs_clicked() {
	if (sim_wfs.get_state() == SwitchButton::CLEAR)
		simcamctrl->set_simwfs(true);
	else
		simcamctrl->set_simwfs(false);
}

void SimCamView::on_message_update() {
	CamView::on_message_update();

	// Set simulation buttons
	if (simcamctrl->get_simseeing())
		sim_seeing.set_state(SwitchButton::OK);
	else
		sim_seeing.set_state(SwitchButton::CLEAR);
	
	if (simcamctrl->get_simwfc())
		sim_wfc.set_state(SwitchButton::OK);
	else
		sim_wfc.set_state(SwitchButton::CLEAR);

	if (simcamctrl->get_simwfcerr())
		sim_wfcerr.set_state(SwitchButton::OK);
	else
		sim_wfcerr.set_state(SwitchButton::CLEAR);

	if (simcamctrl->get_simtel())
		sim_tel.set_state(SwitchButton::OK);
	else
		sim_tel.set_state(SwitchButton::CLEAR);

	if (simcamctrl->get_simwfs())
		sim_wfs.set_state(SwitchButton::OK);
	else
		sim_wfs.set_state(SwitchButton::CLEAR);

	// Set noise-related values in text entries
	e_noisefac.set_value(simcamctrl->get_noisefac());
	e_noiseamp.set_value(simcamctrl->get_noiseamp());
	
	e_seeingfac.set_value(simcamctrl->get_seeingfac());
}
