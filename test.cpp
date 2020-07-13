#include <iostream>

#include "include/libmatrix-client/MatrixSession.h"

using LibMatrix::MatrixSession;

int main(int argc, const char **argv) {
	MatrixSession client{argv[1]};

	if(!client.login(argv[2], argv[3])) {
		std::cout << "Could not login :(" << std::endl;
	} else {
		std::cout << "Logged in :)" << std::endl;
	}

	return 0;
}
