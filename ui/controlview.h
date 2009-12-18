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

#ifndef __CONTROLVIEW_H__
#define __CONTROLVIEW_H__

#include <gtkmm.h>

#include "log.h"

using namespace Gtk;

class FoamControl;

class ControlPage: public VBox {
	Log &log;
	FoamControl foamctrl;
	
	Frame connframe;
	Frame modeframe;
	
	HBox connbox;
	Label host;
	Label port;
	Button connect;
	Button disconnect;
	Entry hostentry;
	Entry portentry;
	HBox cspacer;
	
	HBox modebox;
	Button shutdown;
	HBox mspacer;
	
	void on_connect_clicked();
	void on_disconnect_clicked();
	void on_shutdown_clicked();
	
public:
	ControlPage(Log &log);
	~ControlPage();

	void on_update();
};

#include "foamcontrol.h"

#endif //  __CONTROLVIEW_H__
