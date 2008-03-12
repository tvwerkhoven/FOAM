/*! 
	@file foam_ui.c
	@author @authortim
	@date November 14 2007

	@brief This is a client which can connect to the Control Software

	This file is a simple client which can connect to a CS running
	on some computer over a socket. The program then reads user input
	form \a stdin and sends this to the CS over the socket. Additionally,
	this program can also read data coming back from the CS, and displays
	this on \a stdout, (TODO: or in some graphical window).\n
	\n
	At the moment, this is not much more than a simply telnet client.
*/

#include "foam_ui_library.h"

extern config_t ui_config;


	/*! 
	@brief main loop function
	
	main() continuously checks for activity on \<stdin\>, to check
	for user input, and on the socket, to check if the CS is sending
	data. If something happens, main() calls specific functions to handle
	this activity.
	*/
int main (int argc, char *argv[]) {
	int sock, asock;
	in_addr_t host;
	int port;
	fd_set read_fd_set, active_fd_set;
	char msg[1024];
	int nbytes;
	
//	printf("level: %d and deb: %d info: %d err: %d\n", ui_config.loglevel, LOGDEBUG, LOGINFO, LOGERR);

	logInfo("Starting FOAM User Interface...");

			
	if (parseArgs(argc, argv, &host,&port) != EXIT_SUCCESS)
		return EXIT_FAILURE;
										// Parse the arguments (we ONLY expect host and port, 
										// this might not be very decent, TODO)
	logInfo("Parsed arguments...");
	
	if ((sock = initSockC(host, port, &active_fd_set)) == EXIT_FAILURE) // initialize socket to connect with, add to an fd_set
		return EXIT_FAILURE;
	
	logDebug("Initializing socket successful.");

	FD_SET(STDIN_FILENO, &active_fd_set); 						// add stdin to the set as well
	
	while (1) {

		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			logErr("Error in select: %s", strerror(errno));
			return EXIT_FAILURE;
		}
		
		asock = sockGetActive(&read_fd_set);
		
		if (asock == sock) { 									// Act on socket with server
//			nbytes = recvfrom(sock,buf,sizeof(buf),0,0,0); 		// TODO: nodig?
			
			nbytes = sockRead(asock, msg, &active_fd_set);
		
			if (nbytes == 0) {
				logInfo("Host closed connection, exiting...");
				exit(EXIT_SUCCESS);
			}
			else if (nbytes == EXIT_FAILURE) exit(0);	// Error on socket TODO: what is EXIT_FAILURE is 1?
			else {										// We got data
				logDebug("%d bytes received on the socket: '%s'", nbytes, msg);
			}

		}
		else if (asock == STDIN_FILENO) { 						// we read the user here, write to the socket
			fgets(msg, sizeof(msg), stdin);
			
			msg[strlen(msg)-1] = '\0';					// Strip trailing newline
			logDebug("Data from stdin: %s",msg);
			
			sendMsg(sock, msg);
		}
		else {
			logErr("Error: this must not be happening\n");
		}
	}
	
	return EXIT_SUCCESS;
}

// int sendMsg(const int sock, const char *buf)
// 	return write(sock, buf, sizeof(buf)); // TODO non blocking maken

	/*! 
	@brief Parse the commandline arguments

	@param [in] argc the argument count (from main())
	@param [in] *argv the argument array (from main())
	@param [out] *host the host to connect to
	@param [out] *port the port to connect to
	@return same as write(), number of bytes or -1 on error

	parseArgs() parses the commandline arguments as received by main() and 
	returns EXIT_FAILURE upon failure, or EXIT_SUCCESS upon succes.
	*/
int parseArgs(int argc, char *argv[], in_addr_t *host, int *port) {
	if (argc < 3) {
		logInfo("Call this program as: <name> <ip> <port>. Using defaults now (127.0.0.1:10000)");
		*port = 10000;
		*host = inet_addr("127.0.0.1");
	}
	else {
		logDebug("Parsing host: %s and port: %s", argv[1], argv[2]);
		*port = strtol(argv[2], NULL, 10); 				// atoi() replacement, supposed to be better
		*host = inet_addr(argv[1]);
		if (*host == INADDR_NONE || \
		    *port < 1 || \
		    *port > 65535) // TODO:  comparison between signed and unsigned?
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

	/*! 
	@brief Initialize socket connecting to server

	@param [in] host the host to connect to
	@param [in] port the port to connect to
	@param [out] *cfd_set the fdset the socket will be added to
	@return socket descriptor on success, EXIT_FAILURE otherwise.

	parseArgs() parses the commandline arguments as received by main() and 
	returns EXIT_FAILURE upon failure, or EXIT_SUCCESS upon succes.
	*/
int initSockC(in_addr_t host, int port, fd_set *cfd_set) {
	struct sockaddr_in serv_addr;
	int sock;
	
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		logErr("Socket error: %s", strerror(errno));
		return EXIT_FAILURE;
	}
	logDebug("Socket initialized");
	
	// Get the address fixed:
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = host;
	serv_addr.sin_port = htons(port);
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero)); // TODO: we need to set this padding to zero?
	
	logInfo("Trying to connect to the server...");
	
	// Connect to the server
	if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
		logErr("%s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	logDebug("Connected, adding open connection to socketlist.");
	// Initialize the file descriptor set.
	FD_ZERO(cfd_set);
	FD_SET(sock, cfd_set); // add sock to the set
	
	return sock;
}

	/*! 
	@brief Get the socket descriptor which needs attention.

	@param [in,out] *cfd_set the fd_set with sockets to scan.
	@return the socket needing attention, or EXIT_FAILURE upon error.

	sockGetActive scans cfd_set for activity (looping over all possible 
	FD_SETSIZE sockets), and returns the socket that needs attention. Typically
	will be called after a select() command noticed activity.
	*/
int sockGetActive(fd_set *cfd_set) {
	int i;

	for (i = 0; i < FD_SETSIZE; ++i) 
		if (FD_ISSET(i, cfd_set)) 
			return i;

	return EXIT_FAILURE; // This shouldn't happen (as with all errors)
}
