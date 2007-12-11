/* 
	Library file for the whole AO framework
	Tim van Werkhoven, November 13 2007
*/

#include "ao_library.h"


/*int sockRead(const int sock, char *msg) {
	// TODO: this only accepts STATIC length messages! buffer overflow problem! December 3 2007 is it?
	int nbytes;
	size_t msglen = strlen(msg); // TODO: this goes wrong, socket broken, implement libevent asap
	
	nbytes = recvfrom(sock, msg, msglen, 0, 0, 0); // TODO: nodig? of recv genoeg?
	msg[nbytes] = '\0';
	
	if (nbytes == 0) { 			// EOF, close the socket! 
		close(sock);
		FD_CLR(sock, lfd_set);
		return nbytes;
	}
	
	else if (nbytes == -1) { 	// Error occured, exit and return EXIT_FAILURE
		// TODO: how do we solve this? Cant use logERR?
		printf("Error in recvfrom: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	else { 						// Data on the socket, read it
		return nbytes;
	}
}*/

int printUTC(char **ret) {
	struct tm *utc;
	time_t t;
	t = time(NULL);
	utc = gmtime(&t);
 	*ret = asctime(utc);
	return EXIT_SUCCESS;
}

int sendMsg(const int sock, const char *buf) {
	printf("sending, len: %d", (int) strlen(buf));
	return write(sock, buf, strlen(buf)); // TODO non blocking maken
}
