/*
 shift.cc -- Shift measurements class
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
#ifndef _BSD_SOURCE
// For usleep glibc
#define _BSD_SOURCE
#endif
#include <unistd.h>

#include "io.h"
#include "types.h"

#include "shift.h"

Shift::Shift(Io &io, int nthr): io(io), nworker(nthr), workid(0) {
	io.msg(IO_DEB2, "Shift::Shift()");
	
	// Startup workers
	//! @todo Worker (workers) threads neet attr for scheduling etc (?)
	workers = new pthread::thread[nworker];

	// Use this slot to point to a member function of this class (only used at start)
	sigc::slot<void> funcslot = sigc::mem_fun(this, &Shift::_worker_func);
	for (int w=0; w<nworker; w++)
		workers->create(funcslot);
	
}

Shift::~Shift() {
	io.msg(IO_DEB2, "Shift::~Shift()");
}

void Shift::_worker_func() {
	work_mutex.lock();
	int id = _worker_getid();
	io.msg(IO_XNFO, "Shift::_worker_func() new worker (id=%d n=%d)", id, nworker);
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
			
			float tmp;
			_calc_cog(workpool.img, workpool.res, workpool.crops[myjob], &tmp);
			io.msg(IO_XNFO, "Shift::_worker_func():%d job:%d, val:%f", id, myjob, tmp);

			gsl_vector_float_set(workpool.shifts, myjob*2+0, tmp);
			gsl_vector_float_set(workpool.shifts, myjob*2+1, tmp);
			usleep(50*1000);
		}
		
		workpool.mutex.lock();
		if (++(workpool.done) == nworker-1)
			work_done_cond.broadcast();
		workpool.mutex.unlock();
	}
}

void Shift::_calc_cog(uint8_t *img, coord_t &res, crop_t &crop, float *vec) {
	*vec = drand48();
}

bool Shift::calc_shifts(uint8_t *img, coord_t res, crop_t *crops, int ncrop, gsl_vector_float *shifts, mode_t mode, bool wait) {
	io.msg(IO_DEB2, "Shift::calc_shifts(uint8_t)");
	
	// Setup work parameters
	workpool.mode = mode;
	workpool.img = img;
	workpool.res = res;
	workpool.refimg = NULL;
	workpool.crops = crops;
	workpool.ncrop = ncrop;
	workpool.shifts = shifts;
	workpool.jobid = ncrop-1;
	workpool.done = 0;
	
	// Signal all workers
	work_cond.broadcast();
	
	// Wait until all workers have signalled ready once
	if (wait) {
		io.msg(IO_DEB2, "Shift::calc_shifts(uint8_t): waiting for workers");
		work_done_mutex.lock();
		work_done_cond.wait(work_done_mutex);
		work_done_mutex.unlock();
	}
	
	return true;
}
