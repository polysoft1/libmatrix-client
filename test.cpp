#include <iostream>

#include "include/libmatrix-client/MatrixSession.h"

using LibMatrix::MatrixSession;

int main(int argc, const char **argv) {
	MatrixSession client{argv[1]};

	try {
		client.login(argv[2], argv[3]).get();
	}catch(std::exception e) {
		std::cout << "Could not login :(" << std::endl;
		std::cout << e.what() << std::endl;
		return 1;
	}
	std::cout << "Logged in :)" << std::endl;

//	std::cout << client.getRooms().dump(4) << std::endl;
//	client.sendMessage(argv[4], argv[5]);
	return 0;
}
