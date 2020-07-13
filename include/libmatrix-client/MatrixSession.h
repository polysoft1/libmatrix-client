#ifndef MATRIX_SESSION_H
#define MATRIX_SESSION_H

#include <memory>
#include <string_view>
#include <string>

#include "HTTP.h"

namespace LibMatrix {

class MatrixSession {
private:
	std::unique_ptr<HTTPSessionBase> http;
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
};// End MatrixSession Class

}  // namespace LibMatrix
#endif
