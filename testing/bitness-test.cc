/*
 bitness-test.cc -- reminder about sizeof(type) vs sizeof(type*)
 
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include <stdio.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <stdint.h>
#include <limits.h>

int main() {
//	int *dataint = new int(32);
//	uint32_t *data32 = new uint32_t(32);
//	uint64_t *data64 = new uint64_t(32);
	
	printf("sizeof int: %zu\n", sizeof (int));
	printf("sizeof int*: %zu\n", sizeof (int*));

	printf("sizeof uint32_t: %zu\n", sizeof (uint32_t));
	printf("sizeof uint32_t*: %zu\n", sizeof (uint32_t*));
	
	printf("sizeof uint64_t: %zu\n", sizeof (uint64_t));
	printf("sizeof uint64_t*: %zu\n", sizeof (uint64_t*));
	
	printf("This program is running as %zu bits.\n", sizeof (int*)*8);
	
	size_t as = 32;
	int ai = 32;
	printf("(int) size_t(32): %zu -> %d\n", as, (int) as);
	printf("(size_t) int(32): %d -> %zu\n", ai, (size_t) ai);

	as = (1L << 31L) - 1L;
	ai = ((1L << 31L) - 1L);
	
	printf("(int) size_t(2^31-1): %zu -> %d\n", as, (int) as);
	printf("(size_t) int(2^31-1): %d -> %zu\n", ai, (size_t) ai);
	
	return 0;
}
