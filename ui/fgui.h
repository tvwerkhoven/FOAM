/* fgui.h - the FOAM GUI header file
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
 @file fgui.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM control GUI header
 */

#ifndef __FGUI_H__
#define __FGUI_H__

#include <gtkmm.h>

#include "about.h"
#include "widgets.h"
#include "log.h"
#include "logview.h"
#include "foamcontrol.h"
#include "controlview.h"

using namespace Gtk;

class ConnectDialog: public Dialog {
	FoamControl &foamctrl;
	
	Label label;
	LabeledEntry host;
	LabeledEntry port;
	
	void on_ok();
	void on_cancel();
public:
	ConnectDialog(FoamControl &foamctrl);
	~ConnectDialog() {}
};

class MainMenu: public MenuBar {
	MenuItem file;
	MenuItem help;
	
	Menu filemenu;
	Menu helpmenu;
	
	SeparatorMenuItem sep1;
	SeparatorMenuItem sep2;
	
public:
	ImageMenuItem connect;
	ImageMenuItem quit;
	
	ImageMenuItem about;
	
	MainMenu(Window &window);
	~MainMenu() {};
};


class MainWindow: public Window {
	VBox vbox;
	
	Log log;
	FoamControl foamctrl;

	AboutFOAMGui aboutdialog;
	Notebook notebook;
	ConnectDialog conndialog;
	
	LogPage logpage;
	ControlPage controlpage;
	
	void on_about_activate();
	void on_quit_activate();	
	void on_connect_activate();
	
public:	
	MainMenu menubar;
	
	void on_ctrl_connect_update();
	void on_ctrl_message_update();	
	
	MainWindow();
	~MainWindow() {};
};

#endif // __FGUI_H__
