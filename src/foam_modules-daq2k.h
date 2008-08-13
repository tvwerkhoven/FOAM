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
	@file foam_modules-daq2k.h
	@author @authortim
	@date 2008-07-15
 
	@brief This file contains prototypes to drive a DaqBoard 2000 PCI board
*/

#ifndef FOAM_MODULES_DAQ2k
#define FOAM_MODULES_DAQ2k

#include <stdio.h>
#include <stdlib.h>
#define DLINUX 1			// necessary for next header file
#include "daqx_linuxuser.h"
#include <string.h>			// for strerror
#include <errno.h>			// for errno
#include <unistd.h>			// for usleep

#ifdef FOAM_MODDAQ2K_ALONE
#define FOAM_DEBUG 1				//!< set to 1 for debugging, in that case this module compiles on its own
#endif

/*!
 @brief Defines input and output
 */
typedef enum {
	DAQ_OUTPUT = 0,
	DAQ_INPUT = 1
} io_t;

/*!
 @brief Datatype to hold metadata on daqboard operations.
 
 (user) fields must be supplied by the user, (foam) fields will be
 filled by FOAM.
 */
typedef struct {
	char *device;		//!< (user) device name of the board
	int fd;				//!< (foam) device fd
	int nchans;			//!< (user) number of DAC channels (used) on the board
	float minvolt;		//!< (user) minimum voltage for the DAC ports
	float maxvolt;		//!< (user) maximum voltage for the DAC ports
	io_t iop2conf[4];	//!< (user) port configuration for 8225 chip, {portA, portB, high portC, low portC} DAQ_OUTUT is output, DAQ_INPUT is input
	int dacinit;		//!< (foam) switch to see if DAC is initialized successfully or not
	int iop2init;		//!< (foam) switch to see if IO P2 is initialized successfully or not
} mod_daq2k_board_t;


// public functions
/*!
 @brief Initialize both the digital IO as the DAC ports on a Daqboards
 
 This routine intializes the Daqboard for use lateron. Call this before calling
 other daq routines. On failure, this routine returns EXIT_FAILURE. This 
 means that the digital IO and/or the DACs failed to configure. board->dacinit
 and board->iop2init are set to 0 if either failed to initialize.
 
 If something failed, this will be noticed by the module, and subsequent IO or DAC
 calls are ignored. It is possible to use the digital IO without the DAC or vice versa
 this way.
 
 @param [in] *board The daqboard to initialize
 @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int daq2kInit(mod_daq2k_board_t *board);

/*!
 @brief Close the daqboard opened successfully previously, call this before quitting
 
 @param [in] *board The daqboard to close
 */
void daq2kClose(mod_daq2k_board_t *board);

/*!
 @brief Set a bitpattern on a digital IO port on a specific board
 
 This sets a bitpattern on a P2 port. If the port was previously configured
 as read-only port, the routine will return EXIT_FAILURE before writing. This can
 be ignored if one wants to, although it's unlikely that trying to write to read-only
 configured ports can serve useful purposes.
 
 @param [in] *board The board to use
 @param [in] port The port to write to, 0=portA, 1=portB, 2=portCHigh, 3=portCLow
 @param [in] bitpat The bitpattern to write (the low 8 (port={0,1}) or 4 (port={2,3}) bits are used)
 
 @return EXIT_SUCCESS on (partial) success, EXIT_FAILURE on complete failure.
 */
int daq2kSetP2(mod_daq2k_board_t *board, int port, int bitpat);

/*!
 @brief Write something to a DAC channel on a board
 
 This routine writes a value to a DAC channel, which will change the output
 voltage on that port. The value 'val' is first bitwase AND'd with 0xffff
 so that the value written to the channel is never too high (safety check, the daqboard
 only accepts 16 bit values, such that 0xffff is the highest possible value).
 
 Note that the voltage range for the DaqBoard/2000 series boards is -10 to 10 
 volt, such that the precision is 0.305mV (20 volt/2^16). It is possible
 however, that not the whole voltage range is used, but that only the positive
 voltages are used.
 
 If the board->fd is -1, this function returns with EXIT_SUCCESS immmediately
 
 @param [in] *board The board to use
 @param [in] chan The channel to write to
 @param [in] val The value to write [0, 65535], 16bit
 */
void daq2kSetDAC(mod_daq2k_board_t *board, int chan, int val);

/*!
 @brief Write something to all DAC channels on a board
 
 This routine writes the same value to all DAC channels on a board, see daq2kSetDAC() for details.
 
 If the board->fd is -1, this function returns with EXIT_SUCCESS immmediately
 
 @param [in] *board The board to use
 @param [in] val The value to write [0, 65535], 16bit
 */
void daq2kSetDACs(mod_daq2k_board_t *board, int val);

#endif // #ifndef FOAM_MODULES_DAQ2k
