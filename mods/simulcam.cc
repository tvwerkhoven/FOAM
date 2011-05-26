/*
 simulcam.cc -- atmosphere/telescope simulator camera
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <unistd.h>
#include <stdint.h>
#include <gsl/gsl_matrix.h>
#include <math.h>
#include <fftw3.h>

#include "io.h"
#include "config.h"
#include "pthread++.h"

#include "devices.h"
#include "simseeing.h"
#include "simulwfc.h"
#include "camera.h"
#include "simulcam.h"

using namespace std;

SimulCam::SimulCam(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, SimulWfc &simwfc, const bool online):
Camera(io, ptc, name, simulcam_type, port, conffile, online),
seeing(io, ptc, name + "-seeing", port, conffile),
simwfc(simwfc),
out_size(0), frame_out(NULL), telradius(1.0), telapt(NULL), telapt_fill(0.7),
do_simtel(true), do_simmla(true), do_simwfc(true),
shwfs(io, ptc, name + "-shwfs", port, conffile, *this, false)
{
	io.msg(IO_DEB2, "SimulCam::SimulCam()");
	// Register network commands with base device:
	add_cmd("get noise");
	add_cmd("set noise");
	add_cmd("get noiseamp");
	add_cmd("set noiseamp");
	add_cmd("get seeingfac");
	add_cmd("set seeingfac");
	add_cmd("get windspeed");
	add_cmd("set windspeed");
	add_cmd("get windtype");
	add_cmd("set windtype");
	add_cmd("get telapt_fill");
	add_cmd("set telapt_fill");
	add_cmd("set simwf"); // Alias for 'seeingfac'. If this is 0, wf is multiplied by 0
	add_cmd("set simtel");
	add_cmd("set simmla");
	add_cmd("set simwfc");

	noise = cfg.getdouble("noise", 0.2);
	noiseamp = cfg.getdouble("noiseamp", 0.2);
	seeingfac = cfg.getdouble("seeingfac", 1.0);
	
	if (seeing.cropsize.x != res.x || seeing.cropsize.y != res.y)
		throw std::runtime_error("SimulCam::SimulCam(): Camera resolution and seeing cropsize must be equal.");
	if (simwfc.wfc_sim->size1 != res.x || simwfc.wfc_sim->size2 != res.y)
		throw std::runtime_error("SimulCam::SimulCam(): Camera resolution and simulated wfc size must be equal.");
	
	// Get telescope aperture
	gen_telapt();
	
	cam_thr.create(sigc::mem_fun(*this, &SimulCam::cam_handler));
}


SimulCam::~SimulCam() {
	io.msg(IO_DEB2, "SimulCam::~SimulCam()");
	cam_set_mode(Camera::OFF);
	
	cam_thr.cancel();
	cam_thr.join();
	
	if (frame_out)
		free(frame_out);
}

void SimulCam::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "set") {							// set ...
		string what = popword(line);
	
		if(what == "noise") {							// set noise <double>
			set_var(conn, "noise", popdouble(line), &noise, 0.0, 1.0, "Out of range");
		} else if(what == "noiseamp") {		// set noiseamp <double>
			set_var(conn, "noiseamp", popdouble(line), &noiseamp);
		} else if(what == "telapt_fill") { // set telapt_fill <double>
			set_var(conn, "telapt_fill", popdouble(line), &telapt_fill, 0.0, 1.0, "out of range");
		} else if(what == "seeingfac") {	// set seeingfac <double>
			set_var(conn, "seeingfac", popdouble(line), &seeingfac);
		} else if(what == "windspeed") {	// set windspeed <int> <int>
			int tmpx = popint(line);
			int tmpy = popint(line);
			// Only accept in certain ranges (sanity check)
			if (fabs(tmpx) > res.x/2 || fabs(tmpy) > res.y/2)
				conn->write("error windspeed :values out of range");
			else {
				conn->addtag("windspeed");
				seeing.windspeed.x = tmpx;
				seeing.windspeed.y = tmpy;
				netio.broadcast(format("ok windspeed %d %d", seeing.windspeed.x, seeing.windspeed.y), "windspeed");
			}
		} else if(what == "windtype") {		// set windtype <string>
			conn->addtag("windtype");
			string tmp = popword(line);
			if (tmp == "linear")
				seeing.windtype = SimSeeing::LINEAR;
			else if (tmp == "random")
				seeing.windtype = SimSeeing::RANDOM;
			else {
				tmp = "drifting"; // default case, set tmp to 'drifting' for return message below
				seeing.windtype = SimSeeing::DRIFTING;
			}
			
			netio.broadcast(format("ok windtype %s", tmp.c_str()), "windtype");
		} else if(what == "simwf") {			// set simwfs <bool>
			set_var(conn, "simwf", popdouble(line), &seeingfac);
		} else if(what == "simtel") {			// set simtel <bool>
			set_var(conn, "simtel", popbool(line), &do_simtel);
		} else if(what == "simmla") {			// set simmla <bool>
			set_var(conn, "simmla", popbool(line), &do_simmla);
		} else if(what == "simwfc") {			// set simwfc <bool>
			set_var(conn, "simwfc", popbool(line), &do_simwfc);
		} else
			parsed = false;
	} else if (command == "get") {			// get ...
		string what = popword(line);
	
		if(what == "noise") {							// get noise
			get_var(conn, "noise", noise);
		} else if(what == "noiseamp") {		// get noiseamp
			get_var(conn, "noiseamp", noiseamp);
		} else if(what == "seeingfac") {	// get seeingfac
			get_var(conn, "seeingfac", seeingfac);
		} else if(what == "windspeed") {	// get windspeed
			get_var(conn, "windspeed", format("ok windspeed %x %x", seeing.windspeed.x, seeing.windspeed.y));
		} else
			parsed = false;
	}
	else
		parsed = false;
	
	// If not parsed here, call parent
	if (parsed == false)
		Camera::on_message(conn, orig);
}

void SimulCam::gen_telapt() {
	io.msg(IO_XNFO, "SimulCam::gen_telapt(): init");
	
	if (!telapt) {							// Doesn't exist, callocate (all zeros is goed)
		telapt = gsl_matrix_calloc(res.y, res.x);
	}
	else if ((int) telapt->size1 != res.x || (int) telapt->size2 != res.y) { // Wrong size, re-allocate
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

gsl_matrix *SimulCam::simul_seeing() {
	gsl_matrix *wf = seeing.get_wavefront(seeingfac);
	return wf;
}

void SimulCam::simul_telescope(gsl_matrix *im_in) const {
	if (!do_simtel)
		return;
	
	io.msg(IO_DEB2, "SimulCam::simul_telescope()");
	// Multiply wavefront with aperture
	gsl_matrix_mul_elements (im_in, telapt);
}

void SimulCam::simul_wfc(gsl_matrix *wave_in) const {
	if (!do_simwfc)
		return;
	
	io.msg(IO_DEB2, "SimulCam::simul_wfc()");
	// Add wfc correction to input
	gsl_matrix_add(wave_in, simwfc.wfc_sim);
}
	
void SimulCam::simul_wfs(gsl_matrix *wave_in) const {
	if (!do_simmla)
		return;
	
	if (shwfs.mlacfg.size() <= 0) {
		io.msg(IO_WARN, "SimulCam::simul_wfs(): no microlenses defined?.");
		return;
	}
	
	io.msg(IO_DEB2, "SimulCam::simul_wfs()");
	
	// Apply fourier transform to subimages here
	coord_t sallpos(shwfs.mlacfg[0].lx, 
									shwfs.mlacfg[0].ly);
	coord_t sasize(shwfs.mlacfg[0].tx - shwfs.mlacfg[0].lx,
								 shwfs.mlacfg[0].ty - shwfs.mlacfg[0].ly);
	
	// Get temporary memory
	static gsl_vector *workspace = NULL;
	if (!workspace) workspace = (gsl_vector *) gsl_vector_calloc(sasize.x * sasize.y * 4);
	
	// Setup FFTW parameters
	static fftw_complex *shdata = NULL;
	if (!shdata) shdata = (fftw_complex *) fftw_malloc(sasize.y*2 * sasize.x*2 * sizeof *shdata);
	
	// Set memory to 0
	for (int i=0; i< sasize.y*2 * sasize.x*2; i++)
		shdata[i][0] = shdata[i][1] = 0.0;

	// This is deleted at the end of the routine
	fftw_plan shplan = fftw_plan_dft_2d(sasize.y*2, sasize.x*2, shdata, shdata, FFTW_FORWARD, FFTW_MEASURE);
	double tmp=0;
	// Temporary matrices
	gsl_matrix_view subap;
	gsl_matrix *subapm;
	gsl_matrix_view telapt_crop;
	gsl_matrix *telapt_cropm;
	
	
	for (size_t n=0; n<shwfs.mlacfg.size(); n++) {
		sallpos.x = shwfs.mlacfg[n].lx;
		sallpos.y = shwfs.mlacfg[n].ly;
		sasize.x = shwfs.mlacfg[n].tx - shwfs.mlacfg[n].lx;
		sasize.y = shwfs.mlacfg[n].ty - shwfs.mlacfg[n].ly;

		// Crop out subaperture from larger frame, store as gsl_matrix_view
		subap = gsl_matrix_submatrix(wave_in, sallpos.y, sallpos.x, sasize.y, sasize.x);
		subapm = &(subap.matrix);
		
		// Check if this subaperture is within the bounds of the telescope 
		// aperture for at least telapt_fill. All values of telapt are either 0 or 
		// seeingfac, see gen_telapt(). If the sum is higher than telapt_fill *
		// sasize.y * sasize.x * seeingfac, accept this subaperture. Otherwise set
		// to zero.
		telapt_crop = gsl_matrix_submatrix(telapt, sallpos.y, sallpos.x, sasize.y, sasize.x);
		telapt_cropm = &(telapt_crop.matrix);
		
		double tmp_sum = 0.0;
		for (size_t i=0; i<telapt_cropm->size1; i++)
			for (size_t j=0; j<telapt_cropm->size2; j++)
				tmp_sum += gsl_matrix_get (telapt_cropm, i, j);

		if (tmp_sum < telapt_fill * sasize.y * sasize.x) {
			for (size_t i=0; i<subapm->size1; i++)
				for (size_t j=0; j<subapm->size2; j++)
					gsl_matrix_set (subapm, i, j, 0.0);
			continue;
		}
		
		if ((int) workspace->size != sasize.x * sasize.y * 4) {
			// Re-alloc data if necessary (should be sasize, but this value can vary per subap)
			io.msg(IO_WARN, "SimulCam::simul_wfs() subap sizes unequal, re-allocating. Support might be flaky.");
			// Re-alloc data
			gsl_vector_free(workspace);
			workspace = gsl_vector_calloc(sasize.x * sasize.y * 4);
			// Re-alloc FFTW
			fftw_destroy_plan(shplan);
			fftw_free(shdata);
			shdata = (fftw_complex *) fftw_malloc(sasize.y*2 * sasize.x*2 * sizeof *shdata);
			// Set memory to 0
			for (int i=0; i< sasize.y*2 * sasize.x*2; i++)
				shdata[i][0] = shdata[i][1] = 0.0;
			shplan = fftw_plan_dft_2d(sasize.y*2, sasize.x*2, shdata, shdata, FFTW_FORWARD, FFTW_MEASURE);
		}

		// Set FFTW data to zero, otherwise residuals will be FFT'ed again later
		for (int i=0; i< sasize.y*2 * sasize.x*2; i++)
			shdata[i][0] = shdata[i][1] = 0.0;

		// Copy input wavefront data (subapm) to linear array (shdata) for FFT, 
		// with zero padding around the image.
		for (int i=0; i<sasize.y; i++)
			for (int j=0; j<sasize.x; j++) {
				// Convert real wavefront to complex EM wave E \propto exp(-i phi) = cos(phi) + i sin(phi)
				tmp = gsl_matrix_get(subapm, i, j);
				shdata[(i+sasize.y/2)*sasize.x*2 + (j+sasize.x/2)][0] = cos(tmp);
				shdata[(i+sasize.y/2)*sasize.x*2 + (j+sasize.x/2)][1] = sin(tmp);
				//fprintf(stderr, "%g, cos: %g, \n", tmp, cos(tmp));
			}
		
		// Execute this FFT
		fftw_execute(shplan);
		
		// now calculate the absolute squared value of the FFT output (i.e. the 
		// power), store it in the subapm matrix. This is the image we see on the 
		// screen (after adding noise etc.)
		for (int i=sasize.y*1.5; i<sasize.y*2.5; i++) {
			for (int j=sasize.x*1.5; j<sasize.x*2.5; j++) {
				// Calculate abs(E^2), store in original memory (per quadrant).
				// We need to move the data because we want the origin of the FFT to 
				// be in the center of the matrix.
				
				// Bottom-left FFT quadrant goes to top-right IMG quadrant
				tmp = fabs(pow(shdata[(i % (sasize.y*2))*sasize.x*2 + (j % (sasize.x*2))][0], 2) + 
									 pow(shdata[(i % (sasize.y*2))*sasize.x*2 + (j % (sasize.x*2))][1], 2));
				gsl_matrix_set(subapm, ((i - sasize.y/2) % sasize.y), ((j - sasize.x/2) % sasize.x), tmp);
				//fprintf(stderr, "out: abs(%g^2+%g^2) = %g\n", shdata[i*sasize.y*2 + j][0], shdata[i*sasize.y*2 + j][1], tmp);
			}
		}
		//! @bug The original input wavefront (wave_in) is never set to zero everywhere, it is only overwritten with FFT'ed data in the subapertures, not in between the subapertures.
	}
	
	fftw_destroy_plan(shplan);
}


uint8_t *SimulCam::simul_capture(gsl_matrix *frame_in) {
	io.msg(IO_DEB2, "SimulCam::simul_capture()");
	// Convert frame to uint8_t, scale properly
	double min=0, max=0, noisei=0, fac;
	gsl_matrix_minmax(frame_in, &min, &max);
	fac = 255.0/(max-min);
	
	size_t cursize = frame_in->size1 * frame_in->size2  * sizeof *frame_out;
	if (out_size != cursize) {
		io.msg(IO_DEB2, "SimulCam::simul_capture() reallocing memory, %zu != %zu", out_size, cursize);
		out_size = cursize;
		frame_out = (uint8_t *) realloc(frame_out, out_size);
	}
	
	// Copy and scale, add noise
	double pix=0.0, noise=0.0;
	for (size_t i=0; i<frame_in->size1; i++) {
		for (size_t j=0; j<frame_in->size2; j++) {
			pix = (double) ((gsl_matrix_get(frame_in, i, j) - min)*fac);
			noise=0.0;
			// Add noise only in 'noise' fraction of the pixels, with 'noiseamp' amplitude. Noise is independent of exposure here
			if (drand48() < noise) 
				noisei = drand48() * noiseamp * UINT8_MAX;
			frame_out[i*frame_in->size2 + j] = (uint8_t) clamp(((pix * exposure) + noisei + offset) * gain, 0.0, 1.0*UINT8_MAX);
		}
	}
	
	return frame_out;
}

// From Camera::
void SimulCam::cam_set_exposure(const double value) {
	pthread::mutexholder h(&cam_mutex);
	exposure = value;
}

double SimulCam::cam_get_exposure() {
	pthread::mutexholder h(&cam_mutex);
	return exposure;
}

void SimulCam::cam_set_interval(const double value) {
	pthread::mutexholder h(&cam_mutex);
	interval = value;
}

double SimulCam::cam_get_interval() {
	return interval;
}

void SimulCam::cam_set_gain(const double value) {
	pthread::mutexholder h(&cam_mutex);
	gain = value;
}

double SimulCam::cam_get_gain() {
	return gain;
}

void SimulCam::cam_set_offset(const double value) {
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
	// pthread::setaffinity(&cpuset);
#endif
	
	while (true) {
		//! @todo Should mutex lock each time reading mode, or is this ok?
		switch (mode) {
			case Camera::RUNNING:
			{
				gsl_matrix *wf = simul_seeing();
				simul_wfc(wf);
				simul_telescope(wf);
				simul_wfs(wf);
				uint8_t *frame = simul_capture(wf);
				
				// Need to free gsl matrix if it is returned
				//! @todo We use memory for wf and for frame. This should both be freed when returned. 
				//! @todo 'frame' is the same memory each time, so this does not really queue a *new* image, it's the same memory.
				gsl_matrix *ret = (gsl_matrix *) cam_queue(wf, frame);
				if (ret)
					gsl_matrix_free(ret);
				
				usleep(interval * 1000000);
				break;
			}
			case Camera::SINGLE:
			{
				io.msg(IO_DEB1, "SimulCam::cam_handler() SINGLE");

				gsl_matrix *wf = simul_seeing();
				simul_wfc(wf);
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

void SimulCam::cam_set_mode(const mode_t newmode) {
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
