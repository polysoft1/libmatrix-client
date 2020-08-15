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

using namespace LibMatrix; // Ignore CPPLintBear

using json = nlohmann::json;

void MatrixSession::setHTTPCaller() {
	http = std::make_unique<HTTPClient>(homeserverURL);
}

MatrixSession::MatrixSession() : homeserverURL("") {
	setHTTPCaller();
}

MatrixSession::MatrixSession(std::string url) : homeserverURL(url) {
	setHTTPCaller();
}

std::future<void> MatrixSession::login(std::string uname, std::string password) {
	auto threadResult = std::make_shared<std::promise<void>>();

	json body{
		{"type", MatrixSession::LOGIN_TYPE},
		{"password", password},
		//TODO(kdvalin) Make this configurable
		{"initial_device_display_name", "Testing LibMatrix"},
		{"identifier",
			{
				{"type", MatrixSession::USER_TYPE},
				{"user", uname}  // Ignore CPPLintBear
			}
		}
	};
	Headers headers;

	headers["Content-Type"] = "application/json";
	headers["Accept"] = "application/json";

	auto data = std::make_shared<HTTPRequestData>(HTTPMethod::POST, MatrixURLs::LOGIN);
	data->setBody(body.dump());
	data->setHeaders(std::make_shared<Headers>(headers));

	data->setResponseCallback([this, threadResult](Response result) {
		std::cout << "Made it into the sucessful callback" << std::endl;
		json body = json::parse(result.data);

		switch(result.status) {
		case HTTPStatus::HTTP_OK:
			accessToken = body["access_token"].get<std::string>();
			deviceID = body["device_id"].get<std::string>();
			threadResult->set_value();
			break;
		default:
			std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
			threadResult->set_exception(
				std::make_exception_ptr(std::runtime_error("Hi")));
		}
	});
	data->setErrorCallback([threadResult](std::string reason) {
		std::cout << "Made it into the sucessful callback" << std::endl;
		threadResult->set_exception(
			std::make_exception_ptr(
				std::runtime_error(reason)));
	});

	http->request(data);
	return threadResult->get_future();
}

std::future<json> MatrixSession::getRooms() {
	auto threadResult = std::make_shared<std::promise<json>>();

	if(accessToken.empty()) {
		threadResult->set_exception(
			std::make_exception_ptr(
				std::runtime_error("You need to login first!")));
	} else {
		Headers reqHeaders;
		reqHeaders["Authorization"] = "Bearer " + accessToken;

		auto data = std::make_shared<HTTPRequestData>(HTTPMethod::GET, MatrixURLs::GET_ROOMS);
		data->setHeaders(std::make_shared<Headers>(reqHeaders));

		data->setResponseCallback([threadResult](Response result) {
			json body = json::parse(result.data);

			switch(result.status) {
			case HTTPStatus::HTTP_OK:
				threadResult->set_value(body);
				break;
			default:
				std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
				threadResult->set_exception(
					std::make_exception_ptr(
						std::runtime_error("Could not retrieve rooms")));
			}
		});
		data->setErrorCallback([threadResult](std::string reason) {
			threadResult->set_exception(
				std::make_exception_ptr(std::runtime_error(reason)));
		});
		http->request(data);
	}

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
		auto data = std::make_shared<HTTPRequestData>(HTTPMethod::PUT,
				fmt::format(MatrixURLs::SEND_MESSAGE_FORMAT, roomID, "m1234557"));
		json body{
			{"msgtype", "m.text"},
			{"body", message}
		};
		Headers reqHeaders;
		reqHeaders["Authorization"] = "Bearer " + accessToken;
		data->setBody(body.dump());
		data->setHeaders(std::make_shared<Headers>(reqHeaders));

		data->setResponseCallback([threadResult](Response result) {
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
		});
		data->setErrorCallback([threadResult](std::string reason) {
			threadResult->set_exception(
				std::make_exception_ptr(
					std::runtime_error(reason)));
		});
		http->request(data);
	}

	return threadResult->get_future();
}

std::future<MessageBatchMap> MatrixSession::syncState(std::string token, nlohmann::json filter, int timeout) {
	auto threadedResult = std::make_shared<std::promise<MessageBatchMap>>();

	if(accessToken.empty()) {
		threadedResult->set_exception(
			std::make_exception_ptr(std::runtime_error("You need to log in first!")));
	} else {
		std::string urlParams = "?timeout={:d}";

		if(!token.empty()) {
			urlParams += "&since=" + token;
		}

		auto data = std::make_shared<HTTPRequestData>(HTTPMethod::GET,
			fmt::format(MatrixURLs::SYNC + urlParams, timeout));

		Headers reqHeaders;
		reqHeaders["Authorization"] = "Bearer " + accessToken;

		data->setHeaders(std::make_shared<Headers>(reqHeaders));

		data->setResponseCallback([threadedResult](Response resp) {
			MessageBatchMap output;
			json body = json::parse(resp.data);

			switch(resp.status) {
			default:
				threadedResult->set_exception(
					std::make_exception_ptr(std::runtime_error("Error fetching state")));
				std::cerr << static_cast<int>(resp.status) << std::endl;
				break;
			case HTTPStatus::HTTP_OK:
				json rooms = body["rooms"]["join"];

				for(auto i = rooms.begin(); i != rooms.end(); ++i) {
					std::string roomId = i.key();
					std::vector<Message> messages;

					json parsedMessages = rooms[roomId]["timeline"]["events"];

					for(auto j = parsedMessages.begin(); j != parsedMessages.end(); ++j) {
							std::string id = (*j)["event_id"].get<std::string>();
							std::string sender = (*j)["sender"].get<std::string>();
							std::time_t ts = (*j)["origin_server_ts"].get<std::time_t>();
							MessageStatus messageType = MessageStatus::RECEIVED;

							std::string content;
							json contentLocation = (*j)["content"];

							if( contentLocation["body"].is_null() ) {
								if(contentLocation["membership"].is_string()) {
									std::string action = contentLocation["membership"].get<std::string>();

									if(action.compare("join") == 0) {
										action = "joined";
									} else if (action.compare("leave") == 0) {
										action = "left";
									}

									content = fmt::format("{:s} {:s} the room", contentLocation["displayname"].get<std::string>(), action);
									messageType = MessageStatus::SYSTEM;
								}
							} else {
								content = contentLocation["body"].get<std::string>();
							}

							messages.push_back(
								Message{id, content, sender, ts, messageType});
					}
					std::shared_ptr<MessageBatch> batch = std::make_shared<MessageBatch>(roomId, messages,
						rooms[roomId]["timeline"]["prev_batch"].get<std::string>(),
						"");
					output[roomId] = batch;
				}
				threadedResult->set_value(output);
				break;
			}
		});
		http->request(data);
	}

	return threadedResult->get_future();
}
/*
std::future<nlohmann::json> MatrixSession::getRoomMessages(std::string roomId, int count, std::string from, char directon) {
	auto threadedResult = std::make_shared<std::promise<nlohmann::json>>();
	if(accessToken.empty()) {
		threadedResult->set_exception(
			std::make_exception_ptr(std::runtime_error("You need to log in first!")));
	} else {
		std::string urlParams = "?count={:d}";

		if(!from.empty()) {
			urlParams += "&from=" + from;
		}
		auto data = std::make_shared<HTTPRequestData>(HTTPMethod::GET,
			fmt::format(MatrixURLs::GET_ROOM_MESSAGE_FORMAT + urlParams, roomId, count));
	}
}
*/
