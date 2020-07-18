#ifndef CUSTOM_HTTP_SESSION_H
#define CUSTOM_HTTP_SESSION_H

#include "HTTP.h"
#include <string>
#include <memory>
#include <cpprest/http_client.h>

namespace LibMatrix {

class CPPRESTSDKClient {
private:
	web::http::client::http_client client;
public:
	CPPRESTSDKClient(const std::string& basePath);
	virtual void request(HTTPRequestData&& method);

};

typedef CPPRESTSDKClient HTTPClient;
}  // namespace LibMatrix

#endif

