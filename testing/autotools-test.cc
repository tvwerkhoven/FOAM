/*
 autotools-test.cc -- test autotools variable and chain
 
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


#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <stdio.h>

int main() {
	printf("Hello World\n");
	printf("Package name: %s\n", PACKAGE_NAME);
	printf("Package version: %s\n", PACKAGE_VERSION);
	printf("Package bugreport: %s\n", PACKAGE_BUGREPORT);
#ifdef GIT_REVISION
	printf("GIT_REVISION: %s\n", GIT_REVISION);
#endif
}

