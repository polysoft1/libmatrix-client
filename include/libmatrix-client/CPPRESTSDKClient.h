#ifndef CUSTOM_HTTP_SESSION_H
#define CUSTOM_HTTP_SESSION_H

#include <cpprest/http_client.h>

#include "HTTP.h"
#include <string>
#include <memory>

namespace LibMatrix {

class CPPRESTSDKClient {
private:
	web::http::client::http_client client;
public:
	explicit CPPRESTSDKClient(const std::string& basePath);
	virtual void request(std::shared_ptr<HTTPRequestData> method);
};

typedef CPPRESTSDKClient HTTPClient;
}  // namespace LibMatrix

#endif
