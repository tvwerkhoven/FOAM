/*
 foam.cc -- main FOAM framework file, glues everything together
 Copyright (C) 2008--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
	@file foam.cc
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

	@brief This is the main file for FOAM.

	This is the framework for FOAM, it provides basic functionality and can be 
	customized through deriving this main class to your specific needs. 
*/

// HEADERS //
/***********/

#include <getopt.h>

#include "foam.h"
#include "autoconfig.h"
#include "config.h"
#include "foamtypes.h"
#include "io.h"
#include "protocol.h"
#include "foamctrl.h"
#include "devices.h"

using namespace std;

FOAM::FOAM(int argc, char *argv[]):
nodaemon(false), error(false), conffile(FOAM_DEFAULTCONF), execname(argv[0]),
io(IO_DEB2)
{
	io.msg(IO_DEB2, "FOAM::FOAM()");
	devices = new DeviceManager(io);
	
	if (parse_args(argc, argv)) {
		error = true;
		exit(-1);
	}
	if (load_config()) {
		error = true;
		exit(-1);
	}
	
}

int FOAM::init() {
	io.msg(IO_DEB2, "FOAM::init()");
	
	if (pthread_mutex_init(&mode_mutex, NULL) != 0)
		return io.msg(IO_ERR, "pthread_mutex_init failed.");
	if (pthread_cond_init (&mode_cond, NULL) != 0)
		return io.msg(IO_ERR, "pthread_cond_init failed.");

	// Start networking thread
	if (!nodaemon)
		daemon();	
	
	// Try to load setup-specific modules 
	if (load_modules())
		return io.msg(IO_ERR, "Could not load modules, aborting. Check your code.");
	
	// Verify setup integrity
	if (verify())
		return io.msg(IO_ERR, "Verification of setup failed, aborting. Check your configuration.");
	
	// Show banner
	show_welcome();
	
	return 0;
}

FOAM::~FOAM() {
	io.msg(IO_DEB2, "FOAM::~FOAM()");
	
	// Notify shutdown
	io.msg(IO_WARN, "Shutting down FOAM now");
  protocol->broadcast("warn :shutting down now");
	
	//! \todo disconnect clients gracefully here?			
	
	// Get the end time to see how long we've run
	time_t end = time(NULL);
	struct tm *loctime = localtime(&end);
	char date[64];	
	strftime(date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", loctime);
	
	// Last log message just before closing the logfiles
	io.msg(IO_INFO, "Stopping FOAM at %s", date);
	io.msg(IO_INFO, "Ran for %ld seconds, parsed %ld frames (%.1f FPS).", \
					end-ptc->starttime, ptc->frames, ptc->frames/(float) (end-ptc->starttime));

	//delete devices;
	//delete protocol;
	//delete ptc;
	
//	io.msg(IO_INFO, "Waiting for devices to quit...");
	//! \bug exiting gives EXC_BAD_ACCESS from protocol.h if quitting too fast.
//	usleep(250000);
//	io.msg(IO_INFO, "Bye...");
}

void FOAM::show_version() {
	printf("FOAM (%s version %s, built %s %s)\n", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
	printf("Copyright (c) 2007--2010 Tim van Werkhoven <T.I.M.vanWerkhoven@xs4all.nl>\n");
	printf("\nFOAM comes with ABSOLUTELY NO WARRANTY. This is free software,\n"
				 "and you are welcome to redistribute it under certain conditions;\n"
				 "see the file COPYING for details.\n");
}

void FOAM::show_clihelp(bool error = false) {
	if(error)
		io.msg(IO_ERR | IO_NOID, "Try '%s --help' for more information.\n", execname.c_str());
	else {
		printf("Usage: %s [option]...\n\n", execname.c_str());
		printf("  -c, --config=FILE    Read configuration from FILE.\n"
					 "  -v, --verb[=LEVEL]   Increase verbosity level or set it to LEVEL.\n"
					 "  -q,                  Decrease verbosity level.\n"
					 "      --nodaemon       Do not start network daemon.\n"
					 "  -p, --pidfile=FILE   Write PID to FILE.\n"
					 "  -h, --help           Display this help message.\n"
					 "      --version        Display version information.\n\n");
		printf("Report bugs to Tim van Werkhoven <T.I.M.vanWerkhoven@xs4all.nl>.\n");
	}
}

void FOAM::show_welcome() {
	io.msg(IO_DEB2, "FOAM::show_welcome()");
	
	tm_start = localtime(&(ptc->starttime));
	char date[64];
	strftime (date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", tm_start);	
	
	io.msg(IO_NOID|IO_INFO,"      ___           ___           ___           ___     \n\
     /\\  \\         /\\  \\         /\\  \\         /\\__\\    \n\
    /::\\  \\       /::\\  \\       /::\\  \\       /::|  |   \n\
   /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\     /:|:|  |   \n\
  /::\\~\\:\\  \\   /:/  \\:\\  \\   /::\\~\\:\\  \\   /:/|:|__|__ \n\
 /:/\\:\\ \\:\\__\\ /:/__/ \\:\\__\\ /:/\\:\\ \\:\\__\\ /:/ |::::\\__\\\n\
 \\/__\\:\\ \\/__/ \\:\\  \\ /:/  / \\/__\\:\\/:/  / \\/__/~~/:/  /\n\
      \\:\\__\\    \\:\\  /:/  /       \\::/  /        /:/  / \n\
       \\/__/     \\:\\/:/  /        /:/  /        /:/  /  \n\
                  \\::/  /        /:/  /        /:/  /   \n\
                   \\/__/         \\/__/         \\/__/ \n");
	
	io.msg(IO_INFO, "This is FOAM (version %s, built %s %s)", PACKAGE_VERSION, __DATE__, __TIME__);
	io.msg(IO_INFO, "Starting at %s", date);
	io.msg(IO_INFO, "Copyright (c) 2007--2010 Tim van Werkhoven <T.I.M.vanWerkhoven@xs4all.nl>");
}

int FOAM::parse_args(int argc, char *argv[]) {
	io.msg(IO_DEB2, "FOAM::parse_args()");
	int r, option_index = 0;
	execname = argv[0];
	
	static struct option const long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 1},
		{"verb", required_argument, NULL, 2},
		{"pidfile", required_argument, NULL, 'p'},
		{"nodaemon", no_argument, NULL, 3},
		{NULL, 0, NULL, 0}
	};
	
	while((r = getopt_long(argc, argv, "c:hvqp:", long_options, &option_index)) != EOF) {
		switch(r) {
			case 0:
				break;
			case 'c':
				conffile = optarg;
				break;
			case 'h':
				show_clihelp();
				return -1;
			case 1:
				show_version();
				return -1;
			case 'q':
				io.decVerb();
				break;
			case 'v':
				io.incVerb();
				break;
			case 2:
				if(optarg)
					io.setVerb((int) atoi(optarg));
				else {
					show_clihelp(true);
					return -1;
				}
				break;
			case 'p':
				// pidfile placeholder
				break;
			case 3:
				nodaemon = true;
				break;
			case '?':
				show_clihelp(true);
				return -1;
			default:
				break;
		}
	}
	
	return 0;
}

int FOAM::load_config() {
	io.msg(IO_DEB2, "FOAM::load_config()");
	
	// Load and parse configuration file
	if (conffile == FOAM_DEFAULTCONF) {
		io.msg(IO_WARN, "Using default configuration file '%s', might not work", conffile.c_str());
	}
	
	// Init control and configuration using the config file
	io.msg(IO_INFO, "Initializing control & AO configuration...");
	ptc = new foamctrl(io, conffile);
	if (ptc->error()) 
		return io.msg(IO_ERR, "Coult not parse FOAM configuration");
	
	return 0;
}

int FOAM::verify() {	
	// Check final configuration integrity
	int ret = 0;
	ret += ptc->verify();
	
	return ret;
}

void FOAM::daemon() {
	io.msg(IO_INFO, "Starting daemon at %s:%s...", ptc->listenip.c_str(), ptc->listenport.c_str());
  protocol = new Protocol::Server(ptc->listenport);
  protocol->slot_message = sigc::mem_fun(this, &FOAM::on_message);
  protocol->slot_connected = sigc::mem_fun(this, &FOAM::on_connect);
  protocol->listen();
}

int FOAM::listen() {	
	while (true) {
		switch (ptc->mode) {
			case AO_MODE_OPEN:
				io.msg(IO_DEB1, "FOAM::listen() AO_MODE_OPEN");
				mode_open();
				break;
			case AO_MODE_CLOSED:
				io.msg(IO_DEB1, "FOAM::listen() AO_MODE_CLOSED");
				mode_closed();
				break;
			case AO_MODE_CAL:
				io.msg(IO_DEB1, "FOAM::listen() AO_MODE_CAL");
				mode_calib();
				break;
			case AO_MODE_LISTEN:
				io.msg(IO_INFO, "FOAM::listen() Entering listen loop.");
				// We wait until the mode changed
				protocol->broadcast("ok mode listen");
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_wait(&mode_cond, &mode_mutex);
				pthread_mutex_unlock(&mode_mutex);
				break;
			case AO_MODE_SHUTDOWN:
				io.msg(IO_DEB1, "FOAM::listen() AO_MODE_SHUTDOWN");
				return 0;
				break;
			default:
				io.msg(IO_ERR, "FOAM::listen() UNKNOWN!");
				break;
		}
	}
	
	return -1;
}

int FOAM::mode_open() {
	io.msg(IO_INFO, "FOAM::mode_open()");

	// Run the initialisation function of the modules used
	if (open_init()) {
		io.msg(IO_WARN, "FOAM::open_init() failed.");
		ptc->mode = AO_MODE_LISTEN;
		return -1;
	}
		
	protocol->broadcast("ok mode open");
	
	while (ptc->mode == AO_MODE_OPEN) {
		if (open_loop()) {
			io.msg(IO_WARN, "FOAM::open_loop() failed");
			ptc->mode = AO_MODE_LISTEN;
			return -1;
		}
		ptc->frames++;	// increment the amount of frames parsed
	}
	
	if (open_finish()) {		// check if we can finish
		io.msg(IO_WARN, "FOAM::open_finish() failed.");
		ptc->mode = AO_MODE_LISTEN;
		return -1;
	}
	
	return 0;
}

int FOAM::mode_closed() {	
	io.msg(IO_INFO, "FOAM::mode_closed()");
	
	// Initialize closed loop
	if (closed_init()) {
		io.msg(IO_WARN, "FOAM::closed_init() failed");
		ptc->mode = AO_MODE_LISTEN;
		return -1;
	}
		
	protocol->broadcast("ok mode closed");
	
	// Run closed loop
	while (ptc->mode == AO_MODE_CLOSED) {
		if (closed_loop()) {
			io.msg(IO_WARN, "FOAM::closed_loop() failed.");
			ptc->mode = AO_MODE_LISTEN;
			return -1;
		}		
		ptc->frames++;
	}
	
	// Finish closed loop
	if (closed_finish()) {
		io.msg(IO_WARN, "FOAM::closed_finish() failed.");
		ptc->mode = AO_MODE_LISTEN;
		return -1;
	}
		
	return 0;
}

int FOAM::mode_calib() {
	io.msg(IO_INFO, "FOAM::mode_calib()");
	
	protocol->broadcast("ok mode calib");
	
	// Run calibration inside module
	if (calib()) {
		io.msg(IO_WARN, "FOAM::calib() failed.");
		protocol->broadcast("err calib :calibration failed");
		ptc->mode = AO_MODE_LISTEN;
		return -1;
	}
	
	protocol->broadcast("ok calib");
	
	io.msg(IO_INFO, "Calibration loop done, switching to listen mode.");
	ptc->mode = AO_MODE_LISTEN;
	
	return 0;
}

void FOAM::on_connect(Connection *connection, bool status) {
  if (status) {
    connection->write(":client connected");
    io.msg(IO_DEB1, "Client connected from %s.", connection->getpeername().c_str());
  }
  else {
    connection->write(":client disconnected");
    io.msg(IO_DEB1, "Client from %s disconnected.", connection->getpeername().c_str());
  }
}

void FOAM::on_message(Connection *connection, std::string line) {
  io.msg(IO_DEB1, "FOAM::Got %db: '%s'.", line.length(), line.c_str());
	
	string cmd = popword(line);
	
	if (cmd == "help") {
		connection->write("ok cmd help");
		string topic = popword(line);
    if (show_nethelp(connection, topic, line))
			netio.ok = false;
  }
  else if (cmd == "exit" || cmd == "quit" || cmd == "bye") {
		connection->write("ok cmd exit");
    connection->server->broadcast("ok client disconnected");
    connection->close();
  }
  else if (cmd == "shutdown") {
		connection->write("ok cmd shutdown");
		ptc->mode = AO_MODE_SHUTDOWN;
		pthread_cond_signal(&mode_cond); // signal a change to the threads
  }
  else if (cmd == "broadcast") {
		connection->write("ok cmd broadcast");
		connection->server->broadcast(format("ok broadcast %s :from %s", 
																				 line.c_str(),
																				 connection->getpeername().c_str()));
	}
  else if (cmd == "verb") {
		string var = popword(line);
		if (var == "+") io.incVerb();
		else if (var == "-") io.decVerb();
		else io.setVerb(var);
		
		connection->server->broadcast(format("ok verb %d", io.getVerb()));
	}
  else if (cmd == "get") {
    string var = popword(line);
		if (var == "frames") connection->write(format("ok var frames %d", ptc->frames));
		else if (var == "mode") connection->write(format("ok var mode %s", mode2str(ptc->mode).c_str()));
		else if (var == "devices") connection->write(format("ok var devices %d %s", devices->getcount(), devices->getlist().c_str()));
		else netio.ok = false;
	}
  else if (cmd == "mode") {
    string mode = popword(line);
		if (mode == mode2str(AO_MODE_CLOSED)) {
			connection->write("ok cmd mode closed");
			ptc->mode = AO_MODE_CLOSED;
			pthread_cond_signal(&mode_cond); // signal a change to the main thread
		}
		else if (mode == mode2str(AO_MODE_OPEN)) {
			connection->write("ok cmd mode open");
			ptc->mode = AO_MODE_OPEN;
			pthread_cond_signal(&mode_cond); // signal a change to the main thread
		}
		else if (mode == mode2str(AO_MODE_LISTEN)) {
			connection->write("ok cmd mode listen");
			ptc->mode = AO_MODE_LISTEN;
			pthread_cond_signal(&mode_cond); // signal a change to the main thread
		}
		else {
			connection->write("err cmd mode :mode unkown");
		}
  }
  else {
		netio.ok = false;
  }
}

int FOAM::show_nethelp(Connection *connection, string topic, string /*rest*/) {
	if (topic.size() == 0) {
		connection->write(\
":==== FOAM help ==========================\n"
":help [command]:         Help (on a certain command, if available).\n"
":mode <mode>:            close or open the loop.\n"
":get <var>:              read a system variable.\n"
":verb <level>:           set verbosity to <level>.\n"
":verb <+|->:             increase/decrease verbosity by 1 step.\n"
":broadcast <msg>:        send a message to all connected clients.\n"
":exit or quit:           disconnect from daemon.\n"
":shutdown:               shutdown FOAM.");
	}		
	else if (topic == "mode") {
		connection->write(\
":mode <mode>:            Close or open the AO-loop.\n"
":  mode=open:            opens the loop and only records what's happening with\n"
":                        the AO system and does not actually drive anything.\n"
":  mode=closed:          closes the loop and starts the feedbackloop, \n"
":                        correcting the wavefront as fast as possible.\n"
":  mode=listen:          stops looping and waits for input from the users.");
	}
	else if (topic == "broadcast") {
		connection->write(\
":broadcast <msg>:        broadcast a message to all clients.");
	}
	else if (topic == "get") {
		connection->write(\
											":get <var>:              read a system variable.\n"
											":  mode:                 current mode of operation.\n"
											":  devices:              list of devices.\n"
											":  frames:               number of frames processed.");
	}
	else {
		// Unknown topic
		return -1;
	}
	return 0;
}

string FOAM::mode2str(aomode_t m) {
	switch (m) {
		case AO_MODE_OPEN: return "open";
		case AO_MODE_CLOSED: return "closed";
		case AO_MODE_CAL: return "calib";
		case AO_MODE_LISTEN: return "listen";
		case AO_MODE_UNDEF: return "undef";
		case AO_MODE_SHUTDOWN: return "shutdown";
		default: return "unknown";
	}
}

aomode_t FOAM::str2mode(string m) {
	if (m == "open") return AO_MODE_OPEN;
	else if (m == "closed") return AO_MODE_CLOSED;
	else if (m == "calib") return AO_MODE_CAL;
	else if (m == "listen") return AO_MODE_LISTEN;
	else if (m == "undef") return AO_MODE_UNDEF;
	else if (m == "shutdown") return AO_MODE_SHUTDOWN;
	else return AO_MODE_UNDEF;
}



// DOXYGEN GENERAL DOCUMENTATION //
/*********************************/

/* Doxygen needs the following aliases in the configuration file!

ALIASES += authortim="Tim van Werkhoven (T.I.M.vanWerkhoven@phys.uu.nl)"
ALIASES += license="GPLv2"
ALIASES += name="FOAM"
ALIASES += longname="Modular Adaptive Optics Framework"
*/

/*!	\mainpage FOAM 

	\section aboutdoc About this document
	
	This is the (developer) documentation for FOAM, the Modular Adaptive 
	Optics Framework (yes, FOAM is backwards). It is intended to clarify the
	code and give some help to people re-using or implementing the code for 
	their specific needs.
	
	\section aboutfoam About FOAM
	
	FOAM, the Modular Adaptive Optics Framework, is intended to be a modular
	framework which works independent of hardware and should provide such
	flexibility that the framework can be implemented on any AO system.
	
	A short list of the features of FOAM follows:
 
	\li Portable - It runs on many (Unix) systems and is hardware independent
	\li Scalable - It scales easily and handle multiple cores/CPU's
	\li Extensible - It can be extended with new features or algorithms relatively easily
	\li Usable - It is controllable over a network by multiple clients simultaneously
	\li Free - It licensed under the GPL and therefore can be used by anyone.
	
	For more information, see the FOAM project page at GitHub: http://github.com/tvwerkhoven/FOAM/
	
	\section struct FOAM structure
	
	FOAM uses a base class with the same name that can be derived to specific
	configurations necessary for the AO system. Such a derived FOAM class can
	then use various hardware controller classes to allow actual processing. 
	Also the hardware controller classes are derived from a base 'device' class.
	All devices together are stored in a DeviceManager class.
	
	See FOAM, Device, DeviceManager for more information.
 
	\todo Improve doc
 
	\subsection struct-frame The Framework
	
	The base class itself does very little. Actual functionality should be
	implemented in the derived class. See the FOAM class for the possibilities
	to implement functions for your AO configuration.
 
	The base class provides the following virtual functions to re-implement in
	derived classes
	
	\li load_modules()
	\li open_init()
	\li open_loop()
	\li open_finish()
	\li closed_init()
	\li closed_loop()
	\li closed_finish()
	\li calib()
	\li on_message()
	
	See the documentation on the specific functions for more details on what 
	these functions can be used for.
 
	\section install_sec Installation
	
	FOAM uses the standard compilation process. Run ./configure to check if the
  necessary libraries are installed and to see if you might need to add some
	libraries.
	
	To install FOAM, follow these simple steps:
	\li Download a FOAM release from http://github.com/tvwerkhoven/FOAM/ (either git pull something, or download a release)
	\li run `autoreconf -s -i` to the configure script
	\li Run `./configure --help` to check what options you would like to use
	\li Run `./configure` (with specific options) to prepare FOAM for building. If some libraries are missing, configure will tell you so
	\li Run `make install` which makes the various FOAM packages and installs them in $PREFIX
	\li Run any of the executables built
	\li Connect to localhost:1025 (default) with a telnet client
	\li Type `help' to get a list of FOAM commands
	
	\section drivers Extending FOAM yourself

	As said before, FOAM itself does not do a lot (compile & run 
	<tt>./foam-dummy</tt> to verify), it provides a framework to which devices 
	can be attached. 
	
	If you want to use FOAM in a specific setup, you will probably have to 
	program some software yourself. When doing so, please include Doxygen
	compatible documentation in the source code like this one.
 
	\todo Extend this doc
	
	\subsection ownmake Adapt the build process
	
	After writing your own software, you have to make sure it is actually built.
  To make sure this happens, the most important files are configure.ac and
	the various Makefile.am. configure.ac is the autoconf template and checks
	for libraries and other prerequisites you might need. The various Makefile.am
	are used to build different targets and tell the compilers which source 
	files to include.

	\section network Networking

	The networking language used is stolen from Guus Sliepen's filter_control
	software. Look in the source code for clues on how it works, or connect
  to FOAM with telnet to look at it yourself.
 
	\todo Extend this doc
 
*/
