/*
    protocol.cc -- Multiplexing commands to a remote daemon
    Copyright (C) 2007  Guus Sliepen <G.Sliepen@astro.uu.nl>

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

#include <sys/time.h>
#include <time.h>

#include "format.h"
#include "protocol.h"

#define iterator(c) typedef typeof(c) c##_type; c##_type::iterator
#define foreach(i, c) typedef typeof(c) c##_type; for(c##_type::iterator i = c.begin(); i != c.end(); ++i)

using namespace std;

namespace Protocol {
	void Client::handler() {
		pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);

		while(running) {
			socket.connect(host, port);

			if(!socket.is_connected()) {
				sleep(1);
				continue;
			}

			slot_connected(true);

			string line;

			while(running && socket.readline(line)) {
				if(!name.empty() && popword(line) != name)
					continue;

				slot_message(line);
			}

			slot_connected(false);

			socket.close();
		}
	}

	Client::Client(const std::string &host, const std::string &port, const std::string &name): host(host), port(port), name(name) {
		if(!name.empty())
			prefix = name + ' ';
		running = false;
	};

	Client::~Client() {
		if(running) {
			thread.cancel();
			thread.join();
		}
	}

	void Client::connect() {
		if(running)
			return;

		running = true;
		attr.setstacksize(65536);
		thread.create(&attr, sigc::mem_fun(this, &Client::handler));
	}

	void Client::close() {
		if(!running)
			return;

		socket.close();
	}

	bool Client::is_connected() {
		return socket.is_connected();
	}

	void Client::write(const string &msg) {
		if(!is_connected())
			return;
		socket.write(prefix + msg + "\r\n");
	}

	void Client::write(const void *buf, size_t len) {
		if(!is_connected())
			return;
		socket.write(buf, len);
	}

	string Client::read() {
		return socket.readline();
	}

	bool Client::read(void *buf, size_t len) {
		return socket.read(buf, len);
	}

	string Client::getpeername() {
		return socket.getpeername();
	}

	string Client::getsockname() {
		return socket.getsockname();
	}

	map<string, Server::Port *> Server::Port::ports;
	pthread::mutex Server::Port::globalmutex;

	Server::Port::Port(const std::string &port): port(port) {
		attr.setstacksize(65536);
		thread.create(&attr, sigc::mem_fun(this, &Server::Port::handler));
	}

	Server::Port::~Port() {
		thread.cancel();
		//For some reason this doesn't work
		//thread.join();
	}

	Server::Port::Port *Server::Port::get(Server *server) {
		pthread::mutexholder g(&globalmutex);
		Port *port = ports[server->port];
		if(!port)
			ports[server->port] = port = new Port(server->port);

		pthread::mutexholder h(&port->mutex);
		Server *user = port->users[server->name];
		if(!user)
			port->users[server->name] = server;
		else
			throw exception("Duplicate port+name combination");

		return port;
	}

	void Server::Port::release(Server *server) {
		pthread::mutexholder g(&globalmutex);
		Port *port = ports[server->port];
		if(!port)
			throw exception("Releasing user from unknown port");

		pthread::mutexholder h(&port->mutex);
		Server *user = port->users[server->name];
		port->users.erase(server->name);
		if(!user)
			throw exception("Releasing unknown user from port");

		if(port->users.empty()) {
			ports.erase(server->port);
			delete port;
		}
	}

	void Server::Port::close() {
		socket.close();

		pthread::mutexholder h(&mutex);

		foreach(c, connections)
			delete *c;

		connections.clear();
	}

	void Server::Port::handler() {	
		pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);

		try {
			socket.listen(port);

			while(true) {
				Socket *incoming = socket.accept();
				if(!incoming) {
					sleep(1);
					continue;
				}
				new Connection(this, incoming);
			}

			close();
		} catch(...) {
			close();
			throw;
		}
	}

	Server::Connection::Connection(Port *port, Socket *socket, const void *data): port(port), socket(socket), data(data) {
		server = 0;
		running = true;
		pthread::mutexholder h(&port->mutex);
		port->connections.insert(this);
		attr.setstacksize(65536);
		thread.create(&attr, sigc::mem_fun(this, &Server::Connection::handler));
	}

	Server::Connection::~Connection() {
		if(running) {
			thread.cancel();
			thread.join();
		} else {
			thread.detach();
		}
		socket->close();
		pthread::mutexholder h(&port->mutex);
		port->connections.erase(this);
	}

	void Server::Connection::handler() {
		pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);

		string prevline;

		while(running) {
			string line;
			if(!socket->readline(line))
				break;

			if(line == ",")
				line = prevline;
			else
				prevline = line;

			string name = popword(line);

			{
				pthread::mutexholder h(&port->mutex);
				map<string, Server *>::iterator i = port->users.find(name);
				if(i == port->users.end()) {
					line.insert(0, name + ' ');
					i = port->users.find("");
				}
				if(i == port->users.end())
					continue;
				server = i->second;
			}

			server->slot_message(this, line);
			server = 0;
		}

		running = false;
		delete this;
	}

	Server::Server(const std::string &port, const std::string &name): port(port), name(name) {
		if(!name.empty())
			prefix = name + ' ';
	}

	void Server::listen() {
		theport = Port::get(this);
	}

	Server::~Server() {
		Port::release(this);
	}

	void Server::Connection::close() {
		running = false;
		socket->close();
	}

	bool Server::Connection::is_connected() {
		return socket->is_connected();
	}

	void Server::Connection::write(const string &msg) {
		socket->write(server->prefix + msg + "\r\n");
	}

	void Server::Connection::write(const void *buf, size_t len) {
		socket->write(buf, len);
	}

	void Server::Connection::addtag(const string &tag) {
		if(!server)
			return;
		tags.insert(server->prefix + tag);
	}

	void Server::Connection::deltag(const string &tag) {
		if(!server)
			return;
		tags.erase(server->prefix + tag);
	}

	bool Server::Connection::hastag(const string &tag) {
		if(!server)
			return false;
		return tags.find(server->prefix + tag) != tags.end();
	}

	bool Server::Connection::hastag(const string &tag, const string &prefix) {
		return tags.find(prefix + tag) != tags.end();
	}

	void Server::Connection::write(const string &msg, const string &tag) {
		pthread::mutexholder h(&port->mutex);
		for(set<Connection *>::iterator i = port->connections.begin(); i != port->connections.end(); ++i) {
			Connection *c = *i;
			if(this == c || c->hastag(tag, server->prefix))
				c->socket->write(server->prefix + msg + "\r\n");
		}
	}

	void Server::broadcast(const string &msg) {
		pthread::mutexholder h(&theport->mutex);
		for(set<Connection *>::iterator i = theport->connections.begin(); i != theport->connections.end(); ++i)
			(*i)->socket->write(prefix + msg + "\r\n");
	}

	void Server::broadcast(const string &msg, const string &tag) {
		pthread::mutexholder h(&theport->mutex);
		for(set<Connection *>::iterator i = theport->connections.begin(); i != theport->connections.end(); ++i)
			if((*i)->hastag(tag, prefix))
				(*i)->socket->write(prefix + msg + "\r\n");
	}

	string Server::Connection::read() {
		return socket->readline();
	}

	bool Server::Connection::read(void *buf, size_t len) {
		return socket->read(buf, len);
	}

	string Server::Connection::getpeername() {
		return socket->getpeername();
	}

	string Server::Connection::getsockname() {
		return socket->getsockname();
	}
};
