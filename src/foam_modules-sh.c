/*! 
	@file foam_modules-sh.c
	@author @authortim
	@date January 28, 2008

	@brief This file contains modules and functions related to Shack-Hartmann wavefront sensing.
	
	\section Info
	This module can be used to do Shack Hartmann wavefront sensing and tracking.
	
	\section Functions
	
	The functions provided to the outside world are:
	\li modSelSubapts() - Selects subapertures suitable for tracking
	\li modParseSH() - Tracks targets in previously selected subapertures
	\li modCogTrack() - Center of Gravity tracking module
	\li modCogFind() - Find target using a larger area, used for recovery 
	\li modCalcCtrl() - Calculate WFC control vectors given target displacements

	\section Dependencies
	
	This module does not depend on other modules.

	\section License
	This code is licensed under the GPL, version 2 or later
*/

// HEADERS //
/***********/

#include "foam_modules-sh.h"

// GLOBALS //
/***********/

// FILE *rmsfp;

// ROUTINES //
/************/

int modInitSH(wfs_t *wfsinfo, mod_sh_track_t *shtrack) {
	logInfo(0, "Initializing SH tracking module");
	
	shtrack->subc = calloc(shtrack->cells.x * shtrack->cells.y, sizeof(coord_t));
	shtrack->gridc = calloc(shtrack->cells.x * shtrack->cells.y , sizeof(coord_t));
	// we store displacement coordinates here
	shtrack->disp = gsl_vector_float_calloc(shtrack->cells.x * shtrack->cells.y * 2);
	// we store reference coordinates here
	shtrack->refc = gsl_vector_float_calloc(shtrack->cells.x * shtrack->cells.y * 2);
	
	if (shtrack->subc == NULL || shtrack->gridc == NULL || shtrack->disp == NULL || \
		shtrack->refc == NULL) {
		// this is actually superfluous, errors handled by gsl
		logErr("Could not allocate memory in modInitSH()!");
		return EXIT_FAILURE;
	}
	
	// allocate data for dark (nsubaps.x * nsubaps.y * subapsize). As we're 
	// using bpp images, we need twice that bitdepth for dark and flats. align 
	// to pagesize, which is overkill, but works for me. Needs a cleaner solution
	// !!!:tim:20080512 update alloc alignment
	wfsinfo->dark = valloc((shtrack->cells.x * shtrack->cells.y * shtrack->track.x * shtrack->track.y) * wfsinfo->bpp/8 * 2);
	// allocate data for gain (nsubaps.x * nsubaps.y * subapsize), gain is 16 bits
	wfsinfo->gain = valloc((shtrack->cells.x * shtrack->cells.y * shtrack->track.x * shtrack->track.y) * wfsinfo->bpp/8 * 2);
	// allocate data for corrected img (nsubaps.x * nsubaps.y * subapsize), corr is 8 bit
	wfsinfo->corr = valloc((shtrack->cells.x * shtrack->cells.y * shtrack->track.x * shtrack->track.y) * wfsinfo->bpp/8);

	// check if allocation worked
	if (wfsinfo->dark == NULL || wfsinfo->gain == NULL || wfsinfo->corr == NULL) {
		logErr("Could not allocate memory in modInitSH()!");
		return EXIT_FAILURE;
	}

	
	return EXIT_SUCCESS;
}

/*
 This is not used anymore, deprecated by a more general routine
int modSelSubapts(float *image, coord_t res, int cells[2], int (*subc)[2], int (*apcoo)[2], int *totnsubap, float samini, int samxr) {
	// stolen from ao3.c by CUK :)
	int isy, isx, iy, ix, i, sn=0, nsubap=0; //init sn to zero!!
	float sum=0.0, fi;					// check 'intensity' of a subapt
	float csum=0.0, cs[] = {0.0, 0.0}; 	// for center of gravity
	float cx=0, cy=0;					// for CoG
	float dist, rmin;					// minimum distance
	int csa=0;							// estimate for best subapt
	// int (*subc)[2] = wfsinfo->subc;		// lower left coordinates of the tracker windows
	// int (*apcoo)[2] = wfsinfo->gridc;	// lower left coordinates of the grid windows

//	float *image = wfsinfo->image;		// source image from sensor
//	coord_t res = wfsinfo->res;			// image resolution
//	int *cells = wfsinfo->cells;		// cell resolution used (i.e. 8x8)
	int shsize[] = {res.x/cells[0], res.y/cells[1]};		// subapt pixel resolution
	
	// we store our subaperture map in here when deciding which subapts to use
	int apmap[cells[0]][cells[1]];		// aperture map
	int apmap2[cells[0]][cells[1]];		// aperture map 2
	
	logInfo(0, "Selecting subapertures.");
	for (isy=0; isy<cells[1]; isy++) { // loops over all potential subapertures
		for (isx=0; isx<cells[0]; isx++) {
			// set apmap and apmap2 to zero
			apmap[isx][isy] = 0;
			apmap2[isx][isy] = 0;
			
			// check one potential subapt (isy,isx)
			sum=0.0; cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
			for (iy=0; iy<shsize[1]; iy++) { // sum all pixels in the subapt
				for (ix=0; ix<shsize[0]; ix++) {
					fi = (float) image[isy*shsize[1]*res.x + isx*shsize[0] + ix+ iy*res.x];
					sum += fi;
					// for center of gravity, only pixels above the threshold are used;
					// otherwise the position estimate always gets pulled to the center;
					// good background elimination is crucial for this to work !!!
					fi -= samini;    		// subtract threshold
					if (fi<0.0) fi = 0.0;	// clip
					csum = csum + fi;		// add this pixel's intensity to sum
					cs[0] += + fi * ix;	// center of gravity of subaperture intensity 
					cs[1] += + fi * iy;
				}
			}
			// check if the summed subapt intensity is above zero (e.g. do we use it?)
			if (csum > 0.0) { // good as long as pixels above background exist
				// we add 0.5 to make sure the integer division is 'fair' (e.g. simulate round(), but faster)
				subc[sn][0] = isx*shsize[0]+shsize[0]/4 + (int) (cs[0]/csum+0.5) - shsize[0]/2;	// subapt coordinates
				subc[sn][1] = isy*shsize[1]+shsize[1]/4 + (int) (cs[1]/csum+0.5) - shsize[1]/2;	// TODO: sort this out
							// ^^ coordinate in big image,  ^^ CoG of one subapt, /4 because that's half a trakcer windows

				cx += isx*shsize[0];
				cy += isy*shsize[1];
				apmap[isx][isy] = 1; // set aperture map
				apcoo[sn][0] = isx;
				apcoo[sn][1] = isy;
				sn++;
			} else {
				apmap[isx][isy] = 0; // don't use this subapt
			}
		}
	}
	logInfo(0, "CoG for subapts done, found %d with intensity > 0.", sn);
	
	nsubap = sn; 			// nsubap: variable that contains number of subapertures
	cx = cx / (float) sn; 	// TODO what does this do? why?
	cy = cy / (float) sn;
	
	// determine central aperture that will be reference
	// initial value for rmin is the distance of the first subapt. TODO: this should work, right?
	csa = 0;
	rmin = sqrt((subc[0][0]-cx)*(subc[0][0]-cx) + (subc[0][1]-cy)*(subc[0][1]-cy));

	for (i=0; i<nsubap; i++) {
		dist = sqrt((subc[i][0]-cx)*(subc[i][0]-cx) + (subc[i][1]-cy)*(subc[i][1]-cy));
		if (dist < rmin) {
			rmin = dist;
			csa = i; 		// new best guess for central subaperture
		}
	}
	logInfo(0, "Reference apt best guess: %d (%d,%d)", csa, subc[csa][0], subc[csa][1]);
	// put reference subaperture as subaperture 0
	cs[0]        = subc[0][0]; 		cs[1] 			= subc[0][1];
	subc[0][0]   = subc[csa][0]; 	subc[0][1] 		= subc[csa][1];
	subc[csa][0] = cs[0]; 			subc[csa][1] 	= cs[1];

	// and fix aperture map accordingly
	cs[0]         = apcoo[0][0];	cs[1] 			= apcoo[0][1];
	apcoo[0][0]   = apcoo[csa][0]; 	apcoo[0][1] 	= apcoo[csa][1];
	apcoo[csa][0] = cs[0];			apcoo[csa][1] 	= cs[1];

	// center central subaperture; it might not be centered if there is
	// a large shift between the expected and the actual position in the
	// center of gravity approach above
	// TODO: might not be necessary? leftover?
	cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
	for (iy=0; iy<shsize[1]; iy++) {
		for (ix=0; ix<shsize[0]; ix++) {
			// loop over the whole shsize^2 big ref subapt here, so subc-shsize/4 is the beginning coordinate
			fi = (float) image[(subc[0][1]-shsize[1]/4+iy)*res.x+subc[0][0]-shsize[0]/4+ix];
						
			// for center of gravity, only pixels above the threshold are used;
			// otherwise the position estimate always gets pulled to the center;
			// good background elimination is crucial for this to work !!!
			fi -= samini;
			if (fi < 0.0) fi=0.0;
			csum += fi;
			cs[0] += fi * ix;
			cs[1] += fi * iy;
		}
	}
	
	logInfo(0, "old subx=%d, old suby=%d",subc[0][0],subc[0][1]);
	subc[0][0] += (int) (cs[0]/csum+0.5) - shsize[0]/2; // +0.5 to prevent integer cropping rounding error
	subc[0][1] += (int) (cs[1]/csum+0.5) - shsize[1]/2; // use shsize/2 because or CoG is for a larger subapt (shsize and not shsize/2)
	logInfo(0, "new subx=%d, new suby=%d",subc[0][0],subc[0][1]);

	// enforce maximum radial distance from center of gravity of all
	// potential subapertures if samxr is positive
	if (samxr > 0) {
		sn = 1;
		while (sn < nsubap) {
			if (sqrt((subc[sn][0]-cx)*(subc[sn][0]-cx) + \
				(subc[sn][1]-cy)*(subc[sn][1]-cy)) > samxr) { // TODO: why remove subapts? might be bad subapts
				for (i=sn; i<(nsubap-1); i++) {
					subc[i][0] = subc[i+1][0]; // remove erroneous subapts
					subc[i][1] = subc[i+1][1];
					apcoo[i][0] = apcoo[i+1][0]; // and remove erroneous grid coordinates
					apcoo[i][1] = apcoo[i+1][1];
				}
				nsubap--;
			} else {
				sn++;
			}
		}
	}

	// edge erosion if samxr is negative; 
	// this is good for non-circular apertures
	while (samxr < 0) {
		samxr = samxr + 1;
		// print ASCII map of aperture
		logInfo(0, "ASCII map of aperture:");
		for (isy=0; isy<cells[1]; isy++) {
			logInfo(LOG_NOFORMAT, "%d:", isy);
			for (isx=0; isx<cells[0]; isx++)
				if (apmap[isx][isy] == 0) logInfo(LOG_NOFORMAT, " "); 
				else logInfo(LOG_NOFORMAT, "X");
			logInfo(LOG_NOFORMAT, "\n");
		}
		
		// always take the reference subapt to the new map, 
		// otherwise we lose it during the first copying
		apmap2[apcoo[0][0]][apcoo[0][1]] = 1;
		
		sn = 1; // start with subaperture 1 since subaperture 0 is the reference
		while (sn < nsubap) {
			isx = apcoo[sn][0]; isy = apcoo[sn][1];
			if ((isx == 0) || (isx >= cells[0]-1) || (isy == 0) || (isy >= cells[1]-1) ||
				(apmap[isx-1 % cells[0]][isy] == 0) ||
				(apmap[isx+1 % cells[0]][isy] == 0) ||
				(apmap[isx][isy-1 % cells[1]] == 0) ||
				(apmap[isx][isy+1 % cells[1]] == 0)) {
					
				apmap2[isx][isy] = 0;
				for (i=sn; i<(nsubap-1); i++) {
					subc[i][0] = subc[i+1][0];
					subc[i][1] = subc[i+1][1];
					apcoo[i][0] = apcoo[i+1][0];
					apcoo[i][1] = apcoo[i+1][1];
				}
				nsubap--;
			} else {
				apmap2[isx][isy] = 1;
				sn++;
			}
		}

		// copy new aperture map to old aperture map for next iteration
		for (isy=0; isy<cells[1]; isy++)
			for (isx=0; isx<cells[0]; isx++)
				apmap[isx][isy] = apmap2[isx][isy];

	}
	*totnsubap = nsubap;		// store to external variable
	logInfo(0, "Selected %d usable subapertures", nsubap);
	
	logInfo(0, "ASCII map of aperture:");
	for (isy=0; isy<cells[1]; isy++) {
		logInfo(LOG_NOFORMAT, "%d:", isy);
		for (isx=0; isx<cells[0]; isx++)
			if (apmap[isx][isy] == 0) logInfo(LOG_NOFORMAT, " "); 
			else logInfo(LOG_NOFORMAT, "X");
		logInfo(LOG_NOFORMAT, "\n");
	}
	
	// set remaining subaperture coordinates to 0
	for (sn=nsubap; sn<cells[0]*cells[1]; sn++) {
		subc[sn][0] = 0; 
		subc[sn][1] = 0;
		apcoo[sn][0] = 0; 
		apcoo[sn][1] = 0;
	}
	
	logDebug(0, "final gridcs:");
	for (sn=0; sn<nsubap; sn++) {
		apcoo[sn][1] *= shsize[1];
		apcoo[sn][0] *= shsize[0];
		logDebug(LOG_NOFORMAT, "%d (%d,%d) ", sn, apcoo[sn][0], apcoo[sn][1]);
	}
	logDebug(LOG_NOFORMAT, "\n");
	
	return EXIT_SUCCESS;
}
*/

int modSelSubapts(void *image, foam_datat_t data, mod_sh_align_t align, mod_sh_track_t *shtrack, wfs_t *shwfs) {
	// stolen from ao3.c by CUK
	int isy, isx, iy, ix, i, sn=0, nsubap=0;	//init sn to zero!!
	float sum=0.0, fi=0.0;						// check 'intensity' of a subapt
	float csum=0.0, cs[] = {0.0, 0.0}; 	// for center of gravity
	float cx=0, cy=0;					// for CoG
	float dist, rmin;					// minimum distance
	float samini = shtrack->samini;		// temp copy
	int samxr = shtrack->samxr;		// temp copy
	int csa=0;							// estimate for best subapt
	
	// we store our subaperture map in here when deciding which subapts to use
	int apmap[shtrack->cells.x][shtrack->cells.y];		// aperture map
	int apmap2[shtrack->cells.x][shtrack->cells.y];		// aperture map 2
	
	// cast the void pointer to byte pointer, we know the image is a byte image.
//	uint8_t *byteimg = image;

	/*int max = byteimg[0];
	int min = byteimg[0];
	for (i=0; i<shwfs->res.x*shwfs->res.y; i++) {
		sum += byteimg[i];
		if (byteimg[i] > max) max = byteimg[i];
		else if (byteimg[i] < min) min = byteimg[i];
	}	
	logInfo(0, "Image info: sum: %f, avg: %f, range: (%d,%d)", sum, (float) sum / (shwfs->res.x*shwfs->res.y), min, max);
	 */
	if (align != ALIGN_RECT) {
		logWarn("Other alignments besides simple rectangles not supported by modSelSubapts");
		return EXIT_FAILURE;
	}

	sum = 0;
	
	logInfo(0, "Selecting subapertures now.");
	for (isy=0; isy<shtrack->cells.y; isy++) { // loops over all potential subapertures
		for (isx=0; isx<shtrack->cells.x; isx++) {
			// set apmap and apmap2 to zero
			apmap[isx][isy] = 0;
			apmap2[isx][isy] = 0;
			
			sum=0.0; cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;			
			// check one potential subapt (isy,isx)
			if (data == DATA_UINT8) {		// data is stored in unsigned 8 bit ints
				uint8_t *datapt = (uint8_t *) image;
				for (iy=0; iy<shtrack->shsize.y; iy++) { // sum all pixels in the subapt
					for (ix=0; ix<shtrack->shsize.x; ix++) {
						fi = datapt[isy*shtrack->shsize.y*shwfs->res.x + isx*shtrack->shsize.x + ix+ iy*shwfs->res.x];
						sum += fi;
						// for center of gravity, only pixels above the threshold are used;
						// otherwise the position estimate always gets pulled to the center;
						// good background elimination is crucial for this to work !!!
						fi -= samini;    		// subtract threshold
						if (fi<0.0) fi = 0.0;	// clip
						csum = csum + fi;		// add this pixel's intensity to sum
						cs[0] += + fi * ix;	// center of gravity of subaperture intensity 
						cs[1] += + fi * iy;
					}
				}
			}
			else if (data == DATA_GSL_M_F) {	// data is stored in GSL matrix
				gsl_matrix_float *datapt = (gsl_matrix_float *) image;				
				for (iy=0; iy<shtrack->shsize.y; iy++) { // sum all pixels in the subapt
					for (ix=0; ix<shtrack->shsize.x; ix++) {
						fi = gsl_matrix_float_get(datapt, isy*shtrack->shsize.y + iy, isx*shtrack->shsize.x + ix);
						sum += fi; fi -= samini;
						if (fi<0.0) fi = 0.0;
						csum = csum + fi;
						cs[0] += + fi * ix; cs[1] += + fi * iy;
					}
				}
			}

			// check if the summed subapt intensity is above zero (e.g. do we use it?)
			if (csum > 0.0) { // good as long as pixels above background exist
				// we add 0.5 to make sure the integer division is 'fair' (e.g. simulate round(), but faster)
				shtrack->subc[sn].x = isx*shtrack->shsize.x - shtrack->track.x/2 + (int) (cs[0]/csum+0.5);	// subapt coordinates
				shtrack->subc[sn].y = isy*shtrack->shsize.y - shtrack->track.y/2 + (int) (cs[1]/csum+0.5);
				//shtrack->subc[sn].x = isx*shtrack->shsize.x+shtrack->shsize.x/4 + (int) (cs[0]/csum+0.5) - shtrack->shsize.x/2;	// subapt coordinates
				//shtrack->subc[sn].y = isy*shtrack->shsize.y+shtrack->shsize.y/4 + (int) (cs[1]/csum+0.5) - shtrack->shsize.y/2;	// TODO: sort this out
				// ^^ coordinate in big image,  ^^ CoG of one subapt, /4 because that's half a trakcer windows
				
				cx += isx*shtrack->shsize.x;
				cy += isy*shtrack->shsize.y;
				apmap[isx][isy] = 1; // set aperture map
				shtrack->gridc[sn].x = isx;
				shtrack->gridc[sn].y = isy;
				//logDebug(0, "cog (%.2f,%.2f) subc (%d,%d) gridc (%d,%d) sum %f (min: %f, max: %d)", cs[0], cs[1], shtrack->subc[sn].x, shtrack->subc[sn].y, isx, isy, csum, samini, samxr);
				sn++;
			} else {
				apmap[isx][isy] = 0; // don't use this subapt if the intensity is too low
			}
		}
	}
	logInfo(0, "CoG for subapts done, found %d with intensity > 0.", sn);
	
	nsubap = sn; 			// nsubap: variable that contains number of subapertures
	cx = cx / (float) sn; 	// TODO what does this do? why?
	cy = cy / (float) sn;
	
	// determine central aperture that will be reference
	// initial value for rmin is the distance of the first subapt. TODO: this should work, right?
	csa = 0;
	rmin = sqrt((shtrack->subc[0].x-cx)*(shtrack->subc[0].x-cx) + (shtrack->subc[0].y-cy)*(shtrack->subc[0].y-cy));
	
	for (i=0; i<nsubap; i++) {
		dist = sqrt((shtrack->subc[i].x-cx)*(shtrack->subc[i].x-cx) + (shtrack->subc[i].y-cy)*(shtrack->subc[i].y-cy));
		if (dist < rmin) {
			rmin = dist;
			csa = i; 		// new best guess for central subaperture
		}
	}
	logInfo(0, "Reference apt best guess: %d (%d,%d)", csa, shtrack->subc[csa].x, shtrack->subc[csa].y);
	// put reference subaperture as subaperture 0
	cs[0]				= shtrack->subc[0].x; 		cs[1]				= shtrack->subc[0].y;
	shtrack->subc[0].x	= shtrack->subc[csa].x; 	shtrack->subc[0].y 	= shtrack->subc[csa].y;
	shtrack->subc[csa].x = cs[0];					shtrack->subc[csa].y = cs[1];
	
	// and fix aperture map accordingly
	cs[0]					= shtrack->gridc[0].x;		cs[1]					= shtrack->gridc[0].y;
	shtrack->gridc[0].x		= shtrack->gridc[csa].x;	shtrack->gridc[0].y 	= shtrack->gridc[csa].y;
	shtrack->gridc[csa].x	= cs[0];					shtrack->gridc[csa].y 	= cs[1];
	
	// center central subaperture; it might not be centered if there is
	// a large shift between the expected and the actual position in the
	// center of gravity approach above
	// TODO: might not be necessary? leftover?
	/*
	cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
	for (iy=0; iy<shtrack->shsize.y; iy++) {
		for (ix=0; ix<shtrack->shsize.x; ix++) {
			// loop over the whole shsize^2 big ref subapt here, so subc-shsize/4 is the beginning coordinate
			fi = byteimg[(shtrack->subc[0].y-shtrack->shsize.y/4+iy)* shwfs->res.x + shtrack->subc[0].x-shtrack->shsize.x/4+ix];
			
			// for center of gravity, only pixels above the threshold are used;
			// otherwise the position estimate always gets pulled to the center;
			// good background elimination is crucial for this to work !!!
			fi -= samini;
			if (fi < 0.0) fi=0.0;
			csum += fi;
			cs[0] += fi * ix;
			cs[1] += fi * iy;
		}
	}
	 */
	
	if (data == DATA_UINT8) {		// data is stored in unsigned 8 bit ints
		uint8_t *datapt = (uint8_t *) image;
		for (iy=0; iy<shtrack->shsize.y; iy++) { // sum all pixels in the subapt
			for (ix=0; ix<shtrack->shsize.x; ix++) {
				// !!!:tim:20080514 this might not work if shtrack->track is 
				// (much) bigger than half of shtrack->shsize, in that case we
				// include too much image in our selection below and the CoG might
				// get pulled to other spots accidentally included.
				fi = datapt[(shtrack->subc[0].y - shtrack->track.y/2 + iy)*shwfs->res.x + shtrack->subc[0].x-shtrack->track.x/2 + ix];
				sum += fi;
				// for center of gravity, only pixels above the threshold are used;
				// otherwise the position estimate always gets pulled to the center;
				// good background elimination is crucial for this to work !!!
				fi -= samini;    		// subtract threshold
				if (fi<0.0) fi = 0.0;	// clip
				csum = csum + fi;		// add this pixel's intensity to sum
				cs[0] += + fi * ix;	// center of gravity of subaperture intensity 
				cs[1] += + fi * iy;
			}
		}
	}
	else if (data == DATA_GSL_M_F) {	// data is stored in GSL matrix
		gsl_matrix_float *datapt = (gsl_matrix_float *) data;				
		for (iy=0; iy<shtrack->shsize.y; iy++) { // sum all pixels in the subapt
			for (ix=0; ix<shtrack->shsize.x; ix++) {
				fi = gsl_matrix_float_get(datapt, shtrack->subc[0].y - shtrack->track.y/2 + iy, shtrack->subc[0].x-shtrack->track.x/2 + ix);
				sum += fi; fi -= samini;
				if (fi<0.0) fi = 0.0;
				csum = csum + fi;
				cs[0] += + fi * ix; cs[1] += + fi * iy;
			}
		}
	}
	
	logInfo(0, "old subx=%d, old suby=%d",shtrack->subc[0].x, shtrack->subc[0].y);
	shtrack->subc[0].x += (int) (cs[0]/csum+0.5) - shtrack->track.x; // +0.5 to prevent integer cropping rounding error
	shtrack->subc[0].y += (int) (cs[1]/csum+0.5) - shtrack->track.y; // use track.x|y because or CoG is for a larger subapt (shsize and not shsize/2)
	logInfo(0, "new subx=%d, new suby=%d",shtrack->subc[0].x, shtrack->subc[0].y);
	
	// enforce maximum radial distance from center of gravity of all
	// potential subapertures if samxr is positive
	if (samxr > 0) {
		sn = 1;
		while (sn < nsubap) {
			if (sqrt((shtrack->subc[sn].x-cx)*(shtrack->subc[sn].x-cx) + \
					 (shtrack->subc[sn].y-cy)*(shtrack->subc[sn].y-cy)) > samxr) { // remove subapts, might be bad subapts and not useful
				for (i=sn; i<(nsubap-1); i++) {
					shtrack->subc[i].x = shtrack->subc[i+1].x; // remove erroneous subapts
					shtrack->subc[i].y = shtrack->subc[i+1].y;
					shtrack->gridc[i].x = shtrack->gridc[i+1].x; // and remove erroneous grid coordinates
					shtrack->gridc[i].y = shtrack->gridc[i+1].y;
				}
				nsubap--;
			} else {
				sn++;
			}
		}
	}
	
	// edge erosion if samxr is negative; 
	// this is good for non-circular apertures
	while (samxr < 0) {
		samxr = samxr + 1;
		// print ASCII map of aperture
		logInfo(0, "ASCII map of aperture:");
		for (isy=0; isy<shtrack->cells.y; isy++) {
			logInfo(LOG_NOFORMAT, "%d:", isy);
			for (isx=0; isx<shtrack->cells.x; isx++)
				if (apmap[isx][isy] == 0) logInfo(LOG_NOFORMAT, " "); 
				else logInfo(LOG_NOFORMAT, "X");
			logInfo(LOG_NOFORMAT, "\n");
		}
		
		// always take the reference subapt to the new map, 
		// otherwise we lose it during the first copying
		apmap2[shtrack->gridc[0].x][shtrack->gridc[0].y] = 1;
		
		sn = 1; // start with subaperture 1 since subaperture 0 is the reference
		while (sn < nsubap) {
			isx = shtrack->gridc[sn].x; isy = shtrack->gridc[sn].y;
			if ((isx == 0) || (isx >= shtrack->cells.x-1) || (isy == 0) || (isy >= shtrack->cells.y-1) ||
				(apmap[isx-1 % shtrack->cells.x][isy] == 0) ||
				(apmap[isx+1 % shtrack->cells.x][isy] == 0) ||
				(apmap[isx][isy-1 % shtrack->cells.y] == 0) ||
				(apmap[isx][isy+1 % shtrack->cells.y] == 0)) {
				
				apmap2[isx][isy] = 0;
				for (i=sn; i<(nsubap-1); i++) {
					shtrack->subc[i].x = shtrack->subc[i+1].x; // remove erroneous subapts
					shtrack->subc[i].y = shtrack->subc[i+1].y;
					shtrack->gridc[i].x = shtrack->gridc[i+1].x; // and remove erroneous grid coordinates
					shtrack->gridc[i].y = shtrack->gridc[i+1].y;					
				}
				nsubap--;
			} else {
				apmap2[isx][isy] = 1;
				sn++;
			}
		}
		
		// copy new aperture map to old aperture map for next iteration
		for (isy=0; isy<shtrack->cells.y; isy++)
			for (isx=0; isx<shtrack->cells.x; isx++)
				apmap[isx][isy] = apmap2[isx][isy];
		
	}
	//*totnsubap = nsubap;		// store to external variable
	shtrack->nsubap = nsubap;
	logInfo(0, "Selected %d usable subapertures", nsubap);
	
	// WARNING! I don't know what goes wrong, but somehow x and y 
	// are swapped. I manually reset it here, but this is indeed a 
	// rather nasty solution. This definately needs closer inspection
	/*
	int tmpswap;
	for (sn=0; sn<nsubap; sn++) {
		logDebug(LOG_NOFORMAT, "before: (%d,%d) and (%d,%d) ", shtrack->gridc[sn].x, shtrack->gridc[sn].y, shtrack->subc[sn].x, shtrack->subc[sn].y);
		tmpswap = shtrack->gridc[sn].y;
		shtrack->gridc[sn].y = shtrack->gridc[sn].x;
		shtrack->gridc[sn].x = tmpswap;
		tmpswap = shtrack->subc[sn].y;
		shtrack->subc[sn].y = shtrack->subc[sn].x;
		shtrack->subc[sn].x = tmpswap;
		logDebug(LOG_NOFORMAT, "after: (%d,%d) and (%d,%d)\n", shtrack->gridc[sn].x, shtrack->gridc[sn].y, shtrack->subc[sn].x, shtrack->subc[sn].y);
	}
	*/
	// END WARNING
	
	logInfo(0, "ASCII map of aperture:");
	for (isy=0; isy<shtrack->cells.y; isy++) {
		logInfo(LOG_NOFORMAT, "%d:", isy);
		for (isx=0; isx<shtrack->cells.x; isx++)
			if (apmap[isx][isy] == 0) logInfo(LOG_NOFORMAT, " "); 
			else logInfo(LOG_NOFORMAT, "X");
		logInfo(LOG_NOFORMAT, "\n");
	}
	
	// set remaining subaperture coordinates to 0
	for (sn=nsubap; sn<shtrack->cells.x*shtrack->cells.y; sn++) {
		shtrack->subc[sn].x = 0; 
		shtrack->subc[sn].y = 0;
		shtrack->gridc[sn].x = 0; 
		shtrack->gridc[sn].y = 0;
	}
	
	logDebug(0, "Final gridcs:");
	for (sn=0; sn<nsubap; sn++) {
		shtrack->gridc[sn].y *= shtrack->shsize.y;
		shtrack->gridc[sn].x *= shtrack->shsize.x;
		logDebug(LOG_NOFORMAT, "%d (%d,%d) ", sn, shtrack->gridc[sn].x, shtrack->gridc[sn].y);
	}
	logDebug(LOG_NOFORMAT, "\n");
	
	return EXIT_SUCCESS;
}

/*
void modCogTrackGSL(gsl_matrix_float *image, mod_sh_track_t *shtrack int align, float *aver, float *max) {
//void modCogTrack(gsl_matrix_float *image, int (*subc)[2], int nsubap, coord_t track, float *aver, float *max, float coords[][2]) {
	int ix, iy, sn=0;
	float csx, csy, csum, fi, cmax; 			// variables for center-of-gravity
	float sum = 0;
	
	// loop over all subapertures (treat those all equal)
	for (sn=0; sn < shtrack->nsubap; sn++) {
		csx = 0.0; csy = 0.0; csum = 0.0;
		
		for (iy=0; iy < shtrack->track.y; iy++) {
			for (ix=0; ix < shtrack->track.x; ix++) {
				fi = gsl_matrix_float_get(image, shtrack->subc[sn].y+iy, shtrack->subc[sn].x+ix);
				//image[subc[sn][1]*res.x+shtrack->subc[sn].x + iy*res.x + ix];
				// fi = image->data[(subc[sn][1]+iy) * image->tda + (shtrack->subc[sn].x+ix)];

				if (max != NULL && fi > *max) *max = fi;
				
				csum += fi;				// add this pixel's intensity to sum
				csx += fi * ix; 		// center of gravity of subaperture intensity 
				csy += fi * iy;
			}
		}
		sum += csum;

		if (csum > 0.0) {				// if there is any signal at all
			gsl_vector_float_set(shtrack->disp, 2*sn + 0, \
				csx/csum - shtrack->track.x/2); // positive now
			gsl_vector_float_set(shtrack->disp, 2*sn + 1, \
				csy/csum - shtrack->track.y/2); // /2 because our tracker cells are track[] wide and high
		} 
		else {
			//coords[sn][0] = coords[sn][1] = 0.0;
			gsl_vector_float_set(shtrack->disp, 2*sn + 0, 0.0);
			gsl_vector_float_set(shtrack->disp, 2*sn + 1, 0.0);
		}

		
	}
	
	// Calculate average subaperture intensity
	if (aver != NULL)
		*aver = sum / ((float) (shtrack->track.x * shtrack->track.y * shtrack->nsubap));
}
*/

int modCogTrack(void *image, foam_datat_t data, mod_sh_align_t align, mod_sh_track_t *shtrack, float *aver, float *max) {
	//void modCogTrack(gsl_matrix_float *image, int (*subc)[2], int nsubap, coord_t track, float *aver, float *max, float coords[][2]) {
	int ix, iy, sn=0;
	float csx, csy, csum, fi, cmax; 			// variables for center-of-gravity
	float sum = 0;
	
	// loop over all subapertures (treat those all equal, i.e. no preference for 
	// a reference subaperture)
	for (sn=0; sn < shtrack->nsubap; sn++) {
		csx = 0.0; csy = 0.0; csum = 0.0;
		/*
		for (iy=0; iy < shtrack->track.y; iy++) {
			for (ix=0; ix < shtrack->track.x; ix++) {
				//fi = image[(shtrack->subc[sn].y+iy), shtrack->subc[sn].x+ix);
				//image[subc[sn][1]*res.x+shtrack->subc[sn].x + iy*res.x + ix];
				// fi = image->data[(subc[sn][1]+iy) * image->tda + (shtrack->subc[sn].x+ix)];
				
			}
		}*/
		if (data == DATA_UINT8 && align == ALIGN_SUBAP) {		// data is stored in unsigned 8 bit ints
			uint8_t *datapt = (uint8_t *) data;
			for (iy=0; iy<shtrack->track.y; iy++) { // sum all pixels in the tracker window
				for (ix=0; ix<shtrack->track.x; ix++) {
					fi = datapt[(shtrack->track.y*shtrack->track.x)*sn + (shtrack->track.y+iy)+ix];
					if (max != NULL && fi > *max) *max = fi;
					
					csum += fi;				// add this pixel's intensity to sum
					csx += fi * ix; 		// center of gravity of subaperture intensity 
					csy += fi * iy;					
				}
			}
		}
		else if (data == DATA_GSL_M_F && align == ALIGN_RECT) {	// data is stored in GSL matrix
			gsl_matrix_float *datapt = (gsl_matrix_float *) image;				
			for (iy=0; iy<shtrack->track.y; iy++) { // sum all pixels in the subapt
				for (ix=0; ix<shtrack->track.x; ix++) {
					fi = gsl_matrix_float_get(datapt, (shtrack->subc[sn].y+iy), (shtrack->subc[sn].x+ix));
					if (max != NULL && fi > *max) *max = fi;
					
					csum += fi;				// add this pixel's intensity to sum
					csx += fi * ix; 		// center of gravity of subaperture intensity 
					csy += fi * iy;					
				}
			}
		}
		else {
				logWarn("Unknown datatype/alignment combination in modCogTrack");
				return EXIT_FAILURE;
		}
		sum += csum;
		
		if (csum > 0.0) {				// if there is any signal at all
			gsl_vector_float_set(shtrack->disp, 2*sn + 0, \
								 csx/csum - shtrack->track.x/2); // positive now
			gsl_vector_float_set(shtrack->disp, 2*sn + 1, \
								 csy/csum - shtrack->track.y/2); // /2 because our tracker cells are track[] wide and high
		} 
		else {
			//coords[sn][0] = coords[sn][1] = 0.0;
			gsl_vector_float_set(shtrack->disp, 2*sn + 0, 0.0);
			gsl_vector_float_set(shtrack->disp, 2*sn + 1, 0.0);
		}
		
		
	}
	
	// Calculate average subaperture intensity
	if (aver != NULL)
		*aver = sum / ((float) (shtrack->track.x * shtrack->track.y * shtrack->nsubap));
	
	return EXIT_SUCCESS;
}

void modCogFind(wfs_t *wfsinfo, int xc, int yc, int width, int height, float samini, float *sumout, float *cog) {
	int ix, iy;
	coord_t res = wfsinfo->res;			// image resolution
	float *image = wfsinfo->image;		// source image from sensor
	image += yc * res.x + xc;				// forward the pointer to the right coordinate

	float sum=0.0; float cs[] = {0.0, 0.0}; float csum = 0.0;
	float fi;
	
	for (iy=0; iy<height; iy++) { // sum all pixels in the subapt
		for (ix=0; ix<width; ix++) {
			fi = (float) image[ix+ iy*res.x];
			sum += fi;
			// for center of gravity, only pixels above the threshold are used;
			// otherwise the position estimate always gets pulled to the center;
			// good background elimination is crucial for this to work !!!
			fi -= samini;    		// subtract threshold
			if (fi<0.0) fi = 0.0;	// clip
			csum = csum + fi;		// add this pixel's intensity to sum
			cs[0] += + fi * ix;	// center of gravity of subaperture intensity 
			cs[1] += + fi * iy;
		}
	}
	*sumout = csum;
	cog[0] = cs[0]/csum;
	cog[1] = cs[1]/csum;
}

int modParseSH(gsl_matrix_float *image, int (*subc)[2], int (*gridc)[2], int nsubap, coord_t track, gsl_vector_float *disp, gsl_vector_float *refc) {
	float aver=0.0, max=0.0;
	float rmsx=0.0, rmsy=0.0;
	float maxx=0, maxy=0;
	float coords[nsubap][2];
	int i;

	// track the maxima using CoG
	printf("modcogtrack turned off\n");
	//modCogTrack(image, subc, nsubap, track, &aver, &max, coords);
	
	// logDebug(0 | LOG_SOMETIMES, "Coords: ");	
	for (i=0; i<nsubap; i++) {
		// store the displacement vectors (wrt center of subaperture grid):
		// we get subc which is the coordinate of the tracker window wrt the complete sensor image
		// subtracting the grid coordinat that this subaperture belongs to gives us the vector wrt to
		// the grid coordinate. After this, we subtract a quarter of the size of the subaperture
		// grid. If the tracker window would be centered in the grid, subc-gridc-track.x|y/4 would be zero.
		// Last, we add the coordinate to get the displacement wrt to the center of the grid,
		// instead of wrt to the center of the tracker window.

		gsl_vector_float_set(disp, 2*i+0, subc[i][0] - gridc[i][0] - track.x/2 + (coords[i][0]) - gsl_vector_float_get(refc, 2*i+0));
		gsl_vector_float_set(disp, 2*i+1, subc[i][1] - gridc[i][1] - track.y/2 + (coords[i][1]) - gsl_vector_float_get(refc, 2*i+1));
		// logDebug(LOG_NOFORMAT| LOG_SOMETIMES, "(%.3f,%.3f) ", gsl_vector_float_get(disp, 2*i+0), gsl_vector_float_get(disp, 2*i+0));
		// logDebug(LOG_NOFORMAT| LOG_SOMETIMES, "(%.3f,%.3f) ", coords[i][0], coords[i][1]);

		// Track maximum displacement within the tracker window. This should not be above 1 for long,
		// otherwise the seeing is too bad for the AO system to compensate: if this value is above 1,
		// the tracker window should follow the SH spot, and thus the next frame it should again
		// be below one. If this is not the case the seeing changes too fast.
		if (fabs(coords[i][0]) > maxx) maxx = fabs(coords[i][0]);
		if (fabs(coords[i][1]) > maxy) maxy = fabs(coords[i][1]);

		rmsx += gsl_pow_2((double) gsl_vector_float_get(disp, 2*i+0));
		rmsy += gsl_pow_2((double) gsl_vector_float_get(disp, 2*i+1));
		
		// check for runaway subapts and recover them:
		if (gsl_vector_float_get(disp, 2*i+0) > track.x || 
			gsl_vector_float_get(disp, 2*i+0) < -track.x ||
			gsl_vector_float_get(disp, 2*i+1) > track.y || 
			gsl_vector_float_get(disp, 2*i+1) < -track.y) {
			//logDebug(LOG_NOFORMAT| LOG_SOMETIMES, "Runaway subapt (%d) at: (%.3f,%.3f) ", 2*i, gsl_vector_float_get(disp, 2*i+0), gsl_vector_float_get(disp, 2*i+0));
		}
		// note: coords is relative to the center of the tracker window
		// therefore we can simply update the lower left coord by subtracting the coordinates.
		subc[i][0] += round(coords[i][0]);
		subc[i][1] += round(coords[i][1]);
	}
	// logDebug(LOG_NOFORMAT| LOG_SOMETIMES, "\n");

	rmsx = sqrt(rmsx/nsubap);
	rmsy = sqrt(rmsy/nsubap);
	logInfo(LOG_SOMETIMES, "RMS displacement wrt reference: x: %.3f, y: %.3f", rmsx, rmsy);
	logInfo(LOG_SOMETIMES, "Max abs disp in tracker window (<(.5, .5)): (%.3f, %.3f)", maxx, maxy);
	
	return EXIT_SUCCESS;
}

int modCalcCtrl(control_t *ptc, mod_sh_track_t *shtrack, const int wfs, int nmodes) {
	logDebug(LOG_SOMETIMES, "Calculating WFC ctrls");
	// function assumes presence of dmmodes, singular and wfsmodes...
	// TODO: this has to be placed elsewhere, in the end...
	if (shtrack->dmmodes == NULL || shtrack->singular == NULL || shtrack->wfsmodes == NULL) {
		logWarn("Cannot calculate WFC ctrls, calibration incomplete.");
		return EXIT_FAILURE;
	}
	
	int i,j, wfc;   			// index variables
//	float sum; 					// temporary sum
	int nacttot=0;
	size_t oldsize;
	int nsubap = shtrack->nsubap;
	float wi, alpha;
	
	// calculate total nr of act for all wfc
	for (wfc=0; wfc< ptc->wfc_count; wfc++)
		nacttot += ptc->wfc[wfc].nact;
	
	// set to maxmimum if 0 or less is passed.
	if (nmodes <= 0) 
		nmodes = nacttot;
	else if (nmodes > nacttot) {
		logWarn("nmodes cannot be higher than the total number of actuators, cropping.");
		nmodes = nacttot;
	}

	static gsl_vector_float *work=NULL, *total=NULL; // temp work vector and vector to store all control commands for all WFCs
	if (work == NULL)
		work = gsl_vector_float_calloc(nacttot);
	if (total == NULL)
		total = gsl_vector_float_calloc(nacttot);

	// TODO: this is a hack :P (problem: disp vector is 
	// allocated more space than used, but at allocation time,
	// this is unknown we now tell gsl that the vector is 
	// only as long as we're using, while the actual allocated space is more)
//	oldsize = ptc->wfs[wfs].disp->size;
	oldsize = shtrack->disp->size;
	shtrack->disp->size = nsubap*2;
	
	logDebug(LOG_SOMETIMES, "Calculating control stuff for WFS %d (modes: %d)", wfs, nmodes);
	for (i=0; i < nsubap; i++)
		logDebug(LOG_SOMETIMES | LOG_NOFORMAT, "(%f, %f)", gsl_vector_float_get(shtrack->disp, 2*i+0), gsl_vector_float_get(shtrack->disp, 2*i+1));
		
	logDebug(LOG_SOMETIMES | LOG_NOFORMAT, "\n");
	
//	gsl_linalg_SV_solve(ptc->wfs[wfs].wfsmodes, v, sing, testout, testinrec);	
	
	// Below we calculate the control vector to the actuators using
	// the measured displacements in the subapertures. We use the
	// previously pseudo-inverted influence matrix of the actuators
	// to solve this problem.

	// we multiply the disp vector with the wfsmodes matrix here,
	// using the GSL link to a CBLAS in single precision (floats)
    gsl_blas_sgemv (CblasTrans, 1.0, shtrack->wfsmodes, shtrack->disp, 0.0, work);

	// we multiply the output vector from above with 'nmodes' singular
	// values to get through the second stage of the SVD thing
    for (i = 0; i < nmodes; i++) {
        wi = gsl_vector_float_get (work, i);
        alpha = gsl_vector_float_get (shtrack->singular, i);
        if (alpha != 0)
          alpha = 1.0 / alpha;
        gsl_vector_float_set (work, i, alpha * wi);
      }

	// Finally we multiply the output vector of the previous multiplication
	// with the dmmodes matrix.
    gsl_blas_sgemv (CblasNoTrans, 1.0, shtrack->dmmodes, work, 0.0, total);
	
	
	// // U^T . meas:
	// gsl_blas_sgemv(CblasTrans, 1.0, ptc->wfs[wfs].wfsmodes, ptc->wfs[wfs].disp, 0.0, total);
	// 
	// // diag(1/w_j) . (U^T . meas):
	// for (j=0; j<nmodes; j++)
	// 	gsl_vector_float_set(work, j, gsl_vector_float_get(total, j) /gsl_vector_float_get(ptc->wfs[wfs].singular, j));
	// 
	// // V . (diag(1/w_j) . (U^T . meas)):
	// gsl_blas_sgemv(CblasNoTrans, 1.0, ptc->wfs[wfs].dmmodes, work, 0.0, total);
	
	// After calculating the control vector in one go (one vector for all 
	// controls), we split these up into the seperate vectors assigned
	// to each WFC. In doing so we apply the gain at the same time, 
	// as the calculated controls are actually *corrections* to the
	// control commands already being used
	logDebug(LOG_SOMETIMES, "Storing reconstructed actuator command to seperate vectors:");
	j=0;
	float old, ctrl;
	for (wfc=0; wfc< ptc->wfc_count; wfc++) {
		for (i=0; i<ptc->wfc[wfc].nact; i++) {
			ctrl = gsl_vector_float_get(total, j);
			old = gsl_vector_float_get(ptc->wfc[wfc].ctrl, i);
			
			gsl_vector_float_set(ptc->wfc[wfc].ctrl, i, old- (ctrl*ptc->wfc[wfc].gain.d)); 
			logDebug(LOG_SOMETIMES | LOG_NOFORMAT, "%f -> (%d,%d) ", gsl_vector_float_get(ptc->wfc[wfc].ctrl, i), wfc, i);
			j++;
//			if (j >= nmodes) break;
		}
		logDebug(LOG_SOMETIMES | LOG_NOFORMAT, "\n");
	}
	logDebug(LOG_SOMETIMES | LOG_NOFORMAT, "\n");
	
	// unhack the disp vector (see above)
	shtrack->disp->size = oldsize;
			
	return EXIT_SUCCESS;
}

float sae(float *subapt, float *refapt, int len) {
	// *ip     pointer to first pixel of each subaperture
	// *rp     pointer to first pixel of (shifted) reference

	float sum=0;
	int i;
	
	// loop over all pixels
	for (i=0; i < len; i++)
		sum += fabs(subapt[i] - refapt[i]);

	return sum;
}


// TODO: on the one hand depends on int wfs, on the other hand needs int window[2], fix that
void imcal(float *corrim, float *image, float *darkim, float *flatim, float *sum, float *max, coord_t res, coord_t window) {
	// substract the dark, multiply with the flat (right?)
	// TODO: dark and flat currently ignored, fix that
	int i,j;
	
	// corrim comes from:
	// cp = &corr[sn*shsize[0]*shsize[1]]; // calibrated image (output)
	// image comes form:
	// ip = &image[subc[sn][0]*res.x+subc[sn][1]]; // raw image (input)
		
	// we do shsize/2 because the tracker windows are only a quarter of the
	// total tracker area
	for (i=0; i<window.y; i++) {
		for (j=0; j<window.x; j++) {
			// We correct the image stored in 'image' by applying some dark and flat stuff,
			// and copy it to 'corrim' which has a different format than image.
			// Image is a row-major array of res.x * res.y, where subapertures are hard to
			// read out (i.e. you have to skip pixels and stuff), while corrim is reformatted
			// in such a way that the first shsize[0]*shsize[1] pixels are exactly one subapt,
			// and each consecutive set is again one seperate subapt. This way you can loop
			// through subapts with only one counter (right?)

			corrim[i*window.x + j] = image[i*res.x + j];
			*sum += corrim[i*window.x + j];
			if (corrim[i*window.x + j] > *max) *max = corrim[i*window.x + j];
		}
	}			
	
}
