#include <cctype>
#include "HTTP.h"

namespace LibMatrix {

bool CaseInsensitiveCompare::operator()(const std::string& a, const std::string& b) const noexcept {
	return std::lexicographical_compare(
		a.begin(), a.end(), b.begin(), b.end(),
		[](unsigned char ac, unsigned char bc) { return std::tolower(ac) < std::tolower(bc); });
}

HTTPRequestData::HTTPRequestData(HTTPMethod method, const std::string& subPath)
	: method(method), subPath(subPath)
{}

void HTTPRequestData::setMethod(const HTTPMethod method) {
	this->method = method;
}
void HTTPRequestData::setSubPath(const std::string& subPath) {
	this->subPath = subPath;
}
void HTTPRequestData::setBody(const std::string& data) {
	this->data = data;
}
void HTTPRequestData::setHeaders(std::shared_ptr<Headers> headers) {
	this->headers = headers;
}
void HTTPRequestData::setResponseCallback(ResponseCallback callback) {
	this->callback = callback;
}
void HTTPRequestData::setErrorCallback(ErrorCallback callback) {
	this->errorCallback = callback;
}

HTTPMethod HTTPRequestData::getMethod() const {
	return method;
}
const std::string& HTTPRequestData::getSubPath() const {
	return subPath;
}
const std::string& HTTPRequestData::getBody() const {
	return data;
}
std::shared_ptr<Headers> HTTPRequestData::getHeaders() const {
	return headers;
}
ResponseCallback HTTPRequestData::getResponseCallback() const {
	return callback;
}
ErrorCallback HTTPRequestData::getErrorCallback() const {
	return errorCallback;
}

}

