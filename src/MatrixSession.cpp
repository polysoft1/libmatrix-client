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

// TODO make non-blocking
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


void MatrixSession::httpCall(std::string url, HTTPMethod method, const json &data, ResponseCallback callback, ErrorCallback errCallback) {
	auto reqData = std::make_shared<HTTPRequestData>(method, url);
	if (!data.empty()) {
		reqData->setBody(data.dump());
	}
	Headers headers;
	headers["Content-Type"] = "application/json";
	headers["Accept"] = "application/json";
	headers["Authorization"] = "Bearer " + accessToken;

	reqData->setHeaders(std::make_shared<Headers>(headers));

	reqData->setResponseCallback(callback);

	reqData->setErrorCallback(errCallback);
	http->request(reqData);
}

std::future<void> MatrixSession::getUserDevices(std::unordered_map<std::string, std::shared_ptr<User>> users, int timeout) {
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
						// TODO: Verify the signatures with olm_ed25519_verify
						// Then check to make sure that the user_id and device_id fields
						// match those in the top-level map.
						// Then check to make sure the device and user correspond to any
						// previously seen devices.
						std::string uid = it.key();
						std::shared_ptr<User> current = users.at(uid);
						json userDevices = (*it)[uid];
						for(auto deviceIt = userDevices.begin(); deviceIt != userDevices.end(); ++deviceIt) {
							std::shared_ptr<Device> deviceObj = std::make_shared<Device>();
							json currentDevice = *deviceIt;
							deviceObj->id = deviceIt.key();
							deviceObj->userId = currentDevice["user_id"].get<std::string>();
							deviceObj->encryptAlgos = currentDevice["algorithms"].get<std::vector<std::string>>();
							deviceObj->idKeys = currentDevice["keys"].get<std::unordered_map<std::string, std::string>>();
							deviceObj->signatures = currentDevice["signatures"][uid].get<std::unordered_map<std::string, std::string>>();
							
							current->devices[deviceObj->id] = deviceObj;
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

std::future<void> MatrixSession::updateOlmSessions(std::vector<std::shared_ptr<User>> users)
{
	// https://matrix.org/docs/spec/client_server/r0.4.0#post-matrix-client-r0-keys-claim
	auto promise = std::make_shared<std::promise<void>>();
	if (verifyAuth(promise)) {
		int keysRequested = 0;
		json request = {
			{"timeout", 10000},
		};
		for (std::shared_ptr<User> user : users) {
			for (std::pair<std::string, std::shared_ptr<Device>> device : user->devices) {
				// Request one time key if there is no OlmSession
				if (!device.second->currentSession) {
					request[user->id][device.first] = "signed_curve25519";
					keysRequested++;
				}
			}
		}
		if (keysRequested == 0) {
			promise->set_value();
		} else {

			httpCall(MatrixURLs::CLAIM_KEY_QUERY, HTTPMethod::POST, request, [this, promise, users](Response resp) {

				// TODO: Check the signatures
				// TODO: Account for failures (which can happen)
				// Note, key ID is ignored right now.

				json body = json::parse(resp.data);
				auto keysSection = body["one_time_keys"];
				for (std::shared_ptr<User> user : users) {
					auto userSection = keysSection[user->id];
					for (std::pair<std::string, std::shared_ptr<Device>> device : user->devices) {

						// TODO: Account for discrepancies due to a change in the device list
						// during the duration of this call.
						for (auto& [keyTypeAndName, keySection] : userSection[device.first].items()) {
							if (!device.second->currentSession) {
								device.second->currentSession = std::make_shared<OlmSession>();
								std::string oneTimeKey = keySection["key"];
								std::string curve25519IdentityKey = device.second->idKeys["curve25519:" + device.first];

								size_t randLen = olm_create_outbound_session_random_length(device.second->currentSession.get());
								uint8_t* buffer = new uint8_t[randLen];
								seedArray(buffer, randLen);

								olm_create_outbound_session(device.second->currentSession.get(), e2eAccount->account, curve25519IdentityKey.data(),
									curve25519IdentityKey.length(), oneTimeKey.data(), oneTimeKey.length(), buffer, randLen);
							}
						}
					}
				}

				promise->set_value();

			}, createErrorCallback<void>(promise));
		}
	}
	return promise->get_future();
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