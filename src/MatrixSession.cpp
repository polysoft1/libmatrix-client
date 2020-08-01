
#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

#include <nlohmann/json.hpp>
#include <fmt/core.h>  // Ignore CPPLintBear

#include "MatrixSession.h"
#include "HTTP.h"
#include "HTTPClient.h"

using LibMatrix::HTTPMethod;
using LibMatrix::MatrixSession;
using LibMatrix::HTTPStatus;

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

bool MatrixSession::login(std::string uname, std::string password) {
	bool success, running = true;
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

	HTTPRequestData data(HTTPMethod::POST, MatrixURLs::LOGIN);
	data.setBody(body.dump());
	data.setHeaders(std::make_shared<Headers>(headers));

	data.setResponseCallback([&](Response result) {
		json body = json::parse(result.data);

		switch(result.status) {
		case HTTPStatus::HTTP_OK:
			accessToken = body["access_token"].get<std::string>();
			deviceID = body["device_id"].get<std::string>();
			std::cout << "Successful login!  Device ID is " << deviceID << std::endl;
			success = true;
			break;
		default:
			std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
			success = false;
		}
		running = false;
	});
	data.setErrorCallback([&success, &running](std::string reason) {
		std::cerr << reason << std::endl;
		success = false;
		running = false;
	});
	http->request(std::move(data));

	while(running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return success;
}

json MatrixSession::getRooms() {
	if(accessToken.empty()) {
		throw std::runtime_error("You need to login first!");
	}
	bool running = true;
	json output;

	Headers reqHeaders;
	reqHeaders["Authorization"] = "Bearer " + accessToken;

	HTTPRequestData data(HTTPMethod::GET, MatrixURLs::GET_ROOMS);
	data.setHeaders(std::make_shared<Headers>(reqHeaders));
	data.setResponseCallback([&](Response result) {
		json body = json::parse(result.data);

		switch(result.status) {
		case HTTPStatus::HTTP_OK:
			output = body;
			break;
		default:
			std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
			output = "Error";
		}
		running = false;
	});
	data.setErrorCallback([&running](std::string reason) {
		std::cerr << reason << std::endl;
		running = false;
	});
	http->request(std::move(data));

	while(running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return output;
}

bool MatrixSession::sendMessage(std::string roomID, std::string message) {
	if(accessToken.empty()) {
		throw std::runtime_error("You need to login first!");
	}
	bool success, running = true;
	//TODO(kdvalin) Update transaction IDs to be unique
	HTTPRequestData data(HTTPMethod::PUT, fmt::format(MatrixURLs::SEND_MESSAGE_FORMAT, roomID, "m1234557"));
	json body{
		{"msgtype", "m.text"},
		{"body", message}
	};
	Headers reqHeaders;
	reqHeaders["Authorization"] = "Bearer " + accessToken;
	data.setBody(body.dump());
	data.setHeaders(std::make_shared<Headers>(reqHeaders));

	data.setResponseCallback([&](Response result) {
		json body = json::parse(result.data);

		switch(result.status) {
		case HTTPStatus::HTTP_OK:
			success = true;
			break;
		default:
			std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
			success = false;
		}
		running = false;
	});
	data.setErrorCallback([&success, &running](std::string reason) {
		std::cerr << reason << std::endl;
		running = false;
		success = false;
	});
	http->request(std::move(data));

	while(running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return success;
}

json MatrixSession::getMessages(std::string batchNumber, bool firstTime) {
	if(accessToken.empty()) {
		throw std::runtime_error("You need to login first!");
	}
	bool running = true;

	json output;
	std::string urlParams = "?timeout={:d}&full_state={}";

	if(!batchNumber.empty()) {
		urlParams += "&since={:s}";
	}
	HTTPRequestData data(HTTPMethod::GET, fmt::format(MatrixURLs::SYNC + urlParams, 30000, firstTime, batchNumber));

	Headers reqHeaders;
	reqHeaders["Authorization"] = "Bearer " + accessToken;

	data.setHeaders(std::make_shared<Headers>(reqHeaders));

	data.setResponseCallback([&](Response result) {
		json body = json::parse(result.data);

		switch(result.status) {
		case HTTPStatus::HTTP_OK:
			output = body;
			break;
		default:
			std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
		}
		running = false;
	});
	data.setErrorCallback([&running](std::string reason) {
		std::cerr << reason << std::endl;
		running = false;
	});
	http->request(std::move(data));
	
	while(running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return output;
}

nlohmann::json MatrixSession::getMessages(bool firstTime) {
	return getMessages("", firstTime);
}

nlohmann::json MatrixSession::getAllRoomMessages(std::string roomId) {
	json sync = getMessages();
	json output;
	bool running = true;
	//TODO(kdvalin) Update transaction IDs to be unique
	HTTPRequestData data(HTTPMethod::GET, 
		fmt::format(
			MatrixURLs::GET_ROOM_MESSAGE_FORMAT + "?from={:s}&dir=b&limit={:d}", 
			roomId, 
			sync["rooms"]["join"][roomId]["timeline"]["prev_batch"].get<std::string>(), 
			100000
		)
	);
	output = sync["rooms"]["join"][roomId]["timeline"]["events"];

	Headers reqHeaders;
	reqHeaders["Authorization"] = "Bearer " + accessToken;
	data.setHeaders(std::make_shared<Headers>(reqHeaders));

	data.setResponseCallback([&](Response result) {
		json body = json::parse(result.data);

		switch(result.status) {
		case HTTPStatus::HTTP_OK:
			output += body["chunk"];
			break;
		default:
			std::cerr << static_cast<int>(result.status) << ": " << body << std::endl;
		}
		running = false;
	});
	data.setErrorCallback([&running](std::string reason) {
		std::cerr << reason << std::endl;
		running = false;
	});
	http->request(std::move(data));

	while(running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return output;
}