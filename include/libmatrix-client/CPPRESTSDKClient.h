#ifndef CPPRESTSDK_CLIENT_H
#define CPPRESTSDK_CLIENT_H

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
	virtual void request(HTTPRequestData&& method);
};

typedef CPPRESTSDKClient HTTPClient;
}  // namespace LibMatrix

#endif
