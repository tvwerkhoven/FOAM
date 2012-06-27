/*
 latencies-test.cc -- test latencies of function calls etc.
 
 Copyright (C) 2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#include <sigc++/signal.h>
#include <string>
#include <cstdio>

#include "time++.h"

void subfunc1();
void subfunc2();
void subfunc_work();

sigc::slot<void> slot_funccall;

void subfunc1() {
	static long long int i=0;
	i++;
	subfunc_work();
}

void subfunc2() {
	static long long int i=0;
	i++;
	slot_funccall();
}

void subfunc_work() {
	static long long int i=0;
	i++;
}

int main() {
	slot_funccall = sigc::ptr_fun(subfunc_work);
	
	Time t0;
	Time dur = Time() - t0;
	
	printf("time test: %s\n", dur.c_str());

	dur = t0 - t0;
	
	printf("time test: %s\n", dur.c_str());
	
	printf("function call test:\n");
	// Test function speed
	for (long long int i=0; i<1E9; i++)
		subfunc1();
	
	dur = Time() - t0;
	printf("... took %s seconds\n", dur.c_str());
	
	t0.update();
	printf("signal call test:\n");
	// Test function speed
	for (long long int i=0; i<1E9; i++)
		subfunc2();
	
	dur = Time() - t0;
	printf("... took %s seconds\n", dur.c_str());

	return 0;
}
