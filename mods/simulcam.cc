/*
 simulcam.cc -- atmosphere/telescope simulator camera
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.	If not, see <http://www.gnu.org/licenses/>. 
 */

#include <unistd.h>
#include <gsl/gsl_matrix.h>
#include <math.h>
#include <fftw3.h>

#include "io.h"
#include "config.h"
#include "pthread++.h"

#include "simseeing.h"
#include "camera.h"
#include "simulcam.h"

SimulCam::SimulCam(Io &io, foamctrl *ptc, string name, string port, Path &conffile):
Camera(io, ptc, name, SimulCam_type, port, conffile),
seeing(io, ptc, name + "-seeing", port, conffile),
shwfs(NULL), out_size(0), frame_out(NULL), telradius(1.0), telapt(NULL)
{
	io.msg(IO_DEB2, "SimulCam::SimulCam()");
	
	noise = cfg.getdouble("simnoise", 10);
	
	// Setup seeing parameters
	Path wffile = ptc->confdir + cfg.getstring("wavefront_file");

	coord_t wind;
	wind.x = cfg.getint("windspeed.x", 16);
	wind.y = cfg.getint("windspeed.y", 16);

	SimSeeing::wind_t windtype;
	string windstr = cfg.getstring("windtype", "linear");
	if (windstr == "linear")
		windtype = SimSeeing::LINEAR;
	else
		windtype = SimSeeing::RANDOM;
	
	seeing.setup(wffile, res, wind, windtype);
	
	// Get telescope aperture
	gen_telapt();

	
	cam_thr.create(sigc::mem_fun(*this, &SimulCam::cam_handler));
}

void SimulCam::gen_telapt() {
	io.msg(IO_XNFO, "SimulCam::gen_telapt(): init");
	
	if (!telapt) {							// Doesn't exist, callocate (all zeros is goed)
		telapt = gsl_matrix_calloc(res.y, res.x);
	}
	else if (telapt->size1 != res.x || telapt->size2 != res.y) { // Wrong size, re-allocate
		gsl_matrix_free(telapt);
		telapt = gsl_matrix_calloc(res.y, res.x);
	}
	
	// Calculate aperture size
	float minradsq = pow(((double) min(telapt->size1, telapt->size2))*telradius/2.0, 2);
	
	float pixi, pixj;
	double sum=0;
	for (size_t i=0; i<telapt->size1; i++) {
		for (size_t j=0; j<telapt->size2; j++) {
			// If a pixel falls within the aperture radius, set it to 1
			pixi = ((float) i) - telapt->size1/2;
			pixj = ((float) j) - telapt->size2/2;
			if (pixi*pixi + pixj*pixj < minradsq) {
				sum++;
				gsl_matrix_set (telapt, i, j, 1.0);
			}
		}
	}
}

void SimulCam::simul_telescope(gsl_matrix *im_in) {
	io.msg(IO_DEB2, "SimulCam::simul_telescope()");
	// Multiply wavefront with aperture
	gsl_matrix_mul_elements (im_in, telapt);
}

void SimulCam::simul_wfs(gsl_matrix *wave_in) {
	if (!shwfs)
		io.msg(IO_ERR | IO_FATAL, "SimulCam::simul_wfs(): cannot simulate wavefront without Shwfs reference.");
	
	if (shwfs->mla.nsi <= 0) {
		io.msg(IO_WARN, "SimulCam::simul_wfs(): no microlenses defined?.");
		return;
	}
	
	io.msg(IO_DEB2, "SimulCam::simul_wfs()");
	
	// Apply fourier transform to subimages here
	coord_t sallpos = shwfs->mla.ml[0].llpos;
	coord_t sasize = shwfs->mla.ml[0].size;
	// Get temporary memory
	gsl_vector *workspace = gsl_vector_calloc(sasize.x * sasize.y * 4);
	// Setup FFTW parameters
	fftw_complex *shdata = (fftw_complex *) fftw_malloc(sasize.y*2 * sasize.x*2 * sizeof(fftw_complex));
	// Set memory to 0
	for (int i=0; i< sasize.y*2 * sasize.x*2; i++)
		shdata[i][0] = shdata[i][1] = 0.0;
	fftw_plan shplan = fftw_plan_dft_2d(sasize.y*2, sasize.x*2, shdata, shdata, FFTW_FORWARD, FFTW_MEASURE);
	double tmp=0;
	// Temporary matrices
	gsl_matrix_view subap;
	gsl_matrix *subapm;
	
	for (int n=0; n<shwfs->mla.nsi; n++) {
		sallpos = shwfs->mla.ml[n].llpos;
		sasize = shwfs->mla.ml[n].size;
		
		// Crop out subaperture from larger frame, store as gsl_matrix_view
		subap = gsl_matrix_submatrix(wave_in, sallpos.y, sallpos.x, sasize.y, sasize.x);
		subapm = &(subap.matrix);
		
		if (workspace->size != sasize.x * sasize.y * 4) {
			// Re-alloc data if necessary (should be sasize, but this can vary per subap)
			io.msg(IO_WARN, "SimulCam::simul_wfs() subap sizes unequal, re-allocating. Support might be flaky.");
			// Re-alloc data
			gsl_vector_free(workspace);
			workspace = gsl_vector_calloc(sasize.x * sasize.y * 4);
			// Re-alloc FFTW
			fftw_destroy_plan(shplan);
			fftw_free(shdata);
			shdata = (fftw_complex *) fftw_malloc(sasize.y*2 * sasize.x*2 * sizeof(fftw_complex));
			// Set memory to 0
			for (int i=0; i< sasize.y*2 * sasize.x*2; i++)
				shdata[i][0] = shdata[i][1] = 0.0;
			shplan = fftw_plan_dft_2d(sasize.y*2, sasize.x*2, shdata, shdata, FFTW_FORWARD, FFTW_MEASURE);
		}

		// Set FFTW data to zero, otherwise residuals will be FFT'ed again later
		for (int i=0; i< sasize.y*2 * sasize.x*2; i++)
			shdata[i][0] = shdata[i][1] = 0.0;

		// Copy data to linear array for FFT, with padding
		for (int i=0; i<sasize.y; i++)
			for (int j=0; j<sasize.x; j++) {
				// Convert real wavefront to complex EM wave E \propto exp(-i phi) = cos(phi) + i sin(phi)
				tmp = gsl_matrix_get(subapm, i, j);
				shdata[(i+sasize.y/2)*2*sasize.x + (j+sasize.x)][0] = cos(tmp);
				shdata[(i+sasize.y/2)*2*sasize.x + (j+sasize.x)][1] = sin(tmp);
				//fprintf(stderr, "%g, cos: %g, \n", tmp, cos(tmp));
			}
		
		// Execute this FFT
		fftw_execute(shplan);
		
		// now calculate the absolute squared value of that, store it in the subapt thing
		// also find min and maximum heredouble tmp
		for (int i=0; i<sasize.y/2; i++) {
			for (int j=0; j<sasize.x/2; j++) {
				// Calculate abs(E^2), store in original memory (per quadrant)
				tmp = fabs(pow(shdata[i*sasize.x*2 + j][0], 2) + 
									 pow(shdata[i*sasize.x*2 + j][1], 2));
				gsl_matrix_set(subapm, sasize.y/2+i, sasize.x/2+j, tmp);
				
				tmp = fabs(pow(shdata[i*sasize.x*2 + j + sasize.x*2*3/4][0], 2) + 
									 pow(shdata[i*sasize.x*2 + j + sasize.x*2*3/4][1], 2));
				gsl_matrix_set(subapm, sasize.y/2+i, j, tmp);

				tmp = fabs(pow(shdata[(i+sasize.y*3/4)*sasize.x*2 + j][0], 2) + 
									 pow(shdata[(i+sasize.y*3/4)*sasize.x*2 + j][1], 2));
				gsl_matrix_set(subapm, i, sasize.x/2+j, tmp);

				tmp = fabs(pow(shdata[(i+sasize.y*3/4)*sasize.x*2 + j + sasize.x*2*3/4][0], 2) + 
									 pow(shdata[(i+sasize.y*3/4)*sasize.x*2 + j + sasize.x*2*3/4][1], 2));
				gsl_matrix_set(subapm, i, j, tmp);
				//fprintf(stderr, "out: abs(%g^2+%g^2) = %g\n", shdata[i*sasize.y*2 + j][0], shdata[i*sasize.y*2 + j][1], tmp);
			}
		}
	}
}


uint8_t *SimulCam::simul_capture(gsl_matrix *frame_in) {
	// Convert frame to uint8_t, scale properly
	double min=0, max=0, fac;
	gsl_matrix_minmax(frame_in, &min, &max);
	fac = 255.0/(max-min);
	
	size_t cursize = frame_in->size1 * frame_in->size2  * (sizeof(uint8_t));
	if (out_size != cursize) {
		io.msg(IO_DEB2, "SimulCam::simul_capture() reallocing memory, %zu != %zu", out_size, cursize);
		out_size = cursize;
		frame_out = (uint8_t *) realloc(frame_out, out_size);
	}
	
	// Copy and scale
	double pix=0;
	for (size_t i=0; i<frame_in->size1; i++)
		for (size_t j=0; j<frame_in->size2; j++) {
			pix = (double) ((gsl_matrix_get(frame_in, i, j) - min)*fac + drand48()*noise);
			frame_out[i*frame_in->size2 + j] = (uint8_t) clamp(((pix * exposure) + offset), 0.0, 1.0*UINT8_MAX);
		}
	//((wave_in->data[i*frame_in->tda + j]
	
	return frame_out;
}


SimulCam::~SimulCam() {
	//! @todo stop simulator etc.
	io.msg(IO_DEB2, "SimulCam::~SimulCam()");
	cam_thr.cancel();
	cam_thr.join();
	
	mode = Camera::OFF;
}

// From Camera::
void SimulCam::cam_set_exposure(double value) {
	pthread::mutexholder h(&cam_mutex);
	exposure = value;
}

double SimulCam::cam_get_exposure() {
	pthread::mutexholder h(&cam_mutex);
	return exposure;
}

void SimulCam::cam_set_interval(double value) {
	pthread::mutexholder h(&cam_mutex);
	interval = value;
}

double SimulCam::cam_get_interval() {
	return interval;
}

void SimulCam::cam_set_gain(double value) {
	pthread::mutexholder h(&cam_mutex);
	gain = value;
}

double SimulCam::cam_get_gain() {
	return gain;
}

void SimulCam::cam_set_offset(double value) {
	pthread::mutexholder h(&cam_mutex);
	offset = value;
}

double SimulCam::cam_get_offset() {	
	return offset;
}

void SimulCam::cam_handler() { 
	pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);
	//! @todo make this Mac compatible
#ifndef __APPLE__
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread::setaffinity(&cpuset);
#endif
	
	while (true) {
		//! @todo Should mutex lock each time reading mode, or is this ok?
		switch (mode) {
			case Camera::RUNNING:
			{
				// get_wavefront() allocates new memory, we need to free it later
				gsl_matrix *wf = seeing.get_wavefront();
				simul_telescope(wf);
				simul_wfs(wf);
				uint8_t *frame = simul_capture(wf);
				
				// Need to free gsl matrix if it is returned
				gsl_matrix *ret = (gsl_matrix *) cam_queue(wf, frame);
				if (ret)
					gsl_matrix_free(ret);
				
				usleep(interval * 1000000);
				break;
			}
			case Camera::SINGLE:
			{
				io.msg(IO_DEB1, "SimulCam::cam_handler() SINGLE");

				gsl_matrix *wf = seeing.get_wavefront();
				simul_telescope(wf);
				simul_wfs(wf);
				uint8_t *frame = simul_capture(wf);
				
				// Need to free gsl matrix if it is returned
				gsl_matrix *ret = (gsl_matrix *) cam_queue(wf, frame);
				if (ret)
					gsl_matrix_free(ret);
				
				usleep(interval * 1000000);

				mode = Camera::WAITING;
				break;
			}
			case Camera::OFF:
			case Camera::WAITING:
			case Camera::CONFIG:
			default:
				io.msg(IO_INFO, "SimulCam::cam_handler() OFF/WAITING/UNKNOWN.");
				// We wait until the mode changed
				mode_mutex.lock();
				mode_cond.wait(mode_mutex);
				mode_mutex.unlock();
				break;
		}
	}
}

void SimulCam::cam_set_mode(mode_t newmode) {
	if (newmode == mode)
		return;
	
	switch (newmode) {
		case Camera::RUNNING:
		case Camera::SINGLE:
			// Start camera
			mode = newmode;
			mode_cond.broadcast();
			break;
		case Camera::WAITING:
		case Camera::OFF:
			// Stop camera
			mode = newmode;
			mode_cond.broadcast();
			break;
		case Camera::CONFIG:
			io.msg(IO_INFO, "SimSeeing::cam_set_mode(%s) mode not supported.", mode2str(newmode).c_str());
		default:
			io.msg(IO_WARN, "SimSeeing::cam_set_mode(%s) mode unknown.", mode2str(newmode).c_str());
			break;
	}
}

void SimulCam::do_restart() {
	io.msg(IO_WARN, "SimSeeing::do_restart() not implemented yet.");
}
