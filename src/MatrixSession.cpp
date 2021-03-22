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

MatrixSession::~MatrixSession() {
	clearEncryptAccount();
}

void MatrixSession::postLoginSetup() {
	std::string sig;
	try{
		setupEncryptAccount();
		genIdKeys();
	}catch(Exceptions::OLMException e) {
		clearEncryptAccount();
		throw e;		
	}
	json request = {
		{"device_keys", {
			{"algorithms", MatrixSession::encryptAlgos},
			{"device_id", deviceID},
			{"keys", json::parse(idKeys)},
			{"user_id", userId},
		}}
	};

	try{
		sig = signMessage(request.dump());
	}catch(Exceptions::OLMException e) {
		clearEncryptAccount();
		throw e;
	}

	request["device_keys"]["signatures"] = {
		{userId, {
			{"ed25518:" + deviceID, sig}}
		}
	};
	
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

std::future<void> MatrixSession::sendMessage(std::string roomID, std::string message) {
	auto threadResult = std::make_shared<std::promise<void>>();

	if(accessToken.empty()) {
		threadResult->set_exception(
			std::make_exception_ptr(
				std::runtime_error("You need to login first!")));
	} else {
		//TODO(kdvalin) Update transaction IDs to be unique
		std::string url = fmt::format(MatrixURLs::SEND_MESSAGE_FORMAT, roomID, "m" + std::to_string(nextTransactionID++));
		json body{
			{"msgtype", "m.text"},
			{"body", message}
		};
		httpCall(url, HTTPMethod::PUT, body, [threadResult](Response result) {
			json body = json::parse(result.data);

			switch(result.status) {
			case HTTPStatus::HTTP_OK:
				threadResult->set_value();
				break;
			default:
				std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
				threadResult->set_exception(
					std::make_exception_ptr(std::runtime_error("Boo")));
			}
		}, createErrorCallback<void>(threadResult));
	}

	return threadResult->get_future();
}

std::future<RoomMap> MatrixSession::syncState(nlohmann::json filter, int timeout) {
	auto threadedResult = std::make_shared<std::promise<RoomMap>>();

	if(accessToken.empty()) {
		threadedResult->set_exception(
			std::make_exception_ptr(std::runtime_error("You need to log in first!")));
	} else {
		std::string urlParams = "?timeout={:d}";

		if(!syncToken.empty()) {
			urlParams += "&since=" + syncToken;
		}
		std::string url = fmt::format(MatrixURLs::SYNC + urlParams, timeout);
		httpCall(url, HTTPMethod::GET, {}, [this, threadedResult](Response resp) {
			RoomMap output;
			json body = json::parse(resp.data);

			switch(resp.status) {
			default:
				threadedResult->set_exception(
					std::make_exception_ptr(std::runtime_error("Error fetching state")));
				std::cerr << static_cast<int>(resp.status) << std::endl;
				break;
			case HTTPStatus::HTTP_OK:
				syncToken = body["next_batch"];
				json rooms = body["rooms"]["join"];

				for(auto i = rooms.begin(); i != rooms.end(); ++i) {
					std::string roomId = i.key();
					std::vector<Message> messages;

					std::string name = findRoomName(rooms[roomId]["state"]["events"]);
					bool encrypted = isRoomEncrypted(rooms[roomId]["timeline"]["events"]);
					if(!encrypted) {
						parseUnencryptedMessages(messages, rooms[roomId]["timeline"]["events"]);
					} else {

					}
					std::shared_ptr<Room> room = std::make_shared<Room>(roomId, name, messages, encrypted,
						rooms[roomId]["timeline"]["prev_batch"].get<std::string>(),
						"");
					output[roomId] = room;
				}
				threadedResult->set_value(output);
				break;
			}
		}, createErrorCallback<RoomMap>(threadedResult));
	}

	return threadedResult->get_future();
}

std::future<void> MatrixSession::updateReadReceipt(std::string roomID, LibMatrix::Message message) {
	auto threadedResult = std::make_shared<std::promise<void>>();

	if(accessToken.empty()) {
		threadedResult->set_exception(
			std::make_exception_ptr(std::runtime_error("You need to log in first!")));
	} else if (roomID.empty()) {
		threadedResult->set_value();
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

	return threadedResult->get_future();
}

std::future<std::unordered_map<std::string, User>> MatrixSession::getRoomMembers(std::string roomID) {
	auto threadedResult = std::make_shared<std::promise<std::unordered_map<std::string, User>>>();

	if (accessToken.empty()) {
		threadedResult->set_exception(
			std::make_exception_ptr(std::runtime_error("You need to log in first!")));
	} else if (roomID.empty()) {
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

	return threadedResult->get_future();
}

template <class T>
ErrorCallback MatrixSession::createErrorCallback(std::shared_ptr<std::promise<T>> promiseThread) {
	return ([promiseThread](std::string reason) {
		promiseThread->set_exception(
			std::make_exception_ptr(
				std::runtime_error(reason)
			)
		);
	});
}

void MatrixSession::httpCall(std::string url, HTTPMethod method, json data, ResponseCallback callback, ErrorCallback errCallback) {
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
