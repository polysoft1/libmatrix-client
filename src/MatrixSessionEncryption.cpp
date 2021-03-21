#include <regex>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

#include "MatrixSession.h"
#include "HTTP.h"
#include "HTTPClient.h"
#include "Exceptions.h"

using nlohmann::json;

using namespace LibMatrix; // Ignore CPPLintBear

void MatrixSession::setupEncryptAccount() {
	//Account generation
	uint8_t *buffer;
	encryptAccountBuff = new uint8_t[olm_account_size()];
	encryptAccount = olm_account(encryptAccountBuff);

	std::size_t random_size = olm_create_account_random_length(encryptAccount);
	buffer = new uint8_t[random_size];
	size_t retVal = olm_create_account(encryptAccount, buffer, random_size);

	delete[] buffer;
	buffer = nullptr;

	if (retVal == olm_error()) {
		std::string err = olm_account_last_error(encryptAccount);
		clearEncryptAccount();
		throw Exceptions::OLMException(err, Exceptions::ACC_CREATE);
	}
}
void MatrixSession::genIdKeys() {
	//ID Key generation
	std::size_t idLength = olm_account_identity_keys_length(encryptAccount);
	char *buffer = new char[idLength];

	if(olm_account_identity_keys(encryptAccount, buffer, idLength) == olm_error()) {
		std::string err = olm_account_last_error(encryptAccount);
		delete[] buffer;
		buffer = nullptr;

		throw Exceptions::OLMException(err, Exceptions::ID_KEY_GEN);
	}
	idKeys = std::string(static_cast<char *>(buffer), idLength);
	idKeys = std::regex_replace(idKeys, std::regex("\":"), ":" + deviceID + "\":");
	delete[] buffer;
	buffer = nullptr;
}
void MatrixSession::clearEncryptAccount() {
	olm_clear_account(encryptAccount);
	delete[] encryptAccountBuff;

	encryptAccountBuff = nullptr;
	encryptAccount = nullptr;

	idKeys.clear();
}

std::string MatrixSession::signMessage(std::string message) {
	std::size_t sigSize = olm_account_signature_length(encryptAccount);
	char *buffer = new char[sigSize];
	if(olm_account_sign(encryptAccount, message.data(), message.size(), buffer, sigSize) == olm_error()) {
		std::string err = olm_account_last_error(encryptAccount);
		delete[] buffer;
		buffer = nullptr;

		throw Exceptions::OLMException(err, Exceptions::SIGNATURE);
	}

	std::string sig = std::string(static_cast<char *>(buffer), sigSize);

	delete[] buffer;
	buffer = nullptr;

	return sig;
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