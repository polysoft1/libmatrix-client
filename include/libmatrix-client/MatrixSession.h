#ifndef MATRIX_SESSION_H
#define MATRIX_SESSION_H

#include <string_view>
#include <memory>
#include <string>
#include <future>
#include <unordered_map>
#include <vector>
#include <array>

#include <nlohmann/json.hpp>

#include "HTTPClient.h"
#include "Messages.h"
#include "User.h"
#include "Encryption/Account.h"
#include "Room.h"
#include "typedefs.h"

#include "DLL.h"


namespace LibMatrix {
class Room;

namespace MatrixURLs {
	/**	Format:
	*		All URLs should NOT have a trailing /
	*		*_PREFIX: URL that goes before a different URL (this should not be used directly in a HTTP call obviously)
	*		*_FORMAT: A format string that needs to get passed through fmt or std::format
	*		Categories should be grouped together and separated from each other by 1 newline
	*/
	const std::string CLIENT_PREFIX = "/_matrix/client/r0";
	const std::string MEDIA_PREFIX = "/_matrix/media/r0";

	const std::string LOGIN = CLIENT_PREFIX + "/login";
	const std::string GET_ROOMS = CLIENT_PREFIX + "/joined_rooms";
	const std::string SYNC = CLIENT_PREFIX + "/sync";
	const std::string DEVICE_KEY_QUERY = CLIENT_PREFIX + "/keys/query";

	const std::string SEND_MESSAGE_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/send/m.room.message/{:s}";
	const std::string GET_ROOM_MESSAGE_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/messages";
	const std::string GET_ROOM_ALIAS_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/aliases";
	const std::string READ_MARKER_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/read_markers";
	const std::string GET_ROOM_MEMBERS_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/joined_members";

	const std::string E2E_UPLOAD_KEYS = CLIENT_PREFIX + "/keys/upload";

	const std::string THUMBNAIL_URL_FORMAT = MEDIA_PREFIX + "/thumbnail/{:s}/{:s}";

	const std::string TO_DEVICE_FORMAT = CLIENT_PREFIX + "/sendToDevice/{:s}/{:s}";

}  // namespace MatrixURLs

class MatrixSession {
private:
	std::unique_ptr<HTTPClient> http;
	std::string homeserverURL;
	std::string accessToken;
	std::string deviceID;
	std::string syncToken;
	std::string userId;
	bool req = false;

	std::unique_ptr<Encryption::Account> e2eAccount;

	int nextTransactionID = 999999;

	void setHTTPCaller();
	void postLoginSetup();

	std::future<void> publishOneTimeKeys(nlohmann::json keys);

	void signJSONPayload(nlohmann::json& payload);

	static constexpr std::string_view USER_TYPE = "m.id.user";
	static constexpr std::string_view LOGIN_TYPE = "m.login.password";

	static const std::vector<std::string> encryptAlgos;

	RoomMap roomMap;
public:
	LIBMATRIX_DLL_EXPORT MatrixSession();
	explicit LIBMATRIX_DLL_EXPORT MatrixSession(std::string url);
	LIBMATRIX_DLL_EXPORT ~MatrixSession();

	std::future<const RoomMap&> LIBMATRIX_DLL_EXPORT syncState(nlohmann::json filter = {}, int timeout = 30000);

	std::future<void> LIBMATRIX_DLL_EXPORT login(std::string uname, std::string password);

	std::future<void> LIBMATRIX_DLL_EXPORT sendMessage(std::string roomID, std::string message);

	std::future<void> LIBMATRIX_DLL_EXPORT updateReadReceipt(std::string roomID, LibMatrix::Message message);

	std::future<void> getUserDevices(std::unordered_map<std::string, User> users, int timeout = 10000);
	
	void httpCall(std::string url, HTTPMethod method, const nlohmann::json &data, ResponseCallback callback, ErrorCallback errCallback);
	template <class T>
	ErrorCallback createErrorCallback(std::shared_ptr<std::promise<T>> promiseThread) {
		return ([promiseThread](std::string reason) {
			std::cout << "Errored out, reason: " << reason << std::endl;
			promiseThread->set_exception(
				std::make_exception_ptr(
					std::runtime_error(reason)
				)
			);
			});
	}

	template<class T>
	bool verifyAuth(std::shared_ptr<std::promise<T>> result) {
		bool isEmpty = accessToken.empty();
		if (isEmpty) {
			result->set_exception(
				std::make_exception_ptr(
					std::runtime_error("You need to login first!")));
		}

		return !isEmpty;
	}

	std::future<void> sendMessageRequest(std::string roomId, const nlohmann::json &payload);

	const std::string& getDeviceId() const { return deviceID; }
	const std::string& getUserId() const { return userId; }
	const std::string& getSenderKey() const { return e2eAccount->getIdKeys()[0].getValue(); }
	const std::string& getHomeserverURL() const { return homeserverURL; }

	std::string getNextTransactionID() { return "m" + std::to_string(nextTransactionID++); }
};// End MatrixSession Class

}  // namespace LibMatrix
#endif
