#include "Encryption/MegOlmSession.h"

#include <olm/olm.h>

#include "Encryption/EncryptionUtils.h"
#include "Exceptions.h"

using namespace LibMatrix::Encryption;

MegOlmSession::MegOlmSession() : msgCounter(0), msgLimit(-1), endTime(-1) {
	init();
	size_t randLen = olm_init_outbound_group_session_random_length(outSession);
	uint8_t *buffer = new uint8_t[randLen];
	seedArray(buffer, randLen);

	if(olm_init_outbound_group_session(outSession, buffer, randLen) == olm_error()) {
		throwError(true);
	}
	buffers.push_back(buffer);

	size_t idLen = olm_outbound_group_session_id_length(outSession);
	sessionId = new uint8_t[idLen];
	if(olm_outbound_group_session_id(outSession, sessionId, idLen) == olm_error()) {
		throwError(true);
	}

	size_t keyLen = olm_outbound_group_session_key_length(outSession);
	sessionKey = new uint8_t[keyLen];
	if(olm_outbound_group_session_key(outSession, sessionKey, keyLen) == olm_error()) {
		throwError(true);
	}
	createInboundSession(sessionKey, keyLen);
}

MegOlmSession::MegOlmSession(std::string _sessionKey) : msgCounter(0), msgLimit(-1), endTime(-1) {
	init();
	createInboundSession(reinterpret_cast<uint8_t *>(_sessionKey.data()), _sessionKey.size());

	sessionKey = new uint8_t[_sessionKey.size() + 1];
	for(int i = 0; i < _sessionKey.size(); ++i) {
		sessionKey[i] = _sessionKey[i];
	}
	sessionKey[_sessionKey.size()] = 0;// Null terminate it

	size_t idLen = olm_inbound_group_session_id_length(inSession);
	sessionId = new uint8_t[idLen];
	if(olm_inbound_group_session_id(inSession, sessionId, idLen) == olm_error()) {
		throwError(false);
	}
}

void MegOlmSession::init() {
	size_t randLen = olm_outbound_group_session_size();
	uint8_t *buffer = new uint8_t[randLen];
	outSession = olm_outbound_group_session(buffer);
	buffers.push_back(buffer);

	randLen = olm_inbound_group_session_size();
	buffer = new uint8_t[randLen];
	inSession = olm_inbound_group_session(buffer);
	buffers.push_back(buffer);
}

void MegOlmSession::createInboundSession(uint8_t *_sessionKey, size_t sessionKeyLen) {
	auto res = olm_init_inbound_group_session(inSession, _sessionKey, sessionKeyLen);

	if(res == olm_error()) {
		throwError(false);
	}
}

void MegOlmSession::throwError(bool outbound, Exceptions::OLMError type) {
	std::string err;
	if(outbound){
		err = olm_outbound_group_session_last_error(outSession);
	}else {
		err = olm_inbound_group_session_last_error(inSession);
	}
	clear();
	throw Exceptions::OLMException(err, type);
}

void MegOlmSession::clear() {
	if(outSession) {
		olm_clear_outbound_group_session(outSession);
	}
	if(inSession) {
		olm_clear_inbound_group_session(inSession);
	}
	
	for(uint8_t *buffer : buffers) {
		delete[] buffer;
	}

	outSession = nullptr;
	inSession = nullptr;
	delete[] sessionId;
	delete[] sessionKey;
}

std::string MegOlmSession::decryptMessage(std::string cipher, std::string senderKey, time_t ts) {
	uint8_t *cipherData = reinterpret_cast<uint8_t *>(cipher.data());
	size_t buffLen = olm_group_decrypt_max_plaintext_length(inSession, cipherData, cipher.size());
	uint8_t *buffer = new uint8_t[buffLen];
	uint32_t messageIndex;

	if(olm_group_decrypt(inSession, cipherData, cipher.size(), buffer, buffLen, &messageIndex) == olm_error()) {
		throw Exceptions::OLMException(olm_inbound_group_session_last_error(inSession), Exceptions::OLMError::DECRYPT_MESSAGE);
	}

	if(indexStore.find(messageIndex) == indexStore.end()) { //Message index is not in store
		indexStore[messageIndex] = ts;
	}else if(indexStore[messageIndex] != ts) { //Invalid message!!
		throw Exceptions::OLMException("Invalid message ID after decryption", Exceptions::OLMError::DECRYPT_MESSAGE);
	}

	std::string msg = reinterpret_cast<char *>(buffer);
	delete[] buffer;
	return msg;
}