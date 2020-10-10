#include <fmt/format.h>

#include "MessageUtils.h"
#include "Room.h"

using LibMatrix::Message;
using LibMatrix::MessageStatus;
using json = nlohmann::json;

bool isRoomEncrypted(const json &msg_body) {
	return msg_body[0]["type"].get<std::string>().compare("m.room.encrypted") == 0;
}

std::string findRoomName(const json &body) {
	std::time_t latestTs = 0;
	std::string output = "";
	for(auto i = body.begin(); i != body.end(); ++i) {
		if((*i)["type"].get<std::string>().compare("m.room.name") == 0) {
			std::time_t currentTs = (*i)["origin_server_ts"].get<std::time_t>();

			if(currentTs > latestTs) {
				latestTs = currentTs;
				output = (*i)["content"]["name"];
			}
		}
	}

	return output;
}

void parseMessages(std::vector<Message> &messages, const json &body) {
	for(auto i = body.begin(); i != body.end(); ++i) {
			std::string id = (*i)["event_id"].get<std::string>();
			std::string sender = (*i)["sender"].get<std::string>();
			std::time_t ts = (*i)["origin_server_ts"].get<std::time_t>();

			MessageStatus messageType = MessageStatus::RECEIVED;

			std::string content;
			json contentLocation = (*i)["content"];

			if( contentLocation["body"].is_null() ) {
				if(contentLocation["membership"].is_string()) {
					std::string action = contentLocation["membership"].get<std::string>();
					std::string name = contentLocation["displayname"].is_string() ?
						contentLocation["displayname"].get<std::string>() : "An Unknown User";
					if (action.compare("join") == 0) {
						action = "joined";
					} else if (action.compare("leave") == 0) {
						action = "left";
					}

					content = fmt::format("{:s} {:s} the room", name, action);
					messageType = MessageStatus::SYSTEM;
				}
			} else if (contentLocation["body"].is_string()) {
				content = contentLocation["body"].get<std::string>();
			}

			messages.push_back(
				Message{id, content, sender, ts, messageType});
	}
}
