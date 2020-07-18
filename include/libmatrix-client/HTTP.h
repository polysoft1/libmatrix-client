#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <memory>
#include <map>
#include <functional>
#include <sstream>

namespace LibMatrix {

struct CaseInsensitiveCompare {
	bool operator()(const std::string& a, const std::string& b) const noexcept;
};

using Headers = std::map<std::string, std::string, CaseInsensitiveCompare>;

enum class HTTPMethod {
	CONNECT, DELETE, GET, HEAD, OPTIONS,
	POST, PUT, TRACE, PATCH
};

enum class HTTPStatus {
	HTTP_CONTINUE = 100,
	HTTP_SWITCHING_PROTOCOLS = 101,
	HTTP_PROCESSING = 102,
	HTTP_OK = 200,
	HTTP_CREATED = 201,
	HTTP_ACCEPTED = 202,
	HTTP_NONAUTHORITATIVE = 203,
	HTTP_NO_CONTENT = 204,
	HTTP_RESET_CONTENT = 205,
	HTTP_PARTIAL_CONTENT = 206,
	HTTP_MULTI_STATUS = 207,
	HTTP_ALREADY_REPORTED = 208,
	HTTP_IM_USED = 226,
	HTTP_MULTIPLE_CHOICES = 300,
	HTTP_MOVED_PERMANENTLY = 301,
	HTTP_FOUND = 302,
	HTTP_SEE_OTHER = 303,
	HTTP_NOT_MODIFIED = 304,
	HTTP_USE_PROXY = 305,
	HTTP_USEPROXY = 305,  /// @deprecated
	// UNUSED: 306
	HTTP_TEMPORARY_REDIRECT = 307,
	HTTP_PERMANENT_REDIRECT = 308,
	HTTP_BAD_REQUEST = 400,
	HTTP_UNAUTHORIZED = 401,
	HTTP_PAYMENT_REQUIRED = 402,
	HTTP_FORBIDDEN = 403,
	HTTP_NOT_FOUND = 404,
	HTTP_METHOD_NOT_ALLOWED = 405,
	HTTP_NOT_ACCEPTABLE = 406,
	HTTP_PROXY_AUTHENTICATION_REQUIRED = 407,
	HTTP_REQUEST_TIMEOUT = 408,
	HTTP_CONFLICT = 409,
	HTTP_GONE = 410,
	HTTP_LENGTH_REQUIRED = 411,
	HTTP_PRECONDITION_FAILED = 412,
	HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
	HTTP_REQUESTENTITYTOOLARGE = 413,  /// @deprecated
	HTTP_REQUEST_URI_TOO_LONG = 414,
	HTTP_REQUESTURITOOLONG = 414,  /// @deprecated
	HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
	HTTP_UNSUPPORTEDMEDIATYPE = 415,  /// @deprecated
	HTTP_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
	HTTP_EXPECTATION_FAILED = 417,
	HTTP_IM_A_TEAPOT = 418,
	HTTP_ENCHANCE_YOUR_CALM = 420,
	HTTP_MISDIRECTED_REQUEST = 421,
	HTTP_UNPROCESSABLE_ENTITY = 422,
	HTTP_LOCKED = 423,
	HTTP_FAILED_DEPENDENCY = 424,
	HTTP_UPGRADE_REQUIRED = 426,
	HTTP_PRECONDITION_REQUIRED = 428,
	HTTP_TOO_MANY_REQUESTS = 429,
	HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
	HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
	HTTP_INTERNAL_SERVER_ERROR = 500,
	HTTP_NOT_IMPLEMENTED = 501,
	HTTP_BAD_GATEWAY = 502,
	HTTP_SERVICE_UNAVAILABLE = 503,
	HTTP_GATEWAY_TIMEOUT = 504,
	HTTP_VERSION_NOT_SUPPORTED = 505,
	HTTP_VARIANT_ALSO_NEGOTIATES = 506,
	HTTP_INSUFFICIENT_STORAGE = 507,
	HTTP_LOOP_DETECTED = 508,
	HTTP_NOT_EXTENDED = 510,
	HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511
};

class Response {
public:
	const std::string data;
	const HTTPStatus status;
	Headers headers;

	Response(HTTPStatus status, std::string data)
		: status(status), data(data)
	{}
};

using ResponseCallback = std::function<void(Response)>;
using ErrorCallback = std::function<void(std::string)>;

// TODO: Add a base class and add versions of this for string, binary, and files.
// This is currently the string version.
class HTTPRequestData {
private:
	HTTPMethod method;
	std::string subPath;
	std::string data;
	std::shared_ptr<Headers> headers;
	ResponseCallback callback;
	ErrorCallback errorCallback;
public:
	HTTPRequestData(HTTPMethod method, const std::string& subPath);

	virtual void setMethod(HTTPMethod method);
	virtual void setSubPath(const std::string& subPath);
	virtual void setBody(const std::string& data);
	virtual void setHeaders(std::shared_ptr<Headers> header);
	virtual void setResponseCallback(ResponseCallback callback);
	virtual void setErrorCallback(ErrorCallback callback);

	virtual HTTPMethod getMethod() const;
	virtual const std::string& getSubPath() const;
	virtual const std::string& getBody() const;
	virtual std::shared_ptr<Headers> getHeaders() const;
	virtual ResponseCallback getResponseCallback() const;
	virtual ErrorCallback getErrorCallback() const;
};

class HTTPClientBase {
	/**
	 * This is a non-blocking function to start the request.
	 * The callback stored in the request data will be called when it is done.
	 * An exception should be thrown if there are components missing.
	 */
	virtual void request(HTTPRequestData&& method) = 0;
};

}  // namespace LibMatrix

#endif
