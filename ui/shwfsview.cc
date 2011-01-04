/*
 shwfsview.cc -- shack-hartmann wavefront sensor viewer class
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

#include "wfsview.h"
#include "shwfsctrl.h"
#include "shwfsview.h"

using namespace std;
using namespace Gtk;

ShwfsView::ShwfsView(ShwfsCtrl *ctrl, Log &log, FoamControl &foamctrl, string n): 
WfsView((WfsCtrl *) ctrl, log, foamctrl, n), shwfsctrl(ctrl)
{
	fprintf(stderr, "%x:ShwfsView::ShwfsView()\n", (int) pthread_self());
	
	clear_gui();
	disable_gui();
	
	// finalize
	show_all_children();
}

ShwfsView::~ShwfsView() {
	fprintf(stderr, "%x:ShwfsView::~ShwfsView()\n", (int) pthread_self());
}

void ShwfsView::enable_gui() {
	WfsView::enable_gui();
	fprintf(stderr, "%x:ShwfsView::enable_gui()\n", (int) pthread_self());
}

void ShwfsView::disable_gui() {
	WfsView::disable_gui();
	fprintf(stderr, "%x:ShwfsView::disable_gui()\n", (int) pthread_self());
}

void ShwfsView::clear_gui() {
	WfsView::clear_gui();
	fprintf(stderr, "%x:ShwfsView::clear_gui()\n", (int) pthread_self());
}

