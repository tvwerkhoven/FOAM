#include <unistd.h>
#include <string>
#include <sigc++/signal.h>
#include "protocol.h"
#include "socket.h"
#include "pthread++.h"

using namespace std;
typedef Protocol::Server::Connection Connection;

static Protocol::Server *protocol = 0;

static void on_message(Connection *connection, std::string line);
static void on_connect(Connection *connection, bool status);

int main() {
	protocol = new Protocol::Server("1234");

	protocol->slot_message = sigc::ptr_fun(on_message);
	protocol->slot_connected = sigc::ptr_fun(on_connect);
	protocol->listen();

	while (1)
		sleep(1);

	return 0;
}

static void on_connect(Connection *connection, bool status) {
	fprintf(stderr, "on_connected: %d\n", status);
}

static void on_message(Connection *connection, string line) {
	fprintf(stderr, "on_message: %s\n", line.c_str());
}


