#include <regex>
#include <iostream>

#include "MatrixSession.h"
#include "Exceptions.h"

using namespace LibMatrix;

void MatrixSession::setupEncryptAccount(){
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
void MatrixSession::genIdKeys(){
    //ID Key generation
	std::size_t idLength = olm_account_identity_keys_length(encryptAccount);
	char *buffer = new char[idLength];

	if(olm_account_identity_keys(encryptAccount, buffer, idLength) == olm_error()){
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

std::string MatrixSession::signMessage(std::string message){
    std::size_t sigSize = olm_account_signature_length(encryptAccount);
	char *buffer = new char[sigSize];
	if(olm_account_sign(encryptAccount, message.data(), message.size(), buffer, sigSize) == olm_error()){
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