#include <iostream>

#include "User.h"
#include "include/libmatrix-client/MatrixSession.h"
#include "include/libmatrix-client/Messages.h"
#include "MessageUtils.h"
#include "Exceptions.h"
#include "Room.h"

using namespace LibMatrix;

int main(int argc, const char **argv) {
	MatrixSession client{argv[1]};
	try {
		auto future = client.login(argv[2], argv[3]);
		
		try {
			future.get();
		} catch(Exceptions::OLMException e){
			switch(e.type()) {
				case Exceptions::OLMError::ACC_CREATE:
					std::cerr << "Error: Could not create encryption session.  Reason: " << e.what() << std::endl;
			}
		} 
	}catch(std::runtime_error e) {
		std::cout << "Could not login :(" << std::endl;
		std::cout << e.what() << std::endl;
		return 1;
	}
	std::cout << "Logged in :)" << std::endl;
	
	try {
		auto rooms = client.syncState().get();
		for (auto i = rooms.begin(); i != rooms.end(); ++i) {
			std::cout << i->first << "\n";
			std::cout << (i->second.get()->isEncrypted() ? "Encrypted session" : "Unencrypted session") << std::endl;
			//Print users
			auto users = i->second->requestRoomMembers().get();
			for (auto it = users.begin(); it != users.end(); ++it) {
				User i = it->second;
				std::cout << "\t" << i.id << std::endl;
				std::cout << "\t\t" << i.displayName << std::endl;
				std::cout << "\t\t" << i.avatarURL << std::endl;
				std::cout << "\t\tDevices: " << std::endl;
				for (auto deviceIt = i.devices.begin(); deviceIt != i.devices.end(); ++deviceIt) {
					std::cout << "\t\t\t" << deviceIt->second.get()->id << std::endl;
				}
			}
			std::cout << "\t" << i->second->getName() << "\n";
			std::cout << "\t\tencrypted: " << (i->second->isEncrypted() ? "true" : "false") << "\n";
			for (LibMatrix::Message msg : i->second->getMessages()) {
				std::cout << "\t\t" << msg.id << "\n";
				std::cout << "\t\t" << msg.content << "\n";
				std::cout << "\t\t" << msg.sender << "\n";
			}
		}
	} catch (std::runtime_error e) {
		std::cout << "Runtime error with sync." << std::endl;
		std::cout << e.what() << std::endl;
		return 1;
	} catch (std::future_error e) {
		std::cout << "Future error with sync." << std::endl;
		std::cout << e.what() << std::endl;
		return 1;
	}

	while(true) {
		try {
			std::cout << "Starting request" << std::endl;
			auto patch = client.syncState().get();
			std::cout << "Got response" << std::endl;
			for(auto i = patch.begin(); i != patch.end(); ++i) {
				std::cout << i->first << "\n";

				std::cout << "\t" << i->second->getName() << "\n";
				for(LibMatrix::Message msg : i->second->getMessages()) {
					std::cout << "\t\t" << msg.id << "\n";
					std::cout << "\t\t" << msg.content << "\n";
					std::cout << "\t\t" << msg.sender << "\n";
				}
			}
		}catch(std::runtime_error e){
			std::cerr << e.what() << std::endl;
		}
		std::cin.ignore();//Wait for a key press
	}

/*	LibMatrix::MessageBatchMap result = client.syncState().get();

	for(auto it = result.begin(); it != result.end(); ++it) {
		std::cout << *(it->second) << std::endl;
	}*/

//	client.sendMessage(argv[4], argv[5]).get();
	return 0;
}
