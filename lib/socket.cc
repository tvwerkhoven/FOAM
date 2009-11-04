/*
    socket.cc -- socket creation
    Copyright (C) 2004-2007  Guus Sliepen <guus@sliepen.eu.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <poll.h>

#include "socket.h"
#include <string>

using namespace std;

Socket::Socket(int fd): fd(fd), inlen(0) {}

Socket::Socket(): fd(-1), inlen(0) {}

Socket::Socket(const string &port): fd(-1), inlen(0) {
	listen(port);
}

Socket::Socket(const string &host, const string &port): fd(-1), inlen(0) {
	if(!connect(host, port))
		throw exception((string)"Could not create a socket connected to " + host + " port " + port + ": " + strerror(errno));
}

Socket::~Socket() {
	close();
}

bool Socket::is_connected() {
	return fd >= 0;
}

bool Socket::setblocking(bool blocking) {
	int flags = fcntl(fd, F_GETFL);
	if(blocking)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
	return !fcntl(fd, F_SETFL, flags);
}

bool Socket::connect(const string &host, const string &port) {
	struct addrinfo *ai = NULL, *aip, hint = {0};
	int err, tmpfd = -1;

	close();

	/* Resolve address for listening socket */

	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	err = getaddrinfo(host.c_str(), port.c_str(), &hint, &ai);

	if(err || !ai)
		return false;

	/* Try all addresses until we find one that works */

	for(aip = ai; aip; aip = aip->ai_next) {
		tmpfd = ::socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
		if(tmpfd == -1)
			continue;
		
		int opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

		err = ::connect(tmpfd, aip->ai_addr, aip->ai_addrlen);
		if(err) {
			::close(tmpfd);
			tmpfd = -1;
			continue;
		}

		break;
	}

	freeaddrinfo(ai);

	return (fd = tmpfd) >= 0;
}

void Socket::listen(const string &port) {
	struct addrinfo *ai = NULL, *aip, hint = {0};
	int err;

	close();

	/* Resolve address for listening socket */

	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;

	err = getaddrinfo(NULL, port.c_str(), &hint, &ai);

	if(err || !ai)
		throw exception((string)"Could not resolve port " + port + ": " + strerror(errno));

	/* Try all addresses until we find one that works */

	for(aip = ai; aip; aip = aip->ai_next) {
		fd = ::socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
		if(fd == -1)
			continue;
		
		int opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

		err = ::bind(fd, aip->ai_addr, aip->ai_addrlen) || ::listen(fd, 1);
		if(err) {
			::close(fd);
			fd = -1;
			continue;
		}
		break;
	}

	freeaddrinfo(ai);

	if(fd < 0)
		throw exception((string)"Could not listen on port " + port + ": " + strerror(errno));
}

string Socket::resolve(struct sockaddr *addr, socklen_t addrlen, int flags) {
	char host[NI_MAXHOST], serv[NI_MAXSERV];

	if(getnameinfo(addr, addrlen, host, sizeof host, serv, sizeof serv, flags) < 0)
		return "";
	
	return format("%s/%s", host, serv);
}

string Socket::getpeername() {
	if(fd < 0)
		return "";
	
	struct sockaddr_storage name;
	socklen_t namelen = sizeof name;
	if(::getpeername(fd, (struct sockaddr *)&name, &namelen) < 0)
		return "";
	
	return resolve((struct sockaddr *)&name, namelen);
}

string Socket::getsockname() {
	if(fd < 0)
		return "";
	
	struct sockaddr_storage name;
	socklen_t namelen = sizeof name;
	if(::getsockname(fd, (struct sockaddr *)&name, &namelen) < 0)
		return "";
	
	return resolve((struct sockaddr *)&name, namelen);
}

Socket::Socket *Socket::accept() {
	int newfd = ::accept(fd, 0, 0);

	if(fd < 0)
		return 0;

	return new Socket(newfd);
}

void Socket::close() {
	if(fd < 0)
		return;
	
	::close(fd);
	fd = -1;
	inlen = 0;
}

bool Socket::gets(char *buf, size_t len) {
	char *newline = (char *)memchr(inbuf, '\n', inlen);

	if(fd < 0)
		return false;

	if(!newline) while(inlen < sizeof inbuf) {
		struct pollfd pfd = {fd, POLLIN};
		poll(&pfd, 1, -1);
		ssize_t result = ::read(fd, inbuf + inlen, sizeof inbuf - inlen);
		
		if(result <= 0) {
			if(errno == EINTR || errno == EAGAIN)
				continue;
	
			return false;
		}

		inlen += result;

		if((newline = (char *)memchr(inbuf, '\n', inlen)))
			break;
	};

	size_t linelen = newline + 1 - inbuf;

	if(!newline || linelen >= len) {
		errno = EMSGSIZE;
		return false;
	}

	memcpy(buf, inbuf, linelen);
	buf[linelen - 1] = 0;
	if(linelen > 1 && buf[linelen - 2] == '\r')
		buf[linelen - 2] = 0;

	memmove(inbuf, newline + 1, inlen - linelen);
	inlen -= linelen;

	return true;
}

bool Socket::readline(string &line) {
	char buf[4096];
	if(gets(buf, sizeof buf)) {
		line = buf;
		return true;
	} else {
		return false;
	}
}

string Socket::readline() {
	char buf[4096];
	if(gets(buf, sizeof buf))
		return buf;
	else
		throw exception((string)"Error while reading line from socket: " + strerror(errno));
}

bool Socket::read(void *buf, size_t len) {
	char *p = (char *)buf;
	size_t left = len;
	ssize_t result;

	if(fd < 0)
		return false;

	if(inlen) {
		if(inlen < len) {
			memcpy(p, inbuf, inlen);
			left -= inlen;
			p += inlen;
			inlen = 0;
		} else {
			memcpy(p, inbuf, len);
			memmove(inbuf, inbuf + len, inlen - len);
			inlen -= len;
			return true;
		}
	}

	while(left) {
		struct pollfd pfd = {fd, POLLIN};
		poll(&pfd, 1, -1);
		result = ::read(fd, p, left);

		if(result <= 0) {
			if(errno == EINTR || errno == EAGAIN)
				continue;

			return false;
		}

		p += result;
		left -= result;
	}

	return true;
}	

bool Socket::write(const void *buf, size_t len) {
	const char *p = (const char *)buf;
	size_t left = len;
	ssize_t result;

	if(fd < 0)
		return false;

	while(left) {
		struct pollfd pfd = {fd, POLLOUT};
		poll(&pfd, 1, 1000);
		result = ::write(fd, p, left);

		if(result <= 0) {
			if(errno == EINTR || errno == EAGAIN)
				continue;
			return false;
		}

		p += result;
		left -= result;
	}

	return true;
}	

bool Socket::write(const std::string str) {
	return write(str.c_str(), str.size());
}

bool Socket::readavailable() {
	struct pollfd pfd = {fd, POLLIN};
	return poll(&pfd, 1, 0);
}

bool Socket::writeavailable() {
	struct pollfd pfd = {fd, POLLOUT};
	return poll(&pfd, 1, 0);
}


bool Socket::vprintf(const char *format, va_list va) {
	return write(vformat(format, va));
}	

bool Socket::printf(const char *format, ...) {
	bool result;
	va_list va;
	va_start(va, format);
	result = write(vformat(format, va));
	va_end(va);
	return result;
}

Socket::Socket &Socket::operator<<(const string line) {
	write(line);
	return *this;
}

Socket::Socket &Socket::operator<<(const char *line) {
	write(line, strlen(line));
	return *this;
}
	
Socket::Socket Socket::operator>>(string &line) {
	line = readline();
	return *this;
}
