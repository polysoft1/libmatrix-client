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

#include "DLL.h"

namespace LibMatrix {

class MatrixSession;
class User;

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

	const std::string LIBMATRIX_DLL_EXPORT getEncryptionSessionId() const;
	
	void LIBMATRIX_DLL_EXPORT appendMessages(std::vector<Message> msg); //Used on server sync
	std::future<void> LIBMATRIX_DLL_EXPORT sendMessage(std::string msg); //Used by client to send message, needs fleshing out

	std::future<void> LIBMATRIX_DLL_EXPORT requestRoomKeys();
	void LIBMATRIX_DLL_EXPORT setRoomEncryptionKey(std::string key);

	std::future<std::unordered_map<std::string, User>> LIBMATRIX_DLL_EXPORT requestRoomMembers();

	/**
	 * Shares the room keys with all trusted clients in the room.
	 * Needed when this client is the one starting a new MegOlm session.
	 */
	void LIBMATRIX_DLL_EXPORT shareRoomKeys();
};

} // namespace LibMatrix

#endif
