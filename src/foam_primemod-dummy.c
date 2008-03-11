/*! 
	@file foam_primemod-dummy.c
	@author @authortim
	@date November 30 2007

	@brief This is a dummy module to test the bare FOAM framework capabilities.
*/

// HEADERS //
/***********/

#include "foam_primemod-dummy.h"

int modInitModule(control_t *ptc) {
	logDebug("Running in dummy mode, don't expect great AO results :)");
	return EXIT_SUCCESS;
}

int modOpenInit(control_t *ptc) {
	return EXIT_SUCCESS;
}
void modStopModule(control_t *ptc) {
	// placeholder ftw!
}

int modOpenLoop(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modClosedInit(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modClosedLoop(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modCalibrate(control_t *ptc) {
	return EXIT_SUCCESS;
}