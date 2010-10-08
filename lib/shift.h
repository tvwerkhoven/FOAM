/*
 shift.h -- Shift measurements header file
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


#ifndef HAVE_SHIFT_H
#define HAVE_SHIFT_H

const int NTHREADS = 4;

#include "types.h"
#include "io.h"
#include "pthread++.h"

/*!
 @brief Image shift calculation class (for SHWFS)
 @todo Document this
 */
class Shift {
private:
	Io &io;
	pthread::thread threads[NTHREADS];
	
public:
	typedef enum {
		COG=0,
		CORR
	} mode_t;														//!< Different shift calculation modes

	typedef struct data_t {
		void *data;						//!< Data pointer, type encoded in dt
		dtype_t dt;						//!< Datatype
		coord_t res;					//!< Size
	}

	Shift(Io &io);
	~Shift(void);

};

#endif // HAVE_SHIFT_H
