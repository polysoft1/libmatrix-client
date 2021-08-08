#ifndef __LIBMATRIX_ENCRYPT_MEGOLMSESSION_H__
#define __LIBMATRIX_ENCRYPT_MEGOLMSESSION_H__

#include <ctime>
#include <unordered_map>
#include <vector>
#include <olm/outbound_group_session.h>
#include <olm/inbound_group_session.h>

#include "Exceptions.h"

namespace LibMatrix::Encryption {
class MegOlmSession {
private:
	OlmOutboundGroupSession *outSession;
	OlmInboundGroupSession *inSession;
	std::string sessionId;
	std::string sessionKey;

	std::vector<uint8_t *> buffers;
	//Rotation Information
	int msgLimit;
	int msgCounter;
	std::time_t endTime;

	std::unordered_map<uint32_t, time_t> indexStore;

	void throwError(bool outbound, Exceptions::OLMError type = Exceptions::OLMError::SESSION_CREATE);
	void createInboundSession();
public:
	MegOlmSession();
	MegOlmSession(std::string sessionId);
	~MegOlmSession() { clear(); }

	void init();
	void clear();

	const std::string getSessionId() const { return sessionId; }
	const std::string getSessionKey() const { return sessionKey; }

	std::string decryptMessage(std::string cipher, std::string senderKey, std::time_t ts);
};
}

#endif