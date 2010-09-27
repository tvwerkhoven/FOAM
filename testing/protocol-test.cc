#include <unistd.h>
#include <string>
#include <sigc++/signal.h>
#include "protocol.h"
#include "socket.h"
#include "pthread++.h"

using namespace std;
typedef Protocol::Server::Connection Connection;

void on_message(Connection *connection, std::string line);
void on_connect(Connection *connection, bool status);

static void on_client_msg(std::string line);

int main() {
	string msg;
	
//	Protocol::Server serv1("1234", "SYS");
//	Protocol::Server serv2("1234", "WFS1");

//	serv1.slot_message = sigc::ptr_fun(on_message);
//	serv1.slot_connected = sigc::ptr_fun(on_connect);
//	serv1.listen();

//	serv2.slot_message = sigc::ptr_fun(on_message);
//	serv2.slot_connected = sigc::ptr_fun(on_connect);
//	serv2.listen();
	//	sleep(1);
	
	Protocol::Client client("127.0.0.1","1234");
	client.connect();
	client.slot_message = sigc::ptr_fun(on_client_msg);
	sleep(1);
	
	msg = "SYS hello world";
	fprintf(stderr, "client.write(\"%s\");\n", msg.c_str());
	client.write(msg);

	msg = "WFS1 hello sky!";
	fprintf(stderr, "client.write(\"%s\");\n", msg.c_str());
	client.write(msg);
	
	//while (1)
	
	sleep(2);

	return 0;
}

void on_connect(Connection *connection, bool status) {
	fprintf(stderr, "serv:on_connected: %d\n", status);
}

void on_message(Connection *connection, string line) {
	fprintf(stderr, "%s:on_message: %s\n", connection->server->name.c_str(), line.c_str());
	string word;
	while ((word = popword(line)).length())
		fprintf(stderr, "serv:on_message: %s\n", word.c_str());
	
	fprintf(stderr, "writing\n");
	connection->write("OK, got it");
}

void on_client_msg(std::string line) {
	fprintf(stderr, "cli:on_client_msg: %s\n", line.c_str());
}
