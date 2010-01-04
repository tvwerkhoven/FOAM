/* controlview.h - the FOAM connection control pane
 Copyright (C) 2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*!
 @file controlview.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM connection control pane
 */

#ifndef HAVE_CONTROLVIEW_H
#define HAVE_CONTROLVIEW_H

#include <gtkmm.h>

#include "widgets.h"
#include "log.h"
#include "foamcontrol.h"

using namespace Gtk;

class ControlPage: public VBox {
	Log &log;
	FoamControl &foamctrl;
	
	Frame connframe;
	HBox connbox;
	LabeledEntry host;
	LabeledEntry port;
	Button connect;
	
	Frame modeframe;
	HBox modebox;
	Button mode_listen;
	Button mode_open;
	Button mode_closed;
	Button shutdown;

	Frame calibframe;
	HBox calibbox;
	Label calmode_lbl;
	ComboBoxText calmode_select;
	Button calib;

	Frame statframe;
	HBox statbox;
	LabeledEntry stat_mode;
	LabeledEntry stat_nwfs;
	LabeledEntry stat_nwfc;
	LabeledEntry stat_nframes;

	void on_connect_clicked();
	
	void on_mode_listen_clicked();
	void on_mode_open_clicked();
	void on_mode_closed_clicked();
	void on_shutdown_clicked();

	void on_calib_clicked();
	
public:
	ControlPage(Log &log, FoamControl &foamctrl);
	~ControlPage();

	void on_connect_update();
	void on_message_update();
};

#endif //  HAVE_CONTROLVIEW_H
