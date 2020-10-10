#ifndef __MESSAGE_UTILS_H__
#define __MESSAGE_UTILS_H__

#include <iostream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "Messages.h"

void parseMessages(std::vector<LibMatrix::Message> &messages, const nlohmann::json &body);
std::string findRoomName(const nlohmann::json &body);
bool isRoomEncrypted(const nlohmann::json &msg_body);


#endif
