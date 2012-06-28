/*
 widgets.cc -- custom GTK widgets
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

#include <sys/time.h>

#include "widgets.h"
#include "utils.h"				// For clamp(x, min, max)

using namespace std;
using namespace Gtk;

static void log_term(string msg) {
	FILE *stream=stderr;
	bool showthread=true;
	if (showthread) {
		pthread_t pt = pthread_self();
		unsigned char *ptc = (unsigned char*)(void*)(&pt);
		char thrid[2+2*sizeof(pt)+1];
		sprintf(thrid, "0x");
		for (size_t i=0; i<sizeof(pt); i++)
			sprintf(thrid, "%s%02x", thrid, (unsigned)(ptc[i]));
		
		fprintf(stream, "(%s) %s\n", thrid, msg.c_str());
	} else {
		fprintf(stream, "%s\n", msg.c_str());
	}
}

BarGraph::BarGraph(const int width, const int height):
e_minval("Min"), e_maxval("Max"), e_allval("All"),
b_refresh(Gtk::Stock::REFRESH),
b_autoupd("Auto Update"), e_autointval("", "s"),
gr_align(0.5, 0.5, 0.0, 0.0) {
	// Init lastupd to 0 ('never')
	lastupd.tv_sec = 0;
	lastupd.tv_usec = 0;

	// Set entry fields to non editable (read-only)
	e_minval.set_width_chars(6);
	e_minval.set_editable(false);
	e_maxval.set_width_chars(6);
	e_maxval.set_editable(false);
	e_allval.set_width_chars(14);
	e_allval.set_editable(false);
	
	e_autointval.set_digits(2);
	e_autointval.set_value(1.0);
	e_autointval.set_increments(0.1, 1);
	e_autointval.set_range(0, 10.0);

	// Initialize pixbuf and image
	gr_pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, width, height);
	gr_pixbuf->fill(0xFFFFFF00);
	gr_img.set(gr_pixbuf);
	gr_img.set_double_buffered(false);
	
	// Pack boxes
	hbox0.pack_start(e_minval, PACK_SHRINK);
	hbox0.pack_start(e_maxval, PACK_SHRINK);

	vbox1.pack_start(hbox0, PACK_SHRINK);
	vbox1.pack_start(e_allval, PACK_SHRINK);
	vbox1.pack_start(hsep1, PACK_SHRINK);
	vbox1.pack_start(b_refresh, PACK_SHRINK);

	hbox1.pack_start(b_autoupd, PACK_SHRINK);
	hbox1.pack_start(e_autointval, PACK_SHRINK);

	vbox1.pack_start(hbox1, PACK_SHRINK);

	// Pack & stack boxes in eachother
	gr_events.add(gr_img);
	gr_align.add(gr_events);
	
	// Add vbox1 and gr_align to *this HBox
	pack_start(vbox1, PACK_SHRINK);
	pack_start(gr_align, PACK_SHRINK);
	
	// Connect events
	b_refresh.signal_clicked().connect(sigc::mem_fun(*this, &BarGraph::do_update));
	b_autoupd.signal_clicked().connect(sigc::mem_fun(*this, &BarGraph::on_autoupd_clicked));
	
	refresh_timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &BarGraph::on_timeout), 1000.0/30.0);
}

BarGraph::~BarGraph() {
	//! @todo delete Pixbuf here?
	refresh_timer.disconnect();
}

bool BarGraph::on_timeout() {
	// This function fires 30 times/s, if we do not want that many updates we can throttle it with subi_vecdelay
	struct timeval now;
	gettimeofday(&now, NULL);
	// If b_autoupd is 'clear' we don't want shifts, if 'waiting' we're expecting new shifts soon, otherwise if button is 'OK': get new shifts
	if (b_autoupd.get_state() == SwitchButton::OK && 
			((now.tv_sec - lastupd.tv_sec) + (now.tv_usec - lastupd.tv_usec)/1000000.) > e_autointval.get_value() ) {
		b_autoupd.set_state(SwitchButton::WAITING);
		log_term(format("%s: slot_update", __PRETTY_FUNCTION__));
		slot_update();
		gettimeofday(&lastupd, NULL);
	}
	return true;
}

void BarGraph::on_autoupd_clicked() {
	// Click the 'b_autoupd' button: 
	// -> if CLEAR: set to OK and run on_timeout()
	// -> if WAITING, OK or ERROR: set to CLEAR and stop everything
	if (b_autoupd.get_state() == SwitchButton::CLEAR) {
		b_autoupd.set_state(SwitchButton::OK);
		log_term(format("%s: on_timeout start", __PRETTY_FUNCTION__));
		on_timeout();
		log_term(format("%s: on_timeout done", __PRETTY_FUNCTION__));
	}
	else {
		b_autoupd.set_state(SwitchButton::CLEAR);
		log_term(format("%s: clear", __PRETTY_FUNCTION__));
	}
}

void BarGraph::on_update(const std::vector< double > &graph_vals) {
	if (graph_vals.size() <= 0)
		return;
	
	// range is -1 -- 1 for all modes. Check actual range:
	double min=graph_vals.at(0);
	double max=min;
	string allvals = format("%.4f", graph_vals.at(0));
	for (size_t i=1; i<graph_vals.size(); i++) {
		if (fabs(graph_vals.at(i)) > max)
			max = fabs(graph_vals.at(i));
		else if (fabs(graph_vals.at(i)) < min)
			min = fabs(graph_vals.at(i));
		allvals += format(", %.4f", graph_vals.at(i));
	}
	
	e_minval.set_text(format("%.4f", min));
	e_maxval.set_text(format("%.4f", max));
	e_allval.set_text(allvals);
	
	// make background white
	gr_pixbuf->fill(0xffffff00);
	
	// Draw bars
	uint8_t *out = (uint8_t *) gr_pixbuf->get_pixels();
	
	int nvals = graph_vals.size();
	int w = gr_pixbuf->get_width();
	int h = gr_pixbuf->get_height();
	
	// The pixbuf is w pixels wide, we have nvals values, so each column is colw wide:
	int colw = (w/nvals)-1;
	
	//! @bug this fails for more than ~200 modes, add scrollbar?
	if (colw <= 0) {
		fprintf(stderr, "BarGraph::do_update(): error, too many values, cannot draw!\n");
		return;
	}
	
	uint8_t col[3];
	for (int n = 0; n < nvals; n++) {
		double amp = clamp(graph_vals.at(n), (double)-1.0, (double)1.0);
		int height = amp*h/2.0; // should be between -h/2 and h/2
		
		// Set bar color (red 2%, orange 10% or green rest):
		if (fabs(amp)>0.98) {
			col[0] = 255; col[1] = 000; col[2] = 000; // X11 red
		} else if (fabs(amp)>0.90) {
			col[0] = 255; col[1] = 165; col[2] = 000; // X11 orange
		} else {
			col[0] = 144; col[1] = 238; col[2] = 144; // X11 lightgreen
		}
		
		// Bars going down (first part) or up (second part)
		if (height < 0) {
			for (int x = n*colw; x < (n+1)*colw; x++) {
				for (int y = h/2; y > h/2+height; y--) {
					uint8_t *p = out + 3 * (x + w * y);
					p[0] = col[0]; p[1] = col[1]; p[2] = col[2];
				} } } 
		else {
			for (int x = n*colw; x < (n+1)*colw; x++) {
				for (int y = h/2; y < h/2+height; y++) {
					uint8_t *p = out + 3 * (x + w * y);
					p[0] = col[0]; p[1] = col[1]; p[2] = col[2];
				} } }
	}
	
	// If we were waiting for this update, set button to 'OK' state again. If 
	// this is only a one-time 'b_refresh' event, this probably won't happen.
	if (b_autoupd.get_state() == SwitchButton::WAITING)
		b_autoupd.set_state(SwitchButton::OK);
	
	gr_img.queue_draw();
}


SwitchButton::SwitchButton(const Glib::ustring &lbl): 
Button(lbl), 
col_ok(Gdk::Color("lightgreen")), 
col_warn(Gdk::Color("yellow")), 
col_err(Gdk::Color("red"))
{
	set_state(CLEAR); 
}

void SwitchButton::set_state(enum state s) {
	state = s;
	switch (s) {
		case OK:
			modify_button(col_ok);
			break;
		case WAITING:
			modify_button(col_warn);
			break;
		case ERROR:
			modify_button(col_err);
			break;
		case CLEAR:
		default:
			modify_button();
			break;
	}
}

LabeledSpinEntry::LabeledSpinEntry(const Glib::ustring &pretext, const Glib::ustring &posttext, double lower, double upper, double step_increment, double page_increment, guint digits):
pre(pretext), post(posttext)
{
	pack_start(pre, true, true);
	pack_start(entry, PACK_SHRINK);
	if(!posttext.empty())
		pack_start(post, PACK_SHRINK);
	pre.set_alignment(ALIGN_LEFT);
	post.set_alignment(ALIGN_LEFT);
	entry.set_range(lower, upper);
	entry.set_increments(step_increment, page_increment);
	entry.set_digits(digits);
	entry.set_alignment(ALIGN_RIGHT);
	set_spacing(4);
}

LabeledEntry::LabeledEntry(const Glib::ustring &pretext, const Glib::ustring &posttext):
pre(pretext), post(posttext)
{
	pack_start(pre, true, true);
	pack_start(entry, false, false);
	if(!posttext.empty())
		pack_start(post, PACK_SHRINK);
	pre.set_alignment(ALIGN_LEFT);
	entry.set_alignment(ALIGN_LEFT);
	post.set_alignment(ALIGN_LEFT);
	set_spacing(4);
}

LabeledSpinView::LabeledSpinView(const Glib::ustring &pretext, const Glib::ustring &posttext):
pre(pretext), post(posttext)
{
	pack_start(pre, true, true);
	pack_start(entry, false, false);
	if(!posttext.empty())
		pack_start(post, PACK_SHRINK);
	pre.set_alignment(ALIGN_LEFT);
	entry.set_alignment(ALIGN_RIGHT);
	post.set_alignment(ALIGN_LEFT);
	set_spacing(4);
	entry.set_width_chars(10);
	entry.set_editable(false);
	set_digits(0);
}

LabeledTextView::LabeledTextView(const Glib::ustring &pretext, const Glib::ustring &posttext):
pre(pretext), post(posttext)
{
	pack_start(pre, true, true);
	pack_start(scrolledwindow);
	if(!posttext.empty())
		pack_start(post, PACK_SHRINK);
	scrolledwindow.add(textview);
	scrolledwindow.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrolledwindow.set_shadow_type(SHADOW_ETCHED_IN);
	pre.set_alignment(ALIGN_LEFT);
	post.set_alignment(ALIGN_LEFT);
	set_spacing(4);
}

void PhysUnitGroup::add(const PhysUnit &unit) {
	units.push_back(&unit);
	unit_added(unit);
}

const PhysUnit &PhysUnitGroup::get_unit() {
	return *current;
}

void PhysUnitGroup::set_unit(const PhysUnit &unit) {
	const PhysUnit &prev = *current;
	current = &unit;
	unit_changed(prev);
}

double PhysUnitGroup::to_native(double value) const {
	if(!current)
		return value;
	else
		return current->to_native(value);
}

double PhysUnitGroup::from_native(double value) const {
	if(!current)
		return value;
	else
		return current->from_native(value);
}

Glib::ustring PhysUnitGroup::get_name() const {
	if(!current)
		return "";
	else
		return current->get_name();
}

PhysUnitSpinEntry::PhysUnitSpinEntry(const Glib::ustring &pretext, PhysUnitGroup &unitgroup, double lower, double upper, double step_increment, double page_increment, guint digits):
pre(pretext),
unitgroup(unitgroup),
lower(lower), upper(upper), step_increment(step_increment), page_increment(page_increment), digits(digits)
{
	entry.set_value(unitgroup.from_native(entry.get_value()));
	post.set_text(unitgroup.get_name());
	unitgroup.signal_unit_changed().connect(sigc::mem_fun(*this, &PhysUnitSpinEntry::on_unit_changed));

	pack_start(pre, true, true);
	pack_start(entry, PACK_SHRINK);
	pack_start(post, PACK_SHRINK);

	pre.set_alignment(ALIGN_LEFT);
	post.set_alignment(ALIGN_LEFT);
	entry.set_range(lower, upper);
	entry.set_increments(step_increment, page_increment);
	entry.set_digits(digits);
	entry.set_alignment(ALIGN_RIGHT);
	set_spacing(4);
}

void PhysUnitSpinEntry::on_unit_changed(const PhysUnit &prev) {
	double native = prev.to_native(entry.get_value());

	entry.set_range(unitgroup.from_native(lower), unitgroup.from_native(upper));
	entry.set_increments(unitgroup.from_native(step_increment), unitgroup.from_native(page_increment));
	entry.set_digits(digits - (guint)ceil(log10(unitgroup.from_native(1))));
	post.set_text(unitgroup.get_name());

	entry.set_value(unitgroup.from_native(native));
}

PhysUnitMenu::Item::Item(PhysUnitMenu &unitmenu, const PhysUnit &unit):
RadioMenuItem(unitmenu.itemgroup, unit.get_name()),
unitmenu(unitmenu), unit(unit) {}

void PhysUnitMenu::Item::on_toggled() {
	if(get_active())
		unitmenu.unitgroup.set_unit(unit);
}

PhysUnitMenu::PhysUnitMenu(PhysUnitGroup &unitgroup): unitgroup(unitgroup) {
	vector<const PhysUnit *>::iterator i;

	for(i = unitgroup.units.begin(); i != unitgroup.units.end(); ++i)
		add(*new Item(*this, **i));
}

PhysUnitEntry::PhysUnitEntry(const Glib::ustring &pretext, PhysUnitGroup &unitgroup, bool is_editable):
pre(pretext),
unitgroup(unitgroup)
{
	entry.set_text(format("%lf", unitgroup.from_native(atof(entry.get_text().c_str()))));
	post.set_text(unitgroup.get_name());
	unitgroup.signal_unit_changed().connect(sigc::mem_fun(*this, &PhysUnitEntry::on_unit_changed));

	pack_start(pre, true, true);
	pack_start(entry, PACK_SHRINK);
	pack_start(post, PACK_SHRINK);

	pre.set_alignment(ALIGN_LEFT);
	post.set_alignment(ALIGN_LEFT);
	entry.set_alignment(ALIGN_RIGHT);
	entry.set_has_frame(false);
	entry.set_editable(is_editable);
	set_spacing(4);
}

void PhysUnitEntry::on_unit_changed(const PhysUnit &prev) {
	double native = prev.to_native(atof(entry.get_text().c_str()));

	post.set_text(unitgroup.get_name());

	entry.set_text(format("%lf", unitgroup.from_native(native)));
}

DelayedAdjustment::DelayedAdjustment(Gtk::Adjustment &adjustment, int delay): adjustment(adjustment), delay(delay) {
	adjustment.signal_value_changed().connect(sigc::mem_fun(this, &DelayedAdjustment::on_value_changed));
}

DelayedAdjustment::DelayedAdjustment(Gtk::SpinButton &widget, int delay): adjustment(*widget.get_adjustment()), delay(delay) {
	adjustment.signal_value_changed().connect(sigc::mem_fun(this, &DelayedAdjustment::on_value_changed));
}

DelayedAdjustment::DelayedAdjustment(LabeledSpinEntry &widget, int delay): adjustment(*widget.get_adjustment()), delay(delay) {
	adjustment.signal_value_changed().connect(sigc::mem_fun(this, &DelayedAdjustment::on_value_changed));
}

void DelayedAdjustment::on_value_changed() {
	if(on_timeout_connection.empty())
		Glib::signal_timeout().connect(sigc::mem_fun(this, &DelayedAdjustment::on_timeout), delay);
	else
		delay = true;
}

bool DelayedAdjustment::on_timeout() {
	if(delay) {
		delay = false;
		return true;
	}

	value_changed.emit();

	on_timeout_connection.disconnect();

	return true;
}
