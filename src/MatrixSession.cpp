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

using namespace LibMatrix; // Ignore CPPLintBear

using json = nlohmann::json;

void MatrixSession::setHTTPCaller() {
	http = std::make_unique<HTTPClient>(homeserverURL);
}

MatrixSession::MatrixSession() : homeserverURL(""), syncToken("") {
	setHTTPCaller();
}

MatrixSession::MatrixSession(std::string url) : homeserverURL(url), syncToken("") {
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

std::future<void> MatrixSession::sendMessage(std::string roomID, std::string message) {
	auto threadResult = std::make_shared<std::promise<void>>();

	if(accessToken.empty()) {
		threadResult->set_exception(
			std::make_exception_ptr(
				std::runtime_error("You need to login first!")));
	} else {
		//TODO(kdvalin) Update transaction IDs to be unique
		auto data = std::make_shared<HTTPRequestData>(HTTPMethod::PUT,
				fmt::format(MatrixURLs::SEND_MESSAGE_FORMAT, roomID, "m" + nextTransactionID++));
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

		auto data = std::make_shared<HTTPRequestData>(HTTPMethod::GET,
			fmt::format(MatrixURLs::SYNC + urlParams, timeout));

		Headers reqHeaders;
		reqHeaders["Authorization"] = "Bearer " + accessToken;

		data->setHeaders(std::make_shared<Headers>(reqHeaders));

		data->setResponseCallback([this, threadedResult](Response resp) {
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
					parseMessages(messages, rooms[roomId]["timeline"]["events"]);
					std::string name = findRoomName(rooms[roomId]["state"]["events"]);

					std::shared_ptr<Room> room = std::make_shared<Room>(roomId, name, messages,
						rooms[roomId]["timeline"]["prev_batch"].get<std::string>(),
						"");
					output[roomId] = room;
				}
				threadedResult->set_value(output);
				break;
			}
		});
		http->request(data);
	}

	return threadedResult->get_future();
}
