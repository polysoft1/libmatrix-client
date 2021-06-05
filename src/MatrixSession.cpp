#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <future>
#include <unordered_map>
#include <ctime>

#include <nlohmann/json.hpp>
#include <fmt/core.h>  // Ignore CPPLintBear

#include "MatrixSession.h"
#include "HTTP.h"
#include "HTTPClient.h"
#include "Messages.h"
#include "MessageUtils.h"
#include "UserUtils.h"
#include "Exceptions.h"

#include "Encryption/EncryptionUtils.h"

using namespace LibMatrix; // Ignore CPPLintBear

using json = nlohmann::json;

const std::vector<std::string> LibMatrix::MatrixSession::encryptAlgos({"m.olm.v1.curve25519-aes-sha2", 
	"m.megolm.v1.aes-sha2"});

void MatrixSession::setHTTPCaller() {
	http = std::make_unique<HTTPClient>(homeserverURL);
}

MatrixSession::MatrixSession() : homeserverURL(""), syncToken("") {
	setHTTPCaller();
}

MatrixSession::MatrixSession(std::string url) : homeserverURL(url), syncToken("") {
	setHTTPCaller();
}

MatrixSession::~MatrixSession() {}

void MatrixSession::postLoginSetup() {
	try{
		e2eAccount = std::make_unique<Encryption::Account>();
		e2eAccount->generateIdKeys();
	}catch(Exceptions::OLMException e) {
		e2eAccount.reset(nullptr);
		throw e;
	}

	json keys;
	for(Encryption::IdentityKey i : e2eAccount->getIdKeys()) {
		keys[i.getAlgo() + ":" + deviceID] = i.getValue();
	}

	json request = {
		{"device_keys", {
			{"algorithms", MatrixSession::encryptAlgos},
			{"device_id", deviceID},
			{"keys", keys},
			{"user_id", userId},
		}}
	};

	try{
		signJSONPayload(request);
	}catch(Exceptions::OLMException e) {
		e2eAccount.reset(nullptr);
		throw e;
	}
	
	auto thing = std::make_shared<std::promise<void>>();
	httpCall(MatrixURLs::E2E_UPLOAD_KEYS, HTTPMethod::POST, request, [this, thing](Response result) {
		json body = json::parse(result.data);

		switch(result.status) {
			case HTTPStatus::HTTP_OK:
				std::cout << body.dump(4) << std::endl;
				thing->set_value();
				break;
			default:
				std::cerr << "Status: " << static_cast<int>(result.status) << std::endl;
				std::cerr << "Body: " << body.dump(4) << std::endl;
				thing->set_exception(
					std::make_exception_ptr(std::runtime_error("got invalid status code"))
				);
				break;
		}
	}, createErrorCallback<void>(thing));

	thing.get();
	publishOneTimeKeys(e2eAccount->generateOneTimeKeys()).get();
}

std::future<void> MatrixSession::login(std::string uname, std::string password) {
	auto threadResult = std::make_shared<std::promise<void>>();

	json body{
		{"type", MatrixSession::LOGIN_TYPE},
		{"password", password},
		//TODO(kdvalin) Make this configurable
		{"initial_device_display_name", "Testing LibMatrix123"},
		{"identifier",
			{
				{"type", MatrixSession::USER_TYPE},
				{"user", uname}  // Ignore CPPLintBear
			}
		}
	};

	httpCall(MatrixURLs::LOGIN, HTTPMethod::POST, body, [this, threadResult](Response result) {
		json body = json::parse(result.data);

		switch(result.status) {
		case HTTPStatus::HTTP_OK:
			accessToken = body["access_token"].get<std::string>();
			deviceID = body["device_id"].get<std::string>();
			userId = body["user_id"].get<std::string>();

			try {
				this->postLoginSetup();
			}catch (Exceptions::OLMException e) {
				threadResult->set_exception(std::make_exception_ptr(e));
			}
			threadResult->set_value();
			break;
		default:
			std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
			threadResult->set_exception(
				std::make_exception_ptr(std::runtime_error("Hi")));
		}
	}, createErrorCallback<void>(threadResult));

	return threadResult->get_future();
}

std::future<const RoomMap&> MatrixSession::syncState(nlohmann::json filter, int timeout) { //TODO change signature to reference
	auto threadedResult = std::make_shared<std::promise<const RoomMap&>>();

	if(verifyAuth(threadedResult)) {
		std::string urlParams = "?timeout={:d}";

		if(!syncToken.empty()) {
			urlParams += "&since=" + syncToken;
		}
		std::string url = fmt::format(MatrixURLs::SYNC + urlParams, timeout);
		httpCall(url, HTTPMethod::GET, {}, [this, threadedResult](Response resp) {
			json body = json::parse(resp.data);

			switch(resp.status) {
			default:
				threadedResult->set_exception(
					std::make_exception_ptr(std::runtime_error("Error fetching state")));
				std::cerr << static_cast<int>(resp.status) << std::endl;
				break;
			case HTTPStatus::HTTP_OK:
				std::cout << body << std::endl;
				syncToken = body["next_batch"];
				json rooms = body["rooms"]["join"];

				for(auto i = rooms.begin(); i != rooms.end(); ++i) {
					std::string roomId = i.key();
					std::vector<Message> messages;
					
					//Newly found room
					if(roomMap.find(roomId) == roomMap.end()) {
						std::string name = findRoomName(rooms[roomId]["state"]["events"]);
						bool encrypted = isRoomEncrypted(rooms[roomId]["timeline"]["events"]);
						std::shared_ptr<Room> room = std::make_shared<Room>(*this, roomId, name, messages, encrypted,
							rooms[roomId]["timeline"]["prev_batch"].get<std::string>(),
							"");
						roomMap[roomId] = room;

						if(room->isEncrypted()) {
							room->requestRoomKeys();
						}
					}

					if(!roomMap[roomId]->isEncrypted()) {
						parseUnencryptedMessages(messages, rooms[roomId]["timeline"]["events"]);
					}//TODO once keys are stored, decrypt encrypted messages as well (if key for room exists)
					roomMap[roomId]->appendMessages(messages);
				}
				threadedResult->set_value(roomMap);
				break;
			}
		}, createErrorCallback<const RoomMap&>(threadedResult));
	}

	return threadedResult->get_future();
}

std::future<void> MatrixSession::updateReadReceipt(std::string roomID, LibMatrix::Message message) {
	auto threadedResult = std::make_shared<std::promise<void>>();

	if(verifyAuth(threadedResult)) {
		if (roomID.empty()) {
			threadedResult->set_exception(
				std::make_exception_ptr(std::runtime_error("Invalid roomId parameter")));
		} else {
			json body = {
				{"m.fully_read", message.id},
				{"m.read", message.id}
			};

			std::string url = fmt::format(MatrixURLs::READ_MARKER_FORMAT, roomID);
			httpCall(url, HTTPMethod::POST, body, [this, threadedResult](Response resp) {
				switch(resp.status) {
					default:
						threadedResult->set_exception(
							std::make_exception_ptr(std::runtime_error("Error fetching state")));
						std::cerr << static_cast<int>(resp.status) << std::endl;
						break;
					case HTTPStatus::HTTP_OK:
						threadedResult->set_value();
						break;
				}
			}, createErrorCallback<void>(threadedResult));
		}
	}

	return threadedResult->get_future();
}

std::future<std::unordered_map<std::string, User>> MatrixSession::getRoomMembers(std::string roomID) {
	auto threadedResult = std::make_shared<std::promise<std::unordered_map<std::string, User>>>();

	if (verifyAuth(threadedResult)) {
		if (roomID.empty()) {
			threadedResult->set_exception(
				std::make_exception_ptr(std::runtime_error("Invalid room ID sent!")));
		} else {
			std::string url = fmt::format(MatrixURLs::GET_ROOM_MEMBERS_FORMAT, roomID);
			httpCall(url, HTTPMethod::GET, {}, [this, threadedResult](Response resp) {
				json body = json::parse(resp.data);
				switch(resp.status) {
					default:
						threadedResult->set_exception(
							std::make_exception_ptr(std::runtime_error("Error fetching state")));
						std::cerr << static_cast<int>(resp.status) << std::endl;
						break;
					case HTTPStatus::HTTP_OK:
						json members = body["joined"];
						std::unordered_map<std::string, User> output;

						for(auto i = members.begin(); i != members.end(); ++i) {
							User member = parseUser(homeserverURL, i.key(), *i);
							output[member.id] = member;
						}
						this->getUserDevices(output).get();
						threadedResult->set_value(output);
						break;
				}
			}, createErrorCallback<std::unordered_map<std::string, User>>(threadedResult));
		}
	}

	return threadedResult->get_future();
}

template <class T>
ErrorCallback MatrixSession::createErrorCallback(std::shared_ptr<std::promise<T>> promiseThread) {
	return ([promiseThread](std::string reason) {
		std::cout << "Errored out, reason: " << reason << std::endl;
		promiseThread->set_exception(
			std::make_exception_ptr(
				std::runtime_error(reason)
			)
		);
	});
}

void MatrixSession::httpCall(std::string url, HTTPMethod method, const json &data, ResponseCallback callback, ErrorCallback errCallback) {
	auto reqData = std::make_shared<HTTPRequestData>(method, url);
	reqData->setBody(data.dump());
	Headers headers;
	headers["Content-Type"] = "application/json";
	headers["Accept"] = "application/json";
	headers["Authorization"] = "Bearer " + accessToken;

	reqData->setHeaders(std::make_shared<Headers>(headers));

	reqData->setResponseCallback(callback);

	reqData->setErrorCallback(errCallback);
	http->request(reqData);
}

std::future<void> MatrixSession::getUserDevices(std::unordered_map<std::string, User> users, int timeout) {
	json request = {
		{"timeout", timeout},
		{"token", nextTransactionID}
	};
	std::unordered_map<std::string, std::vector<std::string>> deviceKeyMap;

	for(auto it = users.begin(); it != users.end(); ++it) {
		deviceKeyMap[it->first] = std::vector<std::string>();
	}
	request["device_keys"] = deviceKeyMap;

	auto thing = std::make_shared<std::promise<void>>();
	if(verifyAuth(thing)) {
		httpCall(MatrixURLs::DEVICE_KEY_QUERY, HTTPMethod::POST, request, [this, thing, users](Response result) {
			json body = json::parse(result.data);
			json deviceKeys = body["device_keys"];

			switch(result.status) {
				case HTTPStatus::HTTP_OK:
					for(auto it = deviceKeys.begin(); it != deviceKeys.end(); ++it) {
						std::string uid = it.key();
						User current = users.at(uid);
						json userDevices = (*it)[uid];
						for(auto deviceIt = userDevices.begin(); deviceIt != userDevices.end(); ++deviceIt) {
							std::shared_ptr<Device> deviceObj = std::make_shared<Device>();
							json currentDevice = *deviceIt;
							deviceObj->id = deviceIt.key();
							deviceObj->userId = currentDevice["user_id"].get<std::string>();
							deviceObj->encryptAlgos = currentDevice["algorithms"].get<std::vector<std::string>>();
							deviceObj->idKeys = currentDevice["keys"].get<std::unordered_map<std::string, std::string>>();
							deviceObj->signatures = currentDevice["signatures"][uid].get<std::unordered_map<std::string, std::string>>();
							
							current.devices[deviceObj->id] = deviceObj;
						}
					}
					thing->set_value();
					break;
				default:
					std::cerr << "Status: " << static_cast<int>(result.status) << std::endl;
					std::cerr << "Body: " << body.dump(4) << std::endl;
					thing->set_exception(
						std::make_exception_ptr(std::runtime_error("got invalid status code"))
					);
					break;
			}
		}, createErrorCallback(thing));
	}

	return thing->get_future();
}

std::future<void> MatrixSession::publishOneTimeKeys(json keys) {
	auto promise = std::make_shared<std::promise<void>>();
	if(verifyAuth(promise)) {
		signJSONPayload(keys);
		httpCall(MatrixURLs::E2E_UPLOAD_KEYS, HTTPMethod::POST, keys, [this, promise](Response resp) {
			switch(resp.status) {
				case HTTPStatus::HTTP_OK:
					e2eAccount->publishOneTimeKeys();
					promise->set_value();
					break;
				default:
					promise->set_exception(
						std::make_exception_ptr(
							std::runtime_error("Error posting one time keys")
						)
					);
			}
		}, createErrorCallback<void>(promise));
	}

	return promise->get_future();
}

void MatrixSession::signJSONPayload(json &payload) {
	if(e2eAccount.get() == nullptr) {
		return;
	}

	std::string signature = e2eAccount->sign(payload.dump());
	payload["device_keys"]["signatures"] = {
		{userId, {
			{"ed25518:" + deviceID, signature}}
		}
	};
}

std::future<void> MatrixSession::requestEncryptionKey(std::string roomId, std::string olmSessionId) {
	auto threadResult = std::make_shared<std::promise<void>>();

	//TODO (kdvalin) more unique transaction IDs
	uint8_t num;
	seedArray(&num, 1);
	std::string txnId = std::string("test") + std::to_string(num);

	json req;
	req["messages"] = {
		{userId, {
			{"*", {
				{"action", "request"},
				{"requesting_device_id", deviceID},
				{"request_id", txnId},
				{"body", {
					{"roomId", roomId},
					{"algorithm", "m.megolm.v1.aes-sha2"},
					{"sender_key", e2eAccount->getIdKeys()[0].getValue()},
					{"session_id", olmSessionId}
				}}
			}}
		}}
	};

	std::cout << req.dump(4) << std::endl;
	if(verifyAuth(threadResult)) {
		httpCall(fmt::format(MatrixURLs::TO_DEVICE_FORMAT, "m.room_key_request", txnId), 
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
		}, createErrorCallback<void>(threadResult));
	}

	return threadResult->get_future();
}

std::future<void> MatrixSession::sendMessageRequest(std::string roomId, const nlohmann::json &payload) {
	auto threadResult = std::make_shared<std::promise<void>>();
	if(verifyAuth(threadResult)) {
		//TODO(kdvalin) Update transaction IDs to be unique
		std::string url = fmt::format(MatrixURLs::SEND_MESSAGE_FORMAT, roomId, "m" + std::to_string(nextTransactionID++));
		httpCall(url, HTTPMethod::PUT, payload, [threadResult, payload](Response result) {
				json body = json::parse(result.data);

				switch(result.status) {
				case HTTPStatus::HTTP_OK:
					threadResult->set_value();
					break;
				default:
					std::cout << payload << std::endl;
					std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
					threadResult->set_exception(
						std::make_exception_ptr(std::runtime_error("Boo")));
				}
			}, createErrorCallback<void>(threadResult));
	}
	return threadResult->get_future();
}

template<class T>
bool MatrixSession::verifyAuth(std::shared_ptr<std::promise<T>> result) {
	bool isEmpty = accessToken.empty();
	if(isEmpty) {
		result->set_exception(
			std::make_exception_ptr(
				std::runtime_error("You need to login first!")));
	}

	return !isEmpty;
}