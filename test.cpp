#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_client.h>

#include <iostream>
#include <thread>
#include <chrono>

void testCPPRest() {
	bool completed = false;
	// Skipping SSL shenanigans for now
	web::http::client::http_client client(web::uri("http://example.com"));
	web::http::http_request method(web::http::methods::GET);

	auto request = client.request(method);

	auto test = request.then([&completed](pplx::task<web::http::http_response> task) {
		try {
			web::http::http_response resp = task.get();

			std::cout << resp.status_code() << ": " << resp.reason_phrase() << std::endl;
		}catch(const std::exception & e) {
			std::cerr << e.what() << std::endl;
		}catch(...) {
			std::cerr << "Unknown exception" << std::endl;
		}
		completed = true;
	});

	while(!completed) {
		std::this_thread::sleep_for(std::chrono::microseconds(20));
	}
}

int main(int argc, char **argv) {
	std::cout << "Hello World, this is to test GitHub Action flows" << std::endl;
	testCPPRest();
	return 0;
}
