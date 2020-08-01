#include <iostream>

#include "include/libmatrix-client/MatrixSession.h"

using LibMatrix::MatrixSession;

int main(int argc, const char **argv) {
	MatrixSession client{argv[1]};

	if(!client.login(argv[2], argv[3])) {
	//	std::cout << "Could not login :(" << std::endl;
		return 1;
	}/* else {
		std::cout << "Logged in :)" << std::endl;
	}*/
	//std::cout << client.getRooms().dump(4) << std::endl;
	//client.sendMessage(argv[4], argv[5]);
	nlohmann::json msgs = client.getAllRoomMessages(argv[4]);
	std::cout << msgs.dump(4) << std::endl;
	//std::cout << client.getMessages(msgs["next_batch"].get<std::string>()) << std::endl;
	return 0;
}
