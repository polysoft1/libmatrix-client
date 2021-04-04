#include "Encryption/Account.h"
#include "Exceptions.h"

#include <string>
#include <nlohmann/json.hpp>
#include <iostream>
#include <random>

#include "Encryption/EncryptionUtils.h"

using namespace LibMatrix;
using namespace LibMatrix::Encryption;
using nlohmann::json;

Account::Account() {
    uint8_t *buffer;
	uint8_t *encryptAccountBuff = new uint8_t[olm_account_size()];
	seedArray(encryptAccountBuff, olm_account_size());
	account = olm_account(reinterpret_cast<void*>(encryptAccountBuff));

	size_t random_size = olm_create_account_random_length(account);
	buffer = new uint8_t[random_size];
	seedArray(buffer, random_size);
	size_t retVal = olm_create_account(account, buffer, random_size);

	delete[] buffer;
	buffer = nullptr;

	if (retVal == olm_error()) {
		std::string err = olm_account_last_error(account);
		clear();
		throw Exceptions::OLMException(err, Exceptions::ACC_CREATE);
	}
}

Account::~Account() {
	clear();
}

void Account::clear() {
    olm_clear_account(account);
}

void Account::generateIdKeys() {
	std::size_t idLength = olm_account_identity_keys_length(account);
	uint8_t *buffer = new uint8_t[idLength];
	seedArray(buffer, idLength);
	if(olm_account_identity_keys(account, buffer, idLength) == olm_error()) {
		std::string err = olm_account_last_error(account);
		delete[] buffer;
		buffer = nullptr;

		throw Exceptions::OLMException(err, Exceptions::ID_KEY_GEN);
	}
	std::string idKeyStr = std::string(reinterpret_cast<char *>(buffer), idLength);
	delete[] buffer;
	buffer = nullptr;

	json keys = json::parse(idKeyStr);

	for(auto i = keys.begin(); i != keys.end(); ++i) {
		idKeys.push_back(IdentityKey(i.key(), i->get<std::string>()));
	}
}

std::string Account::sign(std::string message) {
	std::size_t sigSize = olm_account_signature_length(account);
	uint8_t *buffer = new uint8_t[sigSize];
	seedArray(buffer, sigSize);
	if(olm_account_sign(account, message.data(), message.size(), buffer, sigSize) == olm_error()) {
		std::string err = olm_account_last_error(account);
		delete[] buffer;
		buffer = nullptr;

		throw Exceptions::OLMException(err, Exceptions::SIGNATURE);
	}

	std::string sig = std::string(reinterpret_cast<char *>(buffer), sigSize);

	delete[] buffer;
	buffer = nullptr;

	return sig;
}

void Account::generateOneTimeKeys(int num) {
	size_t keySize = olm_account_generate_one_time_keys_random_length(account, num);
	uint8_t *buffer = new uint8_t[keySize];
	
	seedArray(buffer, keySize);

	if(olm_account_generate_one_time_keys(account, num, buffer, keySize) == olm_error()) {
		delete[] buffer;
		buffer = nullptr;

		throw Exceptions::OLMException(olm_account_last_error(account), Exceptions::OTK_GENERATION);
	}
	delete[] buffer;
	buffer = nullptr;

	size_t otkLen = olm_account_one_time_keys_length(account);
	uint8_t *outBuffer = new uint8_t[otkLen];

	if(olm_account_one_time_keys(account, outBuffer, otkLen) == olm_error()) {
		delete[] outBuffer;
		outBuffer = nullptr;
		throw Exceptions::OLMException(olm_account_last_error(account), Exceptions::OTK_GENERATION);
	}

	json keys = json::parse(std::string(
		reinterpret_cast<char*>(outBuffer)
	));

	for(auto i = keys.begin(); i != keys.end(); ++i){
		if(!i->is_object()) {//No clue what that's doing in the generated text if it's not an obj
			throw std::runtime_error("Unknown value in OLM JSON");
		}
		auto next = i->get<json>();
		for(auto j = next.begin(); j != next.end(); ++j){
			oneTimeKeys.push(OneTimeKey(i.key(), j.key(), j->get<std::string>()));
		}
	}

	delete[] outBuffer;
	outBuffer = nullptr;
}
