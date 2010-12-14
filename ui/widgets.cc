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
#include "widgets.h"

using namespace std;
using namespace Gtk;


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

DelayedAdjustment::DelayedAdjustment(Adjustment &adjustment, int delay): adjustment(adjustment), delay(delay) {
	adjustment.signal_value_changed().connect(sigc::mem_fun(this, &DelayedAdjustment::on_value_changed));
}

DelayedAdjustment::DelayedAdjustment(SpinButton &widget, int delay): adjustment(*widget.get_adjustment()), delay(delay) {
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
