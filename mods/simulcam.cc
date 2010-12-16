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

gsl_matrix *SimulCam::simul_telescope(gsl_matrix *im_in) {
	io.msg(IO_DEB2, "SimulCam::simul_telescope()");
	// Multiply wavefront with aperture
	gsl_matrix_mul_elements (im_in, telapt);
	return im_in;
}

uint8_t *SimulCam::simul_wfs(gsl_matrix *wave_in) {
	if (!shwfs)
		io.msg(IO_ERR | IO_FATAL, "SimulCam::simul_wfs(): cannot simulate wavefront without Shwfs reference.");
	
	if (shwfs->mla.nsi <= 0) {
		io.msg(IO_WARN, "SimulCam::simul_wfs(): no microlenses defined?.");
		return NULL;
	}
	
	io.msg(IO_DEB2, "SimulCam::simul_wfs()");
	//! @todo Given a wavefront, image it through a system and return the resulting intensity pattern (i.e. an image).
	double min=0, max=0, fac;
	gsl_matrix_minmax(wave_in, &min, &max);
	fac = 255.0/(max-min);
	
	// Apply fourier transform to subimages here
	coord_t sallpos = shwfs->mla.ml[0].llpos;
	coord_t sasize = shwfs->mla.ml[0].size;
	// Get temporary memory
	gsl_vector *workspace = gsl_vector_calloc(sasize.x * sasize.y * 4);
	// Setup FFTW parameters
	fftw_complex *shout = (fftw_complex *) fftw_malloc(sasize.y*2 * sasize.x*2 * sizeof(fftw_complex));
	fftw_plan shplan = fftw_plan_dft_r2c_2d(sasize.y*2, sasize.x*2,
																					workspace->data, shout, FFTW_MEASURE);
	
	for (int n=0; n<shwfs->mla.nsi; n++) {
		sallpos = shwfs->mla.ml[n].llpos;
		sasize = shwfs->mla.ml[n].size;
		io.msg(IO_DEB2, "SimulCam::simul_wfs() FFT @ %d: (%d,%d)", n, sallpos.x, sallpos.y);
		
		gsl_matrix_view subap = gsl_matrix_submatrix(wave_in, sallpos.y, sallpos.x, sasize.y, sasize.x);
		gsl_matrix *subapm = &(subap.matrix);
		
		gsl_matrix_scale (subapm, 2.0);
		
		// Re-alloc data if necessary (should be sasize, but this can vary per subap)
		if (workspace->size != sasize.x * sasize.y * 4) {
			io.msg(IO_WARN, "SimulCam::simul_wfs() subap sizes unequal, re-allocating. Support might be flaky.");
			gsl_vector_free(workspace);
			workspace = gsl_vector_calloc(sasize.x * sasize.y * 4);
		}

		// Copy data to linear array for FFT, with padding
		for (int i=0; i<sasize.y; i++)
			for (int j=0; j<sasize.x; j++)
				gsl_vector_set(workspace, (i+sasize.y/2)*2*sasize.x + (j+sasize.x), gsl_matrix_get(subapm, i, j));
		

	}
		
		
	
	// we take the subaperture, which is shsize.x * .y big, and put it in a larger matrix
	/*
	nx = (shwfs->shsize.x * 2);
	ny = (shwfs->shsize.y * 2);
	
	// init data structures for images, fill with zeroes
	if (shin == NULL) {
		simparams->shin = fftw_malloc ( sizeof ( fftw_complex ) * nx * ny );
		for (i=0; i< nx*ny; i++)
			simparams->shin[i][0] = simparams->shin[i][1] = 0.0;
	}
	
	if (simparams->shout == NULL) {
		simparams->shout = fftw_malloc ( sizeof ( fftw_complex ) * nx * ny );
		for (i=0; i< nx*ny; i++)
			simparams->shout[i][0] = simparams->shout[i][1] = 0.0;
	}
	
	// init FFT plan, measure how we can compute an FFT the fastest on this machine
	if (simparams->plan_forward == NULL) {
		logDebug(0, "Setting up plan for fftw");
		
		fp = fopen(simparams->wisdomfile,"r");
		if (fp == NULL)	{				// File does not exist, generate new plan
			logInfo(0, "No FFTW plan found in %s, generating new plan, this may take a while.", simparams->wisdomfile);
			simparams->plan_forward = fftw_plan_dft_2d(nx, ny, simparams->shin, simparams->shout, FFTW_FORWARD, FFTW_EXHAUSTIVE);
			
			// Open file for writing now
			fp = fopen(simparams->wisdomfile,"w");
			if (fp == NULL) {
				logDebug(0, "Could not open file %s for saving FFTW wisdom.", simparams->wisdomfile);
				return EXIT_FAILURE;
			}
			fftw_export_wisdom_to_file(fp);
			fclose(fp);
		}
		else {
			logInfo(0, "Importing FFTW wisdom file.");
			logInfo(0, "If this is the first time this program runs on this machine, this is bad.");
			logInfo(0, "In that case, please delete '%s' and rerun the program. It will generate new wisdom which is A Good Thing.", \
							simparams->wisdomfile);
			if (fftw_import_wisdom_from_file(fp) == 0) {
				logWarn("Importing wisdom failed.");
				return EXIT_FAILURE;
			}
			// regenerating plan using wisdom imported above.
			simparams->plan_forward = fftw_plan_dft_2d(nx, ny, simparams->shin, simparams->shout, FFTW_FORWARD, FFTW_EXHAUSTIVE);
			fclose(fp);
		}
	}

	// now we loop over each subaperture:
	for (size_t n=0; n<shwfs->mla.nsi; n++) {
		io.msg(IO_DEB2, "SimulCam::simul_wfs() FFT @ %d: (%d,%d)", n, shwfs->mla.ml[n].pos.x, shwfs->mla.ml[n].pos.y);
	
//	for (yc=0; yc< shwfs->cells.y; yc++) {
//		for (xc=0; xc< shwfs->cells.x; xc++) {
		// we're at subapt (xc, yc) here...
		
		// possible approaches on subapt selection for simulation:
		//  - select only central apertures (use radius)
		//  - use absolute intensity (still partly illuminated apts)
		//  - count pixels with intensity zero
			
		zeropix = 0;
		for (ip=0; ip< shwfs->shsize.y; ip++)
			for (jp=0; jp< shwfs->shsize.x; jp++)
				if (simparams->currimg[yc * shwfs->shsize.y * simparams->currimgres.x + xc*shwfs->shsize.x + ip * simparams->currimgres.x + jp] == 0)
					zeropix++;
		
		// allow one quarter of the pixels to be zero, otherwise set subapt to zero and continue
		if (zeropix > shwfs->shsize.y*shwfs->shsize.x/4) {
			for (ip=0; ip<shwfs->shsize.y; ip++)
				for (jp=0; jp<shwfs->shsize.x; jp++)
					simparams->currimg[yc*shwfs->shsize.y*simparams->currimgres.x + xc*shwfs->shsize.x + ip*simparams->currimgres.x + jp] = 0;
			
			// skip over the rest of the for loop started ~20 lines back
			continue;
		}
		
		// We want to set the helper arrays to zero first
		// otherwise we get trouble? TODO: check this out
		for (i=0; i< nx*ny; i++)
			simparams->shin[i][0] = simparams->shin[i][1] = \
			simparams->shout[i][0] = simparams->shout[i][1] = 0.0;
		//			for (i=0; i< nx*ny; i++)
		
		// add markers to track copying:
		//for (i=0; i< 2*nx; i++)
		//simparams->shin[i][0] = 1;
		
		//for (i=0; i< ny; i++)
		//simparams->shin[nx/2+i*nx][0] = 1;
		
		// loop over all pixels in the subaperture, copy them to subapt:
		// I'm pretty sure these index gymnastics are correct (2008-01-18)
		for (ip=0; ip < shwfs->shsize.y; ip++) { 
			for (jp=0; jp < shwfs->shsize.x; jp++) {
				// we need the ipth row PLUS the rows that we skip at the top (shwfs->shsize.y/2+1)
				// the width is not shwfs->shsize.x but twice that plus 2, so mulyiply the row number
				// with that. Then we need to add the column number PLUS the margin at the beginning 
				// which is shwfs->shsize.x/2 + 1, THAT's the right subapt location.
				// For the image itself, we need to skip to the (ip,jp)th subaperture,
				// which is located at pixel yc * the height of a cell * the width of the picture
				// and the x coordinate times the width of a cell time, that's at least the first
				// subapt pixel. After that we add the subaperture row we want which is located at
				// pixel ip * the width of the whole image plus the x coordinate
				
				simparams->shin[(ip+ ny/4)*nx + (jp + nx/4)][0] = \
				simparams->currimg[yc*shwfs->shsize.y*simparams->currimgres.x + xc*shwfs->shsize.x + ip*simparams->currimgres.x + jp];
				// old: simparams->shin[(ip+ shwfs->shsize.y/2 +1)*nx + jp + shwfs->shsize.x/2 + 1][0] = 
			}
		}
		
		// now image the subaperture, first generate EM wave amplitude
		// this has to be done using complex numbers over the BIG subapt
		// we know that exp ( i * phi ) = cos(phi) + i sin(phi),
		// so we split it up in a real and a imaginary part
		// TODO dit kan hierboven al gedaan worden
		for (ip=shwfs->shsize.y/2; ip<shwfs->shsize.y + shwfs->shsize.y/2; ip++) {
			for (jp=shwfs->shsize.x/2; jp<shwfs->shsize.x+shwfs->shsize.x/2; jp++) {
				tmp = simparams->seeingfac * simparams->shin[ip*nx + jp][0]; // multiply for worse seeing
																																		 //use fftw_complex datatype, i.e. [0] is real, [1] is imaginary
				
				// SPEED: cos and sin are SLOW, replace by taylor series
				// optimilization with parabola, see http://www.devmaster.net/forums/showthread.php?t=5784
				// and http://lab.polygonal.de/2007/07/18/fast-and-accurate-sinecosine-approximation/
				// wrap tmp to (-pi, pi):
				//tmp -= ((int) ((tmp+3.14159265)/(2*3.14159265))) * (2* 3.14159265);
				//simparams->shin[ip*nx + jp][1] = 1.27323954 * tmp -0.405284735 * tmp * fabs(tmp);
				// wrap tmp + pi/2 to (-pi,pi) again, but we know tmp is already in (-pi,pi):
				//tmp += 1.57079633;
				//tmp -= (tmp > 3.14159265) * (2*3.14159265);
				//simparams->shin[ip*nx + jp][0] = 1.27323954 * tmp -0.405284735 * tmp * fabs(tmp);
				
				// used to be:
				simparams->shin[ip*nx + jp][1] = sin(tmp);
				// TvW, disabling sin/cos
				simparams->shin[ip*nx + jp][0] = cos(tmp);
				//simparams->shin[ip*nx + jp][0] = tmp;
			}
		}
		
		// now calculate the  FFT
		fftw_execute ( simparams->plan_forward );
		
		// now calculate the absolute squared value of that, store it in the subapt thing
		// also find min and maximum here
		for (ip=0; ip<ny; ip++) {
			for (jp=0; jp<nx; jp++) {
				tmp = simparams->shin[ip*nx + jp][0] = \
				fabs(pow(simparams->shout[ip*nx + jp][0],2) + pow(simparams->shout[ip*nx + jp][1],2));
				if (tmp > max) max = tmp;
				else if (tmp < min) min = tmp;
			}
		}
		// copy subaparture back to main images
		// note: we don't want the center of the fourier transformed image, but we want all corners
		// because the FT begins in the origin. Therefore we need to start at coordinates
		//  nx-(nx_subapt/2), ny-(ny_subapt/2)
		// e.g. for 32x32 subapts and (nx,ny) = (64,64), we start at
		//  (48,48) -> (70,70) = (-16,-16)
		// so we need to wrap around the matrix, which results in the following
		float tmppixf;
		//uint8_t tmppixb;
		for (ip=ny/4; ip<ny*3/4; ip++) { 
			for (jp=nx/4; jp<nx*3/4; jp++) {
				tmppixf = simparams->shin[((ip+ny/2)%ny)*nx + (jp+nx/2)%nx][0];
				
				simparams->currimg[yc*shwfs->shsize.y*simparams->currimgres.x + xc*shwfs->shsize.x + (ip-ny/4)*simparams->currimgres.x + (jp-nx/4)] = 255.0*(tmppixf-min)/(max-min);
			}
		}
	} // end looping over subapts		

	
	//(coord_t res, coord_t size, coord_t pitch, int xoff, coord_t disp, int &nsubap);
	*/
	// Convert frame to uint8_t, scale properly
	size_t cursize = wave_in->size1 * wave_in->size2  * (sizeof(uint8_t));
	if (out_size != cursize) {
		io.msg(IO_DEB2, "SimulCam::simul_wfs() reallocing memory, %zu != %zu", out_size, cursize);
		out_size = cursize;
		frame_out = (uint8_t *) realloc(frame_out, out_size);
	}
	
	for (size_t i=0; i<wave_in->size1; i++)
		for (size_t j=0; j<wave_in->size2; j++)
			frame_out[i*wave_in->size2 + j] = (uint8_t) ((wave_in->data[i*wave_in->tda + j] - min)*fac);

	return frame_out;
}


void SimulCam::simul_capture(uint8_t *frame) {
	// Apply offset and exposure here
	for (size_t p=0; p<res.x*res.y; p++)
		frame[p] = (uint8_t) clamp((int) ((frame[p] * exposure) + offset), 0, UINT8_MAX);
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
				gsl_matrix_view wf = seeing.get_wavefront();
				io.msg(IO_DEB1, "SimulCam::cam_handler() RUNNING f@%p, f[100]: %g", 
							 wf.matrix.data, wf.matrix.data[100]);
				simul_telescope(&(wf.matrix));
				uint8_t *frame = simul_wfs(&(wf.matrix));
				
				simul_capture(frame);
				
				cam_queue(frame, frame);

				usleep(interval * 1000000);
				break;
			}
			case Camera::SINGLE:
			{
				io.msg(IO_DEB1, "SimulCam::cam_handler() SINGLE");

				gsl_matrix_view wf = seeing.get_wavefront();
				
				simul_telescope(&(wf.matrix));
				
				uint8_t *frame = simul_wfs(&(wf.matrix));

				//simul_capture(frame);
				
				cam_queue(frame, frame);
				
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
