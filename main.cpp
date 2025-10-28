#include "Server.hpp"
#include <cstdlib>

int main(int argc, char **argv) {
	if (argc == 3) {
		int port = atoi(argv[1]);
		if ((1 > port) || (port > 65535)) {
			std::cerr << "Error: wrong port\n";
			return (1);
		}
		Server server(port, std::string(argv[2]));
		try {
			server.startServer();
		}
		catch(const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
			return (1);
		}
	}
	else {
		std::cerr << "Error: wrong number of arguments\n";
		return (1);
	}
	return (0);
}
