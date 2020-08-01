#ifndef MATRIX_SESSION_H
#define MATRIX_SESSION_H

#include <string_view>
#include <memory>
#include <string>

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
	const std::string SYNC = CLIENT_PREFIX + "/sync";

	const std::string SEND_MESSAGE_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/send/m.room.message/{:s}";
	const std::string GET_ROOM_MESSAGE_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/messages";
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

	bool login(std::string uname, std::string password);
	nlohmann::json getRooms();
	bool sendMessage(std::string roomID, std::string message);
	
	nlohmann::json getMessages(std::string batchNumber, bool firstTime = false);
	nlohmann::json getMessages(bool firstTime = false);

	nlohmann::json getAllRoomMessages(std::string roomId);
};// End MatrixSession Class

}  // namespace LibMatrix
#endif
