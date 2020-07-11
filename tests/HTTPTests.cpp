#define BOOST_TEST_MODULE HTTP

#include "../include/libmatrix-client/CPPRESTSDKSession.h"

#include <boost/test/included/unit_test.hpp>

#include <thread>
#include <chrono>

BOOST_AUTO_TEST_CASE( example_org_GET_request ) {
	LibMatrix::CPPRESTSDKSession session;
	bool success = false;
	bool running = true;
	std::string error_msg;

	session.setURL("http://example.org/");
	session.setResponseCallback([&running, &success](LibMatrix::Response resp){
		success = (resp.status == LibMatrix::HTTPStatus::HTTP_OK);
		running = false;
	});
	session.request(LibMatrix::HTTPMethod::GET);
	
	while(running){
		//Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	BOOST_ASSERT(success);
}

BOOST_AUTO_TEST_CASE( invalid_TLD_test ) {
	LibMatrix::CPPRESTSDKSession session;
	bool success = false;
	bool running = true;
	std::string error_msg;

	session.setURL("http://example.gfoakgoawogeriawejhgiogejiujwaegheujawgh/");
	session.setResponseCallback([&running, &success](LibMatrix::Response resp){
		success = false;
		running = false;
	});
	session.setErrorCallback([&running, &success](std::string error) {
		success = (error.compare("Error resolving address") == 0);
		running = false;
	});

	session.request(LibMatrix::HTTPMethod::GET);
	
	while(running){
		//Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	BOOST_ASSERT(success);
}