#include <unistd.h>
#include <string>
#include <sigc++/signal.h>
#include "protocol.h"
#include "socket.h"
#include "pthread++.h"

using namespace std;
typedef Protocol::Server::Connection Connection;

void on_client_msg(std::string line);

int main() {
	string msg;
	unsigned char *img;
	int w=25, h=480;
	img = (unsigned char *) malloc(w*h);
	
	int cad = (int) drand48()*15;
	
	for (int i=0; i<w*h; i++)
		img[i] = (i % cad) * 255;
	
	Protocol::Client client("127.0.0.1","1234", "CAM");
	client.connect();
	client.slot_message = sigc::ptr_fun(on_client_msg);
	//sleep(1);
	
	msg = "hello camera";
	fprintf(stderr, "client.write(\"%s\");\n", msg.c_str());
	client.write(msg);
	//sleep(1);

	msg = "CAM IMG ...";
	fprintf(stderr, "client.write(\"%s\");\n", msg.c_str());
	client.write(format("IMG %zu %d %d %d %d %d", w*h, 0, 0, w, h, 1) + "");
	client.write(img, w*h);
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
