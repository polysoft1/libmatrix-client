#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <chrono>
#include <vector>
#include <utility>

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

class MessageBatch {
public:
	MessageBatch(std::string roomId, std::vector<Message> messages, std::string prev, std::string next) :
		roomId(roomId), messages(messages), prevBatch(prev), nextBatch(next) {}
	MessageBatch(MessageBatch&& in) :
		roomId(in.roomId), messages(std::move(in.messages)), prevBatch(in.prevBatch), nextBatch(in.nextBatch) {}

	std::string roomId;
	std::string prevBatch;
	std::vector<Message> messages;
	std::string nextBatch;

	friend std::ostream& operator<<(std::ostream& output, const MessageBatch& in);

	MessageBatch& operator=(MessageBatch&& in) {
		roomId = in.roomId;
		messages = std::move(messages);
		prevBatch = in.prevBatch;
		nextBatch = in.nextBatch;
		return *this;
	}
};

} // namespace LibMatrix

#endif
