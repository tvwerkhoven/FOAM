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
	
	if (cs_config.use_stderr == true) // Do we want to log this to syslog
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

	if (cs_config.use_stderr == true) // Do we want to log this to syslog?
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
	
	// TODO: what's this? why do we use this?
	int apmap[cells[0]][cells[1]];		// aperture map
	int apmap2[cells[0]][cells[1]];		// aperture map 2
	int apcoo[cells[0] * cells[1]][2];  // subaperture coordinates in apmap
	
	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y
	
	//
	//cells[0]
	for (isy=0; isy<cells[1]; isy++) { // loops over all potential subapertures
		for (isx=0; isx<cells[0]; isx++) {
			// check one potential subapt (isy,isx)
			
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
				subc[sn][0] = isx*shsize[0]+4 + (int) (cs[0]/csum) - shsize[0]/2;	// subapt coordinates
				subc[sn][1] = isy*shsize[1]+4 + (int) (cs[1]/csum) - shsize[1]/2;	// TODO: how does this work? 
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
			fi = (float) image[(subc[0][1]-4+iy)*res[0]+subc[0][0]-4+ix]; // TODO: fix the static '4'
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

	int cfd;
	char *cfn;

	// write subaperture image into file
	cfn = "ao_saim.dat";
	cfd = creat(cfn,0);
	if (write(cfd, image, res[0]*res[1]) != res[0]*res[1])
		logErr("Unable to write subapt img to file");

	close(cfd);
	logInfo("Subaperture definition image saved in file %s",cfn);
}

/*!
@brief Tracks the seeing using correlation tracking

TODO: make prototype
*/
void corrTrack(int wfs, int rs, float cx, float cy, 
               float stx[NS], float sty[NS], float *aver, float *sharp,
               int   msae[NS], int *newref, int track, int mode,
               int   *max) {
	
	//(uint8_t imag[NX*NY], int rs, float cx, float cy,  \
	               float stx[NS], float sty[NS], float *aver, float *sharp, \
	               int   msae[NS], int *newref, int track, int mode, int axes, \
	               int   *max)
	// Stolen from ao3.c by Keller
	
	// we want these defines so the compiler can unroll the loops
	#define NP 5
	#define NO (NP/2)

	if (wfs > ptc.wfs_count) {
		logErr("Error, %d is not a valid WFS identifier.", wfs);
		return EXIT_FAILURE;
	}
	
	float *image = ptc.wfs[wfs].image
	float *dark = ptc.wfs[wfs].darkim
	float *flat = ptc.wfs[wfs].flatim
	float *corr = ptc.wfs[wfs].corrim
	int i, j, si, ix, iy, cmin, sn;
	int d, diff[NP][NP];
	float sig[NP];
	float refaver = 0.0, corr = 1.0;

	float sxx, sxxxx, rnp;     			// fixed values for parabola fit
	float sy,sxy,sxxy,x,y,a,b,da,db; 	// variable values for parabola fit

	int nsubap = ptc.wfs[wfs].nsubap;
	
	res[0] = ptc.wfs[wfs].res[0];	// shortcuts
	res[1] = ptc.wfs[wfs].res[1];
	cells[0] = ptc.wfs[wfs].cells[0];
	cells[1] = ptc.wfs[wfs].cells[1];
	
	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y
	
	float *gp, *dp; // pointers to gain and dark
	float  *ip, *cp, *rp; // pointers to raw, processed image, reference


	uint8_t   max8[8]     __attribute__ ((aligned (32))); 
	/* 8 bytes containing current maxima */
	uint16_t  tmpmean4[4] __attribute__ ((aligned (32))); 
	/* 8 bytes containing current average sum */
	uint16_t  tmpcorr4[4] __attribute__ ((aligned (32))), *sp; 
	/* 8 bytes containing correction factor */

	// when working at the limb, we do not correct for transparency 
	// variations since there is a coupling between subaperture intensity
	// and overall shift
	//
	// --->>> but we should really figure out a way to correct anyway at
	// the limb since the transparency variations influence the limb
	// position; we might use pixels well within the disk to separate the
	// transparency fluctuations from the limb position
	if (ptc.scandir != AO_AXES_XY) corr = 1.0;

	// cimage and refim are global variables
	cp = cimage; // pointer to start of calibrated subaperture images
	sp = tmpcorr4; // pointer to temporary 4-vector storage of correction
	*max = 0; // set maximum of all raw subapertures to 0
	si = 0; // set sum of all intensities to 0

	// precompute some values needed to fit a parabola
	// --->>> we could do this only once, not every time shtracker is called
	sxx   = 0.0;
	sxxxx = 0.0;
	for (ix=0; ix<NP; ix++) {
		x = ix - NO;
		sxx   = sxx   + x*x;
		sxxxx = sxxxx + x*x*x*x;
	}
	rnp = 1.0 / (float) NP;
	da = 1.0 / (sxxxx-rnp*sxx*sxx);
	db = 1.0 / sxx;

	// correction factor for transparency fluctuations (e.g. cirrus)
	// 32768 = 2^15
	for (i=0;i<4;i++) tmpcorr4[i] = (uint16_t) (corr * 32768.0);

	// (big) loop over all subapertures
	for (sn=0; sn<nsubap; sn++) {

		// --->>> might use some pointer incrementing instead of setting
		//        addresses from scratch every time
		ip = &image[subc[sn][0]*res[1]+subc[sn][1]]; // set pointers to various 'images'
		dp = &dark[sn*shsize[0]*shsize[1]];
		gp = &gain[sn*shsize[0]*shsize[1]];
		cp = &cimage[sn*shsize[0]*shsize[1]];

		// dark and flat correct subaperture, determine statistical quantities
		imcal(cp, ip, dp, gp, sp, tmpmean4, max8);

		// add intensities in temporary 4-vector
		for (j=0;j<4;j++) si = si + tmpmean4[j];

		// --->>> should really do a horizontal max() in asm statement
		for (j=0;j<8;j++) {
		//      printf("%d ",max8[j]);
		//      if (max8[j] > *max) *max = max8[j];
		// the following version might be faster because the branch prediction
		// is correct more frequently
					if (max8[j] <= *max) continue;
					*max = max8[j];
				}
//    printf("\n");


// correlation tracking

				cmin = 1000000; /* large positive integer */

				switch (ptc.scandir) {
					case AO_AXES_XY:
					ip = &cimage[sn*shsize[0]*shsize[1]]; // first pixel of each subaperture
					for (ix=-NO;ix<=NO;ix++) {
// --->>> might explicitely unroll the inner loop
						for (iy=-NO;iy<=NO;iy++) {
							rp = &refim[(iy+4)*RX+ix+4]; // first pixel of shifted reference
							d = sae(ip,rp);
// --->>> squaring could be done in assembler code
							diff[ix+NO][iy+NO] = d*d; // square of SAD
//if (sn==0) printf("ix=%d, iy=%d, d=%d\n",ix,iy,d);
//  if (cmin>d) { // FYI: if{} adds over 30us !!!
//    cmin = d;
//      ndx = dx+ix;
//      ndy = dy+iy;
// }
						}
					}
					break;

					case AO_AXES_X:
					ip = &cimage[sn*shsize[0]*shsize[1]]; // first pixel of each subaperture
					for (ix=-NO;ix<=NO;ix++) {
						rp = &refim[4*RX+ix+4]; // first pixel of shifted reference
						d = sae(ip,rp);
						diff[ix+NO][NO] = d;
						if (cmin>d) {
							cmin = d;
//  ndx = dx+ix;
						}
					}
					break;

					case AO_AXES_Y:
					ip = &cimage[sn*shsize[0]*shsize[1]]; // first pixel of each subaperture
					for (iy=-NO;iy<=NO;iy++) {
						rp = &refim[(iy+4)*RX+4]; // first pixel of shifted reference
						d = sae(ip,rp);
						diff[NO][iy+NO] = d;
						if (cmin>d) {
							cmin = d;
//  ndy = dy+iy;
						}
					}
					break;
				} /* end of switch (ptc.scandir) statement */

				asm volatile ("emms           " /* recover from MMX use, allow FP */
					: /* no output */
				: /* no input */
				);

				msae[sn] = cmin;            /* minimum sum of absolute differences */

//    if (sn==0) {
//      for (ix=0;ix<NP;ix++) { 
//for (iy=0;iy<NP;iy++) printf("%d  ",diff[ix][iy]);
//printf("\n");
//      }
//      printf("\n");
//    }

// subpixel interpolation of minimum position
// currently uses two 1-D approaches

				if ((ptc.scandir==AO_AXES_X) || (ptc.scandir==AO_AXES_XY)) {
					if (ptc.scandir==AO_AXES_XY) { // average values over the y-axis
					for (ix=0;ix<NP;ix++) { 
						sig[ix] = 0.0;
						for (iy=0;iy<NP;iy++) sig[ix] = sig[ix] + (float) diff[ix][iy];
					}
				} else {
					for (ix=0;ix<NP;ix++) { 
						sig[ix] = diff[ix][NO];
					}
				}
				sy   = 0.0;
				sxy  = 0.0;
				sxxy = 0.0;
				for (ix=0;ix<NP;ix++) {
					x = ix - NO;
					y = sig[ix];
					sy   = sy + y;
					sxy  = sxy + x*y;
					sxxy = sxxy + x*x*y;
				}

// --->>> this useless line is required for the gcc inline function 
//        to work properly !!!
				if (sn<0) printf("%f\n",sy);

				if (sy>0.0) {
//if (sn==0) printf("rnp=%f sxx=%f sxxxx=%f sxy=%f sxxy=%f\n",
//  rnp,   sxx,   sxxxx,   sxy,   sxxy);
					a = (sxxy-rnp*sxx*sy) * da;
					b = sxy * db;
					if (a!=0.0) stx[sn] = -0.5 * b/a; // +(float) (dx); 
					else stx[sn]=0.0;
					} else
						stx[sn]=0.0;
				} else {
					stx[sn] = 0.0;
				}

				if ((ptc.scandir==AO_AXES_Y) || (ptc.scandir==AO_AXES_XY)) {
					if (ptc.scandir==AO_AXES_XY) { // average values over the x-axis
					for (iy=0;iy<NP;iy++) {
						sig[iy] = 0.0;
						for (ix=0;ix<NP;ix++) sig[iy] = sig[iy] + (float) diff[ix][iy];
					}
				} else {
					for (iy=0;iy<NP;iy++) {
						sig[iy] = diff[NO][iy];
					}
				}
				sy   = 0.0;
				sxy  = 0.0;
				sxxy = 0.0;
				for (ix=0;ix<NP;ix++) {
					x = ix - NO;
					y = sig[ix];
					sy   = sy   + y;
					sxy  = sxy  + x*y;
					sxxy = sxxy + x*x*y;
				}
				if (sy>0.0) {
					a = (sxxy-rnp*sxx*sy) * da;
					b = sxy * db;
					if (a!=0.0) sty[sn] = -0.5 * b/a; // +(float) (dy); 
					else sty[sn]=0.0;
					} else 
						sty[sn]=0.0;
				} else {
					sty[sn] = 0.0;
				}
			} // end of loop over all subapertures

		// average intensity over all subapertures
		*aver  = si / ((float) (shsize[0]*shsize[1]*nsubap));

		// calculate average reference subaperture intensity if new reference 
		// was just taken
			if (*newref == AO_REFIM_NONE) { // new reference is valid
				printf("old correction factor: %f\n",corr);
			refaver = 0.0;  // average reference subaperture intensity
			for (ix=0;ix<shsize[0];ix++)
				for (iy=0;iy<shsize[1];iy++)
					refaver = refaver + (float) refim[(iy+4)*RX+ix+4];
			refaver = refaver/(float) (shsize[0]*shsize[1]);
			corr = 1.0; // reset brightness correction
			*newref = AO_REFIM_GOOD;
			printf("new reference, correction factor: %f\n",corr);
		}

// calculate intensity correction factor based on average intensity
		if (refaver == 0.0)
			corr = 1.0;
		else {
			if (*aver>0) corr = corr * refaver / *aver; // update correction factor
// if correction factor is 2 or larger, then the multiplication
// with 2^15 above 'flips' over to something close to zero; we should
// allow at least a factor of 4, but we still need to catch that case
// (maybe using saturated multiplication)
			if (corr>1.999) corr=1.999;
		}
}