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
#include "olm/olm.h"

#include "HTTPClient.h"
#include "Messages.h"
#include "Room.h"
#include "Users.h"

#include "DLL.h"


namespace LibMatrix {

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

	const std::string SEND_MESSAGE_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/send/m.room.message/{:s}";
	const std::string GET_ROOM_MESSAGE_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/messages";
	const std::string GET_ROOM_ALIAS_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/aliases";
	const std::string READ_MARKER_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/read_markers";
	const std::string GET_ROOM_MEMBERS_FORMAT = CLIENT_PREFIX + "/rooms/{:s}/joined_members";

	const std::string E2E_UPLOAD_KEYS = CLIENT_PREFIX + "/keys/upload";

	const std::string THUMBNAIL_URL_FORMAT = MEDIA_PREFIX + "/thumbnail/{:s}/{:s}";

}  // namespace MatrixURLs

class MatrixSession {
private:
	std::unique_ptr<HTTPClient> http;
	std::string homeserverURL;
	std::string accessToken;
	std::string deviceID;
	std::string syncToken;
	std::string userId;

	OlmAccount *encryptAccount;
	uint8_t *encryptAccountBuff;
	std::string idKeys;

	int nextTransactionID = 999999;

	void setHTTPCaller();
	void postLoginSetup();

	void setupEncryptAccount();
	void genIdKeys();
	void clearEncryptAccount();
	std::string signMessage(std::string message);

	static constexpr std::string_view USER_TYPE = "m.id.user";
	static constexpr std::string_view LOGIN_TYPE = "m.login.password";

	static const std::vector<std::string> encryptAlgos;
public:
	MatrixSession();
	explicit MatrixSession(std::string url);
	~MatrixSession();

	std::future<RoomMap> LIBMATRIX_DLL_EXPORT syncState(nlohmann::json filter = {}, int timeout = 30000);

	std::future<void> LIBMATRIX_DLL_EXPORT login(std::string uname, std::string password);

	std::future<void> LIBMATRIX_DLL_EXPORT sendMessage(std::string roomID, std::string message);

	std::future<void> LIBMATRIX_DLL_EXPORT updateReadReceipt(std::string roomID, LibMatrix::Message message);
	std::future<std::vector<User>> LIBMATRIX_DLL_EXPORT getRoomMembers(std::string roomID);
};// End MatrixSession Class

}  // namespace LibMatrix
#endif
