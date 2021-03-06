/*
 widgets.h -- custom GTK widgets
 Copyright (C) 2009--2010 Guus Sliepen <guus@sliepen.eu.org>
 
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

#ifndef HAVE_WIDGETS_H
#define HAVE_WIDGETS_H

#include <math.h>
#include <gtkmm.h>
#include <vector>
#include "format.h"


/*!
 @brief Similar to ToggleButton, but uses color for status indication, which can be updated without generating signals
 */
class SwitchButton: public Gtk::Button {
public:
	enum state {
		OK=1,
		WAITING,
		ERROR,
		CLEAR,
	};													//!< Various button states, OK, WAITING, ERROR, CLEAR.
private:
	//<! Change the color of this button
	void modify_button(const Gdk::Color &color) {
		this->modify_bg(Gtk::STATE_PRELIGHT, color);
		this->modify_bg(Gtk::STATE_NORMAL, color);
	}
	//!< Reset the button
	void modify_button() {
		this->unset_bg(Gtk::STATE_NORMAL);
		this->unset_bg(Gtk::STATE_PRELIGHT);
	}
	enum state state;						//!< Track state of this button.
public:
	const Gdk::Color col_ok;		//!< Color to use for 'OK' status
	const Gdk::Color col_warn;	//!< Color to use for 'WARNING' status
	const Gdk::Color col_err;		//!< Color to use for 'ERROR' status
	
	void set_state(enum state s);
	enum state get_state() { return state; }	//!< Get the state of this button
	
	SwitchButton(const Glib::ustring &lbl="SwitchButton");
};

class LabeledSpinEntry: public Gtk::HBox {
	Gtk::Label pre;
	Gtk::Label post;

	public:
	LabeledSpinEntry(const Glib::ustring &pretext, const Glib::ustring &posttext = "", double lower = 0, double upper = INFINITY, double step_increment = 1, double page_increment = 1, guint digits = 0);

	Gtk::SpinButton entry;

	void set_value(double value) {
		entry.set_value(value);
	}

	double get_value() const {
		return entry.get_value();
	}

	int get_value_as_int() const {
		return entry.get_value_as_int();
	}

	void set_digits(guint digits) {
		entry.set_digits(digits);
	}

	void set_increments(double step, double page) {
		entry.set_increments(step, page);
	}

	void set_range(double min, double max) {
		entry.set_range(min, max);
	}

	bool get_editable() const {
		return entry.get_editable();
	}

	void spin(Gtk::SpinType direction, double increment) {
		entry.spin(direction, increment);
	}

	Glib::SignalProxy0<void> signal_value_changed() {
		return entry.signal_value_changed();
	}
	
	Glib::SignalProxy0<void> signal_activate() {
		return entry.signal_activate();		
	}

	Gtk::Adjustment *get_adjustment() {
		return entry.get_adjustment();
	}
};

class LabeledEntry: public Gtk::HBox {
	Gtk::Label pre;
	Gtk::Label post;
	
public:
	Gtk::Entry entry;
	
	LabeledEntry(const Glib::ustring &pretext, const Glib::ustring &posttext = "");
	
	void set_size_request(int size) {
		entry.set_size_request(size);
	}
	
	void set_width_chars(int n_chars) {
		entry.set_width_chars(n_chars);
	}
	
	void set_text(const Glib::ustring &text) {
		entry.set_text(text);
	}

	Glib::ustring get_text() const {
		return entry.get_text();
	}
	
	double get_value() {
		return atof(entry.get_text().c_str());
	}
	
	int get_value_as_int() {
		return atoi(entry.get_text().c_str());
	}

	void set_editable(bool editable = true) {
		entry.set_editable(editable);
	}

	bool get_editable() const {
		return entry.get_editable();
	}

	void set_alignment(double alignment) {
		entry.set_alignment(alignment);
	}
};

/*!
 @brief Bar graph used to display SHWFS shift vector and WFC actuator voltages
 */
class BarGraph: public Gtk::HBox {
private:
	Gtk::VBox vbox1;
	Gtk::HBox hbox0;
	LabeledEntry e_minval;										//!< Minimum value in bargraph
	LabeledEntry e_maxval;										//!< Maximum value in bargraph
	LabeledEntry e_allval;										//!< Vector of all values
	
	LabeledSpinEntry e_scale;									//!< Bar graph scale
	
	Gtk::HSeparator hsep1;
	Gtk::Button b_refresh;										//!< Update once button
	
	Gtk::HBox hbox1;
	SwitchButton b_autoupd;										//!< Auto update button
	LabeledSpinEntry e_autointval;						//!< Auto update interval

	Gtk::EventBox gr_events;
	Gtk::Image gr_img;
	Glib::RefPtr<Gdk::Pixbuf> gr_pixbuf;
	Gtk::Alignment gr_align;
	
	bool on_timeout();												//!< Run this 30x/sec to enable auto updating, throttle with e_autointval
	void do_update() { slot_update(); }				//!< Wrapper for slot_update
	
	void on_autoupd_clicked();								//!< Callback for b_autoupd
	sigc::connection refresh_timer;						//!< For Glib::signal_timeout()
	struct timeval lastupd;										//!< Time of last update
	
public:
	BarGraph(const int width=480, const int height=100, const float scl=1.0);
	~BarGraph();

	sigc::slot<void> slot_update;							//!< Slot to request update

	void on_update(const std::vector< double > &graph_vals);
};


class LabeledSpinView: public Gtk::HBox {
	Gtk::Label pre;
	Gtk::Label post;
	Gtk::Entry entry;
	int digits;

	public:
	LabeledSpinView(const Glib::ustring &pretext, const Glib::ustring &posttext = "");

	double get_value() {
		return atof(entry.get_text().c_str());
	}

	int get_value_as_int() {
		return atoi(entry.get_text().c_str());
	}

	void set_value(double value) {
		entry.set_text(format("%.*lf", digits, value));
	}

	void set_digits(int value) {
		digits = value;
		set_value(get_value());
	}

	void set_editable(bool editable = true) {
		entry.set_editable(editable);
	}

	bool get_editable() const {
		return entry.get_editable();
	}
};

class LabeledTextView: public Gtk::HBox {
	Gtk::Label pre;
	Gtk::Label post;
	Gtk::ScrolledWindow scrolledwindow;

	public:
	Gtk::TextView textview;
	LabeledTextView(const Glib::ustring &pretext, const Glib::ustring &posttext = "");

	void set_editable(bool editable = true) {
		textview.set_editable(editable);
	}

	bool get_editable() const {
		return textview.get_editable();
	}
};

class PhysUnit {
	const Glib::ustring name;

	public:
	PhysUnit(const Glib::ustring &name): name(name) {}
	virtual ~PhysUnit() {}
	virtual double to_native(double value) const = 0;
	virtual double from_native(double value) const = 0;

	Glib::ustring get_name() const {
		return name;
	}
};

class AbsolutePhysUnit: public PhysUnit {
	const double factor;

	public:
	AbsolutePhysUnit(const Glib::ustring &name, double factor): PhysUnit(name), factor(factor) {}

	double to_native(double value) const {
		return value * factor;
	}

	double from_native(double value) const {
		return value / factor;
	}
};

class RelativePhysUnit: public PhysUnit {
	const double factor;
	const double offset;

	public:
	RelativePhysUnit(const Glib::ustring &name, double factor, double offset): PhysUnit(name), factor(factor), offset(offset) {}

	double to_native(double value) const {
		return (value * factor) + offset;
	}

	double from_native(double value) const {
		return (value - offset) / factor;
	}
};

class PhysUnitGroup {
	friend class PhysUnitMenu;

	std::vector<const PhysUnit *> units;
	const PhysUnit *current;

	sigc::signal<void, const PhysUnit &> unit_changed;
	sigc::signal<void, const PhysUnit &> unit_added;

	public:
	PhysUnitGroup(): current(0) {}
	void add(const PhysUnit &unit);
	void set_unit(const PhysUnit &unit);
	const PhysUnit &get_unit();
	double to_native(double value) const;
	double from_native(double value) const;
	Glib::ustring get_name() const;

	sigc::signal<void, const PhysUnit &> signal_unit_changed() {
		return unit_changed;
	}

	sigc::signal<void, const PhysUnit &> signal_unit_added() {
		return unit_added;
	}
};

class PhysUnitMenu: public Gtk::Menu {
	friend class Item;
	Gtk::RadioButtonGroup itemgroup;

	class Item: public Gtk::RadioMenuItem {
		const PhysUnitMenu &unitmenu;
		const PhysUnit &unit;
		void on_toggled();
		public:
		Item(PhysUnitMenu &unitmenu, const PhysUnit &unit);
	};

	PhysUnitGroup &unitgroup;
	const PhysUnit *prev;

	public:
	PhysUnitMenu(PhysUnitGroup &unitgroup);
};

class PhysUnitSpinEntry: public Gtk::HBox {
	Gtk::Label pre;
	Gtk::Label post;
	Gtk::SpinButton entry;
	PhysUnitGroup &unitgroup;
	double lower;
	double upper;
	double step_increment;
	double page_increment;
	guint digits;

	void on_unit_changed(const PhysUnit &prev);

	public:
	
	PhysUnitSpinEntry(const Glib::ustring &pretext, PhysUnitGroup &unitgroup, double lower = 0, double upper = INFINITY, double step_increment = 1, double page_increment = 10, guint digits = 0);

	double get_value() {
		return unitgroup.to_native(entry.get_value());
	}

	void set_value(double value) {
		entry.set_value(unitgroup.from_native(value));
	}

	void set_range(double min, double max) {
		entry.set_range(min, max);
	}

	Glib::SignalProxy0<void> signal_value_changed() {
		return entry.signal_value_changed();
	}
};

class PhysUnitEntry: public Gtk::HBox {
	Gtk::Label pre;
	Gtk::Label post;
	Gtk::Entry entry;
	PhysUnitGroup &unitgroup;

	void on_unit_changed(const PhysUnit &prev);

	public:
	
	PhysUnitEntry(const Glib::ustring &pretext, PhysUnitGroup &unitgroup, bool is_editable = true);

	double get_value() {
		return unitgroup.to_native(atof(entry.get_text().c_str()));
	}

	void set_value(double value) {
		entry.set_text(format("%lf", unitgroup.from_native(value)));
	}

	void set_editable(bool editable = true) {
		entry.set_editable(editable);
	}

	bool get_editable() const {
		return entry.get_editable();
	}
};

class DelayedAdjustment {
	Gtk::Adjustment &adjustment;
	int delay;

	sigc::connection on_timeout_connection;
	void on_value_changed();
	bool on_timeout();
	sigc::signal<void> value_changed;

	public:
	DelayedAdjustment(Gtk::Adjustment &adjustment, int delay = 1000);
	DelayedAdjustment(Gtk::SpinButton &widget, int delay = 1000);
	DelayedAdjustment(LabeledSpinEntry &widget, int delay = 1000);

	void set_delay(int value) {delay = value;}
	sigc::signal<void> signal_value_changed() {return value_changed;}
};


#endif // HAVE_WIDGETS_H
