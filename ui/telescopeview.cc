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
telescopectrl(telescopectrl)
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	clear_gui();
	disable_gui();
	
	// finalize
	show_all_children();
}

TelescopeView::~TelescopeView() {
	log.term(format("%s", __PRETTY_FUNCTION__));
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
}

void TelescopeView::on_message_update() {
	DevicePage::on_message_update();
	}
