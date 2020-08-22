#ifndef __ROOM_H__
#define __ROOM_H__

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Messages.h"

namespace LibMatrix {
class Room {
public:
	Room(std::string_view id, std::string name,
		const std::vector<Message> &messages, std::string prevToken = "",
		std::string nextToken = "") :
			id(id), name(name), messages(messages), prevToken(prevToken), nextToken(nextToken) {}
	std::string_view id;
	std::string name;
	std::vector<Message> messages;

	std::string prevToken;
	std::string nextToken;
};

using RoomMap = std::unordered_map<std::string, std::shared_ptr<LibMatrix::Room>>;
} // namespace LibMatrix

#endif
