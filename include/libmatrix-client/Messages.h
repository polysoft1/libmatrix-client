#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <chrono>
#include <vector>
#include <utility>
#include <ctime>

namespace LibMatrix {

enum class MessageStatus {
	FAILURE,
	SYSTEM,
	SENDING,
	SENT,
	RECEIVED
};

struct Message {
	std::string id;
	std::string content;
	std::string sender;
	std::time_t timestamp;
	MessageStatus status;
};

} // namespace LibMatrix

#endif
