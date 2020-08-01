#ifndef CUSTOM_HTTP_SESSION_H
#define CUSTOM_HTTP_SESSION_H

#include "HTTP.h"
#include <functional>
#include <memory>
#include <string>
#include <utility>

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
class CustomHTTPClient : public HTTPClientBase {
private:
	std::unique_ptr<HTTPClientBase> wrappedClient;

public:
	/**
	 * A function to call to initialize the custom session
	 * This should be statically set in a C++ file of
	 * the custom class.
	 */
	static std::function<HTTPClientBase*()> initializer;

	CustomHTTPClient()
		: wrappedClient(initializer()) {}

	virtual void request(std::shared_ptr<HTTPRequestData> method) { wrappedClient->request(method);  }
};

typedef CustomHTTPClient HTTPRequest

}  // namespace LibMatrix

#endif

