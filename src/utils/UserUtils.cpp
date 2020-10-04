#include <nlohmann/json.hpp>
#include <fmt/core.h> // Ignore CPPLintBear

#include "MatrixSession.h"
#include "Users.h"

using namespace LibMatrix; // Ignore CPPLintBear
using json = nlohmann::json;

User parseUser(const std::string& homeserverURL, const std::string& key, const json& user) {
	User result;
	result.id = key;

	if(!user["display_name"].is_string()) {
		result.displayName = key.substr(1, key.find(':') - 1);
	} else {
		result.displayName = user["display_name"].get<std::string>();
	}

	if(user["avatar_url"].is_string()) {
		std::string avatarMxcURL = user["avatar_url"].get<std::string>().substr(6);
		std::string serverName = avatarMxcURL.substr(0, avatarMxcURL.find('/'));
		std::string id = avatarMxcURL.substr(avatarMxcURL.find('/') + 1);

		result.avatarURL = fmt::format(homeserverURL + LibMatrix::MatrixURLs::THUMBNAIL_URL_FORMAT, serverName, id);
	}

	return result;
}
