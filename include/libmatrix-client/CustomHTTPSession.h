#ifndef CUSTOM_HTTP_SESSION_H
#define CUSTOM_HTTP_SESSION_H

#include "HTTP.h"
#include <functional>
#include <string>

namespace LibMatrix {

/**
 * A class for wrapping a custom HTTP session.
 *
 * Nearly any HTTP library can be abstracted with this because
 * the HTTP Restful API is standardized.
 * The harder part for some libs is this requires async requests
 * with callbacks. It is possible to make a sync lib async using
 * threads, but it is more work and harder to get efficient than
 * using a lib that already provides that.
 */
class CustomHTTPSession : public HTTPSessionBase {
private:
	std::unique_ptr<HTTPSessionBase*> wrappedSession;
public:
	/**
	 * A function to call to initialize the custom session
	 * This should be statically set in a C++ file of
	 * the custom class.
	 */
	static std::function<HTTPSessionBase*()> initializer;

	CustomHTTPSession()
		: wrappedSession(initializer()) {}

	virtual void setURL(const std::string& url) {
		wrappedSession->setURL(url);
	}
	virtual void setBody(const std::string& data) {
		wrappedSession->setBody(data);
	}
	virtual void setHeaders(std::shared_ptr<Headers> header) {
		wrappesSession->setHeaders(header);
	}
	virtual void setResponseCallback(std::shared_ptr<ResponseCallback> callback) {
		wrappedSession->setResponseCallback(callback);
	}

	/**
	 * This is a non-blocking function to start the request.
	 * The callback will be called when it is done.
	 * An exception should be thrown if there are components missing.
	 */
	virtual void request(HTTPMethod method) {
		wrappedSession->request(method);
	}
};

typedef CustomHTTPSession HTTPSession

}  // namespace LibMatrix

#endif

