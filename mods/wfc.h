/*
 wfc.h -- a wavefront corrector base class
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_WFC_H
#define HAVE_WFC_H

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include "types.h"
#include "config.h"

/*!
 @brief Base wavefront corrector class. This will be overloaded with the specific WFC type
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 */
class Wfc {
public:
	typedef enum {
		ACT_ADD=0,
		ACT_SET
	} wfc_actmode_t;							//!< Mode to use for actuating (set or add)
	
	string name;									//!< WFC name
	
	int nact;											//!< Number of actuators
	int wfctype;									//!< WFC type/model
	
	virtual int verify(int) = 0;
	virtual int actuate(gsl_vector_float *ctrl, int) = 0;
	virtual int caibrate(int) = 0;
	
	static Wfc *create(config &config);	//!< Initialize new wavefront corrector
	
	virtual ~Wfc() {}
};

#endif // HAVE_WFC_H
