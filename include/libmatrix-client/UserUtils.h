#ifndef __USER_UTILS_H__
#define __USER_UTILS_H__

#include <string>

#include <nlohmann/json.hpp>

#include "Users.h"

LibMatrix::User parseUser(const std::string& homeserverUrl, const std::string& key, const nlohmann::json& user);

#endif
