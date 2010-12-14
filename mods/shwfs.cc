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

// PRIVATE FUNCTIONS //
/*********************/

//template <class T> uint32_t Shwfs::_cog(T *img, int xpos, int ypos, int w, int h, int stride, uint32_t simini, fcoord_t& cog) {
//	uint32_t fi, csum = 0;
//	cog.x = cog.y = 0.0;
//	img += ypos*stride + xpos;
//	for (int iy=-w; iy < w; iy++) { // loop over all pixels
//		for (int ix=-h; ix < h; ix++) {
//			fi = img[ix + iy*stride];
//			if (fi > simini) {
//				csum += fi;
//				cog.x += fi * ix;      // center of gravity of subaperture intensity 
//				cog.y += fi * iy;
//			}
//		}
//	}
//	cog.x /= csum;
//	cog.y /= csum;
//	io.msg(IO_DEB2, "Shwfs::_cog(): subap @ %d,%d got cog=%f,%f (sum=%f).", xpos, ypos, cog.x, cog.y, csum);
//	return csum;		
//}
	
//template <class T> int Shwfs::_cogframe(T *img) {
//	float csum=0;
//	for (int s=0; s<ns; s++) { // Loop over all previously selected subapertures
//		csum += (float) _cog<T>(img, sipos[s].x, sipos[s].y, track.x/2, track.y/2, cam->get_width(), 0, trackpos[s]);
//	}
//	return 0;
//}

Shwfs::sh_simg_t *Shwfs::gen_mla_grid(coord_t res, coord_t size, coord_t pitch, int xoff, coord_t disp, mlashape_t shape, int &nsubap) {
	
	// How many subapertures would fit in the requested size 'res':
	int sa_range_y = (int) ((res.y/2)/pitch.x + 1);
	int sa_range_x = (int) ((res.x/2)/pitch.y + 1);
	
	nsubap = 0;
	Shwfs::sh_simg_t *pattern = NULL;
	
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
				if (pow(fabs(sa_c.x) + size.x/2.0, 2) + pow(fabs(sa_c.y) + size.y/2.0, 2) < pow(res.x/2.0, 2)) {
					//io.msg(IO_DEB2, "SimWfs::gen_mla_grid(): Found subap within bounds.");
					nsubap++;
					pattern = (Shwfs::sh_simg_t *) realloc((void *) pattern, nsubap * sizeof (Shwfs::sh_simg_t));
					pattern[nsubap-1].pos.x = sa_c.x + disp.x;
					pattern[nsubap-1].pos.y = sa_c.y + disp.y;
					pattern[nsubap-1].llpos.x = sa_c.x + disp.x - size.x/2;
					pattern[nsubap-1].llpos.y = sa_c.y + disp.y - size.y/2;
					pattern[nsubap-1].size.x = size.x;
					pattern[nsubap-1].size.y = size.y;
				}
			}
			else {
				// Accept a subimage coordinate (center position) the position + 
				// half-size the subaperture is within the bounds
				if ((fabs(sa_c.x + size.x/2) < res.x/2) && (fabs(sa_c.y + size.y/2) < res.y/2)) {
					//io.msg(IO_DEB2, "SimWfs::gen_mla_grid(): Found subap within bounds.");
					nsubap++;
					pattern = (Shwfs::sh_simg_t *) realloc((void *) pattern, nsubap * sizeof (Shwfs::sh_simg_t));
					pattern[nsubap-1].pos.x = sa_c.x + disp.x;
					pattern[nsubap-1].pos.y = sa_c.y + disp.y;
					pattern[nsubap-1].llpos.x = sa_c.x + disp.x - size.x/2;
					pattern[nsubap-1].llpos.y = sa_c.y + disp.y - size.y/2;
					pattern[nsubap-1].size.x = size.x;
					pattern[nsubap-1].size.y = size.y;
				}
			}
		}
	}
	io.msg(IO_XNFO, "SimWfs::gen_mla_grid(): Found %d subapertures.", nsubap);

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
	return true;
}

int Shwfs::mla_subapsel() {
	
	//! @todo implement this with generic MLA setup
	return 0;
}
/*
	uint32_t csum;
	float sivec[2] = {0};
	fcoord_t cog;
	int *apmap = new int[subap.x * subap.y];
	int *apmap2 = new int[subap.x * subap.y];
	
	io.msg(IO_DEB2, "Shwfs::subapSel()");
	
	for (int isy=0; isy < subap.y; isy++)
		for (int isx=0; isx < subap.x; isx++)
			apmap[isy * subap.x + isx] = apmap2[isy * subap.x + isx] = 0;
	
	ns = 0;
	
	// Get image first
	if (cam->get_dtype() == UINT16) {
		io.msg(IO_DEB2, "Shwfs::subapSel() got UINT16");
		void *tmpimg;
		cam->get_image(&tmpimg);
		uint16_t *img = (uint16_t *) tmpimg;
		for (int isy=0; isy < subap.y; isy++) { // loops over all grid cells
			for (int isx=0; isx < subap.x; isx++) {
				csum = _cog<uint16_t>(img, (int) (isx+0.5) * sisize.x, (int) (isy+0.5) * sisize.y, sisize.x/2, sisize.y/2, cam->get_width(), simini, cog);
				if (csum > 0) {
					apmap[isy * subap.x + isx] = 1;
					sipos[ns].x = (int) ((isx+0.5) * sisize.x); // Subap position
					sipos[ns].y = (int) ((isy+0.5) * sisize.y);
					sivec[0] += sipos[ns].x; // Sum all subap positions
					sivec[1] += sipos[ns].y;
					ns++;
				}
				else
					apmap[isy * subap.x + isx] = 0;
			}
		}
	}
	else if (cam->get_dtype() == UINT8) {
		io.msg(IO_DEB2, "Shwfs::subapSel() got UINT8");
		
		void *tmpimg;
		cam->get_image(&tmpimg);
		uint8_t *img = (uint8_t *) tmpimg;
		
		for (int isy=0; isy < subap.y; isy++) { // loops over all grid cells
			for (int isx=0; isx < subap.x; isx++) {
				csum = _cog<uint8_t>(img, (int) (isx+0.5) * sisize.x, (int) (isy+0.5) * sisize.y, sisize.x/2, sisize.y/2, cam->get_width(), simini, cog);
				if (csum > 0) {
					apmap[isy * subap.x + isx] = 1;
					sipos[ns].x = (int) ((isx+0.5) * sisize.x); // Subap position
					sipos[ns].y = (int) ((isy+0.5) * sisize.y);
					sivec[0] += sipos[ns].x; // Sum all subap positions
					sivec[1] += sipos[ns].y;
					ns++;
				}
				else
					apmap[isy * subap.x + isx] = 0;
			}
		}
	}
	
	// *Average* subaperture position (wrt the image origin)
	sivec[0] /= ns;
	sivec[1] /= ns;
	
	io.msg(IO_XNFO, "Found %d subaps with I > %d.", ns, simini);
	io.msg(IO_XNFO, "Average position: (%f, %f)", sivec[0], sivec[1]);
	
	// Find central aperture by minimizing (subap position - average position)
	int csi = 0;
	float dist, rmin;
	rmin = sqrtf( ((sipos[csi].x - sivec[0]) * (sipos[csi].x - sivec[0])) 
							 + ((sipos[csa].y - sivec[1]) * (sipos[csi].y - sivec[1])));
	for (int i=1; i < ns; i++) {
		dist = sqrt(
								((sipos[i].x - sivec[0]) * (sipos[i].x - sivec[0]))
								+ ((sipos[i].y - sivec[1]) * (sipos[i].y - sivec[1])));
		if (dist < rmin) {
			io.msg(IO_XNFO, "Better central position @ (%d, %d)", sipos[i].x, sipos[i].y);
			rmin = dist;
			csi = i;	// new best guess for central subaperture
		}
	}
	
	io.msg(IO_XNFO, "Central subaperture #%d at (%d,%d)", csi,
				 sipos[csi].x, sipos[csi].y);
	
	mla_print(apmap);
	
	// Edge erosion: erode the outer -simaxr rings of subapertures
	for (int r=simaxr; r<0; r++) {
		int isy, isx;
		io.msg(IO_XNFO, "Running edge erosion iteration...");
		for (int i=0; i<ns; i++) {
			//! \todo not sife, may break down:
			isy = (sipos[i].y / sisize.y);
			isx = (sipos[i].x / sisize.x);
			io.msg(IO_DEB1 | IO_NOLF, "Subap %d/%d @ (%d,%d) @ (%d,%d)...", i, ns, isx, isy, sipos[i].x, sipos[i].y);
			// If this subaperture is on the edge, or it does not have 
			// neighbours in all directions, cut it out
			if (isy == 0 || isy > subap.y ||
					isx == 0 || isx > subap.x ||
					apmap[(isy+0) * subap.x + (isx-1)] == 0 ||
					apmap[(isy-1) * subap.x + (isx+0)] == 0 ||
					apmap[(isy+0) * subap.x + (isx+1)] == 0 ||
					apmap[(isy+1) * subap.x + (isx+0)] == 0) {
				// Don't use this subaperture
				apmap2[isy * subap.x + isx] = 0;
				io.msg(IO_DEB1, " discard.");
				for (int j=i; j<ns-1; j++)
					sipos[j] = sapos[j+1];
				ns--;
				i--;
			}
			else {
				io.msg(IO_DEB1 | IO_NOID, " keep.\n");
				apmap2[isy * subap.x + isx] = 1;
			}
		}
		for (int isy=0; isy < subap.y; isy++)
			for (int isx=0; isx < subap.x; isx++)
				apmap[isy * subap.x + isx] = apmap2[isy * subap.x + isx];
		
		mla_print(apmap);
	}
	
	io.msg(IO_INFO, "Finally found %d subapertures", ns);
	
	return ns;
}
 */
	
//void Shwfs::mla_print(int *map) {
//	for (int isy=0; isy < subap.y; isy++) {
//		for (int isx=0; isx < subap.x; isx++) {
//			if (map[isy * subap.x + isx] == 1) io.msg(IO_XNFO | IO_NOID, "X ");
//			else io.msg(IO_XNFO | IO_NOID, ". ");
//		}
//		io.msg(IO_XNFO | IO_NOID, "\n");
//	}
//}

// PUBLIC FUNCTIONS //
/********************/


Shwfs::Shwfs(Io &io, foamctrl *ptc, string name, string port, Path &conffile, Camera &wfscam):
Wfs(io, ptc, name, shwfs_type, port, conffile, wfscam)
{
	io.msg(IO_DEB2, "Shwfs::Shwfs()");
		
	coord_t sisize;
	sisize.x = cfg.getint("sisizex", 16);
	sisize.y = cfg.getint("sisizey", 16);

	coord_t sipitch;
	sipitch.x = cfg.getint("sipitchx", 32);
	sipitch.y = cfg.getint("sipitchy", 32);
	
	fcoord_t sitrack;
	sitrack.x = sisize.x * cfg.getdouble("sitrackx", 0.5);
	sitrack.y = sisize.y * cfg.getdouble("sitracky", 0.5);
	
	simaxr = cfg.getint("simaxr", -1);
	simini = cfg.getint("simini", 30);
	int xoff = cfg.getint("xoff", 0);
		
	// Start in CoG mode
	mode = Shwfs::COG;
	
	// Generate MLA grid
	mla.ml = gen_mla_grid(cam.get_res(), sisize, sipitch, xoff, coord_t(cam.get_width()/2.,cam.get_height()/2.), Shwfs::SQUARE, mla.nsi);
}
	
Shwfs::~Shwfs() {
	io.msg(IO_DEB2, "Shwfs::~Shwfs()");
	if (mla.ml)
		free(mla.ml);
	
//	delete[] trackpos;
//	delete[] sipos;
//	delete[] shifts;
}

//int Shwfs::measure(int op) {
//	void *tmpimg;
//	cam->get_image(&tmpimg);
//	
//	if (cam->get_dtype() == UINT16) {
//		if (mode == Shwfs::COG) {
//			io.msg(IO_DEB2, "Shwfs::measure() got UINT16, COG");
//			return _cogframe<uint16_t>((uint16_t *) tmpimg);
//		}
//		else
//			return io.msg(IO_ERR, "Shwfs::measure() unknown wfs mode");
//	}
//	else if (cam->get_dtype() == UINT8) {
//		if (mode == COG) {
//			io.msg(IO_DEB2, "Shwfs::measure() got UINT8, COG");
//			return _cogframe<uint8_t>((uint8_t *) tmpimg);
//		}
//		else
//			return io.msg(IO_ERR, "Shwfs::measure() unknown wfs mode");
//	}
//	else
//		return io.msg(IO_ERR, "Shwfs::measure() unknown datatype");
//	
//	return 0;
//}
	
//int Shwfs::verify(int op) {
//	io.msg(IO_DEB2, "Shwfs::verify()");
//	return 0;
//}

//int Shwfs::configure(wfs_prop_t *prop) {
//	io.msg(IO_DEB2, "Shwfs::configure()");
//	return 0;
//}

//int Shwfs::calibrate(int op) {
//	if (op == Shwfs::CAL_SUBAPSEL) {
//		io.msg(IO_DEB2, "Shwfs::calibrate(Shwfs::CAL_SUBAPSEL)");
//		// For SH WFS: select the subapertures to use for processing
//		return subapSel();
//	}
//	else if (op == Shwfs::CAL_PINHOLE) {
//		io.msg(IO_DEB2, "Shwfs::calibrate(Shwfs::CAL_PINHOLE)");
//		//! \todo add calibration for reference coordinates (with pinhole)
//		return 0;
//	}
//	else {
//		io.msg(IO_WARN, "Shwfs::calibrate() Unknown calibration mode.");
//		return -1;
//	}
//	return 0;
//}
