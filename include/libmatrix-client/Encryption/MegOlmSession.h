#ifndef __SESSION_H__
#define __SESSION_H__

#include <ctime>
#include <unordered_map>
#include <olm/outbound_group_session.h>
#include <olm/inbound_group_session.h>

#include "Exceptions.h"

namespace LibMatrix::Encryption {
class MegOlmSession {
private:
	OlmOutboundGroupSession *outSession;
	OlmInboundGroupSession *inSession;
	uint8_t *sessionId = nullptr;
	uint8_t *sessionKey = nullptr;

	//Rotation Information
	int msgLimit;
	int msgCounter;
	std::time_t endTime;

	std::unordered_map<uint32_t, time_t> indexStore;

	void throwError(bool outbound, Exceptions::OLMError type = Exceptions::OLMError::SESSION_CREATE);
	void createInboundSession(uint8_t *_sessionKey, size_t sessionKeyLen);
public:
	MegOlmSession();
	MegOlmSession(std::string sessionId);
	~MegOlmSession() { clear(); }

	void init();
	void clear();

	const std::string getSessionId() const { return reinterpret_cast<char *>(sessionId); }
	const std::string getSessionKey() const { return reinterpret_cast<char *>(sessionKey); }

	std::string decryptMessage(std::string cipher, std::string senderKey, std::time_t ts);
};
}

#endif