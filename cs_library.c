/*! @file cs_library.c
@brief Library file for the Control Software

Tim van Werkhoven, November 13 2007\n
Last: 2008-01-21
*/

#include "cs_library.h"

control_t ptc = { //!< Global struct to hold system characteristics and other data. Initialize with complete but minimal configuration
	.mode = AO_MODE_OPEN,
	.wfs_count = 0,
	.wfc_count = 0,
	.frames = 0
};

config_t cs_config = { 
	.listenip = "0.0.0.0",
	.listenport = 10000,
	.infofd = NULL,
	.errfd = NULL,
	.debugfd = NULL,
	.use_syslog = false,
	.syslog_prepend = "foam",
	.use_stderr = true,
	.loglevel = LOGDEBUG
};

conntrack_t clientlist;

static int formatLog(char *output, const char *prepend, const char *msg) {
	char timestr[9];
	time_t curtime;
	struct tm *loctime;

	curtime = time (NULL);
	loctime = localtime (&curtime);
	strftime (timestr, 9, "%H:%M:%S", loctime);

	output[0] = '\0'; // reset string
	
	strcat(output,timestr);
	strcat(output,prepend);
	strcat(output,msg);
	strcat(output,"\n");
	
	return EXIT_SUCCESS;
}

/* logging functions */
void logInfo(const char *msg, ...) {
	if (cs_config.loglevel < LOGINFO) 		// Do we need this loglevel?
		return;
	
	va_list ap, aq, ar; // We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <info>: ", msg);
	
	if (cs_config.infofd != NULL) // Do we want to log this to a file?
		vfprintf(cs_config.infofd, logmessage , ap);
	
	if (cs_config.use_stderr == true) // Do we want to log this to stderr
		vfprintf(stderr, logmessage, aq);
		
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logErr(const char *msg, ...) {
	if (cs_config.loglevel < LOGERR) 	// Do we need this loglevel?
		return;

	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <error>: ", msg);

	
	if (cs_config.errfd != NULL)	// Do we want to log this to a file?
		vfprintf(cs_config.errfd, logmessage, ap);

	if (cs_config.use_stderr == true) // Do we want to log this to stderr?
		vfprintf(stderr, logmessage, aq);
	
	if (cs_config.use_syslog == true) // Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logDebug(const char *msg, ...) {
	if (cs_config.loglevel < LOGDEBUG) 	// Do we need this loglevel?
		return;
		
	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);

	formatLog(logmessage, " <debug>: ", msg);
	
	if (cs_config.debugfd != NULL) 	// Do we want to log this to a file?
		vfprintf(cs_config.debugfd, logmessage, ap);
	
	if (cs_config.use_stderr == true)	// Do we want to log this to stderr?
		vfprintf(stderr, logmessage, aq);

	
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_DEBUG, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void selectSubapts(float *image, float samini, int samxr, int wfs) {
	// stolen from ao3.c by CUK
	int isy, isx, iy, ix, i, sn=0, nsubap=0; //init sn to zero!!
	float sum=0.0, fi;			// check 'intensity' of a subapt
	float csum=0.0, cs[] = {0.0,0.0}; // for center of gravity
	int shsize[2];			// size of one subapt (in pixels)
	int res[2], cells[2];	// size of whole image, nr of cells
	float cx=0, cy=0;		// for CoG
	float dist, rmin;		// minimum distance
	int csa=0;				// estimate for best subapt
	int (*subc)[2] = ptc.wfs[wfs].subc;	// TODO: does this work?

	res[0] = ptc.wfs[wfs].res[0];	// shortcuts
	res[1] = ptc.wfs[wfs].res[1];
	cells[0] = ptc.wfs[wfs].cells[0];
	cells[1] = ptc.wfs[wfs].cells[1];

	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y
	
	// TODO: what's this? why do we use this?
	int apmap[cells[0]][cells[1]];		// aperture map
	int apmap2[cells[0]][cells[1]];		// aperture map 2
	int apcoo[cells[0] * cells[1]][2];  // subaperture coordinates in apmap
	
	
	//
	//cells[0]
	for (isy=0; isy<cells[1]; isy++) { // loops over all potential subapertures
		for (isx=0; isx<cells[0]; isx++) {
			// check one potential subapt (isy,isx)

			sum=0.0; cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
			for (iy=0; iy<shsize[1]; iy++) { // sum all pixels in the subapt
				for (ix=0; ix<shsize[0]; ix++) {
					fi = (float) image[isy*shsize[1]*res[0] + isx*shsize[0] + ix+ iy*res[0]];
					sum += fi;
					/* for center of gravity, only pixels above the threshold are used;
					otherwise the position estimate always gets pulled to the center;
					good background elimination is crucial for this to work !!! */
					fi -= samini;    		// subtract threshold
					if (fi<0.0) fi=0.0;		// clip
					csum = csum + fi;		// add this pixel's intensity to sum
					cs[0] += + fi * ix;	// center of gravity of subaperture intensity 
					cs[1] += + fi * iy;
				}
			}

			// check if the summed subapt intensity is above zero (e.g. do we use it?)
			if (csum > 0.0) { // good as long as pixels above background exist
				subc[sn][0] = isx*shsize[0]+shsize[0]/4 + (int) (cs[0]/csum) - shsize[0]/2;	// subapt coordinates
				subc[sn][1] = isy*shsize[1]+shsize[1]/4 + (int) (cs[1]/csum) - shsize[1]/2;	// TODO: how does this work? 
							//coordinate in big image, CoG of one subapt, 							
																				// why 4? (partial subapt?)
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
			fi = (float) image[(subc[0][1]-0+iy)*res[0]+subc[0][0]-0+ix]; // TODO: fix the static '4' --> -0 works, but why?
			
			/* for center of gravity, only pixels above the threshold are used;
			otherwise the position estimate always gets pulled to the center;
			good background elimination is crucial for this to work !!! */
			fi -= samini;
			if (fi < 0.0) fi=0.0;
			csum += fi;
			cs[0] += fi * ix;
			cs[1] += fi * iy;
		}
	}
	
	logDebug("old subx=%d, old suby=%d",subc[0][0],subc[0][1]);
	subc[0][0] += (int) (cs[0]/csum+0.5) - shsize[0]/2; // +0.5 rounding error
	subc[0][1] += (int) (cs[1]/csum+0.5) - shsize[1]/2;
	logDebug("new subx=%d, new suby=%d",subc[0][0],subc[0][1]);

	// enforce maximum radial distance from center of gravity of all
	// potential subapertures if samxr is positive
	if (samxr > 0) {
		sn = 1;
		while (sn < nsubap) {
			if (sqrt((subc[sn][0]-cx)*(subc[sn][0]-cx) + \
				(subc[sn][1]-cy)*(subc[sn][1]-cy)) > samxr) { // TODO: why remove subapts? might be bad subapts
				for (i=sn; i<(nsubap-1); i++) {
					subc[i][0] = subc[i+1][0];
					subc[i][1] = subc[i+1][1];
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
		for (isy=0; isy<cells[1]; isy++) {
			for (isx=0; isx<cells[0]; isx++)
				if (apmap[isx][isy] == 0) printf(" "); 
				else printf("X");
			printf("\n");
		}
		
		sn = 1; // start with subaperture 1 since subaperture 0 is the reference
		while (sn < nsubap) {
			isx = apcoo[sn][0]; isy = apcoo[sn][1];
			if ((isx == 0) || (isx >= 15) || (isy == 0) || (isy >= 15) ||
				(apmap[isx-1][isy] == 0) ||
				(apmap[isx+1][isy] == 0) ||
				(apmap[isx][isy-1] == 0) ||
				(apmap[isx][isy+1] == 0)) {
					
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
	ptc.wfs[wfs].nsubap = nsubap; // store to global configuration struct
	logInfo("Selected %d usable subapertures", nsubap);
	
	// set remaining subaperture coordinates to 0
	for (sn=nsubap; sn<cells[0]*cells[1]; sn++) {
		subc[sn][0] = 0; 
		subc[sn][1] = 0;
	}
	
/*	int cfd;
	char *cfn;

	// write subaperture image into file
	cfn = "ao_saim.dat";
	cfd = creat(cfn,0);
	if (write(cfd, image, res[0]*res[1]) != res[0]*res[1])
		logErr("Unable to write subapt img to file");

	close(cfd);
	logInfo("Subaperture definition image saved in file %s",cfn);*/
}

void corrTrack(int wfs, float *aver, float *max, float coords[][2]) {
	// center of gravity tracking here (easy)

	int ix, iy, sn=0;
	float csx, csy, csum, fi; 			// variables for center-of-gravity
	float sum = 0;

	float *image = ptc.wfs[wfs].image;
//	float *dark = ptc.wfs[wfs].darkim
//	float *flat = ptc.wfs[wfs].flatim
	float *corr = ptc.wfs[wfs].corrim;
	int nsubap = ptc.wfs[wfs].nsubap;
	int (*subc)[2] = ptc.wfs[wfs].subc;	// TODO: does this work?
	
	int res[2], cells[2], shsize[2];
	res[0] = ptc.wfs[wfs].res[0];		// shortcuts, TODO: can be done faster like res = ptc.wfs.res ?
	res[1] = ptc.wfs[wfs].res[1];
	cells[0] = ptc.wfs[wfs].cells[0];
	cells[1] = ptc.wfs[wfs].cells[1];

	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x (width)
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y (height)

	*max = 0; // set maximum of all raw subapertures to 0
	
//	uint16_t *gp, *dp; // pointers to gain and dark
	float  *ip, *cp; // pointers to raw, processed image

	logDebug("Starting corrTrack for %d subapts (CoG mode)", nsubap);
	
	// TODO: make imcal process specific tracker window sizes
	// TODO: process center subapt seperately (bigger window)
	
	// loop over all subapertures
	for (sn=0;sn<nsubap;sn++) {

		// --->>> might use some pointer incrementing instead of setting
		//        addresses from scratch every time
		// set pointers to various 'images'
		ip = &image[subc[sn][1]*res[0]+subc[sn][0]]; // raw image (input)
//		fprintf(stderr, "i: %d", subc[sn][0]*res[0]+subc[sn][1]);
//		dp = &dark[sn*shsize[0]*shsize[1]]; // dark (input)
//		gp = &gain[sn*shsize[0]*shsize[1]]; // gain (output)
		cp = &corr[sn*shsize[0]*shsize[1]]; // calibrated image (output)

		// dark and flat correct subaperture, determine statistical quantities
		imcal(cp, ip, NULL, NULL, wfs, &sum, max);

		// center-of-gravity
		csx = 0.0; csy = 0.0; csum = 0.0;
		for (iy=0; iy<shsize[1]; iy++) {
			for (ix=0; ix<shsize[0]; ix++) {
				fi = (float) corr[sn*shsize[0]*shsize[1]+iy*shsize[0]+ix];

				csum += + fi;    // add this pixel's intensity to sum
				csx += + fi * ix; // center of gravity of subaperture intensity 
				csy += + fi * iy;
			}

			if (csum > 0.0) { // if there is any signal at all
				coords[sn][0] = -csx/csum + shsize[0]/2; // negative for consistency with CT 
				coords[sn][1] = -csy/csum + shsize[1]/2;
			//	if (sn<0) printf("%d %f %f\n",sn,stx[sn],sty[sn]);
			} 
			else {
				coords[sn][0] = 0.0;
				coords[sn][1] = 0.0;
			}
		}

	} // end of loop over all subapertures

	// average intensity over all subapertures
	*aver = sum / ((float) (shsize[0]*shsize[1]*nsubap));
}

float sae(float *subapt, float *refapt, long res[2]) {
	// *ip     pointer to first pixel of each subaperture
	// *rp     pointer to first pixel of (shifted) reference

	float sum=0;
	int i,j;
	
	// loop over alle pixels
	for (i=0; i < res[1]; i++) {
		for (j=0; j<res[0]; j++) {
			sum += fabs(subapt[i*res[0] + j] - refapt[i*res[0] + j]);
		}
	}
	return sum;
}

int modParseSH(int wfs) {
	// now calculate the offsets 
	float aver=0.0, max=0.0;
	float coords[ptc.wfs[wfs].nsubap][2];
	int i;
	
	// track the maxima
	corrTrack(wfs, &aver, &max, coords); // TODO: dit werkt niet goed? hoe 2d arrays als pointer meegeven?
	
	// update the subapt locations
	logDebug("We have %d coords for wfs %d:", ptc.wfs[wfs].nsubap, wfs);
	ptc.wfs[wfs].subc[0][0] -= coords[0][0]-ptc.wfs[wfs].res[0]/ptc.wfs[wfs].cells[0]/4;
	ptc.wfs[wfs].subc[0][1] -= coords[0][1]-ptc.wfs[wfs].res[1]/ptc.wfs[wfs].cells[1]/4;
	logDebug("was: (%d,%d), found (%f,%f)", ptc.wfs[wfs].subc[0][0], ptc.wfs[wfs].subc[0][1], coords[0][0], coords[0][1]);	
	for (i=1; i<ptc.wfs[wfs].nsubap; i++) {
		ptc.wfs[wfs].subc[i][0] -= coords[i][0]-ptc.wfs[wfs].res[0]/ptc.wfs[wfs].cells[0]/4;
		ptc.wfs[wfs].subc[i][1] -= coords[i][1]-ptc.wfs[wfs].res[1]/ptc.wfs[wfs].cells[1]/4;
	}
	
	return EXIT_SUCCESS;
}

/*!
@brief ADD DOC
*/
void imcal(float *corrim, float *image, float *darkim, float *flatim, int wfs, float *sum, float *max) {
	// substract the dark, multiply with the flat (right?)
	// TODO: dark and flat currently ignored, fix that
	int shsize[2];
	int i,j;
	
	// corrim comes from:
	// cp = &corr[sn*shsize[0]*shsize[1]]; // calibrated image (output)
	// image comes form:
	// ip = &image[subc[sn][0]*res[0]+subc[sn][1]]; // raw image (input)
	
	shsize[0] = ptc.wfs[wfs].res[0]/ptc.wfs[wfs].cells[0];
	shsize[1] = ptc.wfs[wfs].res[1]/ptc.wfs[wfs].cells[1];
	
	// we do shsize/2 because the tracker windows are only a quarter of the
	// total tracker area
	for (i=0; i<shsize[1]/2; i++) {
		for (j=0; j<shsize[0]/2; j++) {
			// We correct the image stored in 'image' by applying some dark and flat stuff,
			// and copy it to 'corrim' which has a different format than image.
			// Image is a row-major array of res[0] * res[1], where subapertures are hard to
			// read out (i.e. you have to skip pixels and stuff), while corrim is reformatted
			// in such a way that the first shsize[0]*shsize[1] pixels are exactly one subapt,
			// and each consecutive set is again one seperate subapt. This way you can loop
			// through subapts with only one counter (right?)

			corrim[i*shsize[0] + j] = image[i*ptc.wfs[wfs].res[0] + j];			
			*sum += corrim[i*shsize[0] + j];
			if (corrim[i*shsize[0] + j] > *max) *max = corrim[i*shsize[0] + j];
		}
	}			
	
}

// Graphic routines, do we want these in the CS?


int drawSubapts(int wfs, SDL_Surface *screen) {
	if (ptc.wfs[wfs].nsubap == 0)
		return EXIT_SUCCESS;	// if there's nothing to draw, don't draw (shouldn't happen)
		
	int shsize[2];			// size of one subapt (in pixels)
	int res[2], cells[2];	// size of whole image, nr of cells
	int (*subc)[2] = ptc.wfs[wfs].subc;	// TODO: does this work?

	res[0] = ptc.wfs[wfs].res[0];	// shortcuts
	res[1] = ptc.wfs[wfs].res[1];
	cells[0] = ptc.wfs[wfs].cells[0];
	cells[1] = ptc.wfs[wfs].cells[1];	
	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y
	
	int sn=0;
	Slock(screen);
	drawRect(subc[0], shsize, screen);
	int subsize[2] = {shsize[0]/2, shsize[1]/2};
	
	for (sn=1; sn<ptc.wfs[wfs].nsubap; sn++) {
		// subapt with lower coordinates (subc[sn][0],subc[sn][1])
		// first subapt has size (shsize[0],shsize[1]),
		// the rest are (shsize[0]/2,shsize[1]/2)
		drawRect(subc[sn], subsize, screen);
	}
	
	Sulock(screen);
	SDL_Flip(screen);	
	return EXIT_SUCCESS;
}

/*!
document, add color
*/
void drawRect(int coord[2], int size[2], SDL_Surface *screen) {


	// lower line
	drawLine(coord[0], coord[1], coord[0] + size[0], coord[1], screen);
	// top line
	drawLine(coord[0], coord[1] + size[1], coord[0] + size[0], coord[1] + size[1], screen);
	// left line
	drawLine(coord[0], coord[1], coord[0], coord[1] + size[1], screen);
	// right line
	drawLine(coord[0] + size[0], coord[1], coord[0] + size[0], coord[1] + size[1], screen);
	// done


}

/*!
document, add color?
*/
void drawLine(int x0, int y0, int x1, int y1, SDL_Surface*screen) {
	int step = abs(x1-x0);
	int i;
	if (abs(y1-y0) > step) step = abs(y1-y0); // this can be done faster?
	 
	float dx = (x1-x0)/(float) step;
	float dy = (y1-y0)/(float) step;

	DrawPixel(screen, x0, y0, 255, 255, 255);
	for(i=0; i<step; i++) {
		x0 = round(x0+dx); // round because integer casting would floor, resulting in an ugly line (?)
		y0 = round(y0+dy);
		DrawPixel(screen, x0, y0, 255, 255, 255); // draw directly to the screen in white
	}
}

int displayImg(float *img, long res[2], SDL_Surface *screen) {
	// ONLY does float images
	int x, y, i;
	float max=img[0];
	float min=img[0];
	
	//logDebug("Displaying image precalc, min: %f, max: %f (%f,%f).", min, max, img[0], img[100]);
	
	// we need this loop to check the maximum and minimum intensity. Do we need that? can't SDL do that?	
	for (x=0; x < res[0]*res[1]; x++) {
		if (img[x] > max)
			max = img[x];
		if (img[x] < min)
			min = img[x];
	}
	
	logDebug("Displaying image, min: %f, max: %f.", min, max);
	Slock(screen);
	for (x=0; x<res[0]; x++) {
		for (y=0; y<res[1]; y++) {
			i = (int) ((img[y*res[0] + x]-min)/(max-min)*255);
			DrawPixel(screen, x, y, \
				i, \
				i, \
				i);
		}
	}
	Sulock(screen);
	SDL_Flip(screen);
	
	return EXIT_SUCCESS;
}

void Slock(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )	{
		if ( SDL_LockSurface(screen) < 0 )
			return;
	}
}

void Sulock(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
}

void DrawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B) {
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);
	switch (screen->format->BytesPerPixel) {
		case 1: // Assuming 8-bpp
			{
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				*bufp = color;
			}
			break;
		case 2: // Probably 15-bpp or 16-bpp
			{
				Uint16 *bufp;
				bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
				*bufp = color;
			}
			break;
		case 3: // Slow 24-bpp mode, usually not used
			{
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x * 3;
				if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
				{
					bufp[0] = color;
					bufp[1] = color >> 8;
					bufp[2] = color >> 16;
				} else {
					bufp[2] = color;
					bufp[1] = color >> 8;
					bufp[0] = color >> 16;
				}
			}
			break;
		case 4: { // Probably 32-bpp
				Uint32 *bufp;
				bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
				*bufp = color;
			}
			break;
	}
}
