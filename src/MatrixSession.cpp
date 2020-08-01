#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include <future>

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
				std::make_exception_ptr(std::runtime_error("Hi"))
			);
		}
	});
	data->setErrorCallback([threadResult](std::string reason) {
		std::cout << "Made it into the sucessful callback" << std::endl;
		threadResult->set_exception(
			std::make_exception_ptr(
				std::runtime_error(reason)
			)
		);
	});
	http->request(data);
	std::cout << threadResult.use_count() << std::endl;
	return threadResult->get_future();
}

json MatrixSession::getRooms() {
	if(accessToken.empty()) {
		throw std::runtime_error("You need to login first!");
	}
	bool running = true;
	json output;

	Headers reqHeaders;
	reqHeaders["Authorization"] = "Bearer " + accessToken;

	auto data = std::make_shared<HTTPRequestData>(HTTPMethod::GET, MatrixURLs::GET_ROOMS);
	data->setHeaders(std::make_shared<Headers>(reqHeaders));
	data->setResponseCallback([&](Response result) {
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
	data->setErrorCallback([&running](std::string reason) {
		std::cerr << reason << std::endl;
		running = false;
	});
	http->request(data);

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
	auto data = std::make_shared<HTTPRequestData>(HTTPMethod::PUT, fmt::format(MatrixURLs::SEND_MESSAGE_FORMAT, roomID, "m1234557"));
	json body{
		{"msgtype", "m.text"},
		{"body", message}
	};
	Headers reqHeaders;
	reqHeaders["Authorization"] = "Bearer " + accessToken;
	data->setBody(body.dump());
	data->setHeaders(std::make_shared<Headers>(reqHeaders));

	data->setResponseCallback([&](Response result) {
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
	data->setErrorCallback([&success, &running](std::string reason) {
		std::cerr << reason << std::endl;
		running = false;
		success = false;
	});
	http->request(data);

	while(running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return success;
}
