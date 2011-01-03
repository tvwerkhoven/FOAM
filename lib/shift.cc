/*
 shift.cc -- Shift measurements class
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

#include <string>
#include <gsl/gsl_vector.h>
#include <stdlib.h>

#include "io.h"
#include "types.h"

#include "shift.h"

Shift::Shift(Io &io, int nthr): io(io), nworker(nthr), workid(0) {
	io.msg(IO_DEB2, "Shift::Shift()");
	
	// Startup workers
	//! @todo Worker (workers) threads neet attr for scheduling etc (?)
	workers = new pthread::thread[4];

	// Use this slot to point to a member function of this class (only used at start)
	sigc::slot<void> funcslot = sigc::mem_fun(this, &Shift::_worker_func);
	for (int w=0; w<nthr; w++)
		workers->create(funcslot);
	
}

Shift::~Shift() {
	io.msg(IO_DEB2, "Shift::~Shift()");
}

void Shift::_worker_func() {
	work_mutex.lock();
	int id = _worker_getid();
	io.msg(IO_XNFO, "Shift::_worker_func() new worker thread(id=%d nworker=%d): %X", id, nworker, pthread_self());
	work_mutex.unlock();

	while (true) {
		work_mutex.lock();
		io.msg(IO_XNFO, "Shift::_worker_func() worker %d waiting...", id);
		work_cond.wait(work_mutex);
		work_mutex.unlock();
		
		while (true) {
			workpool.mutex.lock();
			int myjob = workpool.jobid--;
			workpool.mutex.unlock();
			if (myjob < 0)
				break;
			
			gsl_vector_float_set(workpool.shifts, myjob*2+0, drand48());
			gsl_vector_float_set(workpool.shifts, myjob*2+1, drand48());
		}
	}
}

bool Shift::calc_shifts(uint8_t *img, coord_t res, crop_t *crops, int ncrop, gsl_vector_float *shifts, mode_t mode) {
	io.msg(IO_DEB2, "Shift::calc_shifts(uint8_t)");
	
	// Setup work parameters
	workpool.mode = mode;
	workpool.img = img;
	workpool.refimg = NULL;
	workpool.crops = crops;
	workpool.ncrop = ncrop;
	workpool.shifts = shifts;
	workpool.jobid = ncrop-1;
	
	// Signal all workers
	work_cond.broadcast();
	
	return true;
}
