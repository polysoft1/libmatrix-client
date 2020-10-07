#ifdef LIBMATRIX_USE_CUSTOM_HTTP_SESSION
#include "CustomHTTPClient.h"

using LibMatrix::HTTPClientBase;

std::function<HTTPClientBase* (const std::string& basePath)> LibMatrix::CustomHTTPClient::initializer
	= std::function<HTTPClientBase* (const std::string& basePath)>();

#endif

