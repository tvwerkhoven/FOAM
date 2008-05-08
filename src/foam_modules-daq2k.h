/*! 
	@file foam_modules-daq2k.h
	@author @authortim
	@date May 05, 2008

	@brief This file contains prototypes to drive a DaqBoard 2000 PCI board

	\section Info

	The IOtech DaqBoard/2000 series are PCI cards which have several digital and analog I/O ports, which 
	can be used to aqcuire data from various sources. This board can also be used to drive tip-tilt
	mirrors, telescope (using analog outputs) and filterwheels (using several digital output ports)
	though, making it a good choice for a general purpose IO board in AO setups.
 
	This module supports multiple daqboards with various DAC channels, and supports the 8225 digital IO chips 
	providing a total of 3 8-bit ports, of which the last port is split into two 4 bit ports.
 
	It does not support 'banks'.
 
	More information can be found at the manufacturers website:\n
	<tt>http://www.iotech.com/catalog/daq/dbseries2.html</tt>
 
	Especially take a look at the programmer's manual which has a useful function reference at the end.
 
	This module can compile on its own:\n
	<tt>gcc foam_modules-daq2k.c -lc -lm -ldaqx -Wall -DFOAM_MODDAQ2K_ALONE=1 -I../../../../misc/daqboard_iotech220_portedto26/include/ -L../../../../misc/daqboard_iotech220_portedto26/lib</tt>

	\section Functions
	
	\li drvInitDaq2k() - Initialize the Daqboard 2000 (call this first!)
	\li drvCloseDaq2k() - Close the Daqboard 2000 (call this at the end!)
	\li drvDaqSetDAC() - Write analog output to specific ports (ranges from 0 to 65535 (16bit))
	\li drvDaqSetDACs() - Write analog output to all (ranges from 0 to 65535 (16bit))
	\li drvDaqSetP2() - Write digital output to specific ports
	
	\section Configuration
	
	Configuration for this modules goes through define statements:
	
	\li	\b FOAM_MODDAQ2K_ALONE (*undef*), ifdef, compiles on its own (implies FOAM_DEBUG)
	\li \b FOAM_DEBUG (*undef*), ifdef, gives lowlevel prinft debug statements

	\section Dependencies
 
	This module depends on the daqx library used to access daqboards.

	\section History
 
	\li 2008-04-14: api change, configuration done with datatypes instead of defines
	\li 2008-04-02: init

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
 @brief Datatype to hold metadata on daqboard operations.
 
 (user) fields must be supplied by the user, (mod) fields will be
 filled by this module.
 */
typedef struct {
	char *device;		//!< (user) device name of the board
	int fd;				//!< (mod) device fd
	int nchans;			//!< (user) number of DAC channels (used) on the board
	float minvolt;		//!< (user) minimum voltage for the DAC ports
	float maxvolt;		//!< (user) maximum voltage for the DAC ports
	int iop2conf[4];	//!< (user) port configuration for 8225 chip, {portA, portB, high portC, low portC} 0 is output, 1 is input
	int dacinit;		//!< (mod) switch to see if DAC is initialized successfully or not
	int iop2init;		//!< (mod) switch to see if IO P2 is initialized successfully or not
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
int drvInitDaq2k(mod_daq2k_board_t *board);

/*!
 @brief Close the daqboard opened successfully previously, call this before quitting
 
 @param [in] *board The daqboard to close
 */
void drvCloseDaq2k(mod_daq2k_board_t *board);

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
int drvDaqSetP2(mod_daq2k_board_t *board, int port, int bitpat);

/*!
 @brief Write something to a DAC channel on a board
 
 This routine writes a value to a DAC channel, which will change the output
 voltage on that port. The value 'val' is first bitwase AND'd with 0xffff
 so that the value written to the channel is never too high (safety check, the daqboard
 only accepts 16 bit values, such that 0xffff is the highest possible value).
 
 If the board->fd is -1, this function returns with EXIT_SUCCESS immmediately
 
 @param [in] *board The board to use
 @param [in] chan The channel to write to
 @param [in] val The value to write [0, 65535], 16bit
 */
void drvDaqSetDAC(mod_daq2k_board_t *board, int chan, int val);

/*!
 @brief Write something to all DAC channels on a board
 
 This routine writes the same value to all DAC channels on a board, see drvDaqSetDAC() for details.
 
 If the board->fd is -1, this function returns with EXIT_SUCCESS immmediately
 
 @param [in] *board The board to use
 @param [in] val The value to write [0, 65535], 16bit
 */
void drvDaqSetDACs(mod_daq2k_board_t *board, int val);

#endif // #ifndef FOAM_MODULES_DAQ2k
