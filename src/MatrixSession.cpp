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
	auto data = std::make_shared<HTTPRequestData>(HTTPMethod::POST, MatrixURLs::DEVICE_KEY_QUERY);
	data->setBody(request.dump());
	Headers headers;

	headers["Content-Type"] = "application/json";
	headers["Accept"] = "application/json";
	headers["Authorization"] = "Bearer " + accessToken;

	data->setHeaders(std::make_shared<Headers>(headers));

	data->setResponseCallback([this, thing, users](Response result) {
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
	});

	data->setErrorCallback([thing](std::string reason) {
		thing->set_exception(
			std::make_exception_ptr(std::runtime_error(reason))
		);
	});
	http->request(data);
	return thing->get_future();
}

std::future<void> MatrixSession::publishOneTimeKeys(json keys) {
	auto promise = std::make_shared<std::promise<void>>();
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