#ifndef __ROOM_H__
#define __ROOM_H__

#include <string>
#include <string_view> // Ignore CPPLintBear
#include <vector>
#include <unordered_map>
#include <memory>
#include <future>

#include "Messages.h"
#include "Encryption/MegOlmSession.h"

namespace LibMatrix {

class MatrixSession;
class Room {
private:
	std::string id;
	std::string name;
	std::vector<Message> messages;
	bool encrypted;
	
	std::unique_ptr<Encryption::MegOlmSession> encryptionSession;

	std::string prevToken;
	std::string nextToken;
	MatrixSession& parentSession;

public:
	Room(MatrixSession& parentSession, std::string_view id, std::string name,
		const std::vector<Message> &messages, bool encrypted = false, std::string prevToken = "",
		std::string nextToken = "");

	const std::string& getName() const { return name; }
	bool isEncrypted() const { return encrypted; } //TODO allow e2e room upgrading
	const std::vector<Message>& getMessages() const { return messages; }

	const std::string getEncryptionSessionId() const;
	
	void appendMessages(std::vector<Message> msg); //Used on server sync
	std::future<void> sendMessage(std::string msg); //Used by client to send message, needs fleshing out

	std::future<void> requestRoomKeys();
	void setRoomEncryptionKey(std::string key);
};

} // namespace LibMatrix

#endif
