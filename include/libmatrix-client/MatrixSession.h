#ifndef MATRIX_SESSION_H
#define MATRIX_SESSION_H

#include <string_view>
#include <memory>
#include <string>
#include <future>

#include <nlohmann/json.hpp>

#include "HTTPClient.h"

namespace LibMatrix {

namespace MatrixURLs {
	/**	Format:
	*		All URLs should NOT have a trailing /
	*		*_PREFIX: URL that goes before a different URL (this should not be used directly in a HTTP call obviously)
	*		*_FORMAT: A format string that needs to get passed through fmt or std::format
	*		Categories should be grouped together and separated from each other by 1 newline
	*/
	const std::string CLIENT_PREFIX = "/_matrix/client/r0";

	const std::string LOGIN = CLIENT_PREFIX + "/login";
	const std::string GET_ROOMS = CLIENT_PREFIX + "/joined_rooms";

	const std::string SEND_MESSAGE_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/send/m.room.message/{:s}";
}  // namespace MatrixURLs

class MatrixSession {
private:
	std::unique_ptr<HTTPClient> http;
	std::string homeserverURL;
	std::string accessToken;
	std::string deviceID;

	void setHTTPCaller();

	static constexpr std::string_view USER_TYPE = "m.id.user";
	static constexpr std::string_view LOGIN_TYPE = "m.login.password";
public:
	MatrixSession();
	explicit MatrixSession(std::string url);

	std::future<void> login(std::string uname, std::string password);
	nlohmann::json getRooms();
	std::future<void> sendMessage(std::string roomID, std::string message);
};// End MatrixSession Class

}  // namespace LibMatrix
#endif
