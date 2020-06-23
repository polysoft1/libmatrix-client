#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>

namespace LibMatrix {
	struct CaseInsensitiveCompare {
		bool operator()(const std::string& a, const std::string& b) const noexcept;
	};

	using Headers = std::map<std::string, std::string, CaseInsensitiveCompare>;

	enum class HTTPMethod {
		CONNECT, DEL, GET, HEAD, OPTIONS,
		POST, PUT, TRACE, PATCH
	};

	class Response {
	public:
		const std::string data;
		const int status;
		Headers headers;

		Response(int status, std::string data)
			: status(status), data(data)
		{}
	};


	class HTTPSessionBase {
	public:
		using ResponseCallback = std::function<void(Response)>;
		virtual void setURL(const std::string& url) = 0;
		virtual void setBody(const std::string& data) = 0;
		virtual void setHeaders(const Headers& header) = 0;
		virtual void setResponseCallback(const ResponseCallback& callback) = 0;
		virtual void request(HTTPMethod method) = 0;
	};
}

#endif // !HTTPS
