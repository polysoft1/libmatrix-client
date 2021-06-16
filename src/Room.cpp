#include "Room.h"

#include <fmt/core.h>  // Ignore CPPLintBear
#include <nlohmann/json.hpp>
#include <iostream>

#include "MatrixSession.h"
#include "Encryption/EncryptionUtils.h"

using namespace LibMatrix;
using json = nlohmann::json;

Room::Room(MatrixSession& parentSession, std::string_view id, std::string name,
	const std::vector<Message> &messages, bool encrypted, std::string prevToken,
	std::string nextToken)
	: id(id), name(name), messages(messages), 
	prevToken(prevToken), nextToken(nextToken), encrypted(encrypted),
	parentSession(parentSession)
{
	if(encrypted) {
		encryptionSession = std::make_unique<Encryption::MegOlmSession>();
	}
}

std::future<void> Room::sendMessage(std::string message) {
	return parentSession.sendMessageRequest(id, {
		{"msgtype", "m.text"},
		{"body", message}
	});
}

const std::string Room::getEncryptionSessionId() const {
	if(!encrypted || !encryptionSession) {
		throw "Not an encrypted room";
	}
	return encryptionSession->getSessionId();
}

std::future<void> Room::requestRoomKeys() {
	auto threadResult = std::make_shared<std::promise<void>>();

	//TODO (kdvalin) more unique transaction IDs
	uint8_t num;
	seedArray(&num, 1);
	std::string txnId = std::string("test") + std::to_string(num);
	std::cerr << "DEBUG: " << encryptionSession->getSessionId() << std::endl;
	json req;
	req["messages"] = {
		{parentSession.getUserId(), {
			{"*", {
				{"action", "request"},
				{"requesting_device_id", parentSession.getDeviceId()},
				{"request_id", txnId},
				{"body", {
					{"roomId", id},
					{"algorithm", "m.megolm.v1.aes-sha2"},
					{"sender_key", parentSession.getSenderKey() },
					{"session_id", encryptionSession->getSessionId() }
				}}
			}}
		}}
	};

	if(parentSession.verifyAuth(threadResult)) {
		parentSession.httpCall(fmt::format(MatrixURLs::TO_DEVICE_FORMAT, "m.room_key_request", txnId), 
			HTTPMethod::PUT, req,
			[threadResult](Response resp) {
				switch(resp.status) {
				case HTTPStatus::HTTP_OK:
					threadResult->set_value();
					break;
				default:
					std::cerr << static_cast<int>(resp.status) << ": " << resp.data << std::endl;
					threadResult->set_exception(
						std::make_exception_ptr(
							std::runtime_error("Invalid request sent")
						)
					);
				}
			}, parentSession.createErrorCallback<void>(threadResult)
		);
	}

	return threadResult->get_future();
}

void Room::appendMessages(std::vector<Message> newMsgs) {
	messages.insert(messages.end(), newMsgs.begin(), newMsgs.end());
}