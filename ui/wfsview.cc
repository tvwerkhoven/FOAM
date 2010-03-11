/*
 wfsview.cc -- FOAM GUI wavefront sensor pane
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
/*!
 @file wfsview.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM GUI wavefront sensor pane
 */

#include "wfsview.h"
#include "widgets.h"

using namespace std;
using namespace Gtk;

WfsPage::WfsPage(Log &log):
log(log),
numwfs("# WFS:")
{
	log.add(Log::DEBUG, "WfsPage init.");
	
	// The top HBox has some status info:
	numwfs.set_editable(false);
	numwfs.set_text("-");
	
	status.set_spacing(4);
	status.pack_start(numwfs, PACK_SHRINK);
	
	// Add the status HBox
	set_spacing(4);
	pack_start(status, PACK_SHRINK);
	
	show_all_children();
}

WfsInfo::WfsInfo(Log &log):
log(log) {
	log.add(Log::DEBUG, "WfsInfo init.");
}
