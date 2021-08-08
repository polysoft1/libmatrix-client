#ifndef __USER_UTILS_H__
#define __USER_UTILS_H__

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

#include "User.h"


std::shared_ptr<LibMatrix::User> parseUser(const std::string& homeserverUrl, const std::string& key, const nlohmann::json& user);

#endif
