/*
 shwfs.cc -- Shack-Hartmann utilities class
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
/*! 
	@file shwfs.ccc
	@author Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>

	@brief This file contains modules and functions related to Shack-Hartmann 
	wavefront sensing.
*/

#include <stdint.h>
#include <math.h>

#include "config.h"
#include "types.h"
#include "camera.h"
#include "io.h"
#include "path++.h"

#include "wfs.h"
#include "shwfs.h"
#include "shift.h"

// PUBLIC FUNCTIONS //
/********************/


Shwfs::Shwfs(Io &io, foamctrl *ptc, string name, string port, Path &conffile, Camera &wfscam, bool online):
Wfs(io, ptc, name, shwfs_type, port, conffile, wfscam, online),
shifts(io, 4),
method(Shift::COG)
{
	io.msg(IO_DEB2, "Shwfs::Shwfs()");
	add_cmd("mla generate");
	add_cmd("mla find");
//	add_cmd("mla store");
//	add_cmd("mla del");
//	add_cmd("mla add");
	add_cmd("get mla");
	add_cmd("set mla");
	add_cmd("calibrate");
	add_cmd("measure");
	
	// Micro lens array parameters:
	
	sisize.x = cfg.getint("sisizex", 16);
	sisize.y = cfg.getint("sisizey", 16);
	if (cfg.exists("sisize"))
		sisize.x = sisize.y = cfg.getint("sisize");
	
	sipitch.x = cfg.getint("sipitchx", 64);
	sipitch.y = cfg.getint("sipitchy", 64);
	if (cfg.exists("sipitch"))
		sipitch.x = sipitch.y = cfg.getint("sipitch");
	
	disp.x = cfg.getint("dispx", cam.get_width()/2);
	disp.y = cfg.getint("dispy", cam.get_height()/2);
	if (cfg.exists("disp"))
		disp.x = disp.y = cfg.getint("disp");
	
	overlap = cfg.getdouble("overlap", 0.5);
	xoff = cfg.getint("xoff", 0);
	
	string shapestr = cfg.getstring("shape", "square");
	if (shapestr == "circular")
		shape = CIRCULAR;
	else
		shape = SQUARE;
	
	// Other paramters:
	simaxr = cfg.getint("simaxr", -1);
	simini = cfg.getint("simini", 30);
	
	// Generate MLA grid
	gen_mla_grid(&mlacfg, cam.get_res(), sisize, sipitch, xoff, disp, shape, overlap);
	
	// Initial calibration
	calibrate();
}

Shwfs::~Shwfs() {
	io.msg(IO_DEB2, "Shwfs::~Shwfs()");
	if (mlacfg.ml)
		free(mlacfg.ml);
	
}

void Shwfs::on_message(Connection *conn, std::string line) {
	io.msg(IO_DEB1, "Shwfs::on_message('%s')", line.c_str()); 
	
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "mla") {
		string what = popword(line);

		if (what == "generate") {
			//! @todo get extra options from line
			gen_mla_grid(&mlacfg, cam.get_res(), sisize, sipitch, xoff, disp, shape, overlap);
		} else if(what == "find") {
			//! @todo get extra options from line
			find_mla_grid(&mlacfg, sisize, simini);
		} else if(what == "store") {
			//! @todo implement
		} else if(what == "del") {
			//! @todo implement
		} else if(what == "add") {
			//! @todo implement
		}
	} else if (command == "get") {
		string what = popword(line);
		
		if (what == "mla") {
			conn->addtag("mla");
			conn->write("ok mla " + get_mla_str());
		} else {
			parsed = false;
			//conn->write("error :Unknown argument " + what);
		}
	} else if (command == "set") {
		string what = popword(line);
		
		if(what == "mla") {
			conn->addtag("mla");
			if (set_mla_str(line))
				conn->write("error mla :Could not parse MLA string");
		} else {
			parsed = false;
			//conn->write("error :Unknown argument " + what);
		}
	} else if (command == "calibrate") {
		calibrate();
		conn->write("ok calibrate");
	} else if (command == "measure") {
		if (measure())
			conn->write("error measure :unknown error in measure()");
		else 
			conn->write("ok measure");
	} else {
		parsed = false;
		//conn->write("error :Unknown command: " + command);
	}
	
	// If not parsed here, call parent
	if (parsed == false)
		Wfs::on_message(conn, orig);
	
}

int Shwfs::measure() {
	if (!is_calib)
		return io.msg(IO_ERR, "Shwfs::measure() calibrate sensor first!");
	
	Camera::frame_t *tmp = cam.get_last_frame();

	if (!tmp)
		return io.msg(IO_ERR, "Shwfs::measure() couldn't get frame, is camera running?");
	
	// Calculate shifts
	
	if (cam.get_depth() == 16) {
		io.msg(IO_DEB2, "Shwfs::measure() got UINT16");
		// Manually cast mlacfg.ml to Shift type, is the same (Shift has it's own types such that it does not depend on other files too much)
		//! @todo Include Shift::crop_t in types.h (?)
		//shifts.calc_shifts((uint16_t *) tmp->image, cam.get_res(), (Shift::crop_t *) mlacfg.ml, mlacfg.nsi, shift_vec, method);
	}
	else if (cam.get_depth() == 8) {
		io.msg(IO_DEB2, "Shwfs::measure() got UINT8");
		shifts.calc_shifts((uint8_t *) tmp->image, cam.get_res(), (Shift::crop_t *) mlacfg.ml, mlacfg.nsi, shift_vec, method);
	}
	else
		return io.msg(IO_ERR, "Shwfs::measure() unknown datatype");
	
	// Convert shifts to basisfunction
	shift_to_basis(shift_vec, wf.basis, wf.wfamp);
	
	return 0;
}

int Shwfs::shift_to_basis(gsl_vector_float *invec, wfbasis basis, gsl_vector_float *outvec) {
	switch (basis) {
		case SENSOR:
			gsl_vector_float_memcpy(outvec, invec);
			break;
		case ZERNIKE:
		case KL:
		case MIRROR:
		case UNDEFINED:
		default:
			break;
	}
	
	return 0;
	
}
int Shwfs::calibrate() {
	//! @todo Needs to setup Zernike & KL & mirror basis functions as well
	shift_vec = gsl_vector_float_alloc(mlacfg.nsi * 2);
	
	switch (wf.basis) {
		case SENSOR:
			io.msg(IO_XNFO, "Shwfs::calibrate(): Calibrating for basis 'SENSOR'");
			wf.nmodes = mlacfg.nsi * 2;
			wf.wfamp = gsl_vector_float_alloc(wf.nmodes);
			break;
		case ZERNIKE:
		case KL:
		case MIRROR:
		case UNDEFINED:
		default:
			io.msg(IO_WARN, "Shwfs::calibrate(): This basis is not implemented yet");
			return -1;
			break;
	}
	
	is_calib = true;
	return 0;
}


int Shwfs::gen_mla_grid(sh_mla_t *mla, coord_t res, coord_t size, coord_t pitch, int xoff, coord_t disp, mlashape_t shape, float overlap) {
	io.msg(IO_DEB2, "Shwfs::gen_mla_grid()");
	
	is_calib = false;

	// How many subapertures would fit in the requested size 'res':
	int sa_range_y = (int) ((res.y/2)/pitch.x + 1);
	int sa_range_x = (int) ((res.x/2)/pitch.y + 1);
	
	mla->nsi = 0;
	
	float minradsq = pow(((double) min(res.x, res.y))/2.0, 2);
	
	coord_t sa_c;
	// Loop over potential subaperture positions 
	for (int sa_y=-sa_range_y; sa_y < sa_range_y; sa_y++) {
		for (int sa_x=-sa_range_x; sa_x < sa_range_x; sa_x++) {
			// Centroid position of current subap is 
			sa_c.x = sa_x * pitch.x;
			sa_c.y = sa_y * pitch.y;
			
			// Offset odd rows: 'sa_y % 2' gives 0 for even rows and 1 for odd rows. 
			// Use this to apply a row-offset to even and odd rows
			sa_c.x -= (sa_y % 2) * xoff * pitch.x;
			
			if (shape == CIRCULAR) {
				if (pow(fabs(sa_c.x) + (overlap-0.5)*size.x, 2) + pow(fabs(sa_c.y) + (overlap-0.5)*size.y, 2) < minradsq) {
//				io.msg(IO_DEB2, "Shwfs::gen_mla_grid(): Found subap within bounds @ (%d, %d)", sa_c.x, sa_c.y);
					(mla->nsi)++;
					mla->ml = (sh_simg_t *) realloc((void *) mla->ml, mla->nsi * sizeof *(mla->ml));
					mla->ml[mla->nsi-1].llpos.x = sa_c.x + disp.x - size.x/2;
					mla->ml[mla->nsi-1].llpos.y = sa_c.y + disp.y - size.y/2;
					mla->ml[mla->nsi-1].size.x = size.x;
					mla->ml[mla->nsi-1].size.y = size.y;
				}
			}
			else {
				// Accept a subimage coordinate (center position) the position + 
				// half-size the subaperture is within the bounds
				if ((fabs(sa_c.x + (overlap-0.5)*size.x) < res.x/2) && (fabs(sa_c.y + (overlap-0.5)*size.y) < res.y/2)) {
//					io.msg(IO_DEB2, "Shwfs::gen_mla_grid(): Found subap within bounds.");
					(mla->nsi)++;
					mla->ml = (sh_simg_t *) realloc((void *) mla->ml, mla->nsi * sizeof *(mla->ml));
					mla->ml[mla->nsi-1].llpos.x = sa_c.x + disp.x - size.x/2;
					mla->ml[mla->nsi-1].llpos.y = sa_c.y + disp.y - size.y/2;
					mla->ml[mla->nsi-1].size.x = size.x;
					mla->ml[mla->nsi-1].size.y = size.y;
				}
			}
		}
	}
	io.msg(IO_XNFO, "Shwfs::gen_mla_grid(): Found %d subapertures.", mla->nsi);
	
	return mla->nsi;
}

bool Shwfs::store_mla_grid(Path &f, bool overwrite) {
	return store_mla_grid(mlacfg, f, overwrite);
}

bool Shwfs::store_mla_grid(sh_mla_t mla, Path &f, bool overwrite) {
	if (f.exists() && !overwrite) {
		io.msg(IO_WARN, "Shwfs::store_mla_grid(): Cannot store MLA grid, file exists.");
		return false;
	}
	if (f.exists() && !f.isfile() && overwrite) {
		io.msg(IO_WARN, "Shwfs::store_mla_grid(): Cannot store MLA grid, path exists, but is not a file.");
		return false;
	}
	
	FILE *fd = f.fopen("w");
	if (fd == NULL) {
		io.msg(IO_ERR, "Shwfs::store_mla_grid(): Could not open file '%s'", f.c_str());
		return false;
	}
	
	fprintf(fd, "# Shwfs:: MLA definition\n");
	fprintf(fd, "# MLA definition, nsi=%d.\n", mla.nsi);
	fprintf(fd, "# Columns: llpos.x, llpos.y, cpos.x, cpos.y, size.x, size.y\n");
	for (int n=0; n<mla.nsi; n++) {
		fprintf(fd, "%d, %d, %d, %d\n", 
						mla.ml[n].llpos.x,
						mla.ml[n].llpos.y,
						mla.ml[n].size.x,
						mla.ml[n].size.y);
	}
	fclose(fd);
	
	io.msg(IO_INFO, "Shwfs::store_mla_grid(): Wrote MLA grid to '%s'.", f.c_str());
	io.msg(IO_XNFO | IO_NOID, "Plot these data in gnuplot with:");
	io.msg(IO_XNFO | IO_NOID, "set key");
	io.msg(IO_XNFO | IO_NOID, "set xrange[0:%d]", cam.get_width());
	io.msg(IO_XNFO | IO_NOID, "set yrange[0:%d]", cam.get_height());	
	io.msg(IO_XNFO | IO_NOID, "set obj 1 ellipse at first %d, first %d size %d,%d front fs empty lw 0.8",
				 cam.get_width()/2, cam.get_height()/2, cam.get_width(), cam.get_height());
	io.msg(IO_XNFO | IO_NOID, "plot 'mla_grid' using 1:2:5:6 title 'subap size' with vectors lt -1 lw 1 heads, 'mla_grid' using 3:4 title 'subap center'");
	
	return true;
}

int Shwfs::find_mla_grid(sh_mla_t *mla, coord_t size, int mini, int nmax, int iter) {
	io.msg(IO_DEB2, "Shwfs::find_mla_grid()");

	is_calib = false;

	// Store current camera count, get last frame
	size_t bcount = cam.get_count();
	Camera::frame_t *f = cam.get_last_frame();
	
	if (f == NULL) {
		io.msg(IO_WARN, "Shwfs::find_mla_grid() Could not get frame, is the camera running?");
		return NULL;
	}
	
	mla->nsi = 0;
	coord_t sapos;
	
	// Find maximum intensity pixels & set area around it to zero until there is 
	// no more maximum above mini or we have reached nmax subapertures
	while (true) {
		size_t maxidx = 0;
		int maxi = 0;
		
		if (cam.get_depth() <= 8) {
			uint8_t *image = (uint8_t *)f->image;
			maxi = _find_max(image, f->size, &maxidx);
		} else {
			uint16_t *image = (uint16_t *)f->image;
			maxi = _find_max(image, f->size/2, &maxidx);
		}
		
		// Intensity too low, done
		if (maxi < mini)
			break;
		
		// Add new subaperture
		(mla->nsi)++;
		mla->ml = (sh_simg_t *) realloc((void *) mla->ml, mla->nsi * sizeof *(mla->ml));

		sapos.x = (maxidx % cam.get_width()) - size.x/2;
		sapos.y = int(maxidx / cam.get_width()) - size.y/2;
		
		mla->ml[mla->nsi-1].llpos.x = sapos.x;
		mla->ml[mla->nsi-1].llpos.y = sapos.y;
		
		io.msg(IO_DEB2, "Shwfs::find_mla_grid(): new! I: %d, idx: %zu, llpos: (%d,%d)", maxi, maxidx, sapos.x, sapos.y);

		// Enough subapertures, done
		if (mla->nsi == nmax)
			break;
		
		// Set the current subaperture to zero
		int xran[] = {max(0, sapos.x), min(cam.get_width(), sapos.x+size.x)};
		int yran[] = {max(0, sapos.y), min(cam.get_height(), sapos.y+size.y)};
		
		if (cam.get_depth() <= 8) {
			uint8_t *image = (uint8_t *)f->image;
			for (int y=yran[0]; y<yran[1]; y++)
				for (int x=xran[0]; x<xran[1]; x++)
					image[y*cam.get_width() + x] = 0;
		} else {
			uint16_t *image = (uint16_t *)f->image;
			for (int y=yran[0]; y<yran[1]; y++)
				for (int x=xran[0]; x<xran[1]; x++)
					image[y*cam.get_width() + x] = 0;
		}
	}
	
	// Done! We have found the maximum intensities now.
		
	for (int it=1; it<iter; it++) {
		//! @todo implement iterative updates
		io.msg(IO_WARN, "Shwfs::find_mla_grid(): iter not yet implemented");
	}
	
	// Get latest camera count. If the difference between begin and end is 
	// bigger than the size of the camera ringbuffer, we were too slow
	size_t ecount = cam.get_count();
	if (ecount - bcount >= cam.get_bufsize()) {
		//! @todo This poses possible problems, what to do?
		io.msg(IO_WARN, "Shwfs::find_mla_grid(): got camera buffer overflow, data might be inaccurate");
	}
	
	return mla->nsi;
}

int Shwfs::mla_subapsel() {

	//! @todo implement this with generic MLA setup
	return 0;
}
 
// PRIVATE FUNCTIONS //
/*********************/

template <class T> 
int Shwfs::_find_max(T *img, size_t nel, size_t *idx) {
	int max=img[0];
	for (size_t p=0; p<nel; p++) {
		if (img[p] > max) {
			max = img[p];
			*idx = p;
		}
	}
	return max;	
}

string Shwfs::get_mla_str(sh_mla_t mla) {
	string ret = format("%d ", mla.nsi);
	
	for (int i=0; i<mla.nsi; i++)
		ret += format("%d %d %d %d ", mla.ml[i].llpos.x, mla.ml[i].llpos.y, mla.ml[i].size.x, mla.ml[i].size.y);
	
	return ret;
}

int Shwfs::set_mla_str(string mla_str) {
	// Syntax shoud be: <n> <x0> <y0> <w0> <h0> [<x1> <y1> <w1> <h1> [...]]
	int nsi = popint(mla_str);
	int x, y, w, h;
	
	is_calib = false;
	
	sh_simg_t *ml_tmp = (sh_simg_t *) malloc(nsi * sizeof *ml_tmp);
	
	for (int i=0; i<nsi; i++) {
		x = popint(mla_str);
		y = popint(mla_str);
		w = popint(mla_str);
		h = popint(mla_str);
		if (x <=0 || y <= 0 || w <= 0 || h <= 0)
			return -1;
		
		ml_tmp[i].llpos.x = x;
		ml_tmp[i].llpos.y = y;
		ml_tmp[i].size.x = w;
		ml_tmp[i].size.y = h;
	}
	
	if (mlacfg.ml)
		free(mlacfg.ml);
	
	mlacfg.nsi = nsi;
	mlacfg.ml = ml_tmp;
	
	netio.broadcast("ok mla " + mla_str);
	return 0;
}

