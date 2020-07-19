#define BOOST_TEST_MODULE HTTP

#include <thread>
#include <chrono>

#include <boost/test/included/unit_test.hpp>

#include "../include/libmatrix-client/CPPRESTSDKClient.h"

BOOST_AUTO_TEST_CASE(example_org_GET_request) {
	LibMatrix::CPPRESTSDKClient client("http://example.org/");
	bool running = true;
	LibMatrix::HTTPStatus result;

	LibMatrix::HTTPRequestData data(LibMatrix::HTTPMethod::GET, "");

	data.setResponseCallback([&running, &result](LibMatrix::Response resp) {
		result = resp.status;
		running = false;
	});
	client.request(std::move(data));

	while(running) {
		// Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Need explicit casts because LibMatrix::HTTPStatus is a enum class which does not impelment operator<<
	BOOST_CHECK_EQUAL(static_cast<int>(LibMatrix::HTTPStatus::HTTP_OK), static_cast<int>(result));
}

BOOST_AUTO_TEST_CASE(invalid_TLD_test) {
	// Need this for BOOST_CHECK_EQUAL_COLLECTIONS
	const std::string expectedErr = "Error resolving address";

	LibMatrix::CPPRESTSDKClient client("http://example.gfoakgoawogeriawejhgiogejiujwaegheujawgh/");
	bool running = true;
	bool success = false;
	std::string error;

	LibMatrix::HTTPRequestData data(LibMatrix::HTTPMethod::GET, "");

	data.setResponseCallback([&running, &success](LibMatrix::Response resp) {
		success = false;
		running = false;
	});
	data.setErrorCallback([&running, &success, &error](std::string respErr) {
		success = true;
		running = false;
		error = respErr;
	});

	client.request(std::move(data));

	while(running) {
		// Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	if(!success) {
		BOOST_FAIL("Somehow the TLD in this test resolved.");
	}
	BOOST_CHECK_EQUAL_COLLECTIONS(error.begin(), error.end(), expectedErr.begin(), expectedErr.end());
}
