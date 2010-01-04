#include <unistd.h>
#include <string>
#include <sigc++/signal.h>
#include "protocol.h"
#include "socket.h"
#include "pthread++.h"

using namespace std;
typedef Protocol::Server::Connection Connection;

static void on_message(Connection *connection, std::string line);
static void on_connect(Connection *connection, bool status);

int main() {
	string msg;
	
	Protocol::Server serv("1234");

	serv.slot_message = sigc::ptr_fun(on_message);
	serv.slot_connected = sigc::ptr_fun(on_connect);
	serv.listen();
//	sleep(1);
	
	Protocol::Client client("127.0.0.1","1234","");
	client.connect();
	sleep(1);
	
	msg = "word1 :word2 word3";
	fprintf(stderr, "client.write(\"%s\");\n", msg.c_str());
	client.write(msg);

	msg = "word1 word2 word3";
	fprintf(stderr, "client.write(\"%s\");\n", msg.c_str());
	client.write(msg);
	
	while (1)
		sleep(1);

	return 0;
}

static void on_connect(Connection *connection, bool status) {
	fprintf(stderr, "serv:on_connected: %d\n", status);
}

static void on_message(Connection *connection, string line) {
	fprintf(stderr, "serv:on_message: %s\n", line.c_str());
	string word;
	while ((word = popword(line)).length())
		fprintf(stderr, "serv:on_message: %s\n", word.c_str());
}


