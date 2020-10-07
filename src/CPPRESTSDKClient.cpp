#ifdef LIBMATRIX_USE_CPPRESTSDK
#include "../include/libmatrix-client/CPPRESTSDKClient.h"

#include <cpprest/rawptrstream.h>
#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_client.h>

using LibMatrix::HTTPClient;
using LibMatrix::HTTPMethod;
using LibMatrix::CPPRESTSDKClient;

CPPRESTSDKClient::CPPRESTSDKClient(const std::string& baseUri)
	: client(utility::conversions::to_string_t(baseUri))
{}

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

void CPPRESTSDKClient::request(std::shared_ptr<HTTPRequestData> data) {
	web::http::method httpMethod = getRequestType(data->getMethod());

	web::http::http_request request(httpMethod);

	// Add headers if set
	if (data->getHeaders()) {
		web::http::http_headers& headers = request.headers();
		for (std::pair<std::string, std::string> item : *data->getHeaders().get()) {
			headers.add(utility::conversions::to_string_t(item.first),
				utility::conversions::to_string_t(item.second));
		}
	}
	request.set_body(utility::conversions::to_string_t(data->getBody()));
	request.set_request_uri(utility::conversions::to_string_t(data->getSubPath()));

	pplx::task task = client.request(request);

	auto test = task.then([this, data](pplx::task<web::http::http_response> task) {
		// exceptions
		try {
			web::http::http_response response = task.get();

			if (data->getResponseCallback()) {
				auto test = response.extract_utf8string().then([this, response, data](utf8string contentString) {
					HTTPStatus status = static_cast<HTTPStatus>(response.status_code());
					Response packagedResponse(status, contentString);

					for (std::pair<const utility::string_t, const utility::string_t> item : response.headers()) {
						packagedResponse.headers.insert(
							std::pair<std::string, std::string>(
								utility::conversions::to_utf8string(item.first),
								utility::conversions::to_utf8string(item.second)));
					}

					try {
						if (data->getResponseCallback())
							data->getResponseCallback()(packagedResponse);
					}
					catch (const std::exception& e) {
						std::cerr << "Uncaught error on callback: " << e.what() << std::endl;
					}
					});
			}
		}
		catch (const std::exception& e) {
			data->getErrorCallback()(e.what());
		}
		catch (...) {
			data->getErrorCallback()("Unknown Error");
		}
	});
}

#endif
