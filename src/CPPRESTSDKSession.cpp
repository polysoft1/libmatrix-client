#include "../include/libmatrix-client/HTTPSession.h"

#ifdef USE_CPPRESTSDK
#include <cpprest/rawptrstream.h>
#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_client.h>

using LibMatrix::HTTPSession;
using LibMatrix::HTTPMethod;
using LibMatrix::CPPRESTSDKSession;

web::http::method getRequestType(const HTTPMethod& method) {
	switch (method) {
	case HTTPMethod::CONNECT:
		return web::http::methods::CONNECT;
	case HTTPMethod::DELETE:
		return web::http::methods::DEL;
	case HTTPMethod::GET:
		return web::http::methods::GET;
	case HTTPMethod::HEAD:
		return web::http::methods::HEAD;
	case HTTPMethod::OPTIONS:
		return web::http::methods::OPTIONS;
	case HTTPMethod::POST:
		return web::http::methods::POST;
	case HTTPMethod::PUT:
		return web::http::methods::PUT;
	case HTTPMethod::TRACE:
		return web::http::methods::TRCE;
	case HTTPMethod::PATCH:
		return web::http::methods::PATCH;
	}
	return U("unknown");
}

CPPRESTSDKSession::CPPRESTSDKSession() {}

void CPPRESTSDKSession::setURL(const std::string& url) {
	this->url = url;
}

void CPPRESTSDKSession::setBody(const std::string& data) {
	this->data = data;
}

void CPPRESTSDKSession::setHeaders(std::shared_ptr<Headers> headers) {
	this->headers = headers;
}

void CPPRESTSDKSession::setResponseCallback(ResponseCallback callback) {
	this->callback = callback;
}

void CPPRESTSDKSession::setErrorCallback(ErrorCallback callback) {
	this->errorCallback = callback;
}

void CPPRESTSDKSession::request(HTTPMethod method) {
	web::http::client::http_client client(
		utility::conversions::to_string_t(url)
	);

	web::http::method httpMethod = getRequestType(method);

	web::http::http_request request(httpMethod);

	// Add headers if set
	if (headers) {
		web::http::http_headers& headers = request.headers();
		for (std::pair<std::string, std::string> item : *this->headers.get()) {
			headers.add(utility::conversions::to_string_t(item.first),
				utility::conversions::to_string_t(item.second));
		}
	}
	request.set_body(utility::conversions::to_string_t(data));
	request.set_request_uri(utility::conversions::to_string_t(this->url));

	pplx::task task = client.request(request);

	auto test = task.then([this](pplx::task<web::http::http_response> task)	{
			// exceptions
			try {
				web::http::http_response response = task.get();

				if (callback) {
					auto test = response.extract_utf8string().then([this, response](utf8string contentString) {
						HTTPStatus status = static_cast<HTTPStatus>(response.status_code());
						Response packagedResponse(status, contentString);

						for (std::pair<const utility::string_t, const utility::string_t> item : response.headers()) {
							packagedResponse.headers.insert(
								std::pair<std::string, std::string>(
									utility::conversions::to_utf8string(item.first),
										utility::conversions::to_utf8string(item.second)));
						}

						try {
							if (callback)
								callback(packagedResponse);
						}
						catch (const std::exception& e)	{
							std::cerr << "Uncaught error on callback: " << e.what() << std::endl;
						}
					});
				}
			} catch (const std::exception& e) {
				errorCallback(e.what());
			} catch (...) {
				errorCallback("Unknown Error");
			}
		});
}

#endif
