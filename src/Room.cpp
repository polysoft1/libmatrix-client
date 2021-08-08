#include "Room.h"

#include <fmt/core.h>  // Ignore CPPLintBear
#include <nlohmann/json.hpp>
#include <iostream>

#include "MatrixSession.h"
#include "User.h"
#include "UserUtils.h"
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

	uint8_t num;
	seedArray(&num, 1);
	std::string txnId = parentSession.getNextTransactionID();
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


std::future<std::unordered_map<std::string, User>> Room::requestRoomMembers() {
	auto threadedResult = std::make_shared<std::promise<std::unordered_map<std::string, User>>>();

	if (parentSession.verifyAuth(threadedResult)) {
		if (id.empty()) {
			threadedResult->set_exception(
				std::make_exception_ptr(std::runtime_error("Invalid room ID sent!")));
		} else {
			std::string url = fmt::format(MatrixURLs::GET_ROOM_MEMBERS_FORMAT, id);
			parentSession.httpCall(url, HTTPMethod::GET, {}, [this, threadedResult](Response resp) {
				json body = json::parse(resp.data);
				switch (resp.status) {
				default:
					threadedResult->set_exception(
						std::make_exception_ptr(std::runtime_error("Error fetching state")));
					std::cerr << static_cast<int>(resp.status) << std::endl;
					break;
				case HTTPStatus::HTTP_OK:
					json members = body["joined"];
					std::unordered_map<std::string, User> output;

					for (auto i = members.begin(); i != members.end(); ++i) {
						User member = parseUser(parentSession.getHomeserverURL(), i.key(), *i);
						output[member.id] = member;
					}
					parentSession.getUserDevices(output).get();
					threadedResult->set_value(output);
					break;
				}
				}, parentSession.createErrorCallback<std::unordered_map<std::string, User>>(threadedResult));
		}
	}

	return threadedResult->get_future();
}


void Room::appendMessages(std::vector<Message> newMsgs) {
	messages.insert(messages.end(), newMsgs.begin(), newMsgs.end());
}