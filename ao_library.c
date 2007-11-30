/* 
	Library file for the whole AO framework
	Tim van Werkhoven, November 13 2007
*/

#include "ao_library.h"

int sockRead(int sock, char *msg, size_t msglen, fd_set *lfd_set) {
	// TODO: this only accepts STATIC length messages! buffer overflow problem!
	int nbytes;
	
	nbytes = recvfrom(sock, msg, msglen, 0, 0, 0); // TODO: nodig? of recv genoeg?
	msg[nbytes] = '\0';
	
	if (nbytes == 0) { 			// EOF, close the socket! 
		close(sock);
		FD_CLR(sock, lfd_set);
		return 0;
	}
	
	else if (nbytes == -1) { 	// Error occured, exit and return EXIT_FAILURE
		perror("error in recvfrom");
		exit(0);
		return EXIT_FAILURE;
	}

	else { 						// Data on the socket, read it
		return nbytes;
	}
}
