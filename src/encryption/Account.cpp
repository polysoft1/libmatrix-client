#include "Encryption/Account.h"
#include "Exceptions.h"

#include <string>
#include <nlohmann/json.hpp>

using namespace LibMatrix;
using namespace LibMatrix::Encryption;
using nlohmann::json;

Account::Account() {
    uint8_t *buffer;
	uint8_t *encryptAccountBuff = new uint8_t[olm_account_size()];
	account = olm_account(reinterpret_cast<void*>(encryptAccountBuff));

	size_t random_size = olm_create_account_random_length(account);
	buffer = new uint8_t[random_size];
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
	char *buffer = new char[idLength];

	if(olm_account_identity_keys(account, buffer, idLength) == olm_error()) {
		std::string err = olm_account_last_error(account);
		delete[] buffer;
		buffer = nullptr;

		throw Exceptions::OLMException(err, Exceptions::ID_KEY_GEN);
	}
	std::string idKeyStr = std::string(static_cast<char *>(buffer), idLength);
	delete[] buffer;
	buffer = nullptr;

	json keys = json::parse(idKeyStr);

	for(auto i = keys.begin(); i != keys.end(); ++i) {
		idKeys.push_back(IdentityKey(i.key(), i->get<std::string>()));
	}
}

std::string Account::sign(std::string message) {
	std::size_t sigSize = olm_account_signature_length(account);
	char *buffer = new char[sigSize];
	if(olm_account_sign(account, message.data(), message.size(), buffer, sigSize) == olm_error()) {
		std::string err = olm_account_last_error(account);
		delete[] buffer;
		buffer = nullptr;

		throw Exceptions::OLMException(err, Exceptions::SIGNATURE);
	}

	std::string sig = std::string(static_cast<char *>(buffer), sigSize);

	delete[] buffer;
	buffer = nullptr;

	return sig;
}