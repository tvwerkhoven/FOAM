/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.

 $Id$
 */
/*! 
 @file foam-simstatic.c
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-04-18
 
 @brief This is a static simulation mode, with just a simple image to work with.
 
 This primemodule can be used to benchmark performance of the AO system if no
 AO hardware (camera, TT, DM) is present. This is branched off of the mcmath
 prime module.
 */

// HEADERS //
/***********/

#include "foam-simstatic.h"
#include "types.h"
#include "imgio.h"
#include "io.h"
#include "shtrack.h"

// We need these for modMessage, see foam_cs.c
extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;

extern Io *io;
Imgio *wfi;

// GLOBALS //
/***********/
Shtrack *shtrack;

int modInitModule(foamctrl *ptc, foamcfg *cs_config) {
	io->msg(IO_INFO, "This is the simstatic prime module, enjoy.");
	
	// Setup configuration directives
	cs_config->datadir = "/Users/tim/workdocs/work/dev/foam/data/"; // datadir
	
	ptc->logfrac = 100;             // log verbose messages only every 100 frames

	// configure WFS 0
	// set image to something static
	wfi = new Imgio(cs_config->datadir + "simstatic-irr.pgm", IMGIO_PGM);
	if (wfi->loadImg())
		io->msg(IO_ERR | IO_FATAL, "Error loading image!");
	
	io->msg(IO_INFO, "Got image %dx%dx%d.", wfi->getWidth(), wfi->getHeight(), wfi->getBitpix());

	ptc->wfs[0].image = (uint8_t *) wfi->getData();
	ptc->wfs[0].name = "SH WFS - static";
	ptc->wfs[0].res.x = wfi->getWidth();
	ptc->wfs[0].res.y = wfi->getHeight();
	ptc->wfs[0].bpp = wfi->getBitpix();
	// this is where we will look for dark/flat/sky images
	ptc->wfs[0].scandir = AO_AXES_XY;
	ptc->wfs[0].id = 0;
	ptc->wfs[0].fieldframes = 1000;     // take 1000 frames for a dark or flatfield
		
	// shtrack configuring
	// we have an image of WxH, with a lenslet array of WlxHl, such that
	// each lenslet occupies W/Wl x H/Hl pixels, and we use track.x x track.y
	// pixels to track the CoG or do correlation tracking.
	shtrack = new Shtrack(8, 8);
	shtrack->setSize(ptc->wfs[0].res);
	shtrack->setShsize(ptc->wfs[0].res.x / 8, ptc->wfs[0].res.y / 8);
	shtrack->setTrack(0.5);
	
	shtrack->setPHFile(cs_config->datadir + FOAM_CONFIG_PRE + \
		"_pinhole.gsldump");
	shtrack->setInfFile(cs_config->datadir + FOAM_CONFIG_PRE + \
		"_influence.gsldump");
	 		
	shtrack->setSamaxr(-1);				// 1 cell row edge erosion
	shtrack->setSamini(30);				// Min intensity for subapts
		
	return EXIT_SUCCESS;
}

int modPostInitModule(foamctrl *ptc, foamcfg *cs_config) {
	return EXIT_SUCCESS;
}

void modStopModule(foamctrl *ptc) {
	delete shtrack;
	delete wfi;
}

// OPEN LOOP ROUTINES //
/*********************/

int modOpenInit(foamctrl *ptc) {
	return EXIT_SUCCESS;
}

int modOpenLoop(foamctrl *ptc) {
	//MMDarkFlatFullByte(&(ptc->wfs[0]), &shtrack);
	
	shtrack->cogFind(&(ptc->wfs[0]));
	
	usleep(1000);
	return EXIT_SUCCESS;
}

int modOpenFinish(foamctrl *ptc) {
	// stop
	return EXIT_SUCCESS;
}

// CLOSED LOOP ROUTINES //
/************************/

int modClosedInit(foamctrl *ptc) {
	return EXIT_SUCCESS;
}

int modClosedLoop(foamctrl *ptc) {
	int sn;
	
	// dark-flat the whole frame
	// MMDarkFlatSubapByte(&(ptc->wfs[0]), &shtrack);

	// try to get the center of gravity 
	shtrack->cogFind(&(ptc->wfs[0]));
	//shCogTrack(ptc->wfs[0].corr, DATA_UINT8, ALIGN_SUBAP, &shtrack, NULL, NULL);

	usleep(1000);
	return EXIT_SUCCESS;
}

int modClosedFinish(foamctrl *ptc) {
	return EXIT_SUCCESS;
}

// MISC ROUTINES //
/*****************/

int modCalibrate(foamctrl *ptc) {
	FILE *fieldfd;
	int i, j, sn;
	float min, max, sum, pix;
	wfs_t *wfsinfo = &(ptc->wfs[0]);
	
	if (ptc->calmode == CAL_DARK) {
		// take dark frames, and average
		io->msg(IO_INFO, "Starting darkfield calibration now");

		// we fake a darkfield here (random pixels between 2-6)
		min = max = 0.0;
		sum = 0.0;
		for (i=0; i<wfsinfo->res.y; i++) {
			 for (j=0; j<wfsinfo->res.x; j++) {
				 pix = (drand48()*4.0)+2.0;
				 gsl_matrix_float_set(wfsinfo->darkim, i, j, pix);
				 if (pix > max) max = pix;
				 else if (pix < min) min = pix;
				 sum += pix;
			 }
		}

		// saving image for later usage
		fieldfd = fopen(wfsinfo->darkfile.c_str(), "w+");	
		if (!fieldfd) return io->msg(IO_WARN, "Could not open darkfield storage file '%s', not saving darkfield (%s).", wfsinfo->darkfile.c_str(), strerror(errno));
		gsl_matrix_float_fprintf(fieldfd, wfsinfo->darkim, "%.10f");
		fclose(fieldfd);
		io->msg(IO_INFO, "Darkfield calibration done (m: %f, m: %f, s: %f, a: %f), and stored to disk.", min, max, sum, sum/(wfsinfo->res.x * wfsinfo->res.y));
	}
	else if (ptc->calmode == CAL_FLAT) {
		io->msg(IO_INFO, "Starting flatfield calibration now");

		// set flat constant so we get a gain of 1
		gsl_matrix_float_set_all(wfsinfo->darkim, 32.0);
		
		// saving image for later usage
		fieldfd = fopen(wfsinfo->flatfile.c_str(), "w+");	
		if (!fieldfd) return io->msg(IO_WARN, "Could not open flatfield storage file '%s', not saving flatfield (%s).", wfsinfo->flatfile.c_str(), strerror(errno));
		gsl_matrix_float_fprintf(fieldfd, wfsinfo->flatim, "%.10f");
		fclose(fieldfd);
		io->msg(IO_INFO, "Flatfield calibration done, and stored to disk.");
	}
	else if (ptc->calmode == CAL_DARKGAIN) {
		io->msg(IO_INFO, "Taking dark and flat images to make convenient images to correct (dark/gain).");
		io->msg(IO_INFO, "Dark and gain fields initialized");
	}
	else if (ptc->calmode == CAL_SUBAPSEL) {
		io->msg(IO_INFO, "Starting subaperture selection now");
		// no need to get get an image, it's alway the same for static simulation
		// run subapsel on this image
		shtrack->sh->ns = shtrack->selSubaps(&(ptc->wfs[0]));

		io->msg(IO_INFO, "Subaperture selection complete, found %d subapertures.", shtrack->sh->ns);
	}
	
	return EXIT_SUCCESS;
}

int modMessage(foamctrl *ptc, Connection *connection, string cmd, string line) {
	// Quick recap of messaging codes:
	// 400 UNKNOWN
	// 401 UNKNOWN MODE
	// 402 MODE REQUIRES ARG
	// 403 MODE FORBIDDEN
	// 300 ERROR
	// 200 OK
	// 201 STATE CHANGE
	if (cmd == "help") {
		string topic = popword(line);
		if (topic.size() == 0) {
			connection->write("This is the simstat module of FOAM.");
		}
		else if (topic == "calib") {
			connection->write("404 :HELP ON CALIB NOT AVAILABLE");
		}
		else {
			return -1;
		}
	}
	else if (cmd == "calib") {
		connection->write("404 :CALIB NOT AVAILABLE");
	}
	else {
		return -1;
	}
	
	// if we end up here, we didn't return 0, so we found a valid command
	return 0;
}
