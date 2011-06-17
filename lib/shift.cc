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

Shift::Shift(Io &io, const int nthr): io(io), nworker(nthr), workid(0) {
	io.msg(IO_DEB2, "Shift::Shift()");
	
	// Startup workers
	//! @todo Worker (workers) threads neet attr for scheduling etc (?)
	//workers = new pthread::thread[nworker];

	// Use this slot to point to a member function of this class (only used at start)
	sigc::slot<void> funcslot = sigc::mem_fun(this, &Shift::_worker_func);
	for (int w=0; w<nworker; w++) {
		pthread::thread tmp = pthread::thread();
		tmp.create(funcslot);
		workers.push_back(tmp);
	}	
}

Shift::~Shift() {
	io.msg(IO_DEB2, "Shift::~Shift()");
}

void Shift::_worker_func() {
	work_mutex.lock();
	int id = _worker_getid();
	io.msg(IO_XNFO, "Shift::_worker_func() new worker (id=%d n=%d)", id, nworker);
	work_mutex.unlock();

	float shift[2];

	while (true) {
		{
			pthread::mutexholder h(&work_mutex);
			io.msg(IO_XNFO, "Shift::_worker_func() worker %d waiting...", id);
			work_cond.wait(work_mutex);
		}
		
		while (true) {
			int myjob=0;
			{
				pthread::mutexholder h(&workpool.mutex);
				myjob = workpool.jobid--;
			}
			if (myjob < 0)
				break;
			
			_calc_cog(workpool.img, workpool.res, workpool.crops[myjob], shift, workpool.mini);
			
			//workpool.shifts->data[workpool.shifts->stride * myjob * 2 + 0]
			//workpool.shifts->data[workpool.shifts->stride * myjob * 2 + 1]

			//! @todo might give problems with 64 bit systems? Better to reserve a block per thread?
			gsl_vector_float_set(workpool.shifts, myjob*2+0, shift[0]);
			gsl_vector_float_set(workpool.shifts, myjob*2+1, shift[1]);
			//usleep(20*1000);
		}
		
		//pthread::mutexholder h(workpool.mutex);
		//! @todo use mutexholders
		
		{
			pthread::mutexholder h(&workpool.mutex);
			// Increment thread done counter, broadcast signal if we are the last thread
			if (++(workpool.done) == nworker-1)
				work_done_cond.broadcast();
		}
	}
}

void Shift::_calc_cog(const uint8_t *img, const coord_t &res, const vector_t &crop, float *v, const uint8_t mini) {
	uint8_t *p;
	float sum;

	sum = v[0] = v[1] = 0.0;
	
	// i,j loop over the pixels inside the crop field for *img
	for (int j=crop.ly; j<crop.ty; j++) {
		// j is the vertical counter, store the beginning of the current row here:
		p = (uint8_t *) img;
		p += (j*res.x) + crop.lx;
		//  img = data origin, j * res.x skips a few rows, crop.llpos.x gives the offset for the current row.
		for (int i=crop.lx; i<crop.tx; i++) {
			// Skip pixels with too low an intensity
			if (*p < mini) {
				p++;
				continue;
			}
			v[0] += *p * i;
			v[1] += *p * j;
			sum += *p;
			p++;
		}
	}
	
	// Sum 0? Then we skip this subimage
	if (sum <= 0) { 
		v[0] = v[1] = 0.0;
		return;
	}

	v[0] = v[0]/sum - crop.lx - (crop.tx - crop.lx)/2;
	v[1] = v[1]/sum - crop.ly - (crop.ty - crop.ly)/2;
}

bool Shift::calc_shifts(const uint8_t *img, const coord_t res, const std::vector<vector_t> &crops, gsl_vector_float *shifts, const method_t method, const bool wait, const uint8_t mini) {
	io.msg(IO_DEB2, "Shift::calc_shifts(uint8_t)");
	
	// Setup work parameters
	workpool.method = method;
	workpool.img = (uint8_t *) img;
	workpool.res = res;
	workpool.refimg = NULL;
	workpool.mini = mini;
	workpool.crops = crops;
	workpool.shifts = shifts;
	workpool.jobid = crops.size()-1;
	workpool.done = 0;
	
	// Signal all workers
	work_cond.broadcast();
	
	// Wait until the work is completed
	if (wait) {
		pthread::mutexholder h(&work_done_mutex);
		work_done_cond.wait(work_done_mutex);
	}
	
	return true;
}
