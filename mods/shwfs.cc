/*
 shwfs.cc -- Shack-Hartmann utilities class
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

// PUBLIC FUNCTIONS //
/********************/


Shwfs::Shwfs(Io &io, foamctrl *ptc, string name, string port, Path &conffile, Camera &wfscam, bool online):
Wfs(io, ptc, name, shwfs_type, port, conffile, wfscam, online),
mode(Shwfs::COG)
{
	io.msg(IO_DEB2, "Shwfs::Shwfs()");
	
	// Micro lens array parameters:
	
	sisize.x = cfg.getint("sisizex", 16);
	sisize.y = cfg.getint("sisizey", 16);
	if (cfg.exists("sisize"))
		sisize.x = sisize.y = cfg.getint("sisize");
	
	sipitch.x = cfg.getint("sipitchx", 32);
	sipitch.y = cfg.getint("sipitchy", 32);
	if (cfg.exists("sipitch"))
		sipitch.x = sipitch.y = cfg.getint("sipitch");
	
	sitrack.x = sisize.x * cfg.getdouble("sitrackx", 0.5);
	sitrack.y = sisize.y * cfg.getdouble("sitracky", 0.5);
	if (cfg.exists("sitrack"))
		sitrack.x = sitrack.y = cfg.getdouble("sitrack");
	
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
	mla.ml = gen_mla_grid(cam.get_res(), sisize, sipitch, xoff, disp, shape, overlap, &(mla.nsi));
}

Shwfs::~Shwfs() {
	io.msg(IO_DEB2, "Shwfs::~Shwfs()");
	if (mla.ml)
		free(mla.ml);
	
}

int Shwfs::measure() {
	Camera::frame_t *tmp = cam.get_last_frame();
	
	if (cam.get_depth() == 16) {
		if (mode == COG) {
			io.msg(IO_DEB2, "Shwfs::measure() got UINT16, COG");
			//return _cogframe<uint16_t>((uint16_t *) tmp->image);
		}
		else
			return io.msg(IO_ERR, "Shwfs::measure() unknown wfs mode");
	}
	else if (cam.get_dtype() == UINT8) {
		if (mode == COG) {
			io.msg(IO_DEB2, "Shwfs::measure() got UINT8, COG");
			//return _cogframe<uint8_t>((uint8_t *) tmp->image);
		}
		else
			return io.msg(IO_ERR, "Shwfs::measure() unknown wfs mode");
	}
	else
		return io.msg(IO_ERR, "Shwfs::measure() unknown datatype");
	
	return 0;
}

Shwfs::sh_simg_t *Shwfs::gen_mla_grid(coord_t res, coord_t size, coord_t pitch, int xoff, coord_t disp, mlashape_t shape, float overlap, int *nsubap) {
	io.msg(IO_DEB2, "Shwfs::gen_mla_grid()");
	
	// How many subapertures would fit in the requested size 'res':
	int sa_range_y = (int) ((res.y/2)/pitch.x + 1);
	int sa_range_x = (int) ((res.x/2)/pitch.y + 1);
	
	*nsubap = 0;
	sh_simg_t *pattern = NULL;
	
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
					//io.msg(IO_DEB2, "Shwfs::gen_mla_grid(): Found subap within bounds @ (%d, %d)", sa_c.x, sa_c.y);
					(*nsubap)++;
					pattern = (sh_simg_t *) realloc((void *) pattern, (*nsubap) * sizeof *pattern);
					pattern[(*nsubap)-1].pos.x = sa_c.x + disp.x;
					pattern[(*nsubap)-1].pos.y = sa_c.y + disp.y;
					pattern[(*nsubap)-1].llpos.x = sa_c.x + disp.x - size.x/2;
					pattern[(*nsubap)-1].llpos.y = sa_c.y + disp.y - size.y/2;
					pattern[(*nsubap)-1].size.x = size.x;
					pattern[(*nsubap)-1].size.y = size.y;
				}
			}
			else {
				// Accept a subimage coordinate (center position) the position + 
				// half-size the subaperture is within the bounds
				if ((fabs(sa_c.x + (overlap-0.5)*size.x) < res.x/2) && (fabs(sa_c.y + (overlap-0.5)*size.y) < res.y/2)) {
					//io.msg(IO_DEB2, "Shwfs::gen_mla_grid(): Found subap within bounds.");
					(*nsubap)++;
					pattern = (sh_simg_t *) realloc((void *) pattern, (*nsubap) * sizeof *pattern);
					pattern[(*nsubap)-1].pos.x = sa_c.x + disp.x;
					pattern[(*nsubap)-1].pos.y = sa_c.y + disp.y;
					pattern[(*nsubap)-1].llpos.x = sa_c.x + disp.x - size.x/2;
					pattern[(*nsubap)-1].llpos.y = sa_c.y + disp.y - size.y/2;
					pattern[(*nsubap)-1].size.x = size.x;
					pattern[(*nsubap)-1].size.y = size.y;
				}
			}
		}
	}
	io.msg(IO_XNFO, "Shwfs::gen_mla_grid(): Found %d subapertures.", nsubap);
	
	return pattern;
}

bool Shwfs::store_mla_grid(Path &f, bool overwrite) {
	return store_mla_grid(mla, f, overwrite);
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
		fprintf(fd, "%d, %d, %d, %d, %d, %d\n", 
						mla.ml[n].llpos.x,
						mla.ml[n].llpos.y,
						mla.ml[n].pos.x,
						mla.ml[n].pos.y,
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

Shwfs::sh_simg_t *Shwfs::find_mla_grid(coord_t size, int *nsubap, int mini, int nmax, int iter) {
	io.msg(IO_DEB2, "Shwfs::find_mla_grid()");

	// Store current camera count, get last frame
	size_t bcount = cam.get_count();
	Camera::frame_t *f = cam.get_last_frame();
	
	if (f == NULL) {
		io.msg(IO_WARN, "Shwfs::find_mla_grid() Could not get frame, is the camera running?");
		return NULL;
	}
	
	*nsubap = 0;
	sh_simg_t *subaps = NULL;
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
		*nsubap++;
		subaps = (sh_simg_t *) realloc((void *) subaps, (*nsubap) * sizeof *subaps);

		sapos.x = maxidx % cam.get_width();
		sapos.y = int(maxidx / cam.get_width());
		
		subaps[*nsubap-1].pos.x = sapos.x;
		subaps[*nsubap-1].pos.y = sapos.y;
		
		// Enough subapertures, done
		if (*nsubap == nmax)
			break;
		
		// Set the current subaperture to zero
		int xran[] = {max(0, sapos.x-size.x/2), min(cam.get_width(), sapos.x+size.x/2)};
		int yran[] = {max(0, sapos.y-size.y/2), min(cam.get_height(), sapos.y+size.y/2)};
		
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
	
	// Done! We have found the maximum intensities now. Store in useful format
		
	for (int it=1; it<iter; it++) {
		//! @todo implement iterations in find_mla_grid()
		io.msg(IO_WARN, "Shwfs::find_mla_grid(): iter not yet implemented");
	}
	
	// Get latest camera count. If the difference between begin and end is 
	// bigger than the size of the camera ringbuffer, we were too slow
	size_t ecount = cam.get_count();
	if (ecount - bcount >= cam.get_bufsize()) {
		//! @todo This poses possible problems, what to do?
		io.msg(IO_WARN, "Shwfs::find_mla_grid(): got camera buffer overflow, data might be inaccurate");
	}

	return subaps;
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
