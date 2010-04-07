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
	customized through the use of certains `hooks'. These hooks are functions that
	are called at crucial moments during the adaptive optics controlling such 
	that the person implementing FOAM on a specific AO setup can customize
	what FOAM does.
 
*/

// HEADERS //
/***********/

#include <getopt.h>

#include "foam.h"
#include "autoconfig.h"
#include "config.h"
#include "types.h"
#include "io.h"
#include "protocol.h"
#include "foamcfg.h"
#include "foamctrl.h"

using namespace std;



FOAM::FOAM(int argc, char *argv[]):
nodaemon(false), error(false), conffile(FOAM_DEFAULTCONF), execname(argv[0]),
io(IO_DEB2)
{
	io.msg(IO_DEB2, "FOAM::FOAM()");
	if (!parse_args(argc, argv)) {
		error = true;
		return;
	}
	if (!load_config()) {
		error = true;
		return;
	}
	
}

bool FOAM::init() {
	io.msg(IO_DEB2, "FOAM::init()");
	
	if (pthread_mutex_init(&mode_mutex, NULL) != 0)
		return io.msg(IO_ERR, "pthread_mutex_init failed.");
	if (pthread_cond_init (&mode_cond, NULL) != 0)
		return io.msg(IO_ERR, "pthread_cond_init failed.");
	
	
	load_modules();
	
	verify();
	
	daemon();
	
	show_welcome();
	
	return true;
}

FOAM::~FOAM() {
	io.msg(IO_DEB2, "FOAM::~FOAM()");
	
	if (ptc->mode != AO_MODE_SHUTDOWN)
		io.msg(IO_WARN, "Incorrect shutdown call, not in shutdown mode!");
	
	io.msg(IO_WARN, "Shutting down FOAM now");
  protocol->broadcast("WARN :SHUTTING DOWN NOW");
	
	// Get the end time to see how long we've run
	time_t end = time(NULL);
	struct tm *loctime = localtime(&end);
	char date[64];	
	strftime(date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", loctime);
	
	// Last log message just before closing the logfiles
	io.msg(IO_INFO, "Stopping FOAM at %s", date);
	io.msg(IO_INFO, "Ran for %ld seconds, parsed %ld frames (%.1f FPS).", \
					end-ptc->starttime, ptc->frames, ptc->frames/(float) (end-ptc->starttime));

	delete protocol;
	delete cs_config;
	delete ptc;
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
					 "  -q,                  Decrease verbosity level or set it to LEVEL.\n"
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

bool FOAM::parse_args(int argc, char *argv[]) {
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
				return false;
			case 1:
				show_version();
				return false;
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
					return false;
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
				return false;
			default:
				break;
		}
	}
	
	return true;
}

bool FOAM::load_config() {
	io.msg(IO_DEB2, "FOAM::load_config()");
	
	// Load and parse configuration file
	if (conffile == "") {
		io.msg(IO_ERR, "No configuration file given.");
		show_clihelp(true);
		return false;
	}
	else if (conffile == FOAM_DEFAULTCONF) {
		io.msg(IO_WARN, "Using default configuration file '%s'.", conffile.c_str());
	}
	
	// Init control and configuration using the config file
	io.msg(IO_INFO, "Initializing control & AO configuration...");
  cs_config = new foamcfg(io, conffile);
	if (cs_config->error()) return io.msg(IO_ERR, "Coult not parse control configuration");	
	ptc = new foamctrl(io, conffile);
	if (ptc->error()) return io.msg(IO_ERR, "Coult not parse AO configuration");
	
	return true;
}

bool FOAM::verify() {	
	// Check final configuration integrity
	int ret = 0;
	ret += ptc->verify();
	ret += cs_config->verify();
	
	if (ret) return false;
	return true;
}

void FOAM::daemon() {
	io.msg(IO_INFO, "Starting daemon at %s:%s...", cs_config->listenip.c_str(), cs_config->listenport.c_str());
  protocol = new Protocol::Server(cs_config->listenport);
  protocol->slot_message = sigc::mem_fun(this, &FOAM::on_message);
  protocol->slot_connected = sigc::mem_fun(this, &FOAM::on_connect);
  protocol->listen();
}

bool FOAM::listen() {
	io.msg(IO_DEB1, "FOAM::listen()");
	
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
				io.msg(IO_INFO, "Entering listen loop.");
				// We wait until the mode changed
				protocol->broadcast("OK MODE LISTEN");
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_wait(&mode_cond, &mode_mutex);
				pthread_mutex_unlock(&mode_mutex);
				break;
			case AO_MODE_SHUTDOWN:
				io.msg(IO_DEB1, "FOAM::listen() AO_MODE_SHUTDOWN");
				return true;
				break;
			default:
				io.msg(IO_DEB1, "FOAM::listen() UNKNOWN!");
				break;
		}
	}
	
	return false;
}

bool FOAM::mode_open() {
	io.msg(IO_INFO, "FOAM::mode_open()");

	if (ptc->wfs_count == 0) {	// we need wavefront sensors
		io.msg(IO_WARN, "No WFSs defined, cannot run open loop.");
		ptc->mode = AO_MODE_LISTEN;
		return false;
	}
	
	// Run the initialisation function of the modules used
	if (!open_init()) {
		io.msg(IO_WARN, "FOAM::open_init() failed.");
		ptc->mode = AO_MODE_LISTEN;
		return false;
	}
		
	protocol->broadcast("OK MODE OPEN");
	
	while (ptc->mode == AO_MODE_OPEN) {
		if (!open_loop()) {
			io.msg(IO_WARN, "FOAM::open_loop() failed");
			ptc->mode = AO_MODE_LISTEN;
			return false;
		}
		ptc->frames++;	// increment the amount of frames parsed
	}
	
	if (!open_finish()) {		// check if we can finish
		io.msg(IO_WARN, "FOAM::open_finish() failed.");
		ptc->mode = AO_MODE_LISTEN;
		return false;
	}
	
	return true;
}

bool FOAM::mode_closed() {	
	io.msg(IO_INFO, "FOAM::mode_closed()");

	// Pre-loop checks
	if (ptc->wfs_count == 0) { // we need wave front sensors
		io.msg(IO_WARN, "No WFSs defined, cannot run closed loop.");
		ptc->mode = AO_MODE_LISTEN;
		return false;
	}
	
	// Initialize closed loop
	if (!closed_init()) {
		io.msg(IO_WARN, "FOAM::closed_init() failed");
		ptc->mode = AO_MODE_LISTEN;
		return false;
	}
		
	protocol->broadcast("OK MODE CLOSED");
	
	// Run closed loop
	while (ptc->mode == AO_MODE_CLOSED) {
		if (!closed_loop()) {
			io.msg(IO_WARN, "FOAM::closed_loop() failed.");
			ptc->mode = AO_MODE_LISTEN;
			return false;
		}		
		ptc->frames++;
	}
	
	// Finish closed loop
	if (!closed_finish()) {
		io.msg(IO_WARN, "FOAM::closed_finish() failed.");
		ptc->mode = AO_MODE_LISTEN;
		return false;
	}
		
	return true;
}

bool FOAM::mode_calib() {
	io.msg(IO_INFO, "FOAM::mode_calib()");
	
	protocol->broadcast("OK MODE CALIB");
	
	// Run calibration inside module
	if (!calib()) {
		io.msg(IO_WARN, "FOAM::calib() failed.");
		protocol->broadcast("ERR CALIB :FAILED");
		ptc->mode = AO_MODE_LISTEN;
		return false;
	}
	
	protocol->broadcast("OK CALIB");
	
	io.msg(IO_INFO, "Calibration loop done, switching to listen mode.");
	ptc->mode = AO_MODE_LISTEN;
	
	return true;
}

void FOAM::on_connect(Connection *connection, bool status) {
  if (status) {
    connection->write("OK CLIENT CONNECTED");
    io.msg(IO_DEB1, "Client connected from %s.", connection->getpeername().c_str());
  }
  else {
    connection->write("OK CLIENT DISCONNECTED");
    io.msg(IO_DEB1, "Client from %s disconnected.", connection->getpeername().c_str());
  }
}

void FOAM::on_message(Connection *connection, std::string line) {
  io.msg(IO_DEB1, "FOAM::Got %db: '%s'.", line.length(), line.c_str());
	string cmd = popword(line);
	string orig = line;
	
	if (cmd == "HELP") {
		connection->write("OK CMD HELP");
		string topic = popword(line);
    if (show_nethelp(connection, topic, line))
			//if (modMessage(ptc, connection, "HELP", orig))
				connection->write("ERR CMD HELP :TOPIC UNKNOWN");
  }
  else if (cmd == "EXIT" || cmd == "QUIT" || cmd == "BYE") {
		connection->write("OK CMD EXIT");
    connection->server->broadcast("OK CLIENT DISCONNECTED");
    connection->close();
  }
  else if (cmd == "SHUTDOWN") {
		connection->write("OK CMD SHUTDOWN");
		ptc->mode = AO_MODE_SHUTDOWN;
		pthread_cond_signal(&mode_cond); // signal a change to the threads
  }
  else if (cmd == "BROADCAST") {
		connection->write("OK CMD BROADCAST");
		connection->server->broadcast(format("OK BROADCAST %s :FROM %s", 
																				 line.c_str(),
																				 connection->getpeername().c_str()));
	}
  else if (cmd == "VERBOSITY") {
		string var = popword(line);
		if (var == "+") io.incVerb();
		else if (var == "-") io.decVerb();
		else io.setVerb(var);
		
		connection->server->broadcast(format("OK VERBOSITY %d", io.getVerb()));
	}
  else if (cmd == "GET") {
    string var = popword(line);
		if (var == "FRAMES") connection->write(format("OK VAR FRAMES %d", ptc->frames));
		else if (var == "NUMWFS") connection->write(format("OK VAR NUMWFS %d", ptc->wfs_count));
		else if (var == "NUMWFC") connection->write(format("OK VAR NUMWFC %d", ptc->wfc_count));
		else if (var == "MODE") connection->write(format("OK VAR MODE %s", mode2str(ptc->mode).c_str()));
//		else if (modMessage(ptc, connection, "GET", orig))
			connection->write("ERR GET :VAR UNKNOWN");
	}
  else if (cmd == "MODE") {
    string mode = popword(line);
		if (mode == mode2str(AO_MODE_CLOSED)) {
			connection->write("OK CMD MODE CLOSED");
			ptc->mode = AO_MODE_CLOSED;
			pthread_cond_signal(&mode_cond); // signal a change to the main thread
		}
		else if (mode == mode2str(AO_MODE_OPEN)) {
			connection->write("OK CMD MODE OPEN");
			ptc->mode = AO_MODE_OPEN;
			pthread_cond_signal(&mode_cond); // signal a change to the main thread
		}
		else if (mode == mode2str(AO_MODE_LISTEN)) {
			connection->write("OK CMD MODE LISTEN");
			ptc->mode = AO_MODE_LISTEN;
			pthread_cond_signal(&mode_cond); // signal a change to the main thread
		}
		else {
			connection->write("ERR CMD MODE :UNKOWN");
		}
  }
  else {
//    if (modMessage(ptc, connection, cmd, line))
			connection->write("ERR CMD :UNKNOWN");
  }
}

bool FOAM::show_nethelp(Connection *connection, string topic, string rest) {
	if (topic.size() == 0) {
		connection->write(\
":help [command]:         Help (on a certain command, if available).\n"
":mode <mode>:            close or open the loop.\n"
":get <var>:              read a system variable.\n"
":verb <level>:           set verbosity to <level>.\n"
":verb <+|->:             increase/decrease verbosity by 1 step.\n"
":broadcast <msg>:        send a message to all connected clients.\n"
":exit or quit:           disconnect from daemon.\n"
":shutdown:               shutdown FOAM.");
		// pass it along to modMessage() as well
//		modMessage(ptc, connection, "help", rest);
	}		
	else if (topic == "MODE") {
		connection->write(\
":mode <mode>:            Close or open the AO-loop.\n"
":  mode=open:            opens the loop and only records what's happening with\n"
":                        the AO system and does not actually drive anything.\n"
":  mode=closed:          closes the loop and starts the feedbackloop, \n"
":                        correcting the wavefront as fast as possible.\n"
":  mode=listen:          stops looping and waits for input from the users.");
	}
	else if (topic == "BROADCAST") {
		connection->write(\
":broadcast <mode>:       broadcast a message to all clients.");
	}
	else if (topic == "GET") {
		connection->write(\
											":get <var>:              read a system variable.\n"
											":  frames:               number of frames processed");
	}
	else {
		// Unknown topic, return error
		return -1;
	}
	return 0;
}

// DOXYGEN GENERAL DOCUMENTATION //
/*********************************/

/* Doxygen needs the following aliases in the configuration file!

ALIASES += authortim="Tim van Werkhoven (T.I.M.vanWerkhoven@phys.uu.nl)"
ALIASES += license="GPLv2"
ALIASES += wisdomfile="fftw_wisdom.dat"
ALIASES += name="FOAM"
ALIASES += longname="Modular Adaptive Optics Framework"
ALIASES += uilib="foam_ui_library.*"
ALIASES += cslib="foam_cs_library.*"
*/


/*!	\mainpage FOAM 

	\section aboutdoc About this document
	
	This is the (developer) documentation for FOAM, the Modular Adaptive Optics Framework 
	 (yes, FOAM is backwards). It is intended to clarify the
	code and give some help to people re-using or implementing the code for their specific needs.
	
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
	
	For more information, see the FOAM wiki at http://www.astro.uu.nl/~astrowik/astrowiki/index.php/FOAM or the documentation
	at http://dotdb.phys.uu.nl/~tim/foam/ . All versions of FOAM are available at the
	Subversion repository located at http://dotdb.phys.uu.nl/svn/foam/ .
	
	\section struct FOAM structure
	
	In this section, the structure of FOAM will be clarified. First, some key 
	concepts are explained which are used within FOAM.
	
	FOAM uses several concepts, for which it is useful to have names. First of 
	all, the complete set of files (downloadable from for example http://dotdb.phys.uu.nl/~tim/foam/) 
	is simply called a \e `distribution', or \e `version' or just \e FOAM.
	
	Historically, there was a FOAM Control Software (CS) and a User Interface 
	(UI), although the latter is not actively being developed as telnet suffices
	for the moment. This explains the \c `_cs' and \c `_ui' prefix in some filenames.
	
	Within the distribution itself (which consists mainly of the Control Software
	at the moment), there are several different components:
 
	\li The \e framework itself, in \c foam_cs.c
	\li \e Prime \e modules, in \c foam_primemod-*
	\li \e Modules, in \c foam_modules-*
	
	To get a running program, the framework, a prime module and zero or more 
	modules are combined into one executable. This program is called a 
	\e `package' and is what performs AO operations.
	
	By default a dummy package is supplied, which take the framework
	and the dummy prime module (which does nothing) which results in the 
	most basic package possible. A more interesting package, the simulation 
	package, is also provided. This package simulates a complete AO system 
	and can be used to test new routines (center-of-gravity tracking, 
	correlation tracking, load distribution) in a reproducible fashion. 

	There are two simulation prime modules, one is `simdyn', which performs
	dynamical simulation, meaning the atmosphere, WFCs, WFSs etc are all simulated
	dynamically, and `simstat', which uses a static WFS image and performs
	the calculations that a normal AO system would do. The first can be useful
	to see if the system works at all, and to improve algorithms qualitatively,
	while the second can be used to benchmark the performance of the system without
	the need of AO hardware.
	
	\subsection struct-frame The Framework
	
	The framework itself does very little. It performs some initialization, allocating 
	memory for control vectors, images and what not. After that it provides a hook for 
	prime modules to initialize themselves. Immediately after that, it splits in a worker 
	thread which does the AO calculations, and in a networking thread which listens for 
	connections on a (TCP) socket. The worker thread starts startThread(), while the
	other thread starts with sockListen(). Communication between the two threads is done 
	with the mutexes `mode_mutex' and `mode_cond'.
	
	The framework provides the following hooks to prime modules: 
	\li modStopModule(foamctrl *ptc)
	\li modInitModule(foamctrl *ptc, foamcfg *cs_config)
	\li modPostInitModule(foamctrl *ptc, foamcfg *cs_config)
	\li modOpenInit(foamctrl *ptc)
	\li modOpenLoop(foamctrl *ptc)
	\li modOpenFinish(foamctrl *ptc)
	\li modClosedInit(foamctrl *ptc)
	\li modClosedLoop(foamctrl *ptc)
	\li modClosedFinish(foamctrl *ptc)
	\li modCalibrate(foamctrl *ptc)
	\li modMessage(foamctrl *ptc, const client_t *client, char *list[], const int count)
	
	See the documentation on the specific functions for more details on what these functions
	can be used for.

	\subsection struct-prime The Prime Modules
	
	Prime modules must define the functions defined in the above list. The prime 
	module should tell what these hooks do, e.g. what happens during open loop, 
	closed loop and during calibration. These functions can be just placeholders,
	which can be seen in \c foam_primemod-dummy.c , or more complex functions 
	which actually do something.
	
	Almost always, prime modules link to modules in turn, by simply including 
	their header files and compiling the source files along with the prime module. 
	The modules contain the functions which do the hard work, and are divided into 
	separate files depending on the functionality of the routines. The prime module
	can thus be seen as a sort of hub connecting the framework with the modules.
	
	\subsection struct-mod The Modules
	
	Modules provide functions which can be used by other modules or by prime 
	modules. If a module uses routines from another module, this module depends 
	on the other module. This is for example the case between the `simdyn' module
	which needs to read in PGM files and therefore depends on the `img' module.
	
	A few default modules are provided with the distribution, and their function
	can be can be read at the top of the header file associated with the module.

	\subsection threading Threading model
 
	As noted before, the framework uses two threads. The main thread is used for
	networking with the clients, while the AO routines are performed in a different
	thread which separates from the main thread at the beginning. 
 
	These threads are not equal, as the first thread is created at program startup while the
	second one splits off this thread. This can be important for example when 
	using SDL on OS X, which requires some initialization code that *is* 
	automatically run when starting SDL from the main thread, but that is not
	run when starting SDL from any other thread. Another issue can be OpenGL,
	which can only be called from one thread, and thus must be initialized in
	the thread it will be called from.
 
	To circumvent possible problems, FOAM provides two initialization routines,
	one is run at the beginning before any threading, modInitModule(), while
	the other is run in the thread that will be controlling the AO, 
	modPostInitModule(). Most configuration consists of things like allocating 
	memory, setting variables, reading files etc which are thread-safe. Other 
	things such as OpenGL initialization might not be thread-safe and might 
	have to be initialized after threading.
 
	Another thing to keep in mind is that all functions are called from the AO
	worker thread, except for the modMessage() function, which is called from
	the networking thread. This gives some problems when using libevent to
	send information to clients as libevent is not entirely thread safe
	(and its thread-safety is unclear as well). Currently, only the networking
	thread does interaction with the user.
 
	\subsection codeskel Code skeleton
 
	To provide some insight in the way FOAM functions, consider this code skeleton:

	\code
main() {
	modInitModule()				// Initialize modules,
								// memory, cameras etc.
	
	pthread (startThread())		// Branch into two threads,
								// one for AO, one for user
								// I/O
	
	sockListen()				// Read & process user I/O.
	exit
}

sockListen() {
	while (true) {				// In a continuous loop,
		parseCmd()				// read the user input and
		modMessage()			// process it.
	}
}

startThread() {
	modPostInitModule()			// After threading, provide
								// an additional init hook.
}

listenLoop() {
	while (ptc->mode != shutdown) { // Run continuously until
								// shutdown.
		switch (ptc->mode) {		// Read the mode requested
								// and switch to that mode.
			case 'open': modeOpen()
			case 'closed': modeClosed()
			case 'calibration': modeCal()
		}
	}
	modStopModule()				// Shutdown the modules.
}

modeOpen() {
	modOpenInit()				// Open loop init hook.
	
	while (ptc->mode == AO_MODE_OPEN) { 
								// Loop until mode changes.
		modOpenLoop()			// Actual work is done here.
	}
	
	modOpenFinish()				// Clean up after open loop.
}

modeClosed() {
	modClosedInit()				// Closed loop works the
								// same as open loop.
	
	while (ptc->mode == AO_MODE_CLOSED) {
		modClosedLoop()
	}
	
	modClosedFinish()
}

modeCal() {
	modCalibrate()				// Calibration mode provides
								// simply one hook.
}
	\endcode
 
	Note that all hooks (starting with mod*) are run from the AO thread, except for
	modMessage(), which is run from the networking thread.
 
	\section install_sec Installation
	
	FOAM currently depends on the following libraries to be present on the system:
	\li \c libevent used to handle networking with several simultaneous connections,
	\li \c SDL which is used to display the sensor output,
	\li \c pthread used to separate functions over thread and distribute load,
	\li \c libgsl used to do singular value decomposition, link to BLAS and various other matrix/vector operation.
	
	For other prime modules, additional libraries may be required. For example,
	the simdyn prime module also requires:
	\li \c fftw3 which is used to compute FFT's to simulate the SH lenslet array,
	\li \c SDL_Image used to read image files files.
	
	Furthermore FOAM requires basic things like a hosted compilation environment, 
	a compiler etc. For a full list of dependencies, see the header files. FOAM 
	is however supplied with an (auto-)configure script which checks these
	things and tells you what the capabilities of FOAM on your system will be. If
	libraries are missing, the configure script will tell you so.
	
	To install FOAM, follow these simple steps:
	\li Download a FOAM release from http://dotdb.phys.uu.nl/~tim/foam/ or check out a version of FOAM from http://dotdb.phys.uu.nl/svn/foam/
	\li Extract the tarball
	\li For an svn checkout: run `autoreconf -s -i` to the configure script
	\li Run `./configure --help` to check what options you would like to use
	\li Run `./configure` (with specific options) to prepare FOAM for building. If some libraries are missing, configure will tell you so
	\li Run `make install` which makes the various FOAM packages and installs them in $PREFIX
	\li Run any of the executables (foamcs-*)
	\li Connect to localhost:10000 (default) with a telnet client
	\li Type `help' to get a list of FOAM commands
	
	\subsection config Configure FOAM
	
	With simulation, make sure you do \b not copy the FFTW wisdom file 
	<tt>'fftw_wisdom.dat'</tt> to new machines, this file contains some 
	simple benchmarking results done by FFTW and are very machine dependent. 
	FOAM will regenerate the file if necessary, but it cannot detect 
	`wrong' wisdom files. Copying bad files is worse than deleting, thus
	if unsure: delete the wisdom file.
	
	\section drivers Developing Packages

	As said before, FOAM itself does not do a lot (compile & run 
	<tt>./foamcs-dummy</tt> to verify), it provides a framework to which modules can be 
	attached. Fortunately, this approach allows for complex bundles of 
	modules, or `packages', as explained previously in \ref struct.
	
	If you want to use FOAM in a specific setup, you'll probably have to 
	program some modules yourself. To do this, start with a `prime module' 
	which \b must contain the functions listed in \ref struct-frame (see 
	foam_primemod-dummy.c for example). 
		
	These functions provide hooks for the package to work with, and if 
	their meaning is not immediately clear, the documentation provides some 
	more details on what these functions do. It is also wise to look at the 
	foamctrl struct which is used throughout FOAM to store data, settings 
	and other information.
	
	Once these functions are defined, you can link the prime module to modules 
	already supplied with FOAM. Simply do this by including the header files 
	of the specific modules you want to add. For information on what certain 
	modules do, look at the documentation of the files. This documentation 
	tells you what the module does, what functions are available, and what 
	other modules this module possibly depends on.
	
	If the default set of modules is insufficient to build a package in your 
	situation, you will have to write your own. This scenario is (unfortunately) 
	very likely, because FOAM does not provide modules to read out all possible 
	framegrabbers/cameras and cannot drive all possible filter wheels, tip-tilt 
	mirrors, deformable mirrors etc. If you have written your own module, please 
	e-mail it to me (Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)) so I can add it to the complete distribution.
	
	\subsection ownmodule Write your own modules
	
	A module in FOAM can take any form, but to keep things at least slightly 
	organised, some conventions apply. These guidelines include:
	
	\li separate functionally different routines in different modules,
	\li Name your modules `foam_modules-\<modulename\>.c' and supply a header file,
	\li Include doxygen compatible information on the module in general in the C file
	\li Include doxygen compatible information on the routines in the header file
	\li Prefix functions that you write with \<modulename\>.
	\li If necessary, provide a datatype to hold module-specific configuration
 
	
	These guidelines are also used in the existing modules so you can easily 
	see what a module does and what functions you have to call to get the module working.
	For examples, see foam_modules-sh.c and foam_modules-sh.h.
	
	\subsection ownmake Adapt the Makefile
	
	Once you have a package, you will need to edit Makefile.am in the src/ 
	directory. But before that, you will need to edit the configure.ac script 
	in the root directory. This is necessary so that at configure time, users 
	can decide what packages to build or not to build, using 
	--enable-\<package\>. In the configure.ac file, you will need to add two 
	parts:
 
	\li Add a check to see if the user wants to build the package or not (see 'TEST FOR USER INPUT')
	\li Add a few lines to check if extra libraries necessary for the package are present (see 'STATIC SIMULATION MODE')
	
	After editing the configure.ac script, the Makefile.am needs to know 
	that we want to build a new package/executable. For an example on how 
	to do this, see the few lines following 'STATIC SIMULATION MODE'. 
	In short: check if the package must be built, if yes, add package to 
	bin_PROGRAMS, and setup various \c _SOURCES, \c _CFLAGS and \c _LDFLAGS 
	variables.
	
	\section network Networking

	Currently, the following status codes are used, based loosely on the HTTP status codes. All
	codes are 3 bytes, followed by a space, and end with a single newline. The length of the message
	can vary.
	
	2xx codes are given upon success:
	\li 200: Successful reception of command, executing immediately,
	\li 201: Executing immediately command succeeded.
	
	4xx codes are errors:
	\li 400: General error, something is wrong
	\li 401: Argument is not known
	\li 402: Command is incomplete (missing argument)
	\li 403: Command given is not allowed at this stage
	\li 404: Previously given and acknowledged command failed.
	
	\section limit_sec Limitations/bugs
	
	There are some limitations to FOAM which are discussed in this section. The list includes:

	\li The subaperture resolution must be a multiple of 4, because of tracking windows
	\li Commands given to FOAM over the socket/network can be at most 1024 characters,
	\li At the moment, most modules work with bytes to process data
	\li On OS X, SDL does not appear to behave nicely, especially during shutdown (this is a problem of the OS X SDL implementation and cannot easily be fixed without using Objective C code)
	
	There might be other limitations or bugs, but these are not listed here 
	because I am not aware of them. If you find some, please let me know (Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)).
 
*/
