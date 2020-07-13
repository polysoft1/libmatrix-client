#ifndef CUSTOM_HTTP_SESSION_H
#define CUSTOM_HTTP_SESSION_H

#include "HTTP.h"
#include <string>
#include <memory>

namespace LibMatrix {

/**
	* A build in session class for CPPRESTSDK
	*/
class CPPRESTSDKSession : public HTTPSessionBase {
private:
	std::string url;
	std::string data;
	std::shared_ptr<Headers> headers;
	ResponseCallback callback;
	ErrorCallback errorCallback;

public:
	CPPRESTSDKSession();

	virtual void setURL(const std::string& url);
	virtual void setBody(const std::string& data);
	virtual void setHeaders(std::shared_ptr<Headers> header);
	virtual void setResponseCallback(ResponseCallback callback);
	virtual void setErrorCallback(ErrorCallback callback);

	/**
		* This is a non-blocking function to start the request.
		* The callback will be called when it is done.
		* An exception should be thrown if there are components missing.
		*/
	virtual void request(HTTPMethod method);
};

typedef CPPRESTSDKSession HTTPSession;

}  // namespace LibMatrix

#endif

