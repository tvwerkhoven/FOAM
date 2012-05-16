/*
 foam.cc -- main FOAM framework file, glues everything together
 Copyright (C) 2008--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

//! @todo Handle signals properly, call shutdown()
//! @bug FOAM quits when GUI quits, perhaps because of unhandled signals?

// HEADERS //
/***********/

#ifdef HAVE_CONFIG_H
// Contains various library we have.
#include "autoconfig.h"
#endif
#include <getopt.h>

#include <config.h>
#include <io.h>
#include <protocol.h>
#include <pthread++.h>
#include <sighandle.h>

#include "foamtypes.h"
#include "foamctrl.h"
#include "devices.h"
#include "foam.h"

using namespace std;

FOAM::FOAM(int argc, char *argv[]):
do_sighandle(true), sighandler(NULL),
do_perflog(false), open_perf(NULL), closed_perf(NULL),
t_closed_l(0.0), it_closed_l(0), t_open_l(0.0), it_open_l(0),
nodaemon(false), error(false), conffile(FOAM_DEFAULTCONF), execname(argv[0]),
io(IO_DEB2),
{
	io.msg(IO_DEB2, "FOAM::FOAM()");
	
	
	if (parse_args(argc, argv)) {
		error = true;
		exit(-1);
	}
	if (do_sighandle) {
    io.msg(IO_INFO, "FOAM::FOAM() Setting up SigHandle.");
		sighandler = auto_ptr<SigHandle> (new SigHandle());
		sighandler->quit_func = sigc::mem_fun(*this, &FOAM::stopfoam);
	}
	if (do_perflog) {
    io.msg(IO_INFO, "FOAM::FOAM() Setting up PerfLog.");
		open_perf = auto_ptr<PerfLog> (new PerfLog());
		closed_perf = auto_ptr<PerfLog> (new PerfLog());
	}
	
  io.msg(IO_INFO, "FOAM::FOAM() Setting up DeviceManager.");
	devices = new foam::DeviceManager(io);
	
	if (load_config()) {
		error = true;
		exit(-1);
	}
}


FOAM::~FOAM() {
	io.msg(IO_DEB2, "FOAM::~FOAM()");
	// Notify shutdown
	io.msg(IO_WARN, "Shutting down FOAM now");
  protocol->broadcast("warn :shutting down now");
	
	// Delete network thread, disconnect clients
	//! @todo This does not disconnect clients
	
	if (ptc->mode != AO_MODE_SHUTDOWN)
		stopfoam();
	
	// Get the end time to see how long we've run
	time_t end = time(NULL);
	struct tm *loctime = localtime(&end);
	char date[64];	
	strftime(date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", loctime);
	
	// Last log message just before closing the logfiles
	io.msg(IO_INFO, "Stopping FOAM at %s, ran for %d seconds.", 
				 date, end-ptc->starttime);
	
	// Delete devices, wait a bit to give the devices time to quit (mostly for Protocol objects used)
	//! @todo This should block in a more proper way instead of sleeping
	delete devices;
	usleep(0.5 * 1e6);
	
	// Delete network IO
	delete protocol;
	delete ptc;
	io.msg(IO_INFO, "FOAM succesfully quit");
}

void FOAM::stopfoam() {
	io.msg(IO_INFO, "FOAM::stopfoam() stopping on signal: %s", sighandler->get_sig_info().c_str());
	
	// Set mode to SHUTDOWN, such that the running program can stop
	ptc->mode = AO_MODE_SHUTDOWN;
	{
		pthread::mutexholder h(&mode_mutex);
		mode_cond.signal();						// signal a change to the threads
	}
	
	io.msg(IO_INFO, "FOAM::stopfoam() waiting for main loop to stop...");
	pthread::mutexholder h(&stop_mutex);
	io.msg(IO_INFO, "FOAM::stopfoam() main loop stoped...");
}

int FOAM::init() {
	io.msg(IO_DEB2, "FOAM::init()");
	
	// Start networking thread
	if (!nodaemon)
		daemon();	
	
	// Try to load setup-specific modules 
	if (load_modules())
		return io.msg(IO_ERR, "Could not load modules, aborting. Check your code & configuration.");
	
	// Verify setup integrity
	if (verify())
		return io.msg(IO_ERR, "Verification of setup failed, aborting. Check your configuration.");
	
	// Show banner
	show_welcome();
	
	return 0;
}

void FOAM::show_version() const {
	printf("FOAM (%s version %s, built %s %s)\n", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
	printf("Copyright (c) 2007--2011 %s\n", PACKAGE_BUGREPORT);
	printf("\nFOAM comes with ABSOLUTELY NO WARRANTY. This is free software,\n"
				 "and you are welcome to redistribute it under certain conditions;\n"
				 "see the file COPYING for details.\n");
}

void FOAM::show_clihelp(const bool error = false) const {
	if(error)
		fprintf(stderr, "Try '%s --help' for more information.\n", execname.c_str());
	else {
		printf("Usage: %s [option]...\n\n", execname.c_str());
		printf("  -c, --config=FILE    Read configuration from FILE.\n"
					 "  -v, --verb[=LEVEL]   Increase verbosity level or set it to LEVEL.\n"
					 "      --showthreads    Prefix logging with thread ID.\n"
					 "  -q,                  Decrease verbosity level.\n"
					 "      --nodaemon       Do not start network daemon.\n"
					 "  -s, --sighandle=0|1  Toggle signal handling (default: 1).\n"
					 "  -p, --perflog=0|1    Toggle performance logging (default: 0).\n"
					 "  -h, --help           Display this help message.\n"
					 "      --version        Display version information.\n\n");
		printf("Report bugs to %s.\n", PACKAGE_BUGREPORT);
	}
}

void FOAM::show_welcome() {
	io.msg(IO_DEB2, "FOAM::show_welcome()");
	
	char date[64];
	struct tm *tmp_tm = localtime(&(ptc->starttime));
	strftime(date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", tmp_tm);	
	
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
	io.msg(IO_INFO, "Copyright (c) 2007--2011 %s", PACKAGE_BUGREPORT);
}

int FOAM::parse_args(int argc, char *argv[]) {
	io.msg(IO_DEB2, "FOAM::parse_args()");
	int r, option_index = 0;
	
	static struct option const long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 1},
		{"verb", required_argument, NULL, 2},
		{"nodaemon", no_argument, NULL, 3},
		{"showthreads", no_argument, NULL, 4},
		{"sighandle", required_argument, NULL, 's'},
		{"perflog", required_argument, NULL, 'p'},
		{NULL, 0, NULL, 0}
	};
	
	while((r = getopt_long(argc, argv, "c:hvqs:p", long_options, &option_index)) != EOF) {
		switch(r) {
			case 0:
				break;
			case 'c':												// Configuration file
				io.msg(IO_DEB2, "FOAM::parse_args() -c: %s", optarg);
				conffile = string(optarg);
				break;
			case '?':												// Help
			case 'h':												// Help
				io.msg(IO_DEB2, "FOAM::parse_args() -h/-?");
				show_clihelp();
				return -1;
			case 1:													// Version info
				io.msg(IO_DEB2, "FOAM::parse_args() --version");
				show_version();
				return -1;
			case 'q':												// Decrease verbosity
				io.msg(IO_DEB2, "FOAM::parse_args() -q");
				io.decVerb();
				break;
			case 's':												// Signal handling
				io.msg(IO_DEB2, "FOAM::parse_args() -s: %s", optarg);
				do_sighandle = bool(strtol(optarg, NULL, 10));
				break;
			case 'p':												// performance logging
				io.msg(IO_DEB2, "FOAM::parse_args() -p");
				do_perflog = bool(strtol(optarg, NULL, 10));
				break;
			case 'v':												// Increase verbosity
				io.msg(IO_DEB2, "FOAM::parse_args() -v");
				io.incVerb();
				break;
			case 2:													// Set verbosity
				io.msg(IO_DEB2, "FOAM::parse_args() --verb: %s", optarg);
				if(optarg)
					io.setVerb((int) atoi(optarg));
				else {
					show_clihelp(true);
					return -1;
				}
				break;
			case 3:													// Don't run daemon
				io.msg(IO_DEB2, "FOAM::parse_args() --nodaemon");
				nodaemon = true;
				break;
			case 4:													// Show threads in logging
				io.msg(IO_DEB2, "FOAM::parse_args() --showthreads");
				io.setdefmask(IO_THR);
				break;
			default:
				io.msg(IO_DEB2, "FOAM::parse_args() unknown");
				show_clihelp();
				return -1;
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

int FOAM::verify() const {	
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
	// Lock this mutex such that the main thread knows we are still running
	pthread::mutexholder h1(&stop_mutex);
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
				{
					pthread::mutexholder h(&mode_mutex);
					mode_cond.wait(mode_mutex);
				}
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
  // Register start time, and current iterations (update every 500 frames)
  Time time1 = Time();
  Time time2 = Time();
  size_t curr_iter = 0;
  double currfps=0;
	
	while (ptc->mode == AO_MODE_OPEN) {
		// Log performance (time, latency)
		openperf_addlog(0);

		if (open_loop()) {
			io.msg(IO_WARN, "FOAM::open_loop() failed");
			ptc->mode = AO_MODE_LISTEN;
			return -1;
		}
    
    // Count openloop iterations, measure time difference, 
    it_open_l++;
    curr_iter++;
    if (curr_iter % 500 == 0) {
      Time time2 = Time();
      time2.update();
      currfps = (time2 - time1)/curr_iter;
      io.msg(IO_INFO, "FOAM::mode_open() # iter: %zu fps: %g.", curr_iter, currfps);
    }
	}
	
  time_t time2 = time(NULL);
  // Add this time to openloop time
  t_open_l += difftime(time2, time1);
  
	if (open_finish()) {		// check if we can finish
		io.msg(IO_WARN, "FOAM::open_finish() failed.");
		ptc->mode = AO_MODE_LISTEN;
		return -1;
	}
	
	return 0;
}

int FOAM::mode_closed() {	
	//! @bug Cannot shutdown from closed loop?
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
		closedperf_addlog(0);
		if (closed_loop()) {
			io.msg(IO_WARN, "FOAM::closed_loop() failed.");
			ptc->mode = AO_MODE_LISTEN;
			return -1;
		}
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

void FOAM::on_connect(const Connection * const conn, const bool status) {
  if (status) {
    conn->write(":client connected");
    io.msg(IO_DEB1, "Client connected from %s.", conn->getpeername().c_str());
  }
  else {
    conn->write(":client disconnected");
    io.msg(IO_DEB1, "Client from %s disconnected.", conn->getpeername().c_str());
  }
}

void FOAM::on_message(Connection *const conn, string line) {
	string cmd = popword(line);
	
	if (cmd == "help") {
		conn->write("ok cmd help");
		string topic = popword(line);
    if (show_nethelp(conn, topic, line))
			conn->write("error help :unknown topic");

  }
  else if (cmd == "exit" || cmd == "quit" || cmd == "bye") {
		conn->write("ok cmd " + cmd);
    conn->server->broadcast("ok client disconnected");
    conn->close();
  }
  else if (cmd == "shutdown") {
		conn->write("ok cmd shutdown");
		ptc->mode = AO_MODE_SHUTDOWN;
		{
			pthread::mutexholder h(&mode_mutex);
			mode_cond.signal();						// signal a change to the threads
		}
  }
  else if (cmd == "broadcast") {
		conn->write("ok cmd broadcast");
		conn->server->broadcast(format("ok broadcast %s :from %s", 
																				 line.c_str(),
																				 conn->getpeername().c_str()));
	}
  else if (cmd == "verb") {
		string var = popword(line);
		if (var == "+") io.incVerb();
		else if (var == "-") io.decVerb();
		else io.setVerb(var);
		
		conn->server->broadcast(format("ok verb %d", io.getVerb()));
	}
  else if (cmd == "get") {
    string var = popword(line);
		if (var == "mode")
			conn->write(format("ok mode %s", mode2str(ptc->mode).c_str()));
		else if (var == "devices")
			conn->write(format("ok devices %s", devices->getlist().c_str()));
		else 
			conn->write("error get :unknown variable");
	}
  else if (cmd == "mode") {
    string mode = popword(line);
		if (mode == mode2str(AO_MODE_CLOSED)) {
			conn->write("ok cmd mode closed");
			ptc->mode = AO_MODE_CLOSED;
			{
				pthread::mutexholder h(&mode_mutex);
				mode_cond.signal();						// signal a change to the threads
			}
		}
		else if (mode == mode2str(AO_MODE_OPEN)) {
			conn->write("ok cmd mode open");
			ptc->mode = AO_MODE_OPEN;
			{
				pthread::mutexholder h(&mode_mutex);
				mode_cond.signal();						// signal a change to the threads
			}
		}
		else if (mode == mode2str(AO_MODE_LISTEN)) {
			conn->write("ok cmd mode listen");
			ptc->mode = AO_MODE_LISTEN;
			{
				pthread::mutexholder h(&mode_mutex);
				mode_cond.signal();						// signal a change to the threads
			}
		}
		else
			conn->write("error cmd mode :mode unkown");
  }
  else
		conn->write("error :unknown command");
}

int FOAM::show_nethelp(const Connection *const conn, string topic, string /*rest*/) {
	if (topic.size() == 0) {
		conn->write(\
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
		conn->write(\
":mode <mode>:            Close or open the AO-loop.\n"
":  mode=open:            opens the loop and only records what's happening with\n"
":                        the AO system and does not actually drive anything.\n"
":  mode=closed:          closes the loop and starts the feedbackloop, \n"
":                        correcting the wavefront as fast as possible.\n"
":  mode=listen:          stops looping and waits for input from the users.");
	}
	else if (topic == "broadcast") {
		conn->write(\
":broadcast <msg>:        broadcast a message to all clients.");
	}
	else if (topic == "get") {
		conn->write(\
											":get <var>:              read a system variable.\n"
											":  mode:                 current mode of operation.\n"
											":  devices:              list of devices.");
	}
	else // Unknown topic
		return -1;

	return 0;
}

string FOAM::mode2str(const aomode_t m) const {
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

aomode_t FOAM::str2mode(const string m) const {
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


/*!	\mainpage FOAM docs
 
  This is the FOAM documentation. It includes both developer documentation as
  well as user manuals.
 
  More information can be found on these pages:
  - \subpage userdocs "User manual"
  - \subpage devdocs "Developer docs"
 
*/

/*!
  \page userdocs User docs
 
  You can find various user documentation here.
 
  There are several FOAM modules that are used for testing. These are:
 
  - \subpage ud_foamdum "FOAM dummy"
  - \subpage ud_foamss "FOAM static-simulation"
  - \subpage ud_foamfs "FOAM full-simulation"
*/

/*!
  \page devdocs Developer docs
 
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
	Also the hardware controller classes are derived from a base 'Device' class.
	All devices together are stored in a DeviceManager class.
	
	See FOAM, Device, DeviceManager for more information.
 
	\subsection struct-frame The Framework
	
	The base class itself does very little. Actual functionality should be
	implemented in the derived class. See the FOAM class for the possibilities
	to implement functions for your AO configuration.
 
	The base class provides the following virtual functions to re-implement in
	derived classes
	
	\li FOAM::load_modules()
	\li FOAM::open_init()
	\li FOAM::open_loop()
	\li FOAM::open_finish()
	\li FOAM::closed_init()
	\li FOAM::closed_loop()
	\li FOAM::closed_finish()
	\li FOAM::calib()
	\li FOAM::on_message(Connection* const, string) (optional)
  \li FOAM::on_connect(const Connection * const, const bool) const (optional)
	
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
 
	\todo Improve documentation on extending FOAM
	
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
 
  Commands are sent to FOAM in plaintext, and upon succesfull reception of the 
  command, an 'ok cmd <CMD>' will be given back. If the command was executed 
  succesfully, this will be sent back as well, or if it concerns a state 
  change of the system, it will broadcast the new setting to all connected 
  clients. See each specific Device on which networking commands and replies 
  are supported.
 
 \section related See also
 
	More information can be found on these pages:
  - \subpage dev "Devices info"
  - \subpage devmngr "Device Manager info"
  - \subpage fgui "FOAM GUI"

*/
