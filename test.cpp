#include <iostream>

#include "Users.h"
#include "include/libmatrix-client/MatrixSession.h"
#include "include/libmatrix-client/Messages.h"
#include "MessageUtils.h"

using LibMatrix::MatrixSession;

int main(int argc, const char **argv) {
	MatrixSession client{argv[1]};
	try {
		auto future = client.login(argv[2], argv[3]);
		if(future.valid()) {
			std::cout << "Moving along!" << std::endl;
			future.get();
		}
	}catch(std::runtime_error e) {
		std::cout << "Could not login :(" << std::endl;
		std::cout << e.what() << std::endl;
		return 1;
	}
	std::cout << "Logged in :)" << std::endl;

	auto rooms = client.syncState().get();
	for(auto i = rooms.begin(); i != rooms.end(); ++i) {
		std::cout << i->first << "\n";
		//Print users
		auto users = client.getRoomMembers(i->first).get();
		for(LibMatrix::User i : users) {
			std::cout << "\t" << i.id << std::endl;
			std::cout << "\t\t" << i.displayName << std::endl;
			std::cout << "\t\t" << i.avatarURL << std::endl;
		}
		std::cout << "\t" << i->second->name << "\n";
		for(LibMatrix::Message msg : i->second->messages) {
			std::cout << "\t" << "\t" << msg.id << "\n";
			std::cout << "\t" << "\t" << msg.content << "\n";
			std::cout << "\t" << "\t" << msg.sender << "\n";
		}
	}

	while(true) {
		auto patch = client.syncState().get();
		for(auto i = patch.begin(); i != patch.end(); ++i) {
			std::cout << i->first << "\n";

			std::cout << "\t" << i->second->name << "\n";
			for(LibMatrix::Message msg : i->second->messages) {
				std::cout << "\t" << "\t" << msg.id << "\n";
				std::cout << "\t" << "\t" << msg.content << "\n";
				std::cout << "\t" << "\t" << msg.sender << "\n";
				client.updateReadReceipt(i->second->id, *(i->second->messages.rbegin()) ).get();
			}
		}
	}

/*	LibMatrix::MessageBatchMap result = client.syncState().get();

	for(auto it = result.begin(); it != result.end(); ++it) {
		std::cout << *(it->second) << std::endl;
	}*/

//	client.sendMessage(argv[4], argv[5]).get();
	return 0;
}
